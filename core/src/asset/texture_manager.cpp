#include <vector>
#include <string>
#include <iostream>
#include <cassert>

// #include <glad/glad.h>
#include "core/opengl.h"

#include <stb_image.h>

#include "texture_manager.h"

namespace Texture_Manager {

    static std::vector<uint32_t> textures;
    static std::vector<std::string> paths;
    //static std::vector<texture_data> texture_data;

    bool loaded_already(const std::string& new_path, size_t& existing_idx) {
        for (size_t i = 0; i < paths.size(); i++) {
            if (new_path == paths[i]) {
                existing_idx = i;
                return true;
            }
        }
        return false;
    }

    void init() {
        //stbi_set_flip_vertically_on_load(true);
        texture_handle zero = load_from_path("../resources/textures/missing.png");
        assert(!textures.empty());
    }

    void cleanup() {
        for (uint32_t texture : textures) {
            if (texture != 0) {
                glDeleteTextures(1, &texture);
            }
        }
        textures.clear();
        paths.clear();
    }

    texture_handle load_from_path(const std::string& file_path) {
        size_t existing_texture_index;
        if (loaded_already(file_path, existing_texture_index)) {
            return existing_texture_index;
        }

        uint32_t texture_id = 0;
        glGenTextures(1, &texture_id);
        int width, height, nrComponents;
        unsigned char* data = stbi_load(file_path.c_str(), &width, &height, &nrComponents, 0);

        if (data) {
            //GLenum format = 0;
            //if (nrComponents == 1) format = GL_RED;
            //else if (nrComponents == 3) format = GL_RGB;
            //else if (nrComponents == 4) format = GL_RGBA;

            GLenum internalFormat = 0, dataFormat = 0;
            if (nrComponents == 1) {
                internalFormat = dataFormat = GL_RED;
            }
            else if (nrComponents == 3) {
                internalFormat = GL_SRGB;
                dataFormat = GL_RGB;
            }
            else if (nrComponents == 4) {
                internalFormat = GL_SRGB_ALPHA;
                dataFormat = GL_RGBA;
            }
            else {
                std::cerr << "Unsupported number of texture components: " << nrComponents << std::endl;
                stbi_image_free(data);
                return 0;
            }


            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            textures.push_back(texture_id);
            paths.push_back(file_path);
            std::cout << "[TEXTURE] Loaded: " << file_path << std::endl;
            return textures.size() - 1;
        }
        else {
            std::cout << "Texture failed to load: " << file_path << std::endl;
            stbi_image_free(data);
            return 0;
        }
    }

    void resize(const texture_handle handle, int width, int height, int mips) {
        uint32_t texture_id = get_ogl_id(handle);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        GLint internal_format;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);

        GLenum format, type;
        switch (internal_format) {
        case GL_RGBA16F:
        case GL_RGBA32F:
            format = GL_RGBA;
            type = GL_FLOAT;
            break;
        case GL_DEPTH_COMPONENT32F:
            format = GL_DEPTH_COMPONENT;
            type = GL_FLOAT;
            break;
        case GL_R8:
            format = GL_RED;
            type = GL_UNSIGNED_BYTE;
            break;
        default:
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            break;
        }

        for (int level = 0; level < mips; level++) {
            int mip_width = std::max(1, width >> level);
            int mip_height = std::max(1, height >> level);

            glTexImage2D(GL_TEXTURE_2D, level, internal_format, mip_width, mip_height, 0, format, type, nullptr);
        }

        if (mips > 1)
            glGenerateMipmap(GL_TEXTURE_2D);

