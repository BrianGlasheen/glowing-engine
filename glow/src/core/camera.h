#pragma once

#define NEAR_PLANE 0.1f

#include "util/math.h"

// Default camera values
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SENSITIVITY =  0.05f;
const float ZOOM        =  45.0f;

class Camera {
public:
    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;
    vec3 world_up;
    float yaw;
    float pitch;
    // camera options
    float mouse_sensitivity;
    float zoom;

    float lastX = 800, lastY = 450; // todo constructor arg

    // constructor with vectors
    Camera(vec3 pos = vec3(0.0f, 0.0f, 0.0f), 
           vec3 world_up_ = vec3(0.0f, 1.0f, 0.0f), 
           float yaw_ = YAW, float pitch_ = PITCH) 
        : front(vec3(0.0f, 0.0f, -1.0f)), 
        mouse_sensitivity(SENSITIVITY), zoom(ZOOM) {
        position = pos;
        world_up = world_up_;
        yaw = yaw_;
        pitch = pitch_;
        update_camera_vectors();
    }

    mat4 get_view_matrix() const {
        return lookAt(position, position + front, up);
    }

    mat4 get_projection(float aspect) const {
        float f = 1.0f / std::tan(radians(zoom) * 0.5f);

        mat4 result(0.0f);
        result[0][0] = f / aspect;
        result[1][1] = f;
        result[2][2] = 0.0f; // infinity
        result[2][3] = -1.0f;
        result[3][2] = NEAR_PLANE;

        return result;
    }

    mat4 get_projection(float aspect, float requested_zoom) const {
        float f = 1.0f / std::tan(radians(requested_zoom) * 0.5f);

        mat4 result(0.0f);
        result[0][0] = f / aspect;
        result[1][1] = f;
        result[2][2] = 0.0f; // infinity
        result[2][3] = -1.0f;
        result[3][2] = NEAR_PLANE;

        return result;
    }
    
    mat4 get_view_rotation_only_matrix() {
        // We can obtain just the rotation by “looking” from origin (0) to front:
        //   eye = (0,0,0)
        //   center = front (the direction we’re “looking”)
        //   up = up
        // That yields a matrix that has no translation (camera at origin).
        return lookAt(vec3(0.0f), front, up);
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void process_mouse_movement(double xpos, double ypos, bool constrainpitch = true) {
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;

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
        front.x = cos(radians(yaw)) * cos(radians(pitch));
        front.y = sin(radians(pitch));
        front.z = sin(radians(yaw)) * cos(radians(pitch));
        front = normalize(front);
        // also re-calculate the right and up vector
        right = normalize(cross(front, world_up));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        up    = normalize(cross(right, front));
    }
};
