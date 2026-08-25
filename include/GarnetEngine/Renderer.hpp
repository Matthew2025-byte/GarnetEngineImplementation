#pragma once
#include <SDL3/SDL.h>
#include "Registry.hpp"
#include "Components.hpp"
#include "TextureManager.hpp"

namespace Garnet {


class Renderer {
    SDL_Renderer* renderer;
    Garnet::TextureManager& textureManager;

    std::vector<SDL_Texture*> TextureIDs;
    uint32_t textureIndex = 0;
    


    void RenderTexture(SDL_Renderer* renderer, Garnet::Components::Transform transform, Garnet::TextureID textureID);
    public:
    Renderer(SDL_Renderer* renderer, Garnet::TextureManager& manager) :
        renderer(renderer), textureManager(manager) {}
    
    void update(Garnet::Registry& registry);
};

}