        printf("[TEXTURE] %s resized to width: %d, height: %d\n", paths[handle].c_str(), width, height);
    }

    texture_handle load_msdf(const std::string& file_path) {
        size_t existing_texture_index;
        if (loaded_already(file_path, existing_texture_index)) {
            return existing_texture_index;
        }

        uint32_t texture_id = 0;
        glGenTextures(1, &texture_id);
        int width, height, nrComponents;
        unsigned char* data = stbi_load(file_path.c_str(), &width, &height, &nrComponents, 0);

        if (data) {
            GLenum internalFormat = 0, dataFormat = 0;
            if (nrComponents == 3) {
                internalFormat = GL_RGB8;
                dataFormat = GL_RGB;
            }
            else {
                std::cerr << "MSDF texture must have 3 channels (RGB). Got: " << nrComponents << std::endl;
                stbi_image_free(data);
                assert(false);
            }

            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

            // no mip maps
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            stbi_image_free(data);
            textures.push_back(texture_id);
            paths.push_back(file_path);
            std::cout << "[MSDF TEXTURE] Loaded: " << file_path << std::endl;
            return textures.size() - 1;
        }
        else {
            std::cerr << "Failed to load MSDF texture: " << file_path << std::endl;
            stbi_image_free(data);
            return 0;
        }
    }

    texture_handle create_depth_texture(int width, int height) {
        uint32_t texture_id = 0;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        textures.push_back(texture_id);
        paths.push_back("depth_texture");

        std::cout << "[DEPTH TEXTURE] Created " << width << "x" << height << std::endl;
        return textures.size() - 1;
    }

    texture_handle create_render_texture(int width, int height, bool hdr) {
        uint32_t texture_id = 0;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        if (hdr) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        }
        else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }

        // Set texture parameters - important for framebuffer textures
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);

        textures.push_back(texture_id);
        paths.push_back(hdr ? "render_texture_hdr" : "render_texture");

        std::cout << "[RENDER TEXTURE] Created " << (hdr ? "HDR " : "") << width << "x" << height << std::endl;
        return textures.size() - 1;
    }

    texture_handle create_bloom_texture(int width, int height) {
        uint32_t texture_id = 0;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        // Calculate number of mip levels
        int mip_levels = 6;

        // Allocate storage for all mip levels
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);

        textures.push_back(texture_id);
        paths.push_back("bloom_texture_with_mips");

        std::cout << "[BLOOM TEXTURE] Created with " << mip_levels << " mip levels: " << width << "x" << height << std::endl;
        return textures.size() - 1;
    }

    texture_handle create_ssao_texture(int width, int height) {
        uint32_t texture_id = 0;
        glGenTextures(1, &texture_id);

        glBindTexture(GL_TEXTURE_2D, texture_id);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);

        textures.push_back(texture_id);
        paths.push_back("ssao_texture");
        return textures.size() - 1;
    }

    texture_handle create_noise_texture(const std::vector<float>& data, int width, int height) {
        uint32_t texture_id = 0;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glBindTexture(GL_TEXTURE_2D, 0);

        textures.push_back(texture_id);
        paths.push_back("noise_texture"); // todo maybe change if multiple of these
        return textures.size() - 1;
    }

    texture_handle create_3d_texture(int width, int height, int layers) {
        uint32_t texture_id = 0;
        glGenTextures(1, &texture_id);

        glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id);
        glTexImage3D(
            GL_TEXTURE_2D_ARRAY, // todo arg?
            0,
            GL_DEPTH_COMPONENT32F, // todo arg
            width,
            height,
            layers,
            0,
            GL_DEPTH_COMPONENT, // todo arg
            GL_FLOAT, // todo arg?
            nullptr);

        //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

        /*glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);*/

        glBindTexture(GL_TEXTURE_2D, 0);

        textures.push_back(texture_id);
        paths.push_back("3d"); // todo maybe change if multiple of these
        return textures.size() - 1;
    }

    texture_handle create_2d_array_texture(int width, int height, int layers) {
        uint32_t texture_id = 0;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id);

        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, width, height, layers, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f};
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

        textures.push_back(texture_id);
        paths.push_back("2d"); // todo maybe change if multiple of these
        return textures.size() - 1;
    }

    void bind(texture_handle texture_id, uint32_t texture_unit) {
        glActiveTexture(GL_TEXTURE0 + texture_unit);
        glBindTexture(GL_TEXTURE_2D, textures[texture_id]);
    }

    void bind_array(texture_handle texture_id, uint32_t texture_unit) {
        glActiveTexture(GL_TEXTURE0 + texture_unit);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textures[texture_id]);
    }

    uint32_t get_ogl_id(texture_handle texture_id) {
        return textures[texture_id];
    }

    size_t get_texture_count() {
        return textures.size();
    }

    std::string get_name(texture_handle texture_id) {
        return paths[texture_id];
    }

    // bindless stuff
    struct BindlessTexture {
        uint32_t gl_id;
        uint64_t handle;
        std::string path;
        int width, height, channels;
    };
    static std::vector<BindlessTexture> bindless_textures;

    bool bindless_loaded_already(const std::string& new_path, size_t& existing_idx) {
        for (size_t i = 0; i < bindless_textures.size(); i++) {
            if (new_path == bindless_textures[i].path) {
                existing_idx = i;
                return true;
            }
        }
        return false;
    }

    uint64_t load_bindless_from_path(const std::string& file_path) {
        size_t existing_texture_index;
        if (bindless_loaded_already(file_path, existing_texture_index)) {
            return bindless_textures[existing_texture_index].handle;
        }

        uint32_t texture_id = 0;
        glGenTextures(1, &texture_id);
        int width, height, nrComponents;
        unsigned char* data = stbi_load(file_path.c_str(), &width, &height, &nrComponents, 0);

        if (data) {
            GLenum internalFormat = 0, dataFormat = 0;
            if (nrComponents == 1) {
                internalFormat = dataFormat = GL_RED;
            }
            else if (nrComponents == 3) {
                internalFormat = GL_SRGB;
                dataFormat = GL_RGB;
            }
            else if (nrComponents == 4) {
                internalFormat = GL_SRGB_ALPHA;
                dataFormat = GL_RGBA;
            }
            else {
                std::cerr << "Unsupported number of texture components: " << nrComponents << std::endl;
                stbi_image_free(data);
                return 0;
            }

            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);

            uint64_t handle = glGetTextureHandleARB(texture_id);
            glMakeTextureHandleResidentARB(handle);

            BindlessTexture tex = { texture_id, handle, file_path, width, height, nrComponents };
            bindless_textures.push_back(tex);

            std::cout << "[BINDLESS TEXTURE] Loaded: " << file_path << std::endl;
            return handle;
        }
        else {
            std::cout << "Bindless texture failed to load: " << file_path << std::endl;
            stbi_image_free(data);
            return 0;
        }
    }
}
