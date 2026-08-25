#pragma once
#include <SDL3_ttf/SDL_textengine.h>
#include <unordered_map>
#include <filesystem>

namespace Garnet {
    class FontManager;
}

class Garnet::FontManager {
    TTF_TextEngine* textEngine;
    std::unordered_map<std::string, TTF_Font*> fonts;

    std::filesystem::path fontDir;

    public:
    FontManager(SDL_Renderer* renderer, std::filesystem::path fontDir);

    /**
     * @brief Loads a ttc font into memory
     * 
     * This method currently only supports ttc types.  More will be added later
     * 
     * @param file name of the file to load (ex. font.ttc)
     * @returns The requested font
     */
    TTF_Font* Load(const char* file);

    /**
     * @brief Finds the requested font in memory
     * 
     * @param font Name of the font to search for
     * @returns A pointer to the requested font
     */
    TTF_Font* getFont(const std::string& font) const;
};