#pragma once
#include "RTLPowerWrapper.h"

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
	std::vector<double> current_measurement;
	std::vector<double> average_measurement;

	int num_averages;

	int64_t get_freq(float val, int units);

public:
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
	std::pair<double, double> get_spectrum(int i);
	int get_num_points();

	bool get_power_status() { return power_wrapper.get_exec_status(); }

	PlotBuilder();
	~PlotBuilder();

};
