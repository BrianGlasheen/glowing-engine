#include "player/player.h"

#include "player/controller_fps.h"
//#include "player/controller_thirdperson.h"
//#include "player/controller_plane.h"

#include "dearimgui/imgui.h"

#include <memory>

Player::Player() : camera(glm::vec3(0.0f, player_height, 0.0f)), debug_camera(glm::vec3(0.0f, player_height, 0.0f)) {
    active_controller = ControllerType::FPS;
}

void Player::init() {
    weapons[Weapon_Id::GLOCK] = std::make_unique<Weapon>(Weapon::GLOCK());
    weapons[Weapon_Id::BOW] = std::make_unique<Weapon>(Weapon::BOW());
    active_weapon = weapons[Weapon_Id::BOW].get();

    physics_id = Physics::create_player_controller();
}

void Player::controller_step(GLFWwindow* window, float deltaTime, Scene& scene) {
    poll_player(window, scene);

    if (out_of_body) {
        move_debug_camera(window, debug_camera);
        return;
    }

    switch (active_controller) {
        //using enum ControllerType; c++ 20 bruh
        case ControllerType::FPS:
            FPS_Controller::process_input(window, deltaTime, scene, camera, model_yaw, physics_id);
            break;
    }

    /*physics_id = Physics::create_player_controller(position, (aabb.max - aabb.min) * scale, false);*/
    //// yanked from process input function of player controller, todo refactor ?
    //Weapon* current_weapon = active_weapon;
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        // reset reload timer
        active_weapon = weapons[Weapon_Id::BOW].get();
    }
    else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        // reset reload timer of held
        active_weapon = weapons[Weapon_Id::GLOCK].get();
    }

    //if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
    //    Animated_Model& model = Model_Manager::get_animated_model(active_weapon->model_id);
    //    model.current_animation += 1;
    //    if (model.current_animation > model.animation_count)
    //        model.current_animation = 0;
    //}
    //if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
    //    Animated_Model& model = Model_Manager::get_animated_model(active_weapon->model_id);
    //    model.current_animation -= 1;
    //    if (model.current_animation < 0)
    //        model.current_animation = model.animation_count - 1;
    //}


    //bool ads_active = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    bool firing = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool reload_requested = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    bool is_sprinting = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

    active_weapon->update(deltaTime, firing, reload_requested, is_sprinting, camera.position, camera.front);
}

void Player::submit_animation_items() {
    // build animation command for item(s) that should be animated
    // player model if in third person, weapon, etc

   
    // weapon logic sets animation which exists in the model, command will use
    Model_Manager::submit_animation_command(active_weapon->model_id); 
}

void Player::submit_render_items(Renderer& renderer) {
    // submit draw commands for player model / weapon / etc
    if (out_of_body)
        renderer.debug_renderer.add_line(camera.position, camera.position - player_height, glm::vec3(1.0f, 0.0f, 0.0f));

    glm::vec3 feet = camera.position - player_height;
    renderer.debug_renderer.add_bbox(feet + glm::vec3(0.1f), feet - glm::vec3(0.1f), glm::vec3(1.0f, 0.0f, 0.0f));

    //Animated_Model& model = Model_Manager::get_animated_model(active_weapon->model_id);
    //for (const Animated_Mesh& mesh : model.m_meshes) {
    //    Draw_Elements_Indirect_Command cmd{ 0 };
    //    cmd.count = mesh.index_count;
    //    cmd.instance_count = 1;
    //    cmd.first_index = mesh.base_index;
    //    cmd.base_vertex = mesh.base_vertex;

    //    Per_Object_Data obj_data;
    //    obj_data.model_matrix = active_weapon->get_model_matrix();
    //    obj_data.normal_matrix = glm::transpose(glm::inverse(obj_data.model_matrix));
    //    const Material& mater = mesh.material;
    //    obj_data.albedo = mater.albedo;
    //    obj_data.normal = mater.normal;
    //    obj_data.met_rough = mater.met_rough;
    //    obj_data.emissive = mater.emissive;
    //    obj_data.amb_occ = mater.amb_occ;
    //    obj_data.emissive_factor = mater.emissive_factor;
    //    obj_data.metallic_factor = mater.metallic_factor; // 4
    //    obj_data.roughness_factor = mater.roughness_factor; // 4
    //    obj_data.base_color = mater.base_color;
    //    obj_data.alpha_cutoff = mater.alpha_cutoff;

    //    renderer.submit_animated_render_command(cmd, obj_data);
    //}
}

void Player::mouse_callback(GLFWwindow* window, double xpos, double ypos) { // need these guys to pass camera
    if (key_toggles[(unsigned)'q'])
        return;

    //if (!out_of_body && !key_toggles[(unsigned)'q'])
    //    controller->mouse_callback(window, camera, xpos, ypos, model_yaw);
    //else // out of body
    //    controller->mouse_callback(window, debug_camera, xpos, ypos, model_yaw);
    if (!out_of_body) {
        switch (active_controller) {
        case ControllerType::FPS:
            FPS_Controller::mouse_callback(window, camera, xpos, ypos, model_yaw);
            break;
        }
    }
    else {
        debug_camera.process_mouse_movement(xpos, ypos);
    }
}

void Player::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    //controller->scroll_callback(window, camera, xoffset, yoffset);
    switch (active_controller) {
        //using enum ControllerType; c++ 20 bruh
        case ControllerType::FPS:
            FPS_Controller::scroll_callback(window, camera, xoffset, yoffset);
            break;
    }
}

