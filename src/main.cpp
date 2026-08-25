#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <GarnetEngine/Garnet.hpp>
#include <iostream>

struct AppState {
    SDL_Window* window;
    SDL_Renderer* renderer;

    Garnet::TextureManager textureManager;
    Garnet::Registry registry;
    Garnet::Renderer renderSystem;

    AppState(SDL_Window* w, SDL_Renderer* r) :
        window(w), renderer(r), textureManager(r), registry(), renderSystem(r, textureManager) {}
};


struct velocity { float x; float y; };

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    // Assign memory for to bind Appstate to appstate
    SDL_Window* window;
    SDL_Renderer* renderer;
    
    
    if (!SDL_CreateWindowAndRenderer("test game", 800, 600, 0, &window, &renderer)) {
        SDL_Log("Failed to load: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    AppState* app = new AppState{window, renderer};
    *appstate = app;


    Garnet::Entity player = app->registry.createEntity();
    app->registry.addComponent<Garnet::Components::Transform>(player, { Garnet::vec2(400, 300) });
    app->registry.addComponent<velocity>(player, { 0, 10 });
    
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetFloatProperty(props, GARNET_SVG_RASTER_WIDTH, 64);
    app->registry.addComponent<Garnet::TextureID>(player, app->textureManager.Load("Eagle.svg", props));
    SDL_DestroyProperties(props);


    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState& app = *static_cast<AppState*>(appstate);
    float dt = 0.001;

    app.registry.each<Garnet::Components::Transform, velocity>([&](Garnet::Entity entity, Garnet::Components::Transform& transform, velocity& vel) {
        transform.position.x -= vel.x * dt;
        transform.position.y -= vel.y * dt;
    });

    app.renderSystem.update(app.registry);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    AppState& app = *static_cast<AppState*>(appstate);

    if (app.renderer)          { SDL_DestroyRenderer(app.renderer); }
    if (app.window)            { SDL_DestroyWindow(app.window); }
    return;
}