#include "glow.h"

#include "core/window.h"
#include "core/renderer.h"
#include "core/entity.h"
#include "core/scene.h"
#include "core/physics.h"
#include "core/audio.h"
#include "core/editor.h"

#include "asset/crosshair.h"
#include "asset/text.h"
#include "asset/texture_manager.h"
#include "asset/model_manager.h"
#include "asset/particle_manager.h"

#include "player/player.h"
#include "util/colors.h"
#include "util/profiler.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>


#include <vector>

namespace Glow {
	float delta_time = 0.0f;
	float last_frame = 0.0f;
	bool editor_mode = 0;

	Window window;
	Renderer renderer;
	Editor editor;
	Player player;

	//Crosshair crosshair(1.0f, 6.0f, 10.0f, 10.0f, 1.0f, glm::vec3(1.0f, 0.5f, 1.0f));
	Scene scene; //.todo move lights here?
	ImGuiUtils::ProfilerGraph gpuGraph(300);

	bool init(const Glow_Init_Info& init_info) {
		if (window.init(init_info.width, init_info.height, init_info.window_name))
			return -1;

		if (renderer.init())
			return -1;

		Texture_Manager::init();

		renderer.setup();

		Particle_Manager::init();

		if (editor.init())
			return -1;

		Audio::init();
		Physics::init();
		Model_Manager::init(std::string(init_info.model_base_path));

		player.init(); // after physics
		window.sync_callbacks(player, renderer, editor, editor_mode);

		// bool loaded = Model_Manager::load_model_indirect("Sponza/glTF/Sponza.gltf");
		//bool loaded = Model_Manager::load_model_indirect("ABeautifulGame/glTF/ABeautifulGame.gltf");

		//bool loaded = Model_Manager::load_model_indirect("f22/scene.gltf");
		//bool loaded = Model_Manager::load_model_indirect("track/scene.gltf");
		//bool loaded = Model_Manager::load_model_indirect("plane.obj");
		//bool loaded = Model_Manager::load_model_indirect("sword2/scene.gltf");
		//bool loaded = Model_Manager::load_model_indirect("astonmartin/scene.gltf");
		//bool loaded = Model_Manager::load_model_cgltf();
		//bool loaded = Model_Manager::load_model_indirect("MetalRoughSpheres/glTF/MetalRoughSpheres.gltf");

		//Model_Manager::upload_data();
		//printf(loaded ? "good\n" : "bad\n");
		scene.load_from_file("../resources/scenes/scene.yaml");

		//model_handle raccoon3 = Model_Manager::load_animated_model_cgltf("glock/scene.gltf");
		//model_handle raccoon4 = Model_Manager::load_animated_model_cgltf("glock2/scene.gltf");
		//model_handle raccoon6 = Model_Manager::load_animated_model("hkm23/scene.gltf");
		//model_handle raccoon6 = Model_Manager::load_animated_model("hkm23/scene.gltf");
		//model_handle raccoon3 = Model_Manager::load_animated_model("hkm23/scene.gltf");
		//model_handle raccoon6 = Model_Manager::load_animated_model_cgltf("hkm23/scene.gltf");
		//model_handle raccoon6ddd = Model_Manager::load_animated_model("hkm23/source/Mark23.fbx");

		// Entity raccoon226 = Entity::Animated_Entity(glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "mp5/scene.gltf", false);
		// scene.include(raccoon226);
		// Entity raccoon226 = Entity::Animated_Entity(glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "akakak/scene.gltf", false);
		// scene.include(raccoon226);

		// Entity raccoon5 = Entity::Animated_Entity(glm::vec3(5.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "glockhell/scene.gltf", false);
		// scene.include(raccoon5);

		// Entity raccoon233323 = Entity(glm::vec3(0.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "goku/scene.gltf", false);
		// scene.include(raccoon233323);

		// Entity raccoon2333233 = Entity(glm::vec3(5.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(3.0f), "924/scene.gltf", false);
		// scene.include(raccoon2333233);


		//model_handle raccoon26 = Model_Manager::load_animated_model_cgltf("vector/scene.gltf");
		
		// Entity e5555 = Entity::Animated_Entity(glm::vec3(10.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(0.025f), "tiger/scene.gltf", false);
		// scene.include(e5555);

		// Entity e55553 = Entity::Animated_Entity(glm::vec3(-10.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(0.5f), "tiger/scene.gltf", false);
		// scene.include(e55553);

		// Entity e555533 = Entity::Animated_Entity(glm::vec3(0.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "tiger/scene.gltf", false);
		// scene.include(e555533);

		// Entity e2323 = Entity(glm::vec3(50.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "tiger/scene.gltf", false);
		// scene.include(e2323);

		// Entity raccoon23333 = Entity(glm::vec3(-15.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(100.0f), "goku/scene.gltf", false);
		// scene.include(raccoon23333);

		// Entity raccoon233332 = Entity(glm::vec3(15.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(100.0f), "goku/scene.gltf", false);
		// scene.include(raccoon233332);

		//Entity raccoon23333232 = Entity::Animated_Entity(glm::vec3(20.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "tiger/scene.gltf", false);
		//scene.include(raccoon23333232);

		// Entity raccoon2333323 = Entity(glm::vec3(50.0f, 25.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(10.0f), "tiger/scene.gltf", false);
		// scene.include(raccoon2333323);
		
		// Entity e55552 = Entity::Animated_Entity(glm::vec3(0.0f, 0.0f, 0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "AnimatedCube/glTF/AnimatedCube.gltf", false);
		// scene.include(e55552);


	/*	Entity d23 = Entity(glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "tiger/scene.gltf", false);
		scene.include(d23);
		printf("here\n");*/

		//model_handle raccoon2 = Model_Manager::load_animated_model_cgltf("glock2/scene.gltf");
		
		//model_handle raccoon2333333 = Model_Manager::load_animated_model("m4a1/scene.gltf");
		//model_handle raccoon2333d333 = Model_Manager::load_animated_model_cgltf("akm/scene.gltf");
		//model_handle raccoon2333d333 = Model_Manager::load_animated_model("pistol2/source/Glock_Anim.fbx");


		//Model_Manager::compare_animation_data(0, 1);
		//model_handle raccoon = Model_Manager::load_animated_model_cgltf("tiger/scene.gltf");

		// model_handle raccoon3 = Model_Manager::load_animated_model("tiger/scene.gltf");
		//model_handle raccoon5 = Model_Manager::load_animated_model("gun/scene.gltf");
		//model_handle raccoon6 = Model_Manager::load_animated_model("glockhell/glock.gltf");
		//model_handle raccoondsds6 = Model_Manager::load_animated_model_cgltf("pistol/scene.gltf");
		//model_handle raccoon4 = Model_Manager::load_animated_model("tadpole/scene.gltf");

		//model_handle raccoon = Model_Manager::load_rigged_model("trex/scene.gltf");
		//model_handle raccoon = Model_Manager::load_animated_model_cgltf("clown/scene.gltf");
		//model_handle raccoon = Model_Manager::load_rigged_model("raccoon/scene.gltf");
		//model_handle raccoon33 = Model_Manager::load_animated_model("dragon/scene.gltf");
		//model_handle raccoon2 = Model_Manager::load_animated_model("dragon/scene.gltf");
		//model_handle raccoon = Model_Manager::load_rigged_model("oddish/scene.gltf");
		//model_handle raccoon = Model_Manager::load_rigged_model("oddish/scene.gltf");

		{
			//glm::vec3 pos2 = glm::vec3(0.0f);
			//glm::vec3 scale2 = glm::vec3(10.0f);
			//Entity e5555(pos2, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), scale2, "Sponza/glTF/Sponza.gltf", false);
			//scene.include(e5555);
		   /* model_handle car232323 = Model_Manager::load_model("911-2");
			pos = glm::vec3(-3.0f, 0.0f, -3.0f);
			scale = glm::vec3(1.0f);
			Entity e5(car232323, pos, true, scale, 1.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
			scene.include(e5);*/
			//Entity gsdgfsd("rainbow_road", glm::vec3(0.0f, -2500.0f, 0.0f), false, glm::vec3(1.0f), 1.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
			//scene.include(gsdgfsd);

			//model_handle sl = Model_Manager::load_model("skyloft/scene.gltf");
			//Entity fsfsfsf(glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), sl, false);
			//scene.include(fsfsfsf);

		// model_handle loaded = Model_Manager::load_model("emeraldsquare/Scene.gltf");

			// model_handle bistro = Model_Manager::load_model("bistro/Scene.gltf");
			// Entity sadasd23232323232332323(glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), bistro, false);
			// scene.include(sadasd23232323232332323);
		}

		Model_Manager::setup_buffers();
		scene.upload_buffers();

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		ImGui_ImplGlfw_InitForOpenGL(window.get_window(), true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
		ImGui_ImplOpenGL3_Init();

		JPH::BodyID ground = Physics::add_box(glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(100.0f, 1.0f, 100.0f), true);
		Physics::optimize_broad_phase();

		/*Font font("tx02");
		Text fpscounter(font, "999", 0, 1, 50.0f, glm::vec3(0.5f, 0.2f, 0.7f));
		Font daysl8r("28DaysLater");
		Text weapon_ammo_text(daysl8r, "200", 1600 - 175, 50, 50.0f, glm::vec3(1.0f, 1.0f, 0.7f));
		Text reserve_ammo_text(daysl8r, "9999", 1600 - 100, 50, 50.0f, glm::vec3(0.5f, 0.5f, 0.35f));*/

		//float debug_size = 20.0f;
		//Text player_position(font, "position: (1.00, 1.00, 1.00)", 1, SCR_HEIGHT - debug_size, debug_size, glm::vec3(1.0f));
		//Text player_facing(font, "facing: (1.00, 1.00, 1.00)", 1, SCR_HEIGHT - (debug_size * 2), debug_size, glm::vec3(1.0f));
		//Text player_holding(font, "holding: weaponweapon", 1, 900 - (debug_size * 3), debug_size, glm::vec3(1.0f));
		//Text screen_text2(font2, "LET ME OUTTTTT", 0, 700, 200.0f, glm::vec3(1.0f, 0.1f, 0.1f));
		/*Font font3("jianjianti");
		Text screen_text3(font3, u8"我爱你", 600, 200, 50.0f, glm::vec3(1.0f, 0.1f, 0.1f));
		Text screen_text4(font3, "hello world!?", 800, 300, 50.0f, glm::vec3(1.0f, 0.1f, 0.1f));*/

		Model_Manager::setup_ssbos();
		Model_Manager::upload_animation_commands();

		// scene.serialize();

		return 0;
	}

	void incandesce() {
		uint32_t frame = 0;

		while (window.open()) {
			legit::Profiler::Instance().BeginFrame();
			float current_time = window.get_time();

			delta_time = current_time - last_frame;
			last_frame = current_time;

			//if (!editor_mode) {
			// todo this controller step will also update holding / player viewmodel visibility stuff like that

			if (!editor_mode) {
				player.controller_step(window.get_window(), delta_time, scene);
				{ PROFILE_SCOPE_COLOR("Physics", legit::Colors::sunFlower);
				Physics::update(); } // default 1/60 delta time

				{ PROFILE_SCOPE_COLOR("update scene dirty", legit::Colors::sunFlower);
				scene.update_dirty(); }
			}
				
			// grab per frame values
			float aspect_ratio = (float)renderer.scr_width / (float)renderer.scr_height;
			
			// todo editor camera vs player camera
			// cull against these no matter what
			glm::mat4 player_view = player.get_body_view_matrix();
			glm::mat4 player_proj = player.camera.get_projection(aspect_ratio);
			glm::mat4 player_viewproj = player_proj * player_view;
			glm::mat4 player_inv_view = glm::inverse(player_view);
			glm::mat4 player_inv_proj = glm::inverse(player_proj);

			// but sometimes we want to see culling results from third person
			glm::vec3 view_pos;
			glm::mat4 active_view, active_proj, active_viewproj, active_inv_view, active_inv_proj;
			if (player.out_of_body) {
				active_view = player.get_debug_view_matrix();
				active_proj = player.debug_camera.get_projection(aspect_ratio);
				view_pos = player.debug_camera.position;
			}
			else {
				active_view = player_view;
				active_proj = player_proj;
				view_pos = player.camera.position;
			}
			active_viewproj = active_proj * active_view;
			active_inv_view = glm::inverse(active_view);
			active_inv_proj = glm::inverse(active_proj);

			{ PROFILE_SCOPE_COLOR("gpu cull", legit::Colors::nephritis);
			  renderer.begin_frame(scene, player_view, player_proj); } // pass scene 
			
			// todo decide if do during editor
				{ PROFILE_SCOPE_COLOR("update bones", legit::Colors::nephritis);
				Model_Manager::update_bones(delta_time); }
				
				{ PROFILE_SCOPE_COLOR("compute skin", legit::Colors::nephritis);
				Model_Manager::update_animated_vertices(scene); }

			// todo use editor or player camera?
			{ // build CSM mats
				PROFILE_SCOPE_COLOR("shadow setup", legit::Colors::nephritis);
				renderer.shadow_setup(player_view, player_inv_view, aspect_ratio, player.camera.zoom);
			}

			// todo implement!!!!
			{
				//PROFILE_SCOPE_COLOR("sort transparent", legit::Colors::nephritis);
				//renderer.sort_blended_draws();
			}

			// todo can also probably stuff into begin_frame()
			// todo prob editor camera
				{ PROFILE_SCOPE_COLOR("build clusters", legit::Colors::emerald);
				if (renderer.forward_plus)
					renderer.build_cluster_pass(active_inv_proj); } // todo pass calc'd already

				// todo prob editor camera
				{ PROFILE_SCOPE_COLOR("cull lights", legit::Colors::greenSea);
				if (renderer.forward_plus)
					renderer.cull_cluster_pass(active_view); } // todo pass calc'd alre
			
			// need for main draw
			{ PROFILE_SCOPE_COLOR("SSAO", legit::Colors::sunFlower);
			  if (renderer.ssao_enabled)
				renderer.ssao_pass(active_proj, active_inv_proj); }
			
			// hm
			{ PROFILE_SCOPE_COLOR("CSM shadow pass", legit::Colors::sunFlower);
			  renderer.shadow_pass(scene); }

			{ PROFILE_SCOPE_COLOR("draw", legit::Colors::clouds);
			  renderer.draw(scene, view_pos, active_view, active_viewproj, player_view, player_proj, player.key_toggles['t']); }

			{ PROFILE_SCOPE_COLOR("particles", legit::Colors::wisteria);
		 	  Particle_Manager::step_particles(delta_time);
			  renderer.particle_pass(delta_time, active_proj, active_view); }
			
			  // post process pass theoretically
			{ PROFILE_SCOPE_COLOR("bloom", legit::Colors::nephritis);
			  if (renderer.bloom_enabled) renderer.bloom_pass(); }
			
			if (renderer.do_draw_light_quads)
				renderer.draw_light_quads(player_proj, player_view);

			if (renderer.draw_skeletons && player.key_toggles['y'])
				renderer.debug_skeletons(scene, active_viewproj);

			renderer.render_skybox(scene.skybox, player_view, player_proj);

			{ PROFILE_SCOPE_COLOR("composite", legit::Colors::turqoise);
			  renderer.composite(); }

			{ PROFILE_SCOPE_COLOR("debug", legit::Colors::carrot);
			  if (player.key_toggles[(unsigned)'r']) {
				renderer.render_debug(active_view, active_proj, scene); 
			
				renderer.debug_renderer.draw_frustum(player.camera.position, player.camera.front, player.camera.up, player.camera.zoom, aspect_ratio, 0.1, 10000, Util::red);
			  }
			}
			
			//renderer.render(player, scene, delta_time, particle_ssbo);
			if (player.key_toggles['l'])
				renderer.debug_cascades(scene);

			// todo figure out whole 2d text system
			// screen space & world space

				//if (!(frame++ % 10)) {
				//    fpscounter.update_text(std::to_string((int)(1.0f / delta_time)));
				//    weapon_ammo_text.update_text(std::to_string(player.active_weapon->current_ammo));
				//    reserve_ammo_text.update_text(std::to_string(player.active_weapon->reserve_ammo));
				//   /* player_position.updateText("pos 1 00 1 00 1 00");
				//    player_facing.updateText("dir 1 00 1 00 1 00");*/
				//    player_holding.update_text("hand " + player.active_weapon->name);
				//}
				//renderer.render_crosshair(crosshair);
				//renderer.render_hud_text(fpscounter);
				//renderer.render_hud_text(weapon_ammo_text);
				//renderer.render_hud_text(reserve_ammo_text);
				//renderer.render_hud_text(player_position);
				//renderer.render_hud_text(player_facing);
				//renderer.render_hud_text(player_holding);

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			if (editor_mode) {
				editor.show();

				ImGui::ShowDemoWindow(); // Show demo window! :)

				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
				ImGui::Begin("Preview");
				static ImVec2 last_size = ImVec2(0, 0);
				ImVec2 size = ImGui::GetContentRegionAvail();
				if (size.x > 0 && size.y > 0 && (size.x != last_size.x || size.y != last_size.y)) {
					renderer.resize((int)size.x, (int)size.y);
					last_size = size;
				}
				ImGui::Image((ImTextureID)(intptr_t)(Texture_Manager::get_ogl_id(renderer.scene_texture)), size, ImVec2(0, 1), ImVec2(1, 0));
				ImGui::End();
				ImGui::PopStyleVar();

				scene.imgui();
				renderer.imgui_pass();

				// player.debug_hud();

				// collect memory stats
				// model vram, texture vram, maybe ssbo vram
			}

			legit::Profiler::Instance().EndFrame();
			auto& tasks = legit::Profiler::Instance().GetTasks();
			gpuGraph.LoadFrameData(tasks.data(), tasks.size());
			gpuGraph.RenderTimings(300, 200, 150, 0, 1.0f / 60.0f);

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			// if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			// {
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				// TODO for OpenGL: restore current GL context.
				glfwMakeContextCurrent(window.get_window());
			// }

			window.present();
			Audio::update();
		}
	}

	void shutdown() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		//Scene.close();
		//Model_Manager::cleanup(); // gotta do this too
		Texture_Manager::cleanup();// gotta do this fk
		Physics::shutdown();
		renderer.shutdown();
		window.shutdown();
	}
}
