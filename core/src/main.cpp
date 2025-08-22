#include "glow.h"

int main() {
    Glow::Glow_Init_Info init_info {};
	init_info.window_name = "GLOW";
	init_info.width = 1600;
	init_info.height = 900;
	init_info.model_base_path = "../resources/models/";
	init_info.texture_base_path = "TODO";
	//const char* scene_path; todo

    if (Glow::init(init_info))
        return 1;

    Glow::incandesce();
    Glow::shutdown();

    return 0;
}
