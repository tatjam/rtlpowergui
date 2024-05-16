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
				Scan sc;
				sc.is_last_of_scan = false;
				sc.is_first_of_scan = false;
				sc.reads.insert(sc.reads.end(), power_wrapper.data->reads.begin(), power_wrapper.data->reads.end());
				if(next_is_first)
				{
					sc.is_first_of_scan = true;
					next_is_first = false;
				}
				if(power_wrapper.data->is_end_of_sweep)
				{
					sc.is_last_of_scan = true;
					next_is_first = true;
				}
				reads_buffer.push_back(sc);
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
					next_is_first = true;
				}
			}
		}
	});
}

PlotBuilder::PlotBuilder()
{
	// Sane defaults: TODO load last settings
	exposed.min_freq = 100.0f;
	exposed.max_freq = 110.0f;
	exposed.min_freq_units = 2;
	exposed.max_freq_units = 2;
	exposed.nbins = 256;
	exposed.gain = 0;
	exposed.percent = 0;
	exposed.use_nsamples = true;
	exposed.nsamples = 20;
	exposed.samplet = 0.01f;
	exposed.samp_rate = 2e6;

	next_is_first = true;

	thread_run = false;
	launch_queued = false;

}

	void PlotBuilder::commit_settings()
	{
		// Sanify
		if(exposed.nbins % 2 != 0)
		{
			exposed.nbins++;
		}

		commited = exposed;

		power_wrapper.stop();
		// We oversample a bit to prevent "overlap" from behaving weirdly
		power_wrapper.set_freq_range(
				get_freq(commited.min_freq, commited.min_freq_units) -
						(commited.percent / 100.0) * commited.samp_rate,
				get_freq(commited.max_freq, commited.max_freq_units) +
						(commited.percent / 100.0) * commited.samp_rate);
		power_wrapper.set_num_bins(commited.nbins);
		power_wrapper.set_overlap(commited.percent);
		if(commited.use_nsamples)
			power_wrapper.set_num_samples(commited.nsamples);
		else
			power_wrapper.set_samp_time(commited.samplet);

		power_wrapper.set_gain((int)(commited.gain * 10.0));

		launch_queued = true;

		// Clear points
		current_measurement.clear();
		current_measurement.resize(std::ceil(get_number_of_scans() * commited.nbins));
	}

	size_t PlotBuilder::get_number_of_scans()
	{
		return std::ceil(get_freq_range() / (double)commited.samp_rate);
	}

	double PlotBuilder::get_hertz_per_bin()
	{
		return (double)commited.samp_rate / (double)commited.nbins;
	}

	double PlotBuilder::get_bin_center_freq(size_t idx)
	{
		return get_low_freq() + idx * get_hertz_per_bin();
	}

	size_t PlotBuilder::get_bin_for_freq(double freq)
	{
		return std::round((freq - get_low_freq()) / get_hertz_per_bin());
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
		for(const auto& sc : reads_buffer)
		{
			for (size_t i = 0; i < sc.reads.size(); i++)
			{
				size_t bin = get_bin_for_freq(sc.reads[i].freq);
				// Preserve only upper percent of scan
				int num_skip_below = (commited.nbins * commited.percent) / 100;
				// Upper side will be overwritten by next one anyway, so don't write it!
				// First scan of sweep cannot be clipped!
				if (sc.is_first_of_scan)
				{
					num_skip_below = 0;
				}
				if (i >= num_skip_below)
				{
					if (bin <= current_measurement.size() - 1)
						current_measurement[bin] = sc.reads[i].power;
				}
			}
		}
		reads_buffer.clear();
		mtx.unlock();

	}

	double PlotBuilder::get_high_freq()
	{
		return get_freq(commited.max_freq, commited.max_freq_units);
	}

	double PlotBuilder::get_low_freq()
	{
		return get_freq(commited.min_freq, commited.min_freq_units);
	}

	double PlotBuilder::get_bin_scale()
	{
		double hertz_per_bin = (double)commited.samp_rate / (double)commited.nbins;
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






