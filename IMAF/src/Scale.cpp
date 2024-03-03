#include "IMAF/Scale.h"
#include <cmath>
#include <atomic>

std::atomic<float> xscale = 1.f;
std::atomic<float> yscale = 1.f;

int IMAF::Scale::Pix(int x, bool x_plane)
{
	if (x_plane)
		return std::lround((float)x * (xscale));
	else
		return std::lround((float)x * (yscale));
}

float IMAF::Scale::Pix(float x, bool x_plane)
{
	if (x_plane)
		return (float)x * (xscale);
	else
		return (float)x * (yscale);
}

void IMAF::Scale::ScaleCallback(float x_scale,float y_scale)
{
	xscale = x_scale;
	yscale = y_scale;
}