#include "pch.h"
#include "tutorials.h"
#include "rasterizer.h"

/* create a window and initialize OpenGL context */
int avenger(const int width, const int height)
{
	Rasterizer rasterizer(width, height, deg2rad(45.0), Vector3(100, -200, 130), Vector3(0, 0, 35), 1.0f, 1000.0f);
	rasterizer.InitDevice();
	rasterizer.InitScenePBR("../data/lod/6887_allied_avenger_gi2.obj");
	rasterizer.LoadBrdfIntTexture("../data/brdf_integration_map_ct_ggx.exr");
	rasterizer.LoadIrradianceTexture("../data/lebombo_irradiance_map.exr");
	rasterizer.LoadEnvTextures({ "../data/lebombo_prefiltered_env_map_001_2048.exr",
								"../data/lebombo_prefiltered_env_map_010_1024.exr",
								"../data/lebombo_prefiltered_env_map_100_512.exr",
								"../data/lebombo_prefiltered_env_map_250_256.exr",
								"../data/lebombo_prefiltered_env_map_500_128.exr",
								"../data/lebombo_prefiltered_env_map_750_64.exr",
								"../data/lebombo_prefiltered_env_map_999_32.exr"

		});
	rasterizer.FinishSetup();
	rasterizer.MainLoop(Vector3(100, -200, 130), true);

	rasterizer.ReleaseDeviceAndScene();

	return 1;

}
int piece(const int width, const int height)
{
	Rasterizer rasterizer(width, height, deg2rad(45.0), Vector3(0, -30, 30), Vector3(0, 0, 0), 1.0f, 1000.0f);
	rasterizer.InitDevice();
	rasterizer.InitScenePBR("../data/kostka/piece_02.obj");
	rasterizer.LoadBrdfIntTexture("../data/brdf_integration_map_ct_ggx.exr");
	rasterizer.LoadEnvTextures({ "../data/lebombo_prefiltered_env_map_001_2048.exr",
								"../data/lebombo_prefiltered_env_map_010_1024.exr",
								"../data/lebombo_prefiltered_env_map_100_512.exr",
								"../data/lebombo_prefiltered_env_map_250_256.exr",
								"../data/lebombo_prefiltered_env_map_500_128.exr",
								"../data/lebombo_prefiltered_env_map_750_64.exr",
								"../data/lebombo_prefiltered_env_map_999_32.exr"

		});
	rasterizer.LoadIrradianceTexture("../data/lebombo_irradiance_map.exr");
	rasterizer.FinishSetup();
	rasterizer.MainLoop(Vector3(0, -20, 60), true);

	rasterizer.ReleaseDeviceAndScene();

	return 1;
}




