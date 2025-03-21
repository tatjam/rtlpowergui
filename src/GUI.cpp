#include "GUI.h"
#include <imgui_internal.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include "portable-file-dialogs.h"

void GUI::gui_function()
{	
	if( !load_measurement_from_bin ) {}
		pb.update();
	
	
	if(show_menu)
	{
		ImGui::Columns(2);
		tight = ImGui::GetColumnWidth(0) <= 300.0f;

		if(first_run)
		{
			ImGui::SetColumnWidth(-1, 250);
			first_run = false;
		}

		if (ImGui::CollapsingHeader("Connection", ImGuiTreeNodeFlags_DefaultOpen)) //
		{
			ImGui::Text("rtl_power_fftw %s", pb.get_power_status() ? "Not connected" : "Connected");	
		}

		if (ImGui::CollapsingHeader("Ranges", ImGuiTreeNodeFlags_DefaultOpen))
		{
			do_ranges_menu();
		}

		if(ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen))
		{
			do_display_menu();
		}

		if (ImGui::CollapsingHeader("Import binary data", ImGuiTreeNodeFlags_OpenOnArrow))
		{
			do_import_menu();
		}

		if (ImGui::CollapsingHeader("Export", ImGuiTreeNodeFlags_OpenOnArrow))
		{
			do_export_menu();
		}

		ImGui::NextColumn();
		do_plot();
	}
}

void GUI::do_connection_menu()
{

}

void GUI::do_ranges_menu()
{
	bool disabled = !pb.can_change_settings();
	if(disabled)
		ImGui::BeginDisabled();

	ImGui::PushItemWidth(200.0f - 80.0f);
	neat_element("Min. Freq");
	ImGui::InputFloat("##minfreq", &pb.exposed.min_freq);
	ImGui::PushItemWidth(70.0f);
	ImGui::SameLine();
	ImGui::Combo("##minfrequnits", &pb.exposed.min_freq_units, units, IM_ARRAYSIZE(units));
	ImGui::PopItemWidth();
	neat_element("Max. Freq");
	ImGui::InputFloat("##maxfreq", &pb.exposed.max_freq);
	ImGui::SameLine();
	ImGui::PushItemWidth(70.0f);
	ImGui::Combo("##maxfrequnits", &pb.exposed.max_freq_units, units, IM_ARRAYSIZE(units));
	ImGui::PopItemWidth();
	ImGui::PopItemWidth();

	ImGui::PushItemWidth(200.0f);
	neat_element("Gain");
	ImGui::SliderFloat("##gain", &pb.exposed.gain, 0.0f, 50.0f);
	neat_element("Bin. Num");
	ImGui::InputInt("##bins", &pb.exposed.nbins);
	neat_element("Sample. Num");
	ImGui::InputInt("##samp", &pb.exposed.nsamples);
	neat_element("Overlap %%");
	ImGui::InputInt("##overlap", &pb.exposed.percent);

	if (ImGui::Button("Commit settings"))
	{
		pb.commit_settings();
		if(update_view)
			update_view_now = true;
	}
	ImGui::SameLine();
	ImGui::Checkbox("Update view", &update_view);
	if(disabled)
		ImGui::EndDisabled();

	ImGui::BeginDisabled(pb.has_baseline() || !pb.baseline.has_value());
	if(ImGui::Button("Match baseline"))
	{
		pb.exposed = pb.baseline.value().settings;
	}
	ImGui::EndDisabled();
	if(!pb.has_baseline() && pb.baseline.has_value())
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.5, 0.05, 0.05, 1.0), "Settings don't match baseline!");
	}
	if(pb.has_baseline())
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.0, 0.0, 0.3, 1.0), "Baseline matched");
	}
	ImGui::PopItemWidth();
}

