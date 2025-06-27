#ifndef CONTROLLER_PLANE_H
#define CONTROLLER_PLANE_H
//
//#include <vector>
//#include <tuple>
//#include <algorithm>
//#include <cmath>
//
//#include <glm/glm.hpp>
//#include <glm/gtc/quaternion.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//
//#include <core/camera.h>
//#include <core/scene.h>
//
//// Constants
//const float PI = 3.14159f;
//const glm::vec3 FORWARD = glm::vec3(0.0f, 0.0f, -1.0f);
//const glm::vec3 UP = glm::vec3(0.0f, 1.0f, 0.0f);
//const glm::vec3 RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
//
//// Helper functions
//inline float sq(float x) { return x * x; }
//inline float scale(float value, float min1, float max1, float min2, float max2) {
//    return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
//}
//
//// Airfoil data for simple NACA airfoils
//// Format: {alpha (angle of attack in degrees), Cl (lift coefficient), Cd (drag coefficient)}
//std::vector<glm::vec3> NACA_0012_data = {
//    {-18.500f, -1.2258f, 0.10236f},
//    {-18.000f, -1.2070f, 0.09738f},
//    {-17.500f, -1.1870f, 0.09241f},
//    {-17.000f, -1.1656f, 0.08746f},
//    {-16.000f, -1.1196f, 0.07759f},
//    {-15.000f, -1.0698f, 0.06775f},
//    {-14.000f, -1.0162f, 0.05794f},
//    {-13.000f, -0.9591f, 0.04815f},
//    {-12.000f, -0.8986f, 0.03840f},
//    {-11.000f, -0.8350f, 0.02865f},
//    {-10.000f, -0.7679f, 0.01893f},
//    {-9.000f, -0.6984f, 0.01211f},
//    {-8.000f, -0.6265f, 0.00708f},
//    {-7.000f, -0.5521f, 0.00375f},
//    {-6.000f, -0.4756f, 0.00165f},
//    {-5.000f, -0.3969f, 0.00056f},
//    {-4.000f, -0.3169f, 0.00011f},
//    {-3.000f, -0.2366f, 0.00002f},
//    {-2.000f, -0.1563f, 0.00012f},
//    {-1.000f, -0.0784f, 0.00052f},
//    {0.000f, 0.0000f, 0.00089f},
//    {1.000f, 0.0784f, 0.00052f},
//    {2.000f, 0.1563f, 0.00012f},
//    {3.000f, 0.2366f, 0.00002f},
//    {4.000f, 0.3169f, 0.00011f},
//    {5.000f, 0.3969f, 0.00056f},
//    {6.000f, 0.4756f, 0.00165f},
//    {7.000f, 0.5521f, 0.00375f},
//    {8.000f, 0.6265f, 0.00708f},
//    {9.000f, 0.6984f, 0.01211f},
//    {10.000f, 0.7679f, 0.01893f},
//    {11.000f, 0.8350f, 0.02865f},
//    {12.000f, 0.8986f, 0.03840f},
//    {13.000f, 0.9591f, 0.04815f},
//    {14.000f, 1.0162f, 0.05794f},
//    {15.000f, 1.0698f, 0.06775f},
//    {16.000f, 1.1196f, 0.07759f},
//    {17.000f, 1.1656f, 0.08746f},
//    {17.500f, 1.1870f, 0.09241f},
//    {18.000f, 1.2070f, 0.09738f},
//    {18.500f, 1.2284f, 0.10229f}
//};
//
//// NACA 2412 airfoil data (better lift for main wings)
//std::vector<glm::vec3> NACA_2412_data = {
//    {-18.000f, -1.1000f, 0.11000f},
//    {-16.000f, -1.0000f, 0.09000f},
//    {-14.000f, -0.9000f, 0.07000f},
//    {-12.000f, -0.8000f, 0.05500f},
//    {-10.000f, -0.6500f, 0.04000f},
//    {-8.000f, -0.5000f, 0.02500f},
//    {-6.000f, -0.3500f, 0.01500f},
//    {-4.000f, -0.2000f, 0.00800f},
//    {-2.000f, -0.0500f, 0.00600f},
//    {0.000f, 0.1200f, 0.00600f},
//    {2.000f, 0.3000f, 0.00700f},
//    {4.000f, 0.4500f, 0.00900f},
//    {6.000f, 0.6000f, 0.01500f},
//    {8.000f, 0.7500f, 0.02500f},
//    {10.000f, 0.8800f, 0.04000f},
//    {12.000f, 1.0000f, 0.05500f},
//    {14.000f, 1.1000f, 0.07000f},
//    {16.000f, 1.2000f, 0.09000f},
//    {18.000f, 1.3000f, 0.11000f}
//};
//
//// International Standard Atmosphere model for air density calculation
//namespace isa {
//    inline float get_air_density(float altitude) {
//        const float sea_level_density = 1.225f; // kg/m^3
//        const float scale_height = 8500.0f;     // meters
//        
//        // Simple exponential model for atmosphere
//        return sea_level_density * exp(-altitude / scale_height);
//    }
//}
//
//// Airfoil definition
//struct Airfoil {
//    const float min_alpha, max_alpha;
//    float cl_max;
//    std::vector<glm::vec3> data;
//
//    Airfoil(const std::vector<glm::vec3>& curve) 
//        : data(curve), 
//          min_alpha(curve.front().x), 
//          max_alpha(curve.back().x),
//          cl_max(0.0f) // Initialize to 0 and update in constructor body
//    {
//        // Find maximum lift coefficient in the data
//        for (const auto& point : data) {
//            cl_max = std::max(cl_max, std::abs(point.y));
//        }
//    }
//
//    // Get lift and drag coefficients for a given angle of attack
//    std::tuple<float, float> sample(float alpha) const {
//        printf("not sampled \n");
//        int i = static_cast<int>(scale(alpha, min_alpha, max_alpha, 0, data.size() - 1));
//        printf("%d %f %f %f", data.size(), alpha, min_alpha, max_alpha);
//        printf("sampled %d\n", i);
//        return { data[i].y, data[i].z };
//
//        // alpha = glm::clamp(alpha, min_alpha, max_alpha);
//        
//        // // Find indices for interpolation
//        // float scaled = scale(alpha, min_alpha, max_alpha, 0, data.size() - 1);
//        // int idx = static_cast<int>(scaled);
//        // int next_idx = std::min(idx + 1, static_cast<int>(data.size() - 1));
//        // printf("sample 3\n");
//        
//        // // Interpolate between data points
//        // float t = scaled - static_cast<float>(idx);
//        // printf("here \n");
//        // float cl = glm::mix(data[idx].y, data[next_idx].y, t);
//        // printf("yeah \n");
//        // float cd = glm::mix(data[idx].z, data[next_idx].z, t);
//        // printf("return \n");
//        
//        // return {cl, cd};
//    }
//};
//
//// Wing/control surface definition
//struct Wing {
//    const Airfoil* airfoil;
//    glm::vec3 center_of_pressure;  // Position relative to center of gravity
//    glm::vec3 normal;              // Normal direction of the wing (usually UP)
//    float area;                    // Surface area in m²
//    float chord;                   // Wing chord length
//    float wingspan;                // Wing span
//    float aspect_ratio;            // Aspect ratio (wingspan²/area)
//    float efficiency_factor = 0.7f; // Oswald efficiency factor
//    float flap_ratio;              // Ratio of the wing that can be deflected
//    float control_input = 0.0f;    // Control input (-1.0 to 1.0)
//
//    Wing(const glm::vec3& position = glm::vec3(0.0f), 
//         float span = 1.0f, 
//         float chord = 1.0f, 
//         const Airfoil* airfoil = nullptr, 
//         const glm::vec3& normal = UP,
//         float flap_ratio = 0.25f)
//        : airfoil(airfoil),
//          center_of_pressure(position),
//          normal(normal),
//          area(span * chord),
//          chord(chord),
//          wingspan(span),
//          aspect_ratio(sq(span) / (span * chord)),
//          flap_ratio(flap_ratio)
//    {
//    }
//
//    void set_control_input(float input) { 
//        control_input = glm::clamp(input, -1.0f, 1.0f); 
//    }
//
//    // wing does force
//    void apply_force() {
//    //    if (!airfoil) return;
//    //    printf("1\n");
//    //    // Get velocity in world space and convert to local space
//    //    glm::vec3 world_velocity = physics.velocity;
//    //    glm::vec3 local_velocity = glm::inverse(physics.orientation) * world_velocity;
//    //    
//    //    printf("2\n");
//    //    // Add contribution of angular velocity at this point
//    //    local_velocity += glm::cross(physics.angular_velocity, center_of_pressure);
//    //    
//    //    printf("3\n");
//    //    float speed = glm::length(local_velocity);
//
//    //    printf("4\n");
//    //    // Skip if speed is too low to generate meaningful forces
//    //    if (speed <= 1.0f) return;
//
//    //    printf("5\n");
//    //    // Drag acts in the opposite direction of velocity
//    //    glm::vec3 drag_direction = glm::normalize(-local_velocity);
//
//    //    printf("6\n");
//    //    // Calculate wing-relative normal based on control input
//    //    glm::vec3 effective_normal = normal;
//    //    if (flap_ratio > 0.0f && std::abs(control_input) > 0.01f) {
//    //        // Deflect the normal based on control input
//    //        // This is a simplification of control surface deflection
//    //    printf("7\n");
//    //        glm::vec3 rotation_axis = glm::cross(normal, drag_direction);
//    //        if (glm::length(rotation_axis) > 0.01f) {
//    //            rotation_axis = glm::normalize(rotation_axis);
//    //            float deflection_angle = control_input * 20.0f * (flap_ratio); // max 20 degrees deflection
//    //            glm::quat deflection = glm::angleAxis(glm::radians(deflection_angle), rotation_axis);
//    //            effective_normal = deflection * normal;
//    //        }
//    //    }
//
//    //    printf("8\n");
//    //    // Lift is perpendicular to drag and wing normal
//    //    glm::vec3 lift_direction = glm::normalize(glm::cross(glm::cross(drag_direction, effective_normal), drag_direction));
//
//    //    printf("9\n");
//    //    printf("%f, %f, %f", drag_direction.x, drag_direction.y, drag_direction.z);
//    //    // Angle between chord line and air flow (angle of attack)
//    //    float angle_of_attack = glm::degrees(std::asin(glm::clamp(glm::dot(drag_direction, effective_normal), -1.0f, 1.0f)));
//
//    //    printf("10\n");
//    //    // Sample airfoil coefficients
//    //    printf("sampling %f", angle_of_attack);
//    //    auto [lift_coeff, drag_coeff] = airfoil->sample(angle_of_attack);
//
//    //    printf("11\n");
//    //    // Adjust lift based on control surface deflection
//    //    if (flap_ratio > 0.0f && std::abs(control_input) > 0.01f) {
//    //        float deflection_ratio = control_input;
//    //        float delta_lift_coeff = sqrt(flap_ratio) * airfoil->cl_max * deflection_ratio * 0.4f;
//    //        lift_coeff += delta_lift_coeff;
//    //    }
//
//    //    printf("12\n");
//    //    // Calculate induced drag (increases with lift)
//    //    float induced_drag_coeff = sq(lift_coeff) / (PI * aspect_ratio * efficiency_factor);
//    //    drag_coeff += induced_drag_coeff;
//
//    //    printf("13\n");
//    //    // Calculate air density based on altitude
//    //    float air_density = isa::get_air_density(physics.position.y);
//
//    //    printf("14\n");
//    //    // Calculate dynamic pressure
//    //    float dynamic_pressure = 0.5f * sq(speed) * air_density * area;
//
//    //    printf("15\n");
//    //    // Calculate lift and drag forces
//    //    glm::vec3 lift = lift_direction * lift_coeff * dynamic_pressure;
//    //    glm::vec3 drag = drag_direction * drag_coeff * dynamic_pressure;
//
//    //    printf("16\n");
//    //    // Transform forces to world space
//    //    glm::vec3 world_force = physics.orientation * (lift + drag);
//    //    
//    //    printf("17\n");
//    //    // Apply forces to the rigid body
//    //    physics.velocity += world_force * (1.0f / physics.mass);
//    //    
//    //    printf("18\n");
//    //    // Calculate torque (cross product of force arm and force)
//    //    glm::vec3 local_torque = glm::cross(center_of_pressure, lift + drag);
//    //    glm::vec3 world_torque = physics.orientation * local_torque;
//    //    
//    //    printf("20\n");
//    //    // Apply torque directly to angular velocity
//    //    physics.angular_velocity += world_torque * 0.01f; // Scaling factor for stability
//    }
//};
//
//// Aircraft engine
//struct Engine {
//    float thrust;       // Maximum thrust in newtons
//    float throttle;     // Current throttle setting (0.0 to 1.0)
//    
//    Engine(float max_thrust = 50000.0f) : 
//        thrust(max_thrust), 
//        throttle(0.0f) {}
//    
//    void set_throttle(float value) {
//        throttle = glm::clamp(value, 0.0f, 1.0f);
//    }
//    
//    void apply_force() {
//        //glm::vec3 forward = physics.orientation * FORWARD;
//        //physics.velocity += forward * (thrust * throttle / physics.mass);
//    }
//};
//
//// Aircraft definition
//struct Plane {
//    Engine engine;
//    std::vector<Wing> wings;
//    float elevator_input = 0.0f;   // Pitch control (-1 to 1)
//    float aileron_input = 0.0f;    // Roll control (-1 to 1)
//    float rudder_input = 0.0f;     // Yaw control (-1 to 1)
//    float flaps_input = 0.0f;      // Flaps (0 to 1)
//    glm::vec3 cockpit_offset;      // Cockpit position relative to center of gravity
//    
//    Plane() : 
//        engine(20000.0f),  // Default engine thrust
//        cockpit_offset(0.0f, 1.0f, 0.0f)  // Default cockpit position
//    {
//        // Initialize with basic airfoils
//        const Airfoil* naca0012 = new Airfoil(NACA_0012_data);
//        const Airfoil* naca2412 = new Airfoil(NACA_2412_data);
//        
//        // Create wings configuration similar to the article
//        const float wing_offset = -1.0f;   // Wing position relative to CG
//        const float tail_offset = -6.0f;   // Tail position relative to CG
//        
//        // Main wings
//        wings.push_back(Wing({wing_offset, 0.0f, -3.0f}, 7.0f, 2.5f, naca2412));      // Left wing
//        wings.push_back(Wing({wing_offset, 0.0f, 3.0f}, 7.0f, 2.5f, naca2412));       // Right wing
//        
//        // Control surfaces
//        wings.push_back(Wing({wing_offset - 1.5f, 0.0f, -2.0f}, 3.0f, 1.2f, naca0012)); // Left aileron
//        wings.push_back(Wing({wing_offset - 1.5f, 0.0f, 2.0f}, 3.0f, 1.2f, naca0012));  // Right aileron
//        wings.push_back(Wing({tail_offset, -0.1f, 0.0f}, 6.5f, 2.7f, naca0012));       // Elevator
//        wings.push_back(Wing({tail_offset, 0.0f, 0.0f}, 5.3f, 3.1f, naca0012, RIGHT)); // Rudder
//    }
//    
//    void update_controls() {
//        // Map control inputs to wing deflections
//        // Main wings - use flaps_input
//        wings[0].set_control_input(flaps_input);  // Left wing
//        wings[1].set_control_input(flaps_input);  // Right wing
//        
//        // Ailerons - opposing deflection for roll
//        wings[2].set_control_input(-aileron_input);  // Left aileron
//        wings[3].set_control_input(aileron_input);   // Right aileron
//        
//        // Elevator - pitch control
//        wings[4].set_control_input(elevator_input);
//        
//        // Rudder - yaw control
//        wings[5].set_control_input(rudder_input);
//    }
//    
//    void apply_aerodynamics(float dt) {
//        //engine.apply_force(physics);
//        
//        // Apply wing forces
//        //int c = 0;
//        //for (auto& wing : wings) {
//        //    printf("wing %d\n", c++);
//        //    wing.apply_force(physics);
//        //}
//        //printf("success!\n");
//    }
//};
//
//// Plane controller
//class Controller_plane : public Controller {
//public:
//    float lastX = 0.0f;
//    float lastY = 0.0f;
//    
//    // Camera settings
//    bool is_third_person = true;
//    float THIRD_PERSON_DISTANCE = 15.0f;
//    float PLANE_HEIGHT = 1.8f;
//    
//    // Input smoothing
//    float elevator_smoothed = 0.0f;
//    float aileron_smoothed = 0.0f;
//    float rudder_smoothed = 0.0f;
//    float throttle_smoothed = 0.5f;
//    bool mouse_control = false;
//    
//    // For view controls
//    bool toggle_mouse_lock = true;
//    bool key_toggles[256] = {false};
//    
//    // Plane instance
//    Plane plane;
//    
//    Controller_plane() {
//        // Calculate inertia tensor for aircraft
//        //player_physics.inertia = glm::mat3(
//        //    48531.0f, -1320.0f, 0.0f,
//        //    -1320.0f, 256608.0f, 0.0f,
//        //    0.0f, 0.0f, 211333.0f
//        //);
//        //
//        //// Compute inverse inertia
//        //player_physics.inverse_inertia = glm::inverse(player_physics.inertia);
//        //
//        //// Initialize the plane's engine
//        //plane.engine.set_throttle(0.5f);
//    }
//    
//    virtual void mouse_callback(GLFWwindow* window, Camera& camera, double xpos, double ypos, float& model_yaw) override {
//        float xoffset = xpos - lastX;
//        float yoffset = lastY - ypos;
//        lastX = xpos;
//        lastY = ypos;
//        
//        if (mouse_control) {
//            // Use mouse for flight controls when in mouse control mode
//            const float sensitivity = 0.05f;
//            elevator_smoothed = glm::clamp(elevator_smoothed - yoffset * sensitivity * 0.01f, -1.0f, 1.0f);
//            aileron_smoothed = glm::clamp(aileron_smoothed + xoffset * sensitivity * 0.01f, -1.0f, 1.0f);
//        } else {
//            // Otherwise use mouse for camera control
//            camera.process_mouse_movement(xoffset, yoffset);
//        }
//    }
//    
//    virtual void scroll_callback(GLFWwindow* window, Camera& camera, double xoffset, double yoffset) override {
//        // Adjust throttle with scroll wheel
//        throttle_smoothed = glm::clamp(throttle_smoothed + static_cast<float>(yoffset) * 0.05f, 0.0f, 1.0f);
//        plane.engine.set_throttle(throttle_smoothed);
//
//        // Also allow camera zoom
//        camera.process_mouse_scroll(static_cast<float>(yoffset));
//    }
//    
//    virtual void char_callback(GLFWwindow* window, uint32_t key) override {
//        // key_toggles[key] = !key_toggles[key];
//        
//        // // Toggle view mode with 'v'
//        // if (key == 'v') {
//        //     is_third_person = !is_third_person;
//        // }
//        
//        // // Toggle mouse control with 'm'
//        // if (key == 'm') {
//        //     mouse_control = !mouse_control;
//        //     if (mouse_control) {
//        //         glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//        //     } else {
//        //         if (!toggle_mouse_lock) {
//        //             glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
//        //         }
//        //     }
//        // }
//    }
//    
//    virtual void process_input(GLFWwindow* window, float deltaTime, Scene& scene, Camera& camera, float& model_yaw) override {
//        // Input smoothing rate
//        // const float SMOOTH_RATE = 2.0f * deltaTime;
//        
//        // // Flight controls with keyboard
//        // float elevator_target = 0.0f;
//        // float aileron_target = 0.0f;
//        // float rudder_target = 0.0f;
//        
//        // // Pitch control
//        // if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
//        //     elevator_target += 1.0f;  // Pitch up
//        // if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
//        //     elevator_target -= 1.0f;  // Pitch down
//            
//        // // Roll control
//        // if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
//        //     aileron_target -= 1.0f;   // Roll left
//        // if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
//        //     aileron_target += 1.0f;   // Roll right
//            
//        // // Yaw control (rudder)
//        // if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
//        //     rudder_target -= 1.0f;    // Yaw left
//        // if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
//        //     rudder_target += 1.0f;    // Yaw right
//            
//        // // Throttle control
//        // if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
//        //     throttle_smoothed = glm::min(throttle_smoothed + 0.5f * deltaTime, 1.0f);
//        // if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
//        //     throttle_smoothed = glm::max(throttle_smoothed - 0.5f * deltaTime, 0.0f);
//            
//        // // Flaps control
//        // if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
//        //     plane.flaps_input = glm::min(plane.flaps_input + 0.5f * deltaTime, 1.0f);
//        // if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
//        //     plane.flaps_input = glm::max(plane.flaps_input - 0.5f * deltaTime, 0.0f);
//        
//        // // If not using mouse control, smooth keyboard inputs
//        // if (!mouse_control) {
//        //     elevator_smoothed = glm::mix(elevator_smoothed, elevator_target, SMOOTH_RATE);
//        //     aileron_smoothed = glm::mix(aileron_smoothed, aileron_target, SMOOTH_RATE);
//        // }
//        
//        // // Always smooth rudder input
//        // rudder_smoothed = glm::mix(rudder_smoothed, rudder_target, SMOOTH_RATE);
//        
//        // // Update plane controls
//        // plane.elevator_input = elevator_smoothed;
//        // plane.aileron_input = aileron_smoothed;
//        // plane.rudder_input = rudder_smoothed;
//        // plane.engine.set_throttle(throttle_smoothed);
//        
//        // // Update plane control surfaces
//        // plane.update_controls();
//        
//        // // Update model yaw based on orientation
//        // model_yaw = glm::degrees(glm::eulerAngles(player_physics.orientation).y);
//    }
//    
//    virtual void update_physics(float deltaTime, Camera& camera) override {
//
//        // // Apply aerodynamic forces
//        //plane.apply_aerodynamics(player_physics, deltaTime);
//        
//        // // Apply gravity
//        // player_physics.velocity.y -= GRAVITY * deltaTime;
//        
//        // // Apply minimum drag/resistance for stability
//        // const float AIR_RESISTANCE = 0.01f;
//        // player_physics.velocity *= (1.0f - AIR_RESISTANCE * deltaTime);
//        // player_physics.angular_velocity *= (1.0f - AIR_RESISTANCE * 2.0f * deltaTime);
//        
//        // // Update position based on velocity
//        // player_physics.position += player_physics.velocity * deltaTime;
//        
//        // // Update orientation based on angular velocity
//        // glm::quat rotation = glm::quat(0.0f, player_physics.angular_velocity.x * 0.5f * deltaTime,
//        //                              player_physics.angular_velocity.y * 0.5f * deltaTime,
//        //                              player_physics.angular_velocity.z * 0.5f * deltaTime);
//        // player_physics.orientation = glm::normalize(player_physics.orientation + player_physics.orientation * rotation);
//        
//        // // Ground collision detection
//        // const float GROUND_LEVEL = 0.0f;
//        // if (player_physics.position.y < GROUND_LEVEL + 1.0f) {
//        //     player_physics.position.y = GROUND_LEVEL + 1.0f;
//        //     player_physics.velocity.y = 0.0f;
//        //     player_physics.isOnGround = true;
//        // } else {
//        //     player_physics.isOnGround = false;
//        // }
//    }
//    
//    virtual void update_camera(Camera& camera, bool crouched, float player_height) override {
//        // Extract forward, up, and right vectors from player physics orientation
//        // glm::vec3 forward = player_physics.orientation * FORWARD;
//        // glm::vec3 up = player_physics.orientation * UP;
//        // glm::vec3 right = player_physics.orientation * RIGHT;
//        
//        // if (is_third_person) {
//        //     // Third-person camera positioned behind the plane
//        //     glm::vec3 offset = -forward * THIRD_PERSON_DISTANCE + up * 3.0f;
//        //     camera.position = player_physics.position + offset;
//            
//        //     // Point camera at plane
//        //     camera.front = -offset;
//        //     camera.front = glm::normalize(camera.front);
//            
//        //     // Recalculate camera vectors
//        //     camera.right = glm::normalize(glm::cross(camera.front, camera.world_up));
//        //     camera.up = glm::normalize(glm::cross(camera.right, camera.front));
//        // } else {
//        //     // First-person from cockpit
//        //     glm::vec3 cockpit_world_pos = player_physics.position + (player_physics.orientation * plane.cockpit_offset);
//        //     camera.position = cockpit_world_pos;
//            
//        //     // Set camera orientation to match plane
//        //     camera.front = forward;
//        //     camera.right = right;
//        //     camera.up = up;
//            
//        //     // Update camera angles to match orientation
//        //     glm::vec3 euler = glm::eulerAngles(player_physics.orientation);
//        //     camera.yaw = -glm::degrees(euler.y) + 90.0f;
//        //     camera.pitch = -glm::degrees(euler.x);
//        // }
//    }
//
//    virtual glm::vec3 get_weapon_position() const override {
//        return glm::vec3(0.0f, -99999.0f, 0.0f);
//    }
//};
#endif