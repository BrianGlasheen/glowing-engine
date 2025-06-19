#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>

enum light_type {
    DIRECTIONAL = 0,
    SPOT,
    POINT
};

class Light {
public:
    light_type type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    float inner_fov;
    float outer_fov;

    unsigned int width, height;
    unsigned int fbo, shadow_map;

    Light(){};

 /*   Light(glm::vec3 pos, glm::vec3 dir, glm::vec3 col, float intens, unsigned int w, unsigned int h)
        : position(pos), direction(dir), color(col), intensity(intens), width(w), height(h)
    {
        generate_fbo(w, h);
    }    
    */
    Light(light_type lt, glm::vec3 pos, glm::vec3 dir, glm::vec3 col, float intens, unsigned int w, unsigned int h, float fov_in = 25.0f, float fov_out = 45.0f) : position(pos), direction(dir), color(col), intensity(intens), inner_fov(fov_in), outer_fov(fov_out), width(w), height(h)
    {
        if (lt == POINT)
            generate_cubemap();
        else
            generate_fbo(w, h);
    }

    static Light create_directional(glm::vec3 dir, glm::vec3 col, float intens, unsigned int w = 1024, unsigned int h = 1024) {
        return Light(DIRECTIONAL, glm::vec3(0.0f), dir, col, intens, w, h);
    }

    static Light create_point(glm::vec3 pos, glm::vec3 col, float intens, unsigned int w = 1024, unsigned int h = 1024) {
        return Light(POINT, pos, glm::vec3(0.0f, -1.0f, 0.0f), col, intens, w, h);
    }

    static Light create_spot(glm::vec3 pos, glm::vec3 dir, glm::vec3 col, float intens, float fov_in, float fov_out, unsigned int w = 1024, unsigned int h = 1024) {
        return Light(SPOT, pos, dir, col, intens, w, h, fov_in, fov_out);
    }

    void generate_fbo(unsigned int width, unsigned int height) {
        // frame buffer
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // depth buffer texture
        glGenTextures(1, &shadow_map);
        glBindTexture(GL_TEXTURE_2D, shadow_map);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map, 0);

        // disable color buffer writes
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            printf("[LIGHT] FB error: 0x%x\n", status);
            assert(false);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void generate_cubemap() {
    }

    void bind_fbo_write() {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        glViewport(0, 0, width, height);
    }

    void bind_fbo_read(unsigned int location) {
        glActiveTexture(GL_TEXTURE0 + location);
        glBindTexture(GL_TEXTURE_2D, shadow_map);
    }
};