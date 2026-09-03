#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <GarnetEngine/Garnet.hpp>
#include <GarnetEngine/Scene.hpp>
#include <iostream>
#include <GarnetEngine/Collision.hpp>

using vec2 = Garnet::vec2;

struct velocity { float x; float y; };
enum colliderType { Circle };
struct ColliderEx : public Garnet::Components::Collider {
    colliderType type;

    ColliderEx(colliderType t, Garnet::vec2 size, Garnet::vec2 offset) : type(t), Garnet::Components::Collider(size, offset) {}
};
struct circleComponent { float radius; };


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


void collisionCircleCircle(Garnet::Components::Transform& transformA, Garnet::Components::Rigidbody& rigidBodyA, float radiusA,
                           Garnet::Components::Transform& transformB, Garnet::Components::Rigidbody& rigidBodyB, float radiusB) {

    using vec2 = Garnet::vec2;
    vec2 delta = transformB.position - transformA.position;
    float distSq = delta.x * delta.x + delta.y * delta.y;

    const float radiusSum = radiusA + radiusB;
    const float radiusSumSq = radiusSum * radiusSum;
            
    if (distSq >= radiusSumSq) { return; }

    float dist = std::sqrt(distSq);
    vec2 normal = delta / dist;

    vec2 relativeVelocity = rigidBodyB.velocity - rigidBodyA.velocity;
    float velAlongNormal = relativeVelocity.dot(normal);

    if (velAlongNormal > 0) { return; }

    float restitution = std::min(rigidBodyA.restitution, rigidBodyB.restitution);
    float impulseMagnitude = -(1 + restitution) * velAlongNormal /(rigidBodyA.inverseMass() + rigidBodyB.inverseMass());

    vec2 impulse = normal * impulseMagnitude;

    rigidBodyA.velocity -= impulse * rigidBodyA.inverseMass();
    rigidBodyB.velocity += impulse * rigidBodyB.inverseMass();

    float overlap = radiusSum - dist;
    vec2 correction = normal * (overlap * 0.5f);

    //transformA.position -= correction;
    //transformB.position += correction;
}

bool overlap(float aMin, float aMax, float bMin, float bMax) {
    return aMin <= bMax && bMin <= aMax;
}
bool AABB(vec2 posA, Garnet::Components::Collider& colliderA, 
          vec2 posB, Garnet::Components::Collider& colliderB) {
    vec2 aMin = posA + colliderA.offset - (colliderA.size * 0.5f);
    vec2 aMax = posA + colliderA.offset + (colliderA.size * 0.5f);
    vec2 bMin = posB + colliderB.offset - (colliderB.size * 0.5f);
    vec2 bMax = posB + colliderB.offset + (colliderB.size * 0.5f);

    bool overlapX = overlap(aMin.x, aMax.x, bMin.x, bMax.x);
    bool overlapY = overlap(aMin.y, aMax.y, bMin.y, bMax.y);
    return overlapX && overlapY;
}

// Currently assuming circle on circle, but will extend to other shapes in the future
void collisionSystem(float dt, Garnet::Registry& registry, const std::vector<Garnet::Entity>& entities) {
    const size_t entityCount = entities.size();

    for (size_t i = 0; i < entityCount; i++) {
        auto& transformA = registry.getComponent<Garnet::Components::Transform>(entities[i]);
        auto& rigidBodyA = registry.getComponent<Garnet::Components::Rigidbody>(entities[i]);
        auto& colliderA = registry.getComponent<ColliderEx>(entities[i]);

        for (size_t j = i + 1; j < entityCount; j++) {
            auto& transformB = registry.getComponent<Garnet::Components::Transform>(entities[j]);
            auto& rigidBodyB = registry.getComponent<Garnet::Components::Rigidbody>(entities[j]);
            auto& colliderB = registry.getComponent<ColliderEx>(entities[j]);

            if (AABB(transformA.position, colliderA, transformB.position, colliderB)) {
                if (colliderA.type == Circle && colliderB.type == Circle) {
                    float radiusA = registry.getComponent<circleComponent>(entities[i]).radius;
                    float radiusB = registry.getComponent<circleComponent>(entities[j]).radius;
                    collisionCircleCircle(transformA, rigidBodyA, radiusA, transformB, rigidBodyB, radiusB);
                }
            }
            
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
    registry.addComponent<ColliderEx>(e1, {Circle, {50, 50}, {0, 0}});
    registry.addComponent<circleComponent>(e1, {25.f});

    auto e2 = registry.createEntity();
    registry.addComponent<Garnet::Components::Transform>(e2, {{500, 300}, 0});
    registry.addComponent<Garnet::TextureID>(e2, app->textureManager.Load("circle.svg", props));
    registry.addComponent<Garnet::Components::Rigidbody>(e2, {{0, 0}, {0, 0}, 1e3f, 0.8f, false});
    registry.addComponent<ColliderEx>(e2, {Circle, {50, 50}, {0, 0}});
    registry.addComponent<circleComponent>(e2, {25.f});

    SDL_SetFloatProperty(props, GARNET_SVG_RASTER_WIDTH, 100);
    auto e3 = registry.createEntity();
    registry.addComponent<Garnet::Components::Transform>(e3, {{300, 150}, 0});
    registry.addComponent<Garnet::TextureID>(e3, app->textureManager.Load("circle1.svg", props));
    registry.addComponent<Garnet::Components::Rigidbody>(e3, {{0, 0}, {0, 10}, 1e3f, 0.8f, false});
    registry.addComponent<ColliderEx>(e3, {Circle, {100, 100}, {0, 0}});
    registry.addComponent<circleComponent>(e3, {50.f});

    SDL_DestroyProperties(props);


    app->scene.bindSystem<Garnet::Components::Transform, Garnet::Components::Rigidbody>(gravitySystem);
    app->scene.bindSystem<Garnet::Components::Transform, Garnet::Components::Rigidbody, ColliderEx>(collisionSystem);
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