#include "texture_manager.h"

// #include <glad/glad.h>
#include "core/opengl.h"
#include "glow_config.h"

#include "stb_image.h"

#include <vector>
#include <string>
#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>

struct DDSHeader {
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipMapCount;
    uint32_t reserved1[11];
    
    struct {
        uint32_t size;
        uint32_t flags;
        uint32_t fourCC;
        uint32_t RGBBitCount;
        uint32_t RBitMask;
        uint32_t GBitMask;
        uint32_t BBitMask;
        uint32_t ABitMask;
    } pixelFormat;
    
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    uint32_t caps4;
    uint32_t reserved2;
};

namespace Texture_Manager {

    // todo can slim down if non bindless maybe
    struct Texture {
        uint32_t gl_id;
        uint64_t handle;
        std::string path;
        int width, height, channels;
        // enum type or something format idk
    };
    static std::vector<Texture> textures;

    void init() {
        //stbi_set_flip_vertically_on_load(true);
        // texture_handle zero = load("../resources/textures/missing.png");
        // assert(!textures.empty());
        // todo set basepath
    }

    void cleanup() {
        for (Texture& texture : textures) {
            if (texture.gl_id != 0) {
                if (texture.handle != 0)
                    glMakeTextureHandleNonResidentARB(texture.handle);
                
                glDeleteTextures(1, &texture.gl_id);
            }
        }
        textures.clear();
    }


    bool loaded_already(const std::string& new_path, size_t& existing_idx) {
        for (size_t i = 0; i < textures.size(); i++) {
            if (new_path == textures[i].path) {
                existing_idx = i;
                return true;
            }
        }
        return false;
    }

    uint64_t load(const std::string& file_path) {
        std::string dds = file_path.substr(0, file_path.find_last_of('.')) + ".dds";
        bool is_dds = std::filesystem::exists(dds);

        std::string target = is_dds ? dds : file_path;

        size_t existing_texture_index;
        if (loaded_already(target, existing_texture_index)) {
            printf("[bindless] %s already loaded\n", target.c_str());
            return textures[existing_texture_index].handle;
        }

        if (is_dds)
            return load_dds(target);
        else
            return load_non_dds(target);
    }

    uint64_t load_dds(const std::string& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open DDS file: " << file_path << std::endl;
            return 0;
        }
        
        char magic[4];
        file.read(magic, 4);
        if (std::string(magic, 4) != "DDS ") {
            std::cerr << "Invalid DDS file format" << std::endl;
            return 0;
        }
        
        DDSHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        GLenum format = get_ogl_format(header.pixelFormat.fourCC);
        if (format == 0) {
            std::cerr << "Unsupported DDS format" << std::endl;
            return 0;
        }
        
        uint32_t mipCount = (header.mipMapCount > 0) ? header.mipMapCount : 1;
        uint32_t blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
        
        std::vector<uint8_t> data;
        uint32_t width = header.width;
        uint32_t height = header.height;
        
        for (uint32_t mip = 0; mip < mipCount; mip++) {
            uint32_t mipSize = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
            
            size_t currentPos = data.size();
            data.resize(currentPos + mipSize);
            file.read(reinterpret_cast<char*>(&data[currentPos]), mipSize);
            
            width = std::max(1u, width / 2);
            height = std::max(1u, height / 2);
        }
        
        GLuint texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        
        width = header.width;
        height = header.height;
        uint32_t offset = 0;

        // Texture tex = { texture_id, handle, file_path, width, height, 0 };
        Texture& texture = textures.emplace_back();
        texture.gl_id = texture_id;
        texture.path = file_path;
        texture.width = width;
        texture.height = height;

        for (uint32_t mip = 0; mip < mipCount; mip++) {
            uint32_t mipSize = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
            
            glCompressedTexImage2D(GL_TEXTURE_2D, mip, format, width, height, 0, mipSize, &data[offset]);
            
            offset += mipSize;
            width = std::max(1u, width / 2);
            height = std::max(1u, height / 2);
        }
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

#if BINDLESS
        uint64_t handle = glGetTextureHandleARB(texture_id);
        glMakeTextureHandleResidentARB(handle);
        texture.handle = handle;
#endif
        glBindTexture(GL_TEXTURE_2D, 0);
        
        std::cout << "[DDS] Loaded DDS texture: " << file_path << " (" << header.width << "x" << header.height << ", " << mipCount << " mips)" << std::endl;
        
#if BINDLESS
        return texture.handle;
#else
        return textures.size() - 1;
#endif
    }
    
    uint64_t load_non_dds(const std::string& file_path) {
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

            // here
            uint32_t texture_id = 0;
            glGenTextures(1, &texture_id);

            // emplace
            Texture& texture = textures.emplace_back();
            texture.gl_id = texture_id;
            texture.path = file_path;
            texture.width = width;
            texture.height = height;
            texture.channels = nrComponents; // todo need?

            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);

#if BINDLESS
            uint64_t handle = glGetTextureHandleARB(texture_id);
            glMakeTextureHandleResidentARB(handle);
            // Texture tex = { texture_id, handle, file_path, width, height, nrComponents };
            // textures.push_back(tex);
            texture.handle = handle;
#endif
            glBindTexture(GL_TEXTURE_2D, 0);

            std::cout << "[TEXTURE] Loaded NON-DDS texture: " << file_path << " (" << texture.width << "x" << texture.height << std::endl;

#if BINDLESS
            return texture.handle;
#else
            return textures.size() - 1;
