#pragma once
#include <thread>
#include <condition_variable>
#include <atomic>
#include <vector>

struct Readout
{
	double freq;
	double power;
};

struct RTLPowerData
{
	bool is_end_of_sweep;
	std::vector<Readout> reads;
};

// To prevent needless overhead, we only re-launch the power software
// when parameters (start and end frequency / bins) are changed.
// So the whole thing runs in a thread and returns data back gradually to the rest of the software
class RTLPowerWrapper
{
private:

	// In Hertz
	uint64_t min_f, max_f;
	int nbins;
	int overlap_percent;

	bool use_stime;
	double stime;
	int gain;
	int snum;

	std::thread thread;

	std::atomic<bool> thread_run;
	std::atomic<bool> closing;

	bool skipped_prev;
	void process_line(const std::string& line);

public:


	// Will stop after next run is done
	void stop();

	// The following functions can only be called if stopped / stopping
	void set_freq_range(uint64_t min, uint64_t max);
	void set_num_bins(int nbins);
	void set_overlap(int percent);
	void set_num_samples(int nsamples);
	double set_samp_time(double stime);

	void launch();

	// This cvar will be notified once data is available, use
	// the data_mtx to lock it
	std::condition_variable data_available;
	std::mutex data_mtx;

	// Set to point to data if new data is available (to prevent spurious wake up, use
	// as predicate). Handle / copy data and set back to nullptr (do not free!)
	RTLPowerData* data;
	// Sample data is written here, copy it and continue operation on data available
	RTLPowerData front_buffer;
	RTLPowerData back_buffer;
};
