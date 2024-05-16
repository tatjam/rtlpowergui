#include "RTLPowerWrapper.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <signal.h>
#include <poll.h>
#include <cassert>

void RTLPowerWrapper::launch()
{
	std::stringstream cmdbuild;
	cmdbuild << "rtl_power_fftw";
	cmdbuild <<
	"-b " << nbins <<
	" -f " << min_f << ":" << max_f <<
	" -g " << gain <<
	" -o " << overlap_percent;
	if(use_stime)
		cmdbuild << " -t " << stime;
	else
		cmdbuild << " -n " << snum;

	// Launch the program

	// [0] = read, [1] = write
	int pipefd[2];
	pipe(pipefd);
	int pid = fork();
	if(pid == 0)
	{
		// Child process
		close(pipefd[0]);
		dup2(pipefd[1], 1); // redirect stdout (fd 1) to pipe, this replaces the fd
		close(pipefd[1]); 	// so we dont need the pipe itself, as it's stdout itself now
		// Launch the process replacing ourselves
	}
	else
	{
		close(pipefd[1]);
	}

	thread_run = true;
	closing = false;

	// Launch worker thread
	thread = std::thread([&cmdbuild, pid, pipefd, this]()
	{
		std::string readbuffer;
		char inbuffer[128];

		// Setup poll
		struct pollfd pfd =
		{
				pipefd[0], POLLIN, 0
		};

		while(true)
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

			if(!thread_run && !closing)
			{
				// This won't close immediately, it will finish the sweep
				// and neatly stop RTL-SDR
				kill(pid, SIGINT);
				closing = true;
			}
			if(closing)
			{
				// Check if the process has finally died, to exit the loop
				// as no more data can be gotten
				if(kill(pid, 0) == -1)
					break;
			}
		}

		close(pipefd[0]);
	});


}

void RTLPowerWrapper::stop()
{
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
}

double RTLPowerWrapper::set_samp_time(double nstime)
{
	assert(!thread_run);
	stime = nstime;
}

void RTLPowerWrapper::process_line(const std::string &line)
{
	if(line[0] == '#')
	{
		return;
	}

	if(line.empty())
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