#endif
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

        printf("[TEXTURE] %s resized to width: %d, height: %d\n", textures[handle].path.c_str(), width, height);
    }

    texture_handle load_msdf(const std::string& file_path) {
        size_t existing_texture_index;
        if (loaded_already(file_path, existing_texture_index)) {
            return existing_texture_index;
        }

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

            uint32_t texture_id = 0;
            glGenTextures(1, &texture_id);

            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

            // no mip maps
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            stbi_image_free(data);

            Texture& texture = textures.emplace_back();
            texture.gl_id = texture_id;
            texture.path = file_path;
            texture.width = width;
            texture.height = height;

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

        // todo ifdef runtime me
        // glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        Texture& texture = textures.emplace_back();
        texture.gl_id = texture_id;
        texture.path = "";
        texture.width = width;
        texture.height = height;

        std::cout << "[DEPTH TEXTURE] Created " << width << "x" << height << std::endl;
        return textures.size() - 1;
    }

    texture_handle create_picking_texture(int width, int height) {
        uint32_t texture_id = 0;

        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindTexture(GL_TEXTURE_2D, 0);

        Texture& texture = textures.emplace_back();
        texture.gl_id = texture_id;
        texture.path = "picking_texture";
        texture.width = width;
        texture.height = height;

        std::cout << "[PICKING TEXTURE] Created " << width << "x" << height << std::endl;
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

        Texture& texture = textures.emplace_back();
        texture.gl_id = texture_id;
        texture.path = hdr ? "render_texture_hdr" : "render_texture";
        texture.width = width;
        texture.height = height;

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

        Texture& texture = textures.emplace_back();
        texture.gl_id = texture_id;
        texture.path = "bloom_texture_with_mips";
        texture.width = width;
        texture.height = height;

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

        Texture& texture = textures.emplace_back();
        texture.gl_id = texture_id;
        texture.path = "ssao";
        texture.width = width;
        texture.height = height;

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

        Texture& texture = textures.emplace_back();
        texture.gl_id = texture_id;
        texture.path = "noise_texture";
        texture.width = width;
        texture.height = height;

        return textures.size() - 1;
    }

    // texture_handle create_3d_texture(int width, int height, int layers) {
    //     uint32_t texture_id = 0;
    //     glGenTextures(1, &texture_id);

    //     glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id);
    //     glTexImage3D(
    //         GL_TEXTURE_2D_ARRAY, // todo arg?
    //         0,
    //         GL_DEPTH_COMPONENT32F, // todo arg
    //         width,
    //         height,
    //         layers,
    //         0,
    //         GL_DEPTH_COMPONENT, // todo arg
    //         GL_FLOAT, // todo arg?
    //         nullptr);

    //     //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //     //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //     glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //     glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //     glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    //     glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    //     constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    //     glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

    //     /*glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    //     glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);*/

    //     glBindTexture(GL_TEXTURE_2D, 0);

    //     Texture& texture = textures.emplace_back();
    //     texture.gl_id = texture_id;
    //     texture.path = "3d";  // todo maybe change if multiple of these
    //     texture.width = width;
    //     texture.height = height;

    //     return textures.size() - 1;
    // }

    texture_handle create_2d_array_texture(int width, int height, int layers) {
        uint32_t texture_id = 0;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id);

        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, width, height, layers, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

        Texture& texture = textures.emplace_back();
        texture.gl_id = texture_id;
        texture.path = "2d";  // todo maybe change if multiple of these
        texture.width = width;
        texture.height = height;

        return textures.size() - 1;
    }

    texture_handle load_heightmap(const std::string& name) {
        uint32_t texture_id = 0;

        int width, height, nrComponents;
        float* data = stbi_loadf(name.c_str(), &width, &height, &nrComponents, 1);

        if (data) {
            glGenTextures(1, &texture_id);
            glBindTexture(GL_TEXTURE_2D, texture_id);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            stbi_image_free(data);

            Texture& texture = textures.emplace_back();
            texture.gl_id = texture_id;
            texture.path = "heightmap";  // todo maybe change if multiple of these
            texture.width = width;
            texture.height = height;

            //paths.push_back(file_path);
            std::cout << "[TEXTURE] heightmap loaded: " << name << std::endl;
            return textures.size() - 1;
        }
        else {
            std::cout << "heightmap texture failed to load: " << name << std::endl;
            const char* reason = stbi_failure_reason();
               std::cerr << "Failed to load heightmap: " << reason << std::endl;
            stbi_image_free(data);
            assert(false);
            return 0;
        }
    }

    void bind(texture_handle texture_id, uint32_t texture_unit) {
        glActiveTexture(GL_TEXTURE0 + texture_unit);
        glBindTexture(GL_TEXTURE_2D, textures[texture_id].gl_id);
    }

    void bind_array(texture_handle texture_id, uint32_t texture_unit) {
        glActiveTexture(GL_TEXTURE0 + texture_unit);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textures[texture_id].gl_id);
    }

    uint32_t get_ogl_id(texture_handle texture_id) {
        return textures[texture_id].gl_id;
    }

    size_t get_texture_count() {
        return textures.size();
    }

    std::string get_name(texture_handle texture_id) {
        return textures[texture_id].path;
    }

    uint32_t get_ogl_format(uint32_t fourCC) {
        switch (fourCC) {
            case 0x31545844: // "DXT1"
                return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            case 0x33545844: // "DXT3"
                return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            case 0x35545844: // "DXT5"
                return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            case 0x55344342: // "BC4U"
                return GL_COMPRESSED_RED_RGTC1;
            case 0x53344342: // "BC4S"
                return GL_COMPRESSED_SIGNED_RED_RGTC1;
            case 0x55354342: // "BC5U"
                return GL_COMPRESSED_RG_RGTC2;
            case 0x53354342: // "BC5S"
                return GL_COMPRESSED_SIGNED_RG_RGTC2;
            default:
                return 0;
        }
    }
}
