//#include <dearimgui/imgui.h>
//#include <dearimgui/imgui_impl_glfw.h>
//#include <dearimgui/imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/window.h"
//#include "core/renderer.h"
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
#include <vector>
#include <glm/gtc/random.hpp>

#include "util/profiler.h"

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

int main() 
{
    float delta_time = 0.0f;
    float last_frame = 0.0f;
    bool editor_mode = 1;

    Window window;
    if (window.init(1600, 900, "GLOW"))
        return -1;

    Renderer renderer;
    if (renderer.init())
        return -1;

    //Texture_Manager::init();

    Editor editor;
    if (editor.init())
        return -1;

    Audio::init();
    Physics::init();

    //Texture_Manager::init();
    Model_Manager::init("../resources/models/");

    Player player;
    window.sync_callbacks(player, renderer, editor, editor_mode);

    Crosshair crosshair(1.0f, 6.0f, 10.0f, 10.0f, 1.0f, glm::vec3(1.0f, 0.5f, 1.0f));
    
    Scene scene("star"); //.todo move lights here?

   // bool loaded = Model_Manager::load_model_indirect("Sponza/glTF/Sponza.gltf");
    //bool loaded = Model_Manager::load_model_indirect("ABeautifulGame/glTF/ABeautifulGame.gltf");
    //bool loaded = Model_Manager::load_model_indirect("bistro/Scene.gltf");
    //bool loaded = Model_Manager::load_model_indirect("emeraldsquare/Scene.gltf");
    //bool loaded = Model_Manager::load_model_indirect("f22/scene.gltf");
    //bool loaded = Model_Manager::load_model_indirect("track/scene.gltf");
    //bool loaded = Model_Manager::load_model_indirect("plane.obj");
    //bool loaded = Model_Manager::load_model_indirect("sword2/scene.gltf");
    //bool loaded = Model_Manager::load_model_indirect("astonmartin/scene.gltf");
    //bool loaded = Model_Manager::load_model_indirect("MetalRoughSpheres/glTF/MetalRoughSpheres.gltf");
    //bool loaded = Model_Manager::load_model_indirect("MetalRoughSpheres/glTF/MetalRoughSpheres.gltf");

    //Model_Manager::upload_data();
    //printf(loaded ? "good\n" : "bad\n");

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

    //Entity e2323322(glm::vec3(5.0f, 30.0f, 10.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "f22", true);
    //scene.include(e2323322);    
    
    //Entity e232lamp3322(glm::vec3(0.0f, 1.0f, 0.0f), glm::quat(0.707f, -0.707f, 0.0f, 0.0f), glm::vec3(0.025f), "lamp", false);
    //scene.include(e232lamp3322);

    for (int j = 0; j < 10; j++) {
        printf("%d\n", j);
        Entity dsadasdasdasda(glm::vec3(0.0f, 1 + j * 1.2f, -5.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "die/scene.gltf", true);
        scene.include(dsadasdasdasda);
    }
    //model_handle raccoon = Model_Manager::load_animated_model("tiger/scene.gltf");
    //model_handle raccoon2 = Model_Manager::load_animated_model("tiger/scene.gltf");

    for (uint32_t i = 0; i < 500; i++) {
        //model_handle raccoon3 = Model_Manager::load_animated_model("tiger/scene.gltf");
        model_handle raccoon = Model_Manager::load_animated_model("goku/scene.gltf");
    }
    //model_handle raccoon = Model_Manager::load_rigged_model("trex/scene.gltf");
    //model_handle raccoon = Model_Manager::load_rigged_model("clown/scene.gltf");
    //model_handle raccoon = Model_Manager::load_rigged_model("raccoon/scene.gltf");
    //model_handle raccoon = Model_Manager::load_animated_model("dragon/scene.gltf");
    //model_handle raccoon2 = Model_Manager::load_animated_model("dragon/scene.gltf");
    //model_handle raccoon = Model_Manager::load_rigged_model("oddish/scene.gltf");
    //model_handle raccoon = Model_Manager::load_rigged_model("oddish/scene.gltf");

    {
        //glm::vec3 pos = glm::vec3(0.0f);
        //glm::vec3 scale = glm::vec3(10.0f);
        //Entity e5555(pos, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), scale, "Sponza/glTF/Sponza.gltf", false);
        //scene.include(e5555);
       /* model_handle car232323 = Model_Manager::load_model("911-2");
        pos = glm::vec3(-3.0f, 0.0f, -3.0f);
        scale = glm::vec3(1.0f);
        Entity e5(car232323, pos, true, scale, 1.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        scene.include(e5);*/
        //Entity gsdgfsd("rainbow_road", glm::vec3(0.0f, -2500.0f, 0.0f), false, glm::vec3(1.0f), 1.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        //scene.include(gsdgfsd);

        //Entity fdfsdfsdfsdfsdf("skyloft", glm::vec3(0.0f, 0.0f, 0.0f), false, glm::vec3(1.0f), 1.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        //scene.include(fdfsdfsdfsdfsdf);

          /*Entity sadasd23232323232332323(glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "bistro/Scene.gltf", false);
         scene.include(sadasd23232323232332323);*/
    }

    Model_Manager::setup_buffers(); // upload MDI verts / inds to gpu
    renderer.setup_indirect();


    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window.get_window(), true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();
    ImGuiUtils::ProfilerGraph gpuGraph(300);

    Font font("tx02");
    Text fpscounter(font, "999", 0, 1, 50.0f, glm::vec3(0.5f, 0.2f, 0.7f));
    Font daysl8r("28DaysLater");
    Text weapon_ammo_text(daysl8r, "200", 1600 - 175, 50, 50.0f, glm::vec3(1.0f, 1.0f, 0.7f));
    Text reserve_ammo_text(daysl8r, "9999", 1600 - 100, 50, 50.0f, glm::vec3(0.5f, 0.5f, 0.35f));

    float debug_size = 20.0f;
    //Text player_position(font, "position: (1.00, 1.00, 1.00)", 1, SCR_HEIGHT - debug_size, debug_size, glm::vec3(1.0f));
    //Text player_facing(font, "facing: (1.00, 1.00, 1.00)", 1, SCR_HEIGHT - (debug_size * 2), debug_size, glm::vec3(1.0f));
    Text player_holding(font, "holding: weaponweapon", 1, 900 - (debug_size * 3), debug_size, glm::vec3(1.0f));
    //Text screen_text2(font2, "LET ME OUTTTTT", 0, 700, 200.0f, glm::vec3(1.0f, 0.1f, 0.1f));
    /*Font font3("jianjianti");
    Text screen_text3(font3, u8"我爱你", 600, 200, 50.0f, glm::vec3(1.0f, 0.1f, 0.1f));
    Text screen_text4(font3, "hello world!?", 800, 300, 50.0f, glm::vec3(1.0f, 0.1f, 0.1f));*/

    // ground 
    JPH::BodyID ground = Physics::add_box(glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(100.0f, 1.0f, 100.0f), true);
    Physics::optimize_broad_phase();


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
    SSBO particle_ssbo;
    particle_ssbo.init();
    particle_ssbo.set_data(sizeof(Particle) * MAX_PARTICLES, particles.data(), GL_DYNAMIC_DRAW);
    ///////////////////////////////////////////////////////////
    Model_Manager::setup_ssbos();

    // render loop
    uint32_t frame = 0;
    printf("RENDERING\n");
    while (window.open()) {
        legit::Profiler::Instance().BeginFrame();
        float current_time = window.get_time();

        delta_time = current_time - last_frame;
        last_frame = current_time;

        //if (!(frame++ % 10)) {
        //    fpscounter.update_text(std::to_string((int)(1.0f / delta_time)));
        //    weapon_ammo_text.update_text(std::to_string(player.active_weapon->current_ammo));
        //    reserve_ammo_text.update_text(std::to_string(player.active_weapon->reserve_ammo));
        //   /* player_position.updateText("pos 1 00 1 00 1 00");
        //    player_facing.updateText("dir 1 00 1 00 1 00");*/
        //    player_holding.update_text("hand " + player.active_weapon->name);
        //}
        {
            PROFILE_SCOPE_COLOR("update_bones", legit::Colors::sunFlower);
            if (player.key_toggles[(unsigned)'c'])
                Model_Manager::update_bones_from_animation(0, current_time);
            else
                Model_Manager::update_bones_from_animation_compute(0, current_time);
        }
        //printf("ct: %f\n", current_time);

        //if (!editor_mode) {
            player.controller_step(window.get_window(), delta_time, scene);
        {
            PROFILE_SCOPE_COLOR("Physics", legit::Colors::sunFlower);
            Physics::update(); // default 1/60 delta time
        }
        {
            PROFILE_SCOPE_COLOR("scene dirty", legit::Colors::peterRiver);
            scene.update_dirty();
        }
        // render scene
        renderer.render(player, scene, delta_time, particle_ssbo);

        {
            PROFILE_SCOPE_COLOR("debug", legit::Colors::carrot);
            if (player.key_toggles[(unsigned)'r'])
                renderer.render_debug(player);
        }

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
        
        renderer.imgui_pass();

        legit::Profiler::Instance().EndFrame();
        auto& tasks = legit::Profiler::Instance().GetTasks();
        gpuGraph.LoadFrameData(tasks.data(), tasks.size());
        gpuGraph.RenderTimings(300, 200, 150, 0, 1.0f / 60.0f);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        window.present();
        Audio::update();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    //Model_Manager::cleanup();
    Texture_Manager::cleanup();
    Physics::shutdown();
    renderer.shutdown();
    window.shutdown();
    return 0;
}
