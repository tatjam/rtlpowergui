#include "PlotBuilder.h"
#include <iostream>
#include <cmath>
#include <limits>
#include <sstream>

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
	exposed.nsamples = 20;
	exposed.samp_rate = 2e6;

	next_is_first = true;

	thread_run = false;
	launch_queued = false;

	num_average_hold = 20;
	update_averaging();

	// Average
	baseline_mode = 1;


}

void PlotBuilder::commit_settings()
{
	// Sanify
	if(exposed.nbins % 2 != 0)
	{
		exposed.nbins++;
	}

	current.settings = exposed;

	power_wrapper.stop();
	// We oversample a bit to prevent "overlap" from behaving weirdly
	power_wrapper.set_freq_range(
			current.get_freq(current.settings.min_freq, current.settings.min_freq_units) -
					(current.settings.percent / 100.0) * current.settings.samp_rate,
			current.get_freq(current.settings.max_freq, current.settings.max_freq_units) +
					(current.settings.percent / 100.0) * current.settings.samp_rate);
	power_wrapper.set_num_bins(current.settings.nbins);
	power_wrapper.set_overlap(current.settings.percent);
	power_wrapper.set_num_samples(current.settings.nsamples);

	power_wrapper.set_gain((int)(current.settings.gain * 10.0));

	launch_queued = true;

	// Clear points
	current.spectrum.clear();
	current.spectrum.resize(std::ceil(current.get_number_of_scans() * current.settings.nbins));
	update_averaging();
}

void PlotBuilder::stop()
{
	power_wrapper.stop();
	launch_queued = false;
}

size_t Measurement::get_number_of_scans()
{
	return std::ceil(get_freq_range() / (double)settings.samp_rate);
}

double Measurement::get_hertz_per_bin()
{
	return (double)settings.samp_rate / (double)settings.nbins;
}

double Measurement::get_bin_center_freq(size_t idx)
{
	return get_low_freq() + idx * get_hertz_per_bin();
}

size_t Measurement::get_bin_for_freq(double freq)
{
	return std::round((freq - get_low_freq()) / get_hertz_per_bin());
}

int64_t Measurement::get_freq(float val, int units)
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
		if(sc.is_first_of_scan)
		{
			if(measurement_count < num_average_hold)
			{
				measurement_count++;
			}
		}
		for (size_t i = 0; i < sc.reads.size(); i++)
		{
			size_t bin = current.get_bin_for_freq(sc.reads[i].freq);
			// Preserve only upper percent of scan
			int num_skip_below = (current.settings.nbins * current.settings.percent) / 100;
			// Upper side will be overwritten by next one anyway, so don't write it!
			// First scan of sweep cannot be clipped!
			if (sc.is_first_of_scan)
			{
				num_skip_below = 0;
			}
			if (i >= num_skip_below && bin <= current.spectrum.size() - 1)
			{
				current.spectrum[bin] = sc.reads[i].power;

				if(measurement_count > 0)
				{
					// Insert current measurement to FIFO
					prev_measurements[measurement_count - 1][bin] = current.spectrum[bin];

					// Move back FIFO for this sample
					for (int j = 0; j < measurement_count - 1; j++)
					{
						prev_measurements[j][bin] = prev_measurements[j + 1][bin];
					}

					// Current value is average of all in array
					current.average[bin] = 0;
					current.max[bin] = -9999;
					current.min[bin] = 9999;
					for (int j = 0; j < measurement_count; j++)
					{
						current.average[bin] += prev_measurements[j][bin];
						current.max[bin] = std::max(prev_measurements[j][bin], current.max[bin]);
						current.min[bin] = std::min(prev_measurements[j][bin], current.min[bin]);
					}
					current.average[bin] /= measurement_count;

				}

				if(has_baseline())
				{
					current.spectrum[bin] -= baseline.value().get_baseline_bin(baseline_mode)[bin];
					current.max[bin] -= baseline.value().get_baseline_bin(baseline_mode)[bin];
					current.min[bin] -= baseline.value().get_baseline_bin(baseline_mode)[bin];
					current.average[bin] -= baseline.value().get_baseline_bin(baseline_mode)[bin];
				}
			}
		}
	}
	reads_buffer.clear();
	mtx.unlock();
}

double Measurement::get_high_freq()
{
	return get_freq(settings.max_freq, settings.max_freq_units);
}

double Measurement::get_low_freq()
{
	return get_freq(settings.min_freq, settings.min_freq_units);
}

double Measurement::get_bin_scale()
{	
	double hertz_per_bin = (double)settings.samp_rate / (double)settings.nbins;
	return hertz_per_bin;
}

std::string Measurement::to_csv()
{
	std::stringstream o;
	settings.to_stringstream(o);
	o << "freq,spectrum,avg,max,min" << std::endl;
	// Write each measurement in bin mode, with meta-data for users who analyze otherwise
	for(size_t i = 0; i < spectrum.size(); i++)
	{
		o <<
			get_bin_center_freq(i) << "," <<
			spectrum[i] << "," <<
			average[i] << "," <<
			max[i] << "," <<
			min[i] << std::endl;
	}
	return o.str();
}

Measurement Measurement::from_csv(const std::string &str)
{
	return Measurement();
}

