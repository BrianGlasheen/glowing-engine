#pragma once

#include <cstdint>

//#include "glow_config.h"

namespace Glow {
	struct Glow_Init_Info {
		const char* window_name;
		uint32_t width;
		uint32_t height;
		const char* model_base_path;
		const char* texture_base_path;
		//const char* scene_path; todo
	};

	bool init(const Glow_Init_Info& init_info);
	void incandesce();
	void shutdown();
}