void GUI::neat_element(const char *name)
{
	if(tight)
	{
		char buf[4];
		strncpy(buf, name, 3);
		buf[3] = '\0';
		ImGui::TextUnformatted(buf);
		ImGui::SameLine();
		ImGui::SetCursorPosX(40.0f);
	}
	else
	{
		ImGui::TextUnformatted(name);
		ImGui::SameLine();
		ImGui::SetCursorPosX(100.0f);
	}
}

void GUI::do_display_menu()
{
	ImGui::PushItemWidth(200.0f);
	if(ImGui::Button("Recenter view"))
	{
		update_view_now = true;
	}

	neat_element("History Len");
	if(ImGui::InputInt("##numhistory", &pb.num_average_hold))
	{
		pb.update_averaging();
	}
	ImGui::Text("Saved: %i", pb.measurement_count);
	ImGui::PopItemWidth();
}

void GUI::do_plot()
{
	ImPlot::BeginPlot("Test", ImVec2(-1, -1), ImPlotFlags_NoTitle | ImPlotFlags_NoFrame);
	ImPlot::SetupAxisFormat(ImAxis_X1, MetricFormatter, (void*)"Hz");
	ImPlot::SetupAxisFormat(ImAxis_Y1, "%g dB");
	if(update_view && update_view_now)
	{
		if(pb.has_baseline())
		{
			ImPlot::SetupAxesLimits(pb.current.get_low_freq(), pb.current.get_high_freq(),
									-30, 50.0, ImPlotCond_Always);
		}
		else
		{
			ImPlot::SetupAxesLimits(pb.current.get_low_freq(), pb.current.get_high_freq(),
									-80.0, 0.0, ImPlotCond_Always);
		}
		update_view_now = false;
	}
	ImPlot::PlotLine("Spectrum", pb.current.spectrum.data(), pb.current.spectrum.size(),
					 pb.current.get_bin_scale(), pb.current.get_low_freq());
	ImPlot::HideNextItem();
	ImPlot::PlotLine("Averages", pb.current.average.data(), pb.current.spectrum.size(),
					 pb.current.get_bin_scale(), pb.current.get_low_freq());
	ImPlot::HideNextItem();
	ImPlot::PlotLine("Maximums", pb.current.max.data(), pb.current.spectrum.size(),
					 pb.current.get_bin_scale(), pb.current.get_low_freq());
	ImPlot::HideNextItem();
	ImPlot::PlotLine("Minimums", pb.current.min.data(), pb.current.spectrum.size(),
					 pb.current.get_bin_scale(), pb.current.get_low_freq());
	ImPlot::EndPlot();
}

void GUI::do_plot_watterflow()
{
	
	ImPlot::SetupAxisFormat(ImAxis_X1, MetricFormatter, (void*)"Hz");
	ImPlot::SetupAxisFormat(ImAxis_Y1, "%g dB");

	ImPlot::BeginPlot("##Plot1",ImVec2(-1, -2),ImPlotFlags_NoMouseText);
	ImPlot::SetupAxes(NULL,NULL,ImPlotAxisFlags_NoTickLabels,ImPlotAxisFlags_Lock);
	ImPlot::SetupAxesLimits(pb.current.get_low_freq(), pb.current.get_high_freq(),
							-80.0, 0.0, ImPlotCond_Always);

	ImPlot::SetupAxisLimits(ImAxis_Y1,0,20);
	ImPlot::SetupAxisFormat(ImAxis_Y1,"%g kHz");
	//ImPlot::PlotHeatmap("##Heat", pb.current.spectrum.data(),pb.current.spectrum.size(), 200, pb.exposed.nbins, 0.0, pb.current.get_bin_scale(), NULL);
	ImGui::SameLine();
    ImPlot::ColormapScale("##Scale", -80.0, 0.0,{100, 320.0f},"%g dB");
	ImPlot::EndPlot();
}