/*
	Read information about recorded spectrum from metafile 
*/
void Measurement::from_binFile_meta(const std::string& fname, Measurement& binaryData)
{
	/*
		Sample recorded metafile:
		5000 # frequency bins (columns)
		85 # scans (rows)
		88000000 # startFreq (Hz)
		107996000 # endFreq (Hz)
		4000 # stepFreq (Hz)
		0.0025 # effective integration time secs
		0.0352941 # avgScanDur (sec)
		2025-03-02 11:41:45 UTC # firstAcqTimestamp UTC
		2025-03-02 11:42:45 UTC # lastAcqTimestamp UTC
	*/

	if( fname.empty())
	{
		throw std::runtime_error("Cannot open empty *.met file: " + fname);
	}
	
	std::ifstream cfg_file(fname);
	std::unordered_map<std::string, std::string> metaData;

    if(!cfg_file.good())
	{
		throw std::runtime_error("Cannot open *.met file: " + fname);
	}
 
    std::string line;
    while(std::getline(cfg_file, line))
    {
        std::regex re(R"XXX(^(\d+.*)\s*#\s*(\S+))XXX", std::regex::optimize);
        std::smatch match;
        if(std::regex_search(line, match, re))
        {
            if(match.length(2))
            {
                std::string key = match.str(2), value = match.str(1);
                std::cout << "Settings in metafile: " << key << " -> " << value << std::endl;
				metaData[key]=value;
            }
        }
        else
            throw std::runtime_error("Invalid line in file: " + fname + " -> " + line);
    }

	binaryData.settings.min_freq = std::stoi(metaData.at("startFreq"));
	binaryData.settings.max_freq = std::stoi(metaData.at("endFreq"));
	binaryData.settings.nbins    = std::stoi(metaData.at("frequency"));
	binaryData.stepFreq          = std::stoi(metaData.at("stepFreq"));
	binaryData.numScans = std::stoi(metaData.at("scans")); 

	// Set default settings
	binaryData.settings.min_freq_units = 0;
	binaryData.settings.max_freq_units = 0;
	binaryData.settings.gain           = 20;
	binaryData.settings.percent        = 0;
	binaryData.settings.nsamples       = 20;

	// hertz_per_bin = (double)settings.samp_rate / (double)settings.nbins;
	// Calculate sample_rate based on information from metafile 
	binaryData.settings.samp_rate      = binaryData.stepFreq * binaryData.settings.nbins; //2e6;

	// Clear all previous measurements
	binaryData.spectrum.clear();
	binaryData.spectrum.resize(binaryData.settings.nbins);

	binaryData.average.clear();
	binaryData.average.resize(binaryData.spectrum.size());

	binaryData.max.clear();
	binaryData.max.resize(binaryData.spectrum.size());

	binaryData.min.clear();
	binaryData.min.resize(binaryData.spectrum.size());
}

/*
	Read raw data from *.bin file 
*/
void Measurement::from_binFile_raw(const std::string& fname, Measurement& binaryData)
{
	if( fname.empty())
	{
		throw std::runtime_error("Cannot open empty *.bin file: " + fname);
	}

	std::ifstream bin_file(fname, std::ios::binary);
	if(!bin_file.good())
	{
		throw std::runtime_error("Cannot open *.bin file: " + fname);
	}
	 
	double f;
	std::vector<float> buf((binaryData.numScans * binaryData.settings.nbins));    // create buffer
	bin_file.read(reinterpret_cast<char*>(buf.data()), buf.size()*sizeof(float)); // read all binary data from file to buffer

	double meas;
	for( size_t n = 0; n < binaryData.numScans; n++ )
	{
		std::cout << "Scan load: "<< n << std::endl;
		for( size_t i = 0; i < binaryData.settings.nbins; i++ )
		{	
			meas = buf[i+(n * binaryData.settings.nbins)];
			binaryData.spectrum[i] = meas;
			if ( n==0 ) // calculate avarage based on first scan
			{
				binaryData.average[i] = meas;
				binaryData.max[i] = meas;
				binaryData.min[i] = meas;
			} else {
				binaryData.average[i] += meas ;
				binaryData.max[i] = std::max(meas, binaryData.max[i]);
				binaryData.min[i] = std::min(meas, binaryData.min[i]);
			}
		}
	}
		
	for( size_t i = 0; i < binaryData.settings.nbins; i++ )
	{
		binaryData.average[i] /= binaryData.numScans ;
	}
		
}

std::vector<double>& Measurement::get_baseline_bin(int baseline_mode)
{
	if(baseline_mode == 0)
		return spectrum;
	else if(baseline_mode == 1)
		return average;
	else if(baseline_mode == 2)
		return max;
	else
		return min;
}

void PlotBuilder::update_averaging()
{
	prev_measurements.clear();
	prev_measurements.resize(num_average_hold);
	for(size_t i = 0; i < num_average_hold; i++)
	{
		prev_measurements[i].resize(current.spectrum.size());
	}

	current.average.clear();
	current.average.resize(current.spectrum.size());
	current.max.resize(current.spectrum.size());
	current.min.resize(current.spectrum.size());
	measurement_count = 0;
}

bool PlotBuilder::has_baseline()
{
	return baseline.has_value() && (baseline.value().settings == current.settings);
}

void Settings::to_stringstream(std::stringstream& outs)
{

	outs << samp_rate << std::endl;
	outs << min_freq << std::endl;
	outs << min_freq_units << std::endl;
	outs << max_freq << std::endl;
	outs << max_freq_units << std::endl;
	outs << gain << std::endl;
	outs << nbins << std::endl;
	outs << percent << std::endl;
	outs << nsamples << std::endl;

}

Settings Settings::from_stringstream(std::stringstream& ss)
{
	Settings out;
	ss >> out.samp_rate >> out.min_freq >> out.min_freq_units >> out.max_freq >> out.max_freq_units >>
		out.gain >> out.nbins >> out.percent >> out.nsamples;
	return out;
}

bool Settings::operator==(const Settings &b) const
{
	return min_freq == b.min_freq && max_freq == b.max_freq && min_freq_units && b.min_freq_units
			&& max_freq_units == b.max_freq_units && b.percent == percent;
}
