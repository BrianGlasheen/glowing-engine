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

int main() 
{
    float delta_time = 0.0f;
    float last_frame = 0.0f;
    bool editor_mode = 0;

    Window window;
    if (window.init(1600, 900, "GLOW"))
        return -1;

    Renderer renderer;
    if (renderer.init())
        return -1;

    Texture_Manager::init();

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
    
    Scene scene("sky"); //.todo move lights here?

    model_handle plane = Model_Manager::load_model("plane.obj", 0);
    glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(50.0f, 1.0f, 50.0f);
    Entity e(pos, rot, scale, plane, false);
    scene.include(e);

    //Entity e233232332(glm::vec3(3.0f, 1.7f, -5.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(.05f), "ak47", false);
    //scene.include(e233232332);

    Entity e2323322(glm::vec3(5.0f, 30.0f, 10.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "f22", true);
    scene.include(e2323322);

    for (int j = 0; j < 10; j++) {
        Entity dsadasdasdasda(glm::vec3(0.0f, 1.05f + j, -5.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(0.003f), "die", true);
        scene.include(dsadasdasdasda);
    }

    {
        //model_handle gdfhgsd = Model_Manager::load_model("sponza");
        //pos = glm::vec3(0.0f);
        //scale = glm::vec3(0.1f);
        //Entity e5555(gdfhgsd, pos, false, scale, 1.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
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

          //Entity sadasd23232323232332323("bistro", glm::vec3(0.0f), false, glm::vec3(0.025f), 1.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
         //scene.include(sadasd23232323232332323);
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window.get_window(), true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

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

    // render loop
    uint32_t frame = 0;
    printf("RENDERING\n");
    while (window.open()) {
        float currentFrame = window.get_time();

        delta_time = currentFrame - last_frame;
        last_frame = currentFrame;

        if (!(frame++ % 10)) {
            fpscounter.update_text(std::to_string((int)(1.0f / delta_time)));
            weapon_ammo_text.update_text(std::to_string(player.active_weapon->current_ammo));
            reserve_ammo_text.update_text(std::to_string(player.active_weapon->reserve_ammo));
           /* player_position.updateText("pos 1 00 1 00 1 00");
            player_facing.updateText("dir 1 00 1 00 1 00");*/
            player_holding.update_text("hand " + player.active_weapon->name);
        }

        // todo maybe refactor all into jolt controller hmmm
        // controller step takes input
        // updates physics according to active controller
        // updates camera
        // draws hud (weapon, etc)

        if (!editor_mode) {
            player.controller_step(window.get_window(), delta_time, scene);
            Physics::update(); // default 1/60 delta time
        }

        // render scene
        renderer.render(player, scene, delta_time);

        if (!player.key_toggles[(unsigned)'r'])
            renderer.render_debug(player);

        // render scene deferred pipeline
        // renderer.render_scene_deferred(player, scene, delta_time);
        // TODO: clustered forward 

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Light");
        ImGui::SliderFloat3("pos", &renderer.spotlight.position.x, -10.0f, 10.0f);
        ImGui::SliderFloat3("dir", &renderer.spotlight.direction.x, -1.0f, 1.0f);
        ImGui::SliderFloat3("color", &renderer.spotlight.color.x, 0.0f, 1.0f); // 1.0f;     
        ImGui::SliderFloat("itensity", &renderer.spotlight.intensity, 0.0f, 10000.0f); // 1.0f;  
        ImGui::SliderFloat("FOV outer", &renderer.spotlight.outer_fov, 0.0f, 180.0f); // 1.0f;     
        ImGui::SliderFloat("FOV inner", &renderer.spotlight.inner_fov, 0.0f, 180.0f); // 1.0f;        
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SliderFloat3("directional_light_direction", &renderer.directional_light.direction.x, -1.0f, 1.0f);
        ImGui::SliderFloat3("directional_light_color", &renderer.directional_light.color.x, -1.0f, 1.0f);
        ImGui::SliderFloat("directional_light_intensity", &renderer.directional_light.intensity, 0.0f, 2.0f);
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SliderFloat3("point_light_pos", &renderer.point_light.position.x, -15.0f, 15.0f);
        ImGui::SliderFloat3("point_light_color", &renderer.point_light.color.x, 0.0f, 1.0f);
        ImGui::SliderFloat("point_light_intensity", &renderer.point_light.intensity, 0.0f, 1000.0f);
        ImGui::SliderFloat("point light farplane", &renderer.penis, 0.0f, 50.0f);
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::SliderFloat("amb light", &renderer.ambient_light, 0.0f, 1.0f);
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        ImGui::Checkbox("use alpha", &renderer.use_alpha_clipping);
        ImGui::SliderFloat("alpha cutoff", &renderer.alpha_cutoff, 0.0, 1.0f);
        ImGui::End();

        //player.debug_hud();
        //if (renderer.editor_mode) {
            //renderer.render_gizmo(scene, player);
        //}
        //else {
            renderer.render_crosshair(crosshair);
            renderer.render_hud_text(fpscounter);
            renderer.render_hud_text(weapon_ammo_text);
            renderer.render_hud_text(reserve_ammo_text);
            //renderer.render_hud_text(player_position);
            //renderer.render_hud_text(player_facing);
            renderer.render_hud_text(player_holding);
        //}
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
