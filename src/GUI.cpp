#include "GUI.h"
#include <imgui_internal.h>

void GUI::gui_function()
{
	pb.update();
	if(show_menu)
	{
		ImGui::Columns(2);
		tight = ImGui::GetColumnWidth(0) <= 300.0f;

		if(first_run)
		{
			ImGui::SetColumnWidth(-1, 200);
			first_run = false;
		}
		if (ImGui::CollapsingHeader("Connection", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("rtl_power_fftw running? %i", pb.get_power_status());

		}
		if (ImGui::CollapsingHeader("Ranges", ImGuiTreeNodeFlags_DefaultOpen))
		{
			do_ranges_menu();
		}

		if(ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen))
		{
			do_display_menu();
		}
		if (ImGui::CollapsingHeader("Measure", ImGuiTreeNodeFlags_DefaultOpen))
		{
			do_measure_menu();
		}
		if (ImGui::CollapsingHeader("Export"))
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

	ImGui::PopItemWidth();

}

void GUI::neat_element(const char *name)
{
	if(tight)
	{
		char buf[3];
		strncpy(buf, name, 3);
		buf[3] = '\0';
		ImGui::Text(buf);
		ImGui::SameLine();
		ImGui::SetCursorPosX(40.0f);
	}
	else
	{
		ImGui::Text(name);
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
	if(update_view && update_view_now)
	{
		ImPlot::SetupAxesLimits(pb.current.get_low_freq(), pb.current.get_high_freq(),
								-80.0, 0.0, ImPlotCond_Always);
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

void GUI::do_export_menu()
{
	if(ImGui::Button("To CSV"))
	{

	}
	if(ImGui::Button("To CSV (+ Load baseline)"))
	{

	}
	if(ImGui::Button("Screenshot"))
	{

	}


}

void GUI::do_measure_menu()
{
	if(ImGui::Button("Load baseline"))
	{

	}

	neat_element("Bline Mode");
	ImGui::Combo("##bline_mode", &pb.baseline_mode, baseline_mode, IM_ARRAYSIZE(baseline_mode));

}

GUI::GUI()
{
	pb.launch();

}
