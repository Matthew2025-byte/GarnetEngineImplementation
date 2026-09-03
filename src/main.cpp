#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <GarnetEngine/Garnet.hpp>
#include <GarnetEngine/Scene.hpp>
#include <iostream>
#include <GarnetEngine/Collision.hpp>

using vec2 = Garnet::vec2;
namespace Components = Garnet::Components;

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



void updatePos(float dt, Garnet::Entity _, Components::Transform& transform, Components::Rigidbody& rigidbody) {
    transform.position.x += rigidbody.velocity.x * dt;
    transform.position.y += rigidbody.velocity.y * dt;
}

Garnet::Entity createCircle(Garnet::Registry& registry, Garnet::TextureManager& textureManager, float radius, vec2 pos, const char* texture) {
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetFloatProperty(props, GARNET_SVG_RASTER_WIDTH, radius * 2);
    auto entity = registry.createEntity();
    registry.addComponent<Components::Transform>(entity, {pos, 0});
    registry.addComponent<Garnet::TextureID>(entity, textureManager.Load(texture, props));
    registry.addComponent<Components::Rigidbody>(entity, Components::Rigidbody(1e3f, 0.8f, false));
    registry.addComponent<Components::Collider>(entity, Components::Collider({radius * 2, radius * 2}, Components::colliderType::Circle));
    registry.addComponent<Components::circleCollider>(entity, {radius});

    SDL_DestroyProperties(props);
    return entity;
}


#define GRAVITY_CONSTANT 1
void gravitySystem(float dt, Garnet::Registry& registry, const std::vector<Garnet::Entity>& entities) {
    for (auto primaryE : entities) {
        auto& primaryPos = registry.getComponent<Components::Transform>(primaryE).position;
        auto& primaryRigidbody = registry.getComponent<Components::Rigidbody>(primaryE);

        for (auto secondaryE : entities) {
            if (primaryE == secondaryE) continue;
            auto& secondaryPos = registry.getComponent<Components::Transform>(secondaryE).position;

            Garnet::vec2 direction = (secondaryPos - primaryPos).normalized();
            float magnitudeSq = (secondaryPos - primaryPos).x * (secondaryPos - primaryPos).x + (secondaryPos - primaryPos).y * (secondaryPos - primaryPos).y;
            float force = GRAVITY_CONSTANT * (primaryRigidbody.mass * registry.getComponent<Components::Rigidbody>(secondaryE).mass) / magnitudeSq;

            primaryRigidbody.velocity.x += direction.x * force * dt;
            primaryRigidbody.velocity.y += direction.y * force * dt;
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

    createCircle(registry, app->textureManager, 25.f, {100, 300}, "circle.svg");
    createCircle(registry, app->textureManager, 25.f, {300, 300}, "circle.svg");
    createCircle(registry, app->textureManager, 50.f, {300, 150}, "circle1.svg");

    app->scene.bindSystem<Components::Transform, Components::Rigidbody>(gravitySystem);
    app->scene.bindSystem<Components::Transform, Components::Rigidbody, Components::Collider>(Garnet::CollisionSystem::collisionSystem);
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