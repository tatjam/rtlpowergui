#include "hello_imgui/hello_imgui.h"

int main(void)
{
	HelloImGui::RunnerParams imgui_params;
	imgui_params.appWindowParams.windowTitle = "RTL-SDR spectrum analyzer";

	auto gui_function = []()
	{
		ImGui::Columns(2);
		// Settings / measurements
		ImGui::NextColumn();
	};

	imgui_params.callbacks.ShowGui = gui_function;
	imgui_params.callbacks.SetupImGuiStyle = []()
	{
		ImGuiTheme::ApplyTheme(ImGuiTheme::ImGuiTheme_MicrosoftStyle);
	};

	HelloImGui::Run(imgui_params);

	return 0;

}
