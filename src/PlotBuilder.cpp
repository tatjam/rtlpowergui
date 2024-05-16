#include "PlotBuilder.h"
#include <iostream>
#include <cmath>

int PlotBuilder::get_num_points()
{
	return current_measurement.size();
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
				mtx.lock();
				reads_buffer.insert(reads_buffer.end(), power_wrapper.data->reads.begin(), power_wrapper.data->reads.end());
				if(power_wrapper.data->is_end_of_sweep)
				{
					end_of_sweeps.push_back(reads_buffer.size());
				}
				mtx.unlock();
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

PlotBuilder::PlotBuilder()
{
	// Sane defaults: TODO load last settings
	min_freq = 100.0f;
	max_freq = 110.0f;
	min_freq_units = 2;
	max_freq_units = 2;
	nbins = 256;
	gain = 0;
	percent = 0;
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

	// Clear points
	current_measurement.clear();
	double df = get_freq(max_freq, max_freq_units) - get_freq(min_freq, min_freq_units);

	// The number of bins is the number of FFT bins per scan
	// but the total bandwidth of the device is 2MHz (TODO: Changeable)
	// (With overlap factor)
	double hertz_per_scan = 2e6 * ((100 - percent) / 100.0);
	double scans = df / hertz_per_scan;
	double hertz_per_bin = hertz_per_scan / (double)nbins;

	current_measurement.resize(nbins * std::ceil(scans * nbins));
}

double PlotBuilder::get_bin_center_freq(size_t idx)
{
	double df = get_freq(max_freq, max_freq_units) - get_freq(min_freq, min_freq_units);
	double hertz_per_scan = 2e6 * ((100 - percent) / 100.0);
	double scans = df / hertz_per_scan;
	double hertz_per_bin = hertz_per_scan / (double)nbins;

	return get_freq(min_freq, min_freq_units) + idx * hertz_per_bin;
}

size_t PlotBuilder::get_bin_for_freq(double freq)
{
	double df = get_freq(max_freq, max_freq_units) - get_freq(min_freq, min_freq_units);
	double hertz_per_scan = 2e6 * ((100 - percent) / 100.0);
	double scans = df / hertz_per_scan;
	double hertz_per_bin = hertz_per_scan / (double)nbins;

	return std::round((freq - get_freq(min_freq, min_freq_units)) / hertz_per_bin);
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

void PlotBuilder::update()
{
	mtx.lock();
	for(size_t i = 0; i < reads_buffer.size(); i++)
	{
		size_t bin = get_bin_for_freq(reads_buffer[i].freq);
		//std::cout << "Freq: " << reads_buffer[i].freq << " intens: " << reads_buffer[i].power << " to bin " << bin << std::endl;
		current_measurement[bin] = reads_buffer[i].power;
	}
	reads_buffer.clear();
	end_of_sweeps.clear();
	mtx.unlock();

}

double PlotBuilder::get_low_freq()
{
	return get_freq(min_freq, min_freq_units);
}

double PlotBuilder::get_bin_scale()
{
	double df = get_freq(max_freq, max_freq_units) - get_freq(min_freq, min_freq_units);
	double hertz_per_scan = 2e6 * ((100 - percent) / 100.0);
	double scans = df / hertz_per_scan;
	double hertz_per_bin = hertz_per_scan / (double)nbins;
	return hertz_per_bin;
}

std::pair<size_t, size_t> PlotBuilder::get_plot_ranges(double lfreq, double hfreq)
{
	int low = get_bin_for_freq(lfreq);
	int high = get_bin_for_freq(hfreq);
	if(low < 0)
	{
		low = 0;
	}
	if(high > current_measurement.size() - 1)
	{
		high = current_measurement.size() - 1;
	}

	return std::make_pair(low, high - low);

}


