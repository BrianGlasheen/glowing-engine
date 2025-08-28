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

#include "player/player.h"
#include "core/ssbo.h"
#include "util/profiler.h"
#include "util/frustum.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/random.hpp"

#include "dearimgui/imgui_impl_glfw.h"
#include "dearimgui/imgui_impl_opengl3.h"

#include <vector>

namespace Glow {
	void iterate_entities(Scene& scene, const glm::vec3& view_pos, const float& aspect_ratio); // hm forward declaration i guess it works

	// ▀█▀ █▀█ █▀▄ █▀█   █▄░█ █░█ █▄▀ █▀▀
	// ░█░ █▄█ █▄▀ █▄█   █░▀█ █▄█ █░█ ██▄
	struct Particle {
		glm::vec3 position;
		float ttl;
		glm::vec3 velocity;
		float max_ttl;
		glm::vec4 color_start;
		glm::vec4 color_end;
		float size_start;
		float size_end;
		glm::vec2 padding;
	};

	SSBO particle_ssbo;

	// ▀█▀ █▀█ █▀▄ █▀█   █▄░█ █░█ █▄▀ █▀▀
	// ░█░ █▄█ █▄▀ █▄█   █░▀█ █▄█ █░█ ██▄

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

	bool compute_culling = true;

	bool init(const Glow_Init_Info& init_info) {
		if (window.init(init_info.width, init_info.height, init_info.window_name))
			return -1;

		if (renderer.init())
			return -1;

		//Texture_Manager::init();

		if (editor.init())
			return -1;

		Audio::init();
		Physics::init();
		Model_Manager::init(std::string(init_info.model_base_path));

		player.init(); // after physics
		window.sync_callbacks(player, renderer, editor, editor_mode);

		// bool loaded = Model_Manager::load_model_indirect("Sponza/glTF/Sponza.gltf");
		//bool loaded = Model_Manager::load_model_indirect("ABeautifulGame/glTF/ABeautifulGame.gltf");
		//bool loaded = Model_Manager::load_model_indirect("bistro/Scene.gltf");

		//bool loaded = Model_Manager::load_model_indirect("f22/scene.gltf");
		//bool loaded = Model_Manager::load_model_indirect("track/scene.gltf");
		//bool loaded = Model_Manager::load_model_indirect("plane.obj");
		//bool loaded = Model_Manager::load_model_indirect("sword2/scene.gltf");
		//bool loaded = Model_Manager::load_model_indirect("astonmartin/scene.gltf");
		//bool loaded = Model_Manager::load_model_cgltf();
		//bool loaded = Model_Manager::load_model_indirect("MetalRoughSpheres/glTF/MetalRoughSpheres.gltf");

		//Model_Manager::upload_data();
		//printf(loaded ? "good\n" : "bad\n");
		scene.init("sky");

		model_handle plane = Model_Manager::load_model("plane.obj");
		glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::quat rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 scale = glm::vec3(500.0f, 1.0f, 500.0f);
		Entity e(pos, rot, scale, plane, false);
		scene.include(e);

		//pos = glm::vec3(0.0f, 0.0f, 0.0f);
		//rot = glm::quat(0.707f, 0.707f, 0.0f, 0.0f);
		//scale = glm::vec3(100.0, 1.0f, 100.0f);
		//Entity e3232(pos, rot, scale, plane, false);
		//scene.include(e3232);

		//pos = glm::vec3(0.0f, 0.0f, 0.0f);
		//rot = glm::quat(0.707f, 0.0f, 0.0f, 0.707f);
		//scale = glm::vec3(100.0, 1.0f, 100.0f);
		//Entity e3233332(pos, rot, scale, plane, false);
		//scene.include(e3233332);

		//pos = glm::vec3(0.0f, 25.0f, 0.0f);
		//rot = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		//scale = glm::vec3(500.0f, 1.0f, 500.0f);
		//Entity edddddd(pos, rot, scale, plane, false);
		//scene.include(edddddd);


		//Entity e233232332(glm::vec3(3.0f, 1.7f, -5.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(.05f), "ak47", false);
		//scene.include(e233232332);

		//Entity e2323322(glm::vec3(5.0f, 30.0f, 10.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "f22/scene.gltf", true);
		//scene.include(e2323322);

		//model_handle id = Model_Manager::load_model_cgltf("f22/scene.gltf");
		model_handle id = Model_Manager::load_model_cgltf("MetalRoughSpheres/glTF/MetalRoughSpheres.gltf");
		Entity e232332232323(glm::vec3(0.0f, 0.0f, -10.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(10.0f), id, false);
		scene.include(e232332232323);

		//Entity e232lamp3322(glm::vec3(0.0f, 1.0f, 5.0f), glm::quat(1.0f, 0.0, 0.0f, 0.0f), glm::vec3(1.0f), "blendtest/glTF/AlphaBlendModeTest.gltf", false);
		//scene.include(e232lamp3322);

		//Entity d12321313123(glm::vec3(0.0f, 1.0f, 5.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "CompareAlphaCoverage/glTF/CompareAlphaCoverage.gltf", false);
		//scene.include(d12321313123);

		Entity d12321313d123(glm::vec3(5.0f, 5.0f, 5.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "f22/scene.gltf", true);
		scene.include(d12321313d123);

		//Entity d12321313d1233(glm::vec3(-5.0f, 1.0f, 5.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "CommercialRefrigerator/glTF/CommercialRefrigerator.gltf", true);
		//scene.include(d12321313d1233);

		for (int j = 0; j < 100; j++) {
			printf("%d\n", j);
			Entity dsadasdasdasda(glm::vec3(0.0f, 1 + j * 1.2f, -5.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "die/scene.gltf", true);
			scene.include(dsadasdasdasda);
		}

		//model_handle raccoon3 = Model_Manager::load_animated_model_cgltf("glock/scene.gltf");
		//model_handle raccoon4 = Model_Manager::load_animated_model_cgltf("glock2/scene.gltf");
		//model_handle raccoon5 = Model_Manager::load_animated_model("glockhell/scene.gltf");
		//model_handle raccoon6 = Model_Manager::load_animated_model("hkm23/scene.gltf");
		//model_handle raccoon6 = Model_Manager::load_animated_model("hkm23/scene.gltf");
		//model_handle raccoon3 = Model_Manager::load_animated_model("hkm23/scene.gltf");
		//model_handle raccoon6 = Model_Manager::load_animated_model_cgltf("hkm23/scene.gltf");
		//model_handle raccoon6ddd = Model_Manager::load_animated_model("hkm23/source/Mark23.fbx");

		//model_handle raccoon226 = Model_Manager::load_animated_model_cgltf("gun/scene.gltf");

		//model_handle raccoon26 = Model_Manager::load_animated_model_cgltf("vector/scene.gltf");
		//model_handle raccoondddddd2 = Model_Manager::load_animated_model("tiger/scene.gltf");
		//model_handle raccoon2 = Model_Manager::load_animated_model_cgltf("glock2/scene.gltf");
		//model_handle raccoon23333 = Model_Manager::load_animated_model("goku/scene.gltf");
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
			glm::vec3 pos2 = glm::vec3(0.0f);
			glm::vec3 scale2 = glm::vec3(10.0f);
			Entity e5555(pos2, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), scale2, "Sponza/glTF/Sponza.gltf", false);
			scene.include(e5555);
		   /* model_handle car232323 = Model_Manager::load_model("911-2");
			pos = glm::vec3(-3.0f, 0.0f, -3.0f);
			scale = glm::vec3(1.0f);
			Entity e5(car232323, pos, true, scale, 1.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
			scene.include(e5);*/
			//Entity gsdgfsd("rainbow_road", glm::vec3(0.0f, -2500.0f, 0.0f), false, glm::vec3(1.0f), 1.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
			//scene.include(gsdgfsd);

			model_handle sl = Model_Manager::load_model("skyloft/scene.gltf");
			Entity fsfsfsf(glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), sl, false);
			scene.include(fsfsfsf);

		//model_handle loaded = Model_Manager::load_model("emeraldsquare/Scene.gltf");

			//model_handle bistro = Model_Manager::load_model("bistro/Scene.gltf");
			//Entity sadasd23232323232332323(glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), loaded, false);
			//scene.include(sadasd23232323232332323);
		}

		Model_Manager::setup_buffers(); // upload MDI verts / inds to gpu
		renderer.setup_indirect();
		scene.upload_buffers();

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
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


		// todo move
		const int MAX_PARTICLES = 10000;
		std::vector<Particle> particles(MAX_PARTICLES);
		for (auto& p : particles) {
			p.position = glm::vec3(0.0f);
			p.ttl = 0.0f;
			p.velocity = glm::vec3(0.0f);
			p.max_ttl = 0.0f;
			p.color_start = glm::vec4(0.0f);
			p.color_end = glm::vec4(0.0f);
			p.size_start = 0.0f;
			p.size_end = 0.0f;
			// p.padding;
		}
		//SSBO particle_ssbo;
		particle_ssbo.init();
		particle_ssbo.set_data(sizeof(Particle) * MAX_PARTICLES, particles.data(), GL_DYNAMIC_DRAW);
		///////////////////////////////////////////////////////////
		Model_Manager::setup_ssbos();

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
			player.controller_step(window.get_window(), delta_time, scene);
			{
				PROFILE_SCOPE_COLOR("Physics", legit::Colors::sunFlower);
				Physics::update(); // default 1/60 delta time
			}

			{
				PROFILE_SCOPE_COLOR("update scene dirty", legit::Colors::sunFlower);
				scene.update_dirty();
			}

			// grab per frame values
			float aspect_ratio = (float)renderer.scr_width / (float)renderer.scr_height;

			// against these no matter what
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

			renderer.begin_frame(); 
			Model_Manager::begin_animation_frame();

			{ // build CSM mats
				PROFILE_SCOPE_COLOR("shadow setup", legit::Colors::nephritis);
				renderer.shadow_setup(player_view, player_inv_view, aspect_ratio, player.camera.zoom);
			}

			// iterate entites
			// per entity submit stuff to renderer, animation system, game logic systems
			{
				PROFILE_SCOPE_COLOR("shadow setup", legit::Colors::nephritis);
				if (!compute_culling)
					iterate_entities(scene, player.camera.position, aspect_ratio);
			}

			//{ // MOVE TO ITERATE ENTITIES
			//	PROFILE_SCOPE_COLOR("scene dirty", legit::Colors::peterRiver);
			//	scene.update_dirty();
			//}

			{
				PROFILE_SCOPE_COLOR("sort transparent", legit::Colors::nephritis);
				renderer.sort_blended_draws();
			}
			
			// player submit render items
			player.submit_animation_items();
			player.submit_render_items(renderer);

			// iterate particles + submit etc
			// other cool things todo! B)

			renderer.upload_render_commands();

			{
				PROFILE_SCOPE_COLOR("build clusters", legit::Colors::emerald);
				if (renderer.forward_plus)
					renderer.build_cluster_pass(active_inv_proj); // todo pass calc'd already
			}

			{
				PROFILE_SCOPE_COLOR("cull lights", legit::Colors::greenSea);
				if (renderer.forward_plus)
					renderer.cull_cluster_pass(active_view);// todo pass calc'd alre
			}
			{
				PROFILE_SCOPE_COLOR("SSAO", legit::Colors::sunFlower);
				if (renderer.ssao_enabled)
					renderer.ssao_pass(active_proj, active_inv_proj);
			}
			{
				PROFILE_SCOPE_COLOR("update_bones", legit::Colors::sunFlower);
				Model_Manager::update_bones_from_animation_compute(0, current_time);

			}
			//printf("ct: %f\n", current_time);

			// render scene

			{
				PROFILE_SCOPE_COLOR("draw", legit::Colors::clouds);
				if (compute_culling)
					renderer.compute_cull_draw(scene, view_pos, active_view, active_viewproj, player_view, player_proj);
				else
					renderer.draw(scene, player_view, active_viewproj, player.camera.position, active_proj);
			}

			//{
			//	PROFILE_SCOPE_COLOR("particles", legit::Colors::wisteria);
			//	renderer.particle_pass(delta_time, particles, player);
			//}
			// post process pass theoretically
			{
				PROFILE_SCOPE_COLOR("bloom", legit::Colors::nephritis);
				if (renderer.bloom_enabled)
					renderer.bloom_pass();
			}
			if (renderer.do_draw_light_quads)
				renderer.draw_light_quads(player_proj, player_view);

			renderer.render_skybox(scene.skybox, player_view, player_proj);

			{
				PROFILE_SCOPE_COLOR("composite", legit::Colors::turqoise);
				renderer.composite();
			}

			{
				PROFILE_SCOPE_COLOR("debug", legit::Colors::carrot);
				if (player.key_toggles[(unsigned)'r'])
					renderer.render_debug(active_view, active_proj);
			}
			//renderer.render(player, scene, delta_time, particle_ssbo);
			if (player.key_toggles['l'])
				renderer.debug_cascades();

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

			// render scene deferred pipeline
			// renderer.render_scene_deferred(player, scene, delta_time);

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			ImGui::Begin("particle");
			ImGui::Checkbox("compute culling", &compute_culling);     
			ImGui::SliderFloat3("pos", &renderer.emitter_position.x, 0.0f, 50.0f); // 1.0f;     
			ImGui::SliderFloat3("acceleration dir", &renderer.acceleration_direction.x, -1.0f, 1.0f);
			ImGui::SliderFloat("acceleration mag", &renderer.acceleration_force, -17.0f, 19.0f);
			ImGui::SliderFloat2("life_range", &renderer.life_range.x, 0.0, 10.0f);
			ImGui::SliderFloat4("color_start_base", &renderer.color_start_base.x, 0.0f, 1.0f);
			ImGui::SliderFloat4("color_start_end", &renderer.color_end_base.x, 0.0f, 1.0f);
			ImGui::SliderFloat3("velocity_base", &renderer.velocity_base.x, -20.0f, 20.0f);
			ImGui::SliderFloat3("velocity_random_bias", &renderer.velocity_random_bias.x, -1.0, 1.0);
			ImGui::SliderFloat("velocity_mag", &renderer.velocity_mag, -100.0f, 100.0f);
			ImGui::SliderFloat("emission_rate", &renderer.emission_rate, 0.0f, 20000.0f);
			ImGui::End();

			player.debug_hud();
			renderer.imgui_pass();

			// collect memory stats
			// model vram, texture vram, maybe ssbo vram

			legit::Profiler::Instance().EndFrame();
			auto& tasks = legit::Profiler::Instance().GetTasks();
			gpuGraph.LoadFrameData(tasks.data(), tasks.size());
			gpuGraph.RenderTimings(300, 200, 150, 0, 1.0f / 60.0f);

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			window.present();
			Audio::update();
		}
	}

	// todo nuke function
	void iterate_entities(Scene& scene, const glm::vec3& view_pos, const float& aspect_ratio) {
		float FAR_PLAN_MOVE_ME_LATER = 1000.0;
		Util::Frustum frustum(player.camera.position, player.camera.front, player.camera.right, player.camera.up, glm::radians(player.camera.zoom), aspect_ratio, 0.1f, FAR_PLAN_MOVE_ME_LATER);

		for (Entity& entity : scene.entities) {

			// check if entity is dirty
			// recompute transform

			// todo store model aabb in entity so dont have to fetch from model manager unless intersects
			Model mind = Model_Manager::get_model_ind(entity.model_id);
			Util::AABB model_aabb = Util::transform_aabb(mind.m_aabb, entity.get_model_matrix());

			// test against player view frustum
			bool inf_far = true;
			if (frustum.intersectsAABB(model_aabb, inf_far)) {
				renderer.debug_renderer.add_bbox(model_aabb.min, model_aabb.max, glm::vec3(1.0f, 1.0f, 1.0f));

				for (uint32_t i = 0; i < mind.m_meshes.size(); i++) {
					Per_Object_Data obj_data;

					obj_data.model_matrix = entity.get_model_matrix() * mind.m_meshes[i].transform;
					Util::AABB obj_aabb = Util::transform_aabb(mind.m_meshes[i].aabb, entity.get_model_matrix());

					if (!frustum.intersectsAABB(obj_aabb, true)) {
						renderer.debug_renderer.add_bbox(obj_aabb.min, obj_aabb.max, glm::vec3(1.0f, 0.0f, 1.0f));
						continue;
					}
					renderer.debug_renderer.add_bbox(obj_aabb.min, obj_aabb.max, glm::vec3(0.0f, 1.0f, 0.0f));

					Draw_Elements_Indirect_Command draw_command;
					draw_command.count = mind.m_meshes[i].index_count;
					draw_command.instance_count = 1;
					draw_command.first_index = mind.m_meshes[i].base_index;
					draw_command.base_vertex = mind.m_meshes[i].base_vertex;
					// renderer will set draw_command.base_instance for correct blend mode

					obj_data.normal_matrix = glm::transpose(glm::inverse(obj_data.model_matrix));
					const Material& mater = mind.m_meshes[i].material;
					obj_data.albedo = mater.albedo;
					obj_data.normal = mater.normal;
					obj_data.met_rough = mater.met_rough;
					obj_data.emissive = mater.emissive;
					obj_data.amb_occ = mater.amb_occ;
					obj_data.emissive_factor = mater.emissive_factor;
					obj_data.metallic_factor = mater.metallic_factor; // 4
					obj_data.roughness_factor = mater.roughness_factor; // 4
					obj_data.base_color = mater.base_color;
					obj_data.alpha_cutoff = mater.alpha_cutoff;

					renderer.submit_render_command(draw_command, obj_data, mater.blend_mode, view_pos, obj_aabb);
				}
			}
			else {
				renderer.debug_renderer.add_bbox(model_aabb.min, model_aabb.max, glm::vec3(1.0f, 0.0f, 1.0f));
			}

			// test against shadow cascades
			// check if entity interescts each cascade
			// add to cascade command buffer + per obj data
		}
	}

	void shutdown() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		//Model_Manager::cleanup(); // gotta do this too
		Texture_Manager::cleanup();// gotta do this fk
		Physics::shutdown();
		renderer.shutdown();
		window.shutdown();
	}
}
