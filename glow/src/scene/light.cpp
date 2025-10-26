//#include "scene/light.h"
//
//#include <cstdio>
//
//// #include <glad/glad.h>
//#include "core/opengl.h"
//
//Light::Light(Light_Type lt,
//             glm::vec3 pos,
//             glm::vec3 dir,
//             glm::vec3 col,
//             float intens,
//             uint32_t w,
//             uint32_t h,
//             float fov_in,
//             float fov_out)
//        : type(lt),
//        position(pos),
//        direction(dir),
//        color(col),
//        intensity(intens),
//        inner_fov(fov_in),
//        outer_fov(fov_out),
//        width(w), height(h)
//{
//    if (lt == POINT)
//        generate_cubemap(w);
//    else
//        generate_fbo(w, h);
//}
//
//// TODO CLEAN TF UP HOLY
//Light Light::create_directional(glm::vec3 dir, glm::vec3 col, float intens, uint32_t w, uint32_t h) {
//    return Light(Light_Type::DIRECTIONAL, glm::vec3(0.0f), dir, col, intens, w, h);
//}
//
//Light Light::create_point(glm::vec3 pos, glm::vec3 col, float intens, uint32_t w, uint32_t h) {
//    return Light(Light_Type::POINT, pos, glm::vec3(0.0f, -1.0f, 0.0f), col, intens, w, h);
//}
//
//Light Light::create_spot(glm::vec3 pos, glm::vec3 dir, glm::vec3 col, float intens, float fov_in, float fov_out, uint32_t w, uint32_t h) {
//    return Light(Light_Type::SPOT, pos, dir, col, intens, w, h, fov_in, fov_out);
//}
//
//void Light::generate_fbo(uint32_t width, uint32_t height) {
//    // frame buffer
//    glGenFramebuffers(1, &fbo);
//    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//
//    // depth buffer texture
//    glGenTextures(1, &shadow_map);
//    glBindTexture(GL_TEXTURE_2D, shadow_map);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
//    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
//    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
//
//    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map, 0);
//
//    // disable color buffer writes
//    glDrawBuffer(GL_NONE);
//    glReadBuffer(GL_NONE);
//
//    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
//    if (status != GL_FRAMEBUFFER_COMPLETE) {
//        printf("[LIGHT] FB error: 0x%x\n", status);
//        assert(false);
//    }
//
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//}
//
//void Light::generate_cubemap(uint32_t width) {
//    glGenTextures(1, &cube_depth);
//    glBindTexture(GL_TEXTURE_2D, cube_depth);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, width, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glBindTexture(GL_TEXTURE_2D, 0);
//
//    // Create the cube map
//    glGenTextures(1, &shadow_map);
//    glBindTexture(GL_TEXTURE_CUBE_MAP, shadow_map);
//    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
//
//
//    for (uint32_t i = 0; i < 6; i++) {
//        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_R32F, width, width, 0, GL_RED, GL_FLOAT, NULL);
//    }
//
//    // Create the FBO
//    glGenFramebuffers(1, &fbo);
//    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, cube_depth, 0);
//
//    // Disable writes to the color buffer
//    glDrawBuffer(GL_NONE);
//    // Disable reads from the color buffer
//    glReadBuffer(GL_NONE);
//
//    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
//
//    if (Status != GL_FRAMEBUFFER_COMPLETE) {
//        printf("FB error, status: 0x%x\n", Status);
//        assert(false);
//    }
//
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
//}
//
//void Light::bind_fbo_write() {
//    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
//    glViewport(0, 0, width, height);
//}
//
//void Light::bind_cubemap_face_write(uint32_t face) {
//    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
//    glViewport(0, 0, width, width);  // set the width/height of the shadow map!
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, face, shadow_map, 0);
//    glDrawBuffer(GL_COLOR_ATTACHMENT0);
//}
//
//void Light::bind_fbo_read(uint32_t location) {
//    glActiveTexture(GL_TEXTURE0 + location);
//    glBindTexture(type == Light_Type::POINT ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, shadow_map);
//}
