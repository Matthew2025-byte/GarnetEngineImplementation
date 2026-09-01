#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <GarnetEngine/Garnet.hpp>
#include <GarnetEngine/Scene.hpp>
#include <iostream>

struct velocity { float x; float y; };


struct AppState {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    Garnet::TextureManager textureManager;
    Garnet::Renderer Renderer;
    Garnet::Scene scene;
    Garnet::SceneManager sceneManager;

    AppState(SDL_Window* w, SDL_Renderer* r) :
        window(w), renderer(r), sceneManager(r), textureManager(r), Renderer(r, textureManager) {}
};



void updatePos(float dt, Garnet::Entity _, Garnet::Components::Transform& transform, velocity& vel) {
    transform.position.x -= vel.x * dt;
    transform.position.y -= vel.y * dt;
}

#define GRAVITY_CONSTANT 100000.0f
void gravitySystem(float dt, Garnet::Registry& registry, const std::vector<Garnet::Entity>& entities) {
    for (auto primaryE : entities) {
        auto& primaryPos = registry.getComponent<Garnet::Components::Transform>(primaryE).position;

        for (auto secondaryE : entities) {
            if (primaryE == secondaryE) continue;
            auto& secondaryPos = registry.getComponent<Garnet::Components::Transform>(secondaryE).position;

            Garnet::vec2 direction = (secondaryPos - primaryPos).normalized();
            float magnitudeSq = (secondaryPos - primaryPos).x * (secondaryPos - primaryPos).x + (secondaryPos - primaryPos).y * (secondaryPos - primaryPos).y;
            float force = -GRAVITY_CONSTANT / magnitudeSq;

            registry.getComponent<velocity>(primaryE).x += direction.x * force * dt;
            registry.getComponent<velocity>(primaryE).y += direction.y * force * dt;
        }
    }
}

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
    Garnet::Registry& registry = app->scene.getInitRegistry();

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetFloatProperty(props, GARNET_SVG_RASTER_WIDTH, 50);
    
    auto e1 = registry.createEntity();
    auto e2 = registry.createEntity();
    registry.addComponent<Garnet::Components::Transform>(e1, {{400, 200}, 0});
    registry.addComponent<velocity>(e1, {15, -10});
    registry.addComponent<Garnet::Components::Transform>(e2, {{400, 100}, 0});
    registry.addComponent<velocity>(e2, {-15, 10});
    registry.addComponent<Garnet::TextureID>(e1, app->textureManager.Load("circle.svg", props));
    registry.addComponent<Garnet::TextureID>(e2, app->textureManager.Load("circle.svg", props));

    SDL_DestroyProperties(props);


    app->scene.bindSystem<Garnet::Components::Transform, velocity>(gravitySystem);
    app->scene.bind(updatePos);
    
    app->sceneManager.addScene("test", app->scene);
    app->sceneManager.start();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState& app = *static_cast<AppState*>(appstate);

    app.Renderer.update(app.sceneManager.getRenderRegistry());

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
    delete &app;

    return;
}