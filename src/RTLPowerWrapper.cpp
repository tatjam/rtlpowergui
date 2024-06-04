#include "RTLPowerWrapper.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <signal.h>
#include <poll.h>
#include <cassert>
#include <iostream>
#include <sys/wait.h>

void RTLPowerWrapper::launch()
{
	if(thread.joinable())
	{
		thread_run = false;
		thread.join();
	}

	std::stringstream cmdbuild;
	cmdbuild << "rtl_power_fftw";
	cmdbuild <<
	" -b " << nbins <<
	" -f " << min_f << ":" << max_f <<
	" -g " << gain <<
	" -o " << overlap_percent <<
	" -c";
	if(use_stime)
		cmdbuild << " -t " << stime;
	else
		cmdbuild << " -n " << snum;

	// Launch the program

	// [0] = read, [1] = write
	int pipefd[2];
	if(pipe(pipefd) == -1)
	{
		std::cerr << "Unable to create pipe" << std::endl;
	}
	cur_pid = fork();
	if(cur_pid == 0)
	{
		// Child process
		close(pipefd[0]);
		dup2(pipefd[1], 1); // redirect stdout (fd 1) to pipe, this replaces the fd
		//close(2); // close stderr
		close(pipefd[1]); 	// so we dont need the pipe itself, as it's stdout itself now
		// Launch the process replacing ourselves
		execl("/bin/sh", "sh", "-c", cmdbuild.str().c_str(), nullptr);
	}
	else
	{
		close(pipefd[1]);
	}

	thread_run = true;

	// Launch worker thread
	thread = std::thread([this](int readfd)
	{
		std::string readbuffer;
		char inbuffer[128];

		// Setup poll
		struct pollfd pfd =
		{
				readfd, POLLIN
		};

		while(thread_run)
		{
			poll(&pfd, 1, 100);
			if(pfd.revents & POLLIN)
			{
				// Read as much as we can
				size_t nread = read(pfd.fd, inbuffer, sizeof(inbuffer));
				for(size_t i = 0; i < nread; i++)
				{
					readbuffer.push_back(inbuffer[i]);
					if(inbuffer[i] == '\n')
					{
						process_line(readbuffer);
						readbuffer.clear();
					}
				}
			}
		}


		close(readfd);
	}, pipefd[0]);


}

void RTLPowerWrapper::stop()
{
	if(cur_pid != 0)
	{
		// TODO: Use SIGINT to smoothly shut off!
		kill(cur_pid, SIGTERM);
		kill(cur_pid, SIGTERM);
		wait(nullptr);
	}
	thread_run = false;
}

void RTLPowerWrapper::set_freq_range(uint64_t min, uint64_t max)
{
	assert(!thread_run);
	min_f = min;
	max_f = max;
}

void RTLPowerWrapper::set_num_bins(int bins)
{
	assert(!thread_run);
	nbins = bins;
}

void RTLPowerWrapper::set_overlap(int percent)
{
	assert(!thread_run);
	overlap_percent = percent;
}

void RTLPowerWrapper::set_num_samples(int nsamples)
{
	assert(!thread_run);
	snum = nsamples;
	use_stime = false;
}

double RTLPowerWrapper::set_samp_time(double nstime)
{
	assert(!thread_run);
	stime = nstime;
	use_stime = true;
}

void RTLPowerWrapper::set_gain(int ngain)
{
	assert(!thread_run);
	gain = ngain;
}

void RTLPowerWrapper::process_line(const std::string &line)
{
	if(line[0] == '#')
	{
		return;
	}

	if(line == "\n")
	{
		if(skipped_prev)
		{
			// End of sweep, notify with flag, and do nothing else
			back_buffer.is_end_of_sweep = true;
		}
		skipped_prev = true;
	}
	else
	{
		if(skipped_prev)
		{
			// A spectrum was obtained last call,
			// Put back buffer back to front and notify
			data_mtx.lock();
			front_buffer = back_buffer;
			data = &front_buffer;
			data_mtx.unlock();
			data_available.notify_one();

			// Clear back buffer
			back_buffer = RTLPowerData();
			back_buffer.is_end_of_sweep = false;
			skipped_prev = false;
		}

		// Format is frequency [Hz] as scientific notation number
		// followed by a space and then spectral density dB/Hz (arbitrarily referenced)
		Readout read{};
		std::stringstream s(line);
		s >> read.freq >> read.power;
		back_buffer.reads.push_back(read);
	}
}

bool RTLPowerWrapper::is_stopped()
{
	return !thread_run;
}

RTLPowerWrapper::RTLPowerWrapper()
{
	thread_run = false;
	cur_pid = 0;
	skipped_prev = false;
	data = nullptr;
}

RTLPowerWrapper::~RTLPowerWrapper()
{
	stop();
	if(thread.joinable())
	{
		thread_run = false;
		thread.join();
	}

}

bool RTLPowerWrapper::get_exec_status()
{
	// -1 means it's not running
	return kill(cur_pid, 0) == -1;
}

