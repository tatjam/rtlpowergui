#include "hello_imgui/hello_imgui.h"
#include "implot.h"
#include "PlotBuilder.h"

int main(void)
{
	HelloImGui::RunnerParams imgui_params;
	imgui_params.appWindowParams.windowTitle = "RTL-SDR spectrum analyzer";

	PlotBuilder pb;
	pb.launch();

	bool show_menu = true;
	bool first_run = true;
	bool update_view = true;
	bool update_view_now = true;
	auto gui_function = [
			&show_menu, &pb, &first_run, &update_view, &update_view_now]()
	{
		pb.update();
		const char* units[] = {"Hz", "kHz", "MHz", "GHz"};
		//pb.update(ImGui::GetIO().DeltaTime);
		if(show_menu)
		{
			ImGui::Columns(2);
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
				bool disabled = !pb.can_change_settings();
				if(disabled)
					ImGui::BeginDisabled();
				ImGui::Text("Min");
				ImGui::SameLine();
				ImGui::PushItemWidth(120.0f);
				ImGui::InputFloat("##minfreq", &pb.exposed.min_freq);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				ImGui::PushItemWidth(70.0f);
				ImGui::Combo("##minfrequnits", &pb.exposed.min_freq_units, units, IM_ARRAYSIZE(units));
				ImGui::PopItemWidth();
				ImGui::Text("Max");
				ImGui::SameLine();
				ImGui::PushItemWidth(120.0f);
				ImGui::InputFloat("##maxfreq", &pb.exposed.max_freq);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				ImGui::PushItemWidth(70.0f);
				ImGui::Combo("##maxfrequnits", &pb.exposed.max_freq_units, units, IM_ARRAYSIZE(units));
				ImGui::PopItemWidth();
				ImGui::Text("Gain");
				ImGui::SameLine();
				ImGui::SliderFloat("##gain", &pb.exposed.gain, 0.0f, 50.0f);
				ImGui::Text("Bins");
				ImGui::SameLine();
				ImGui::InputInt("##bins", &pb.exposed.nbins);
				ImGui::Text("Samp");
				ImGui::SameLine();
				ImGui::InputInt("##samp", &pb.exposed.nsamples);
				ImGui::Text("Overlap");
				ImGui::SameLine();
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
			}
			if(ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if(ImGui::Button("Recenter view"))
				{
					update_view_now = true;
				}

				ImGui::Text("Num history");
				ImGui::SameLine();
				if(ImGui::InputInt("##numhistory", &pb.num_average_hold))
				{
					pb.update_averaging();
				}
				ImGui::Text("Saved: %i", pb.measurement_count);

			}
			if (ImGui::CollapsingHeader("Measure", ImGuiTreeNodeFlags_DefaultOpen))
			{

			}
			if (ImGui::CollapsingHeader("Calibrate"))
			{

			}
			if (ImGui::CollapsingHeader("Export"))
			{

			}
			// Settings / measurements
			ImGui::NextColumn();
		}
		float x[3] = {1.0, 2.0, 3.0};
		float y[3] = {0.0, 4.0, -3.0};
		ImPlot::BeginPlot("Test", ImVec2(-1, -1));
		if(update_view && update_view_now)
		{
			ImPlot::SetupAxesLimits(pb.get_low_freq(), pb.get_high_freq(),
									-80.0, 0.0, ImPlotCond_Always);
			update_view_now = false;
		}
		auto limits = ImPlot::GetPlotLimits();
		auto extents = pb.get_plot_ranges(limits.X.Min, limits.X.Max);
		ImPlot::PlotLine("Spectrum", pb.current_measurement.data() + extents.first, extents.second,
						 pb.get_bin_scale(), pb.get_bin_center_freq(extents.first));
		ImPlot::HideNextItem();
		ImPlot::PlotLine("Averages", pb.averaged_measurement.data() + extents.first, extents.second,
						 pb.get_bin_scale(), pb.get_bin_center_freq(extents.first));
		ImPlot::HideNextItem();
		ImPlot::PlotLine("Maximums", pb.max_measurement.data() + extents.first, extents.second,
						 pb.get_bin_scale(), pb.get_bin_center_freq(extents.first));
		ImPlot::HideNextItem();
		ImPlot::PlotLine("Minimums", pb.min_measurement.data() + extents.first, extents.second,
						 pb.get_bin_scale(), pb.get_bin_center_freq(extents.first));
		ImPlot::EndPlot();
	};

	imgui_params.callbacks.ShowGui = gui_function;
	imgui_params.callbacks.PostInit = []()
	{
		ImPlot::CreateContext();
	};
	imgui_params.callbacks.BeforeExit = []()
	{
		ImPlot::DestroyContext();
	};
	imgui_params.callbacks.SetupImGuiStyle = []()
	{
		ImGuiTheme::ApplyTheme(ImGuiTheme::ImGuiTheme_MicrosoftStyle);
	};

	imgui_params.fpsIdling.enableIdling = true;
	imgui_params.fpsIdling.fpsIdle = 30.0f;

	HelloImGui::Run(imgui_params);

	return 0;

}
