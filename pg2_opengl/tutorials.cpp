#include "pch.h"
#include "tutorials.h"
#include "rasterizer.h"

/* create a window and initialize OpenGL context */
int tutorial_1(const int width, const int height)
{
	Rasterizer rasterizer(width, height, deg2rad(45.0), Vector3(100, -200, 80), Vector3(0, 0, 35), 1.0f, 1000.0f);
	rasterizer.InitDeviceAndScenePBR("../data/lod/6887_allied_avenger_gi2.obj");
	rasterizer.MainLoop( Vector3(0,0,400), true);

	/*Rasterizer rasterizer(width, height, deg2rad(45.0), Vector3(0, -30, 30), Vector3(0, 0, 0), 1.0f, 1000.0f);
	rasterizer.InitDeviceAndScenePBR("../data/kostka/piece_02.obj");
	rasterizer.MainLoop(Vector3(0, 100, 50),true);*/

	rasterizer.ReleaseDeviceAndScene();
	
	return 1;

}




