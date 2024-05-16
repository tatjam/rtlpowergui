#pragma once
#include "RTLPowerWrapper.h"
#include <map>

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

	int num_averages;

	int64_t get_freq(float val, int units);
	std::mutex mtx;
	std::vector<Readout> reads_buffer;
	// To allow overwriting
	std::vector<int> end_of_sweeps;



public:

	double get_bin_center_freq(size_t idx);
	size_t get_bin_for_freq(double freq);

	void update();
	std::atomic<bool> launch_queued = false;

	float min_freq;
	float max_freq;
	// 0 = Hz
	// 1 = kHz
	// 2 = MHz
	// 3 = GHz
	int min_freq_units;
	int gain;
	int max_freq_units;
	int nbins;
	int percent;
	bool use_nsamples;
	int nsamples;
	double samplet;

	void commit_settings();

	bool can_change_settings();

	void launch();
	// Updates dynamic graphs
	//void update(float dt);

	// Returns Hertz / dB/Hz
	std::vector<double> current_measurement;
	int get_num_points();

	double get_low_freq();
	double get_bin_scale();

	// Returns min and length to plot given low and high frequencies
	std::pair<size_t, size_t> get_plot_ranges(double lfreq, double hfreq);

	bool get_power_status() { return power_wrapper.get_exec_status(); }

	PlotBuilder();
	~PlotBuilder();

};
