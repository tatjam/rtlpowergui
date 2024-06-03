#pragma once
#include "RTLPowerWrapper.h"
#include <map>

struct Settings
{
	int samp_rate;
	float min_freq;
	float max_freq;
	// 0 = Hz
	// 1 = kHz
	// 2 = MHz
	// 3 = GHz
	int min_freq_units;
	float gain;
	int max_freq_units;
	int nbins;
	int percent;
	bool use_nsamples;
	int nsamples;
	double samplet;
};

struct Scan
{
	std::vector<Readout> reads;
	bool is_first_of_scan;
	bool is_last_of_scan;
};

struct Measurement
{
	Settings settings;
	std::vector<double> spectrum;
	std::vector<double> average;
	std::vector<double> max;
	std::vector<double> min;
};

// Runs in a thread to build the raw plots for ImGui
// with no blocking
// and uses the RTLPowerWrapper to get its data
// Basically the whole application except GUI is here
class PlotBuilder
{
private:
	RTLPowerWrapper power_wrapper;
	std::thread thread;
	bool thread_run;
	// We neatly subdivide the freq spectrum, and round samples
	// (this will nearly never be an issue)

	int64_t get_freq(float val, int units);
	std::mutex mtx;
	std::vector<Scan> reads_buffer;
	std::atomic<bool> next_is_first;

	double bandwidths[4] =
			{
			100e3,
			1e6,
			2e6
			};

public:

	int baseline_mode;

	double get_bin_center_freq(size_t idx);
	size_t get_bin_for_freq(double freq);

	void update();
	std::atomic<bool> launch_queued = false;

	Settings exposed;

	void commit_settings();
	void update_averaging();

	bool can_change_settings();

	void launch();
	// Updates dynamic graphs
	//void update(float dt);

	// Returns Hertz / dB/Hz
	Measurement current;
	int num_average_hold;
	// Number of measurements since last update_averaging
	// growing until it's equal to num_average_hold
	int measurement_count;
	std::vector<std::vector<double>> prev_measurements;

	int get_num_points();

	double get_low_freq();
	double get_high_freq();
	double get_freq_range() { return get_high_freq() - get_low_freq(); }
	size_t get_number_of_scans();
	double get_hertz_per_bin();
	double get_bin_scale();

	// Returns min and length to plot given low and high frequencies
	std::pair<size_t, size_t> get_plot_ranges(double lfreq, double hfreq);

	bool get_power_status() { return power_wrapper.get_exec_status(); }

	PlotBuilder();
	~PlotBuilder();

};
