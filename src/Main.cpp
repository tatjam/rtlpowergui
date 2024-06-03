#include "GUI.h"

int main(void)
{
	HelloImGui::RunnerParams imgui_params;
	imgui_params.appWindowParams.windowTitle = "RTL-SDR spectrum analyzer";

	GUI gui;


	imgui_params.callbacks.ShowGui = [&gui]() {gui.gui_function(); };
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
