#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <GarnetEngine/Garnet.hpp>
#include <GarnetEngine/Scene.hpp>
#include <iostream>
#include <GarnetEngine/Collision.hpp>

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



void updatePos(float dt, Garnet::Entity _, Garnet::Components::Transform& transform, Garnet::Components::Rigidbody& rigidbody) {
    transform.position.x += rigidbody.velocity.x * dt;
    transform.position.y += rigidbody.velocity.y * dt;
}

// Currently assuming circle on circle, but will extend to other shapes in the future
void collisionSystem(float dt, Garnet::Registry& registry, const std::vector<Garnet::Entity>& entities) {
    using vec2 = Garnet::vec2;
    const size_t entityCount = entities.size();

    for (size_t i = 0; i < entityCount; i++) {
        auto& transformA = registry.getComponent<Garnet::Components::Transform>(entities[i]);
        auto& rigidBodyA = registry.getComponent<Garnet::Components::Rigidbody>(entities[i]);

        for (size_t j = i + 1; j < entityCount; j++) {
            auto& transformB = registry.getComponent<Garnet::Components::Transform>(entities[j]);
            auto& rigidBodyB = registry.getComponent<Garnet::Components::Rigidbody>(entities[j]);

            vec2 delta = transformB.position - transformA.position;
            float distSq = delta.x * delta.x + delta.y * delta.y;

            const float radiusSum = 50.f;
            const float radiusSumSq = radiusSum * radiusSum;
            
            if (distSq > radiusSumSq) { continue; }

            float dist = std::sqrt(distSq);
            vec2 normal = delta / dist;

            vec2 relativeVelocity = rigidBodyB.velocity - rigidBodyA.velocity;
            float velAlongNormal = relativeVelocity.dot(normal);

            if (velAlongNormal > 0) { continue; }

            float restitution = std::min(rigidBodyA.restitution, rigidBodyB.restitution);
            float impulseMagnitude = -(1 + restitution) * velAlongNormal /(rigidBodyA.inverseMass() + rigidBodyB.inverseMass());

            vec2 impulse = normal * impulseMagnitude;

            rigidBodyA.velocity -= impulse * rigidBodyA.inverseMass();
            rigidBodyB.velocity += impulse * rigidBodyB.inverseMass();

            float overlap = radiusSum - dist;
            vec2 correction = normal * (overlap * 0.5f);

            transformA.position -= correction;
            transformB.position += correction;
        }
    }    
}

#define GRAVITY_CONSTANT 1
void gravitySystem(float dt, Garnet::Registry& registry, const std::vector<Garnet::Entity>& entities) {
    for (auto primaryE : entities) {
        auto& primaryPos = registry.getComponent<Garnet::Components::Transform>(primaryE).position;
        auto& primaryRigidbody = registry.getComponent<Garnet::Components::Rigidbody>(primaryE);

        for (auto secondaryE : entities) {
            if (primaryE == secondaryE) continue;
            auto& secondaryPos = registry.getComponent<Garnet::Components::Transform>(secondaryE).position;

            Garnet::vec2 direction = (secondaryPos - primaryPos).normalized();
            float magnitudeSq = (secondaryPos - primaryPos).x * (secondaryPos - primaryPos).x + (secondaryPos - primaryPos).y * (secondaryPos - primaryPos).y;
            float force = GRAVITY_CONSTANT * (primaryRigidbody.mass * registry.getComponent<Garnet::Components::Rigidbody>(secondaryE).mass) / magnitudeSq;

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

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetFloatProperty(props, GARNET_SVG_RASTER_WIDTH, 50);
    

    auto e1 = registry.createEntity();
    registry.addComponent<Garnet::Components::Transform>(e1, {{100, 300}, 0});
    registry.addComponent<Garnet::TextureID>(e1, app->textureManager.Load("circle.svg", props));
    registry.addComponent<Garnet::Components::Rigidbody>(e1, {{0, 0}, {0, 0}, 1e3f, 0.8f, false});

    auto e2 = registry.createEntity();
    registry.addComponent<Garnet::Components::Transform>(e2, {{500, 300}, 0});
    registry.addComponent<Garnet::TextureID>(e2, app->textureManager.Load("circle.svg", props));
    registry.addComponent<Garnet::Components::Rigidbody>(e2, {{0, 0}, {0, 0}, 1e3f, 0.8f, false});

    auto e3 = registry.createEntity();
    registry.addComponent<Garnet::Components::Transform>(e3, {{300, 150}, 0});
    registry.addComponent<Garnet::TextureID>(e3, app->textureManager.Load("circle.svg", props));
    registry.addComponent<Garnet::Components::Rigidbody>(e3, {{0, 0}, {0, 10}, 1e3f, 0.8f, false});

    SDL_DestroyProperties(props);


    app->scene.bindSystem<Garnet::Components::Transform, Garnet::Components::Rigidbody>(gravitySystem);
    app->scene.bindSystem<Garnet::Components::Transform, Garnet::Components::Rigidbody>(collisionSystem);
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