void GUI::do_export_menu()
{
	if(ImGui::Button("To CSV and load..."))
	{
		const std::string& data = pb.current.to_csv();
		auto now = std::chrono::system_clock::now();
		std::time_t cur_time = std::chrono::system_clock::to_time_t(now);
		std::tm* tm = localtime(&cur_time);
		char fname[64];
		size_t num = std::strftime(fname, sizeof(fname), "%Y-%m-%d %H:%M:%S.csv", tm);
		std::ofstream f(fname, std::ios::trunc);
		f << data << std::endl;
		perform_load(pb.current);
	}
	ImGui::SameLine();

	if(ImGui::Button("Load..."))
	{
		auto file = pfd::open_file("Load measurement", ".", {"CSV files", "*.csv"}).result();
		if(!file.empty())
		{
			std::ifstream in(file[0]);
			std::string file_contents;
			in >> file_contents;
			Measurement mes = Measurement::from_csv(file_contents);
			perform_load(mes);
		}
	}

	ImGui::SameLine();

	if(ImGui::Button("Capture..."))
	{
		perform_load(pb.current);
	}

	ImGui::Checkbox("... as baseline + settings", &save_and_load_baseline);

	neat_element("Bline Mode");
	ImGui::Combo("##bline_mode", &pb.baseline_mode, baseline_mode, IM_ARRAYSIZE(baseline_mode));

	ImGui::BeginDisabled(!pb.baseline.has_value());
	if(ImGui::Button("Clear baseline"))
	{
		pb.baseline.reset();
		update_view_now = true;
		save_and_load_baseline = true;
	}
	ImGui::EndDisabled();
}

void GUI::do_import_menu()
{

	if(ImGui::Button("Import file ..."))
	{	
		pb.stop();
		save_and_load_baseline = false;
		load_measurement_from_bin = true;

		auto file = pfd::open_file("Import measurement file", ".", {"Binary data and Metadata", "*.met"}).result();
		
		// Read previously dumped file with metadata
		std::string filename = file[0].erase(file[0].size()-4, 4);

		Measurement::from_binFile_meta( filename + ".met", pb.current );

		Measurement::from_binFile_raw( filename + ".bin", pb.current );
		
		perform_load(pb.current);

		load_measurement_from_bin = false;
	}
}

GUI::GUI()
{
	pb.launch();
	save_and_load_baseline = true;
	load_measurement_from_bin = false;
}

void GUI::perform_load(Measurement& meas)
{
	if(save_and_load_baseline)
	{
		if(pb.has_baseline())
		{
			// Add back the baseline, otherwise we get "cancellation" effect
			for(size_t i = 0; i < meas.spectrum.size(); i++)
			{
				meas.spectrum[i] += pb.baseline.value().get_baseline_bin(pb.baseline_mode)[i];
				meas.average[i] += pb.baseline.value().get_baseline_bin(pb.baseline_mode)[i];
				meas.min[i] += pb.baseline.value().get_baseline_bin(pb.baseline_mode)[i];
				meas.max[i] += pb.baseline.value().get_baseline_bin(pb.baseline_mode)[i];
			}
		}
		pb.exposed = meas.settings;
		pb.baseline = meas;
		pb.commit_settings();
		if(update_view)
			update_view_now = true;

		save_and_load_baseline = false;
	}

	if(load_measurement_from_bin)
	{
		pb.exposed = meas.settings;
		pb.baseline = meas;
		if(update_view)
			update_view_now = true;
		
	}

}

int MetricFormatter(double value, char* buff, int size, void* data) {
	int ret;
    const char* unit = (const char*)data;
    static double v[]      = {1000000000,1000000,1000,1,0.001,0.000001,0.000000001};
    static const char* p[] = {"G","M","k","","m","u","n"};
    if (value == 0) {
        ret = snprintf(buff,size,"0 %s", unit);
        return ret;
    }
    for (int i = 0; i < 7; ++i) {
        if (fabs(value) >= v[i]) {
            ret = snprintf(buff,size,"%g %s%s",value/v[i],p[i],unit);
            return ret;
        }
    }
    ret = snprintf(buff,size,"%g %s%s",value/v[6],p[6],unit);
	return ret;
}