void Player::char_callback(GLFWwindow* window, uint32_t key) {
    key_toggles[key] = !key_toggles[key]; // set this key in our player key toggles
    process_player_toggles(window); // run through local stuff based on these keytoggles
    //controller->char_callback(window, key); // set controller key toggles

    if (key == 'b')
        out_of_body = !out_of_body;
}

glm::mat4 Player::get_model_matrix() {
    glm::mat4 model = glm::mat4(1.0f);
    //model = glm::translate(model, position);
    //model = glm::rotate(model, -glm::radians(model_yaw - 90), glm::vec3(0, 1, 0));
    //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
    return model;
}

float Player::get_camera_zoom() const {
    if (out_of_body)
        return debug_camera.zoom;
    else
        return camera.zoom;
}

//Camera& active_camera() {
//    if (out_of_body)
//        return debug_camera;
//    else
//        return camera;
//}

glm::mat4 Player::get_body_view_matrix() const {
    return camera.get_view_matrix();
}

glm::mat4 Player::get_debug_view_matrix() const {
    return debug_camera.get_view_matrix();
}

glm::mat4 Player::get_view_matrix() const {
    if (out_of_body)
        return debug_camera.get_view_matrix();
    else
        return camera.get_view_matrix();
}

glm::vec3 Player::get_view_position() const {
    if (out_of_body)
        return camera.position;
    else
        return debug_camera.position;
}

void Player::debug_hud() {
    ImGui::Begin("Weapon");
    ImGui::SliderFloat3("wep offset", &active_weapon->wep_pos.x, -75.0, 75.0);
    ImGui::SliderFloat("rot", &active_weapon->wep_rot.y, -180.f, 180.0f);
    ImGui::SliderFloat("scale", &active_weapon->wep_scale, 0.0f, 2.0f);
    ImGui::End();
}

//private

// key toggle state that is more specific to the idea of a player than a controller
// think noclip mode vs plane throttle
void Player::process_player_toggles(GLFWwindow* window) {
    // R
    // TOGGLE MOUSE
    // if (!key_toggles[(unsigned)'q'])
    //     glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // else
    //     glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    //if (key_toggles[(unsigned)'i']) {
    //    printf("[CONTROLLER] setting fps \n");
    //    controller = controllers[ControllerType::FPS].get();
    //    key_toggles[(unsigned)'i'] = false;
    //}
    //if (key_toggles[(unsigned)'o']) {
    //    printf("[CONTROLLER] setting third person\n");
    //    controller = controllers[ControllerType::THIRDPERSON].get();
    //    key_toggles[(unsigned)'o'] = false;
    //}
    //if (key_toggles[(unsigned)'p']) {
    //    printf("[CONTROLLER] setting plane\n");
    //    controller = controllers[ControllerType::PLANE].get();
    //    key_toggles[(unsigned)'p'] = false;
    //}
}
// stuff we're polling for every frame that wouldnt be captured by a keycallback
// maybe can breakup into 'meta player' vs physics/game state?
void Player::poll_player(GLFWwindow* window, Scene& scene) {
    // CTRL + C
    // CLOSE WINDOW
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    // ^ example meta player control ^
    //            vs
    // v    game state control      v
    crouched = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;

    /*  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            player_physics.position.x += camera.front.x * 15.0f;
            player_physics.position.y += camera.front.y * 8.0f;
            player_physics.position.z += camera.front.z * 15.0f;
            dashing = true;
        } else {
            dashing = false;
        }*/

    bool f1_is_pressed = (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS);
    if (f1_is_pressed && !f1_was_pressed) {
        // spawn glock
        Audio::play_audio("beep.wav", 0.1f);
        Entity e(camera.position + camera.front * 5.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "glock", true);
        scene.include(e);
        scene.upload_buffers();
    }

    //if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) {
    //    Audio::play_audio("beep.wav", 0.1f);
    //    Entity e("deagle", camera.position + camera.front * 5.0f, true, glm::vec3(0.05f), 1.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    //    scene.include(e);
    //}

    //if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS) {
    //    Audio::play_audio("beep.wav", 0.1f);
    //    Entity e("sword", camera.position + camera.front * 5.0f, true, glm::vec3(1.0f), 0.1f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    //    scene.include(e);
    //}

    if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS) {
        Audio::play_audio("beep.wav", 0.1f);
        Entity e(camera.position + camera.front * 5.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "f22/scene.gltf", true);
        scene.include(e);
        scene.upload_buffers();
    }

    if (glfwGetKey(window, GLFW_KEY_F6) == GLFW_PRESS) {
        Audio::play_audio("beep.wav", 0.1f);
        Entity e(camera.position + camera.front * 5.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "link", true);
        scene.include(e);
        scene.upload_buffers();
    }

    if (glfwGetKey(window, GLFW_KEY_F7) == GLFW_PRESS) {
        Audio::play_audio("beep.wav", 0.1f);
        Entity e(camera.position + camera.front * 5.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "fuzziebox", true);
        scene.include(e);
        scene.upload_buffers();
    }

    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) {
        Physics::optimize_broad_phase();
    }
}

void Player::move_debug_camera(GLFWwindow* window, Camera& camera) {
    glm::vec3 movement(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movement.z += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movement.z -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movement.x -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movement.x += 1.0f;

    if (glm::length(movement) > 0.0f)
        movement = glm::normalize(movement);

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        movement *= 100.0f;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.position += glm::vec3(0.0f, 1.0f, 0.0f);
    }

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        camera.position -= glm::vec3(0.0f, 1.0f, 0.0f);
    }

    // Convert camera-relative movement to world space
    glm::vec3 forward = glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z));
    glm::vec3 right = glm::normalize(glm::cross(forward, camera.world_up));
    glm::vec3 acceleration = forward * movement.z + right * movement.x;

    camera.position += acceleration;
}
