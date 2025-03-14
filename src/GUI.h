#pragma once
#include "hello_imgui/hello_imgui.h"
#include "implot.h"
#include "PlotBuilder.h"

int MetricFormatter(double value, char* buff, int size, void* data);

class GUI
{
private:
	PlotBuilder pb;

	constexpr static const char* units[] = {"Hz", "kHz", "MHz", "GHz"};
	constexpr static const char* baseline_mode[] = {"Spectrum", "Average", "Max", "Min"};
	bool save_and_load_baseline;
	bool load_measurement_from_bin;

	bool tight = false;
	bool show_menu = true;
	bool first_run = true;
	bool update_view = true;
	bool update_view_now = true;

	void do_import_menu();
	void do_export_menu();
	void do_connection_menu();
	void do_ranges_menu();
	void do_display_menu();
	void do_plot();
	void do_plot_watterflow();
	void neat_element(const char* name);

	void perform_load(Measurement& meas);
public:


	void gui_function();
	GUI();
};