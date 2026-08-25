#pragma once
#include <unordered_map>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <filesystem>

namespace Garnet {
    struct TextureID {
        uint32_t index = 0;

        bool isValid() {
            return (index != NULL);
        }
    };
    constexpr TextureID InvalidTexture = {0};
}





#define GARNET_SVG_RASTER_WIDTH "Garnet.LoadFile.SVG.Raster_Width"

namespace Garnet {

class TextureManager {
    std::unordered_map<std::string, TextureID> TextureCache;
    std::vector<SDL_Texture*> Textures;

    SDL_Renderer* renderer;
    std::filesystem::path root_folder;

    SDL_Texture* LoadTextureFromSVG(const char* filepath, int size);

    public:
    explicit TextureManager(SDL_Renderer* renderer) 
        : TextureManager(renderer, "assets/textures/") {}
    explicit TextureManager(SDL_Renderer* renderer, const char* texture_root_folder) :
        renderer(renderer), root_folder(std::filesystem::path(texture_root_folder)) { Textures.push_back(nullptr); }

    /**
     * @brief Loads a texture into memory
     * The actual path of the file is calculated at runtime relative to the
     * texture manager's configured root folder. The caller only needs to
     * provide the texture's filename.  If a texture has already been loaded
     * its existing TextureID is returned instead of loading it again
     * 
     * @param file Name of the file (ex. player.png)
     * @param properties Allows for configuration of filetype specific properties
     * @returns Returns a TextureID
     */
    TextureID Load(const char* file, SDL_PropertiesID properties=0);

    /**
     * @brief Finds a loaded texture
     * 
     * @param id TextureID to look for
     * @returns An pointer to a SDL_Texture
     */
    SDL_Texture* getTexture(TextureID id);
};

}