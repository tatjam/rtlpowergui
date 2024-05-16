#include "PlotBuilder.h"
#include <iostream>

int PlotBuilder::get_num_points()
{
	return 10;
}

void PlotBuilder::launch()
{
	if(thread.joinable())
	{
		thread.join();
	}

	commit_settings();

	thread_run = true;
	thread = std::thread([this]()
 	{
		while(thread_run)
		{
			std::unique_lock<std::mutex> lock(power_wrapper.data_mtx);
			auto st = power_wrapper.data_available.wait_for(lock,
												  std::chrono::milliseconds(200));

			if(st != std::cv_status::timeout && power_wrapper.data)
			{
				power_wrapper.data = nullptr;
			}

			if(launch_queued)
			{
				if(power_wrapper.is_stopped())
				{
					std::cout << "RELAUNCHING" << std::endl;
					power_wrapper.launch();
					launch_queued = false;
				}
			}
		}
	});
}

std::pair<double, double> PlotBuilder::get_spectrum(int i)
{
	return std::make_pair(i * 10e6, i * i);
}

PlotBuilder::PlotBuilder()
{
	// Sane defaults: TODO load last settings
	min_freq = 100.0f;
	max_freq = 110.0f;
	min_freq_units = 2;
	max_freq_units = 2;
	nbins = 256;
	gain = 0;
	percent = 70;
	use_nsamples = true;
	nsamples = 20;
	samplet = 0.01f;

	thread_run = false;
	launch_queued = false;

}

void PlotBuilder::commit_settings()
{
	power_wrapper.stop();
	power_wrapper.set_freq_range(get_freq(min_freq, min_freq_units),
								 get_freq(max_freq, max_freq_units));
	power_wrapper.set_num_bins(nbins);
	power_wrapper.set_overlap(percent);
	if(use_nsamples)
		power_wrapper.set_num_samples(nsamples);
	else
		power_wrapper.set_samp_time(samplet);

	power_wrapper.set_gain(gain);

	launch_queued = true;

}

int64_t PlotBuilder::get_freq(float val, int units)
{
	if(val < 0.0f)
	{
		return 0.0f;
	}

	float mult = 1.0f;
	if(units == 1)
		mult = 1e3f;
	else if(units == 2)
		mult = 1e6f;
	else if(units == 3)
		mult = 1e9f;

	return (int64_t)(val * mult);
}

bool PlotBuilder::can_change_settings()
{
	return !launch_queued;
}

PlotBuilder::~PlotBuilder()
{
	thread_run = false;
	thread.join();
}
