#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Default camera values
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SENSITIVITY =  0.05f;
const float ZOOM        =  45.0f;

class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 world_up;
    float yaw;
    float pitch;
    // camera options
    float mouse_sensitivity;
    float zoom;

    // constructor with vectors
    Camera(glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f), 
           glm::vec3 world_up_ = glm::vec3(0.0f, 1.0f, 0.0f), 
           float yaw_ = YAW, float pitch_ = PITCH) 
        : front(glm::vec3(0.0f, 0.0f, -1.0f)), 
        mouse_sensitivity(SENSITIVITY), zoom(ZOOM) {
        position = pos;
        world_up = world_up_;
        yaw = yaw_;
        pitch = pitch_;
        update_camera_vectors();
    }

    glm::mat4 get_view_matrix() const {
        return glm::lookAt(position, position + front, up);
    }
    
    glm::mat4 get_view_rotation_only_matrix() {
        // We can obtain just the rotation by “looking” from origin (0) to front:
        //   eye = (0,0,0)
        //   center = front (the direction we’re “looking”)
        //   up = up
        // That yields a matrix that has no translation (camera at origin).
        return glm::lookAt(glm::vec3(0.0f), front, up);
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void process_mouse_movement(float xoffset, float yoffset, bool constrainpitch = true) {
        xoffset *= mouse_sensitivity;
        yoffset *= mouse_sensitivity;
        yaw   += xoffset;
        pitch += yoffset;
        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainpitch) {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }
        // update front, right and up Vectors using the updated Euler angles
        update_camera_vectors();
    }
    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void process_mouse_scroll(float yoffset) {
        zoom -= (float)yoffset;
        if (zoom < 1.0f)
            zoom = 1.0f;
        if (zoom > 360.0f)
            zoom = 360.0f;
    }

// private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void update_camera_vectors() {
        // calculate the new front vector
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);
        // also re-calculate the right and up vector
        right = glm::normalize(glm::cross(front, world_up));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        up    = glm::normalize(glm::cross(right, front));
    }
};
