#pragma once

#define PixX(x) IMAF::Scale::Pix(x,true)
#define PixY(y) IMAF::Scale::Pix(y,false)
#define Pixs(x,y) PixX(x),PixY(y)

namespace IMAF 
{
	namespace Scale 
	{
		//struct ScreenSize
		//{
		//	int x;
		//	int y;

		//	ScreenSize(int x, int y) : x(x), y(y) {};
		//	ScreenSize() : x(0), y(0) {};

		//	float Ratio(const ScreenSize& other)
		//	{
		//		return ((float)other.x / (float)x) < ((float)other.y / (float)y) ? ((float)other.x / (float)x) : ((float)other.y / (float)y);
		//	}
		//};

		//The base_size is the screen size which the application will be developed for. If the screen size is different, the values will scale accordingly.
		//void Init(HWND application_handle, bool perfect_scale = false, ScreenSize base_size = { 1920,1080 });

		int Pix(int x, bool x_plane = true);
		float Pix(float x, bool x_plane = true);

		//This function should be passed to 'AddScaleCallback'
		void ScaleCallback(float xscale,float yscale);
	}
}