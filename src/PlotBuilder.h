#pragma once
#include "RTLPowerWrapper.h"

// Runs in a thread to build the raw plots for ImGui
// without blocking (uses double buffering too)
// and uses the RTLPowerWrapper to get its data
class PlotBuilder
{
public:
	std::vector<float> points;

};
