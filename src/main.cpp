#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <GarnetEngine/Garnet.hpp>
#include <GarnetEngine/Scene.hpp>
#include <iostream>

struct ThreadData {
    Garnet::Registry* registries[2];
    SDL_Mutex* mutexes[2];
    SDL_AtomicInt* renderReg;
    SDL_AtomicInt* running;
};
struct velocity { float x; float y; };


struct AppState {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    Garnet::textureManager textureManager;
    Garnet::Renderer Renderer;
    Garnet::Registry registry;
    Garnet::Scene scene;
    Garnet::sceneManager manager;

    AppState(SDL_Window* w, SDL_Renderer* r) :
        window(w), renderer(r), manager(r), textureManager(r), Renderer(r, textureManager) {}
};



int SDLCALL ThreadLogic(void* args) {
    ThreadData& data = *static_cast<ThreadData*>(args);
    Garnet::Registry lastBuffer = *data.registries[!SDL_GetAtomicInt(data.renderReg)];

    constexpr Uint64 target_ns = 16'666'667;
    int update_count = 0;

    // dt calculation
    Uint64 last_ticks = SDL_GetTicksNS();
    while (SDL_GetAtomicInt(data.running)) {
        Uint64 frame_start = SDL_GetTicksNS();
        int activeBuff = SDL_GetAtomicInt(data.renderReg);
        if (activeBuff == -1) { break; };

        float dt = (frame_start - last_ticks) / 1'000'000'000.f;
        last_ticks = frame_start;

        lastBuffer.each<Garnet::Components::Transform, velocity>([&](Garnet::Entity entity, Garnet::Components::Transform& transform, velocity& vel) {
            transform.position.x -= vel.x * dt;
            transform.position.y -= vel.y * dt;
        });
        
        SDL_LockMutex(data.mutexes[!activeBuff]);
        *data.registries[!activeBuff] = lastBuffer;
        SDL_UnlockMutex(data.mutexes[!activeBuff]);
        SDL_SetAtomicInt(data.renderReg, !activeBuff);

        //SDL_Log("Physics update: %d", update_count++);

        Uint64 frame_end = SDL_GetTicksNS();
        Uint64 elapsed = frame_end - frame_start;
        if (elapsed < target_ns) {
            SDL_DelayNS(target_ns - elapsed);
        }
        
    }
    return 0;
}

void updatePos(float dt, Garnet::Entity _, Garnet::Components::Transform& transform, velocity& vel) {
    transform.position.x -= vel.x * dt;
    transform.position.y -= vel.y * dt;
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
    

    Garnet::Entity player = app->registry.createEntity();
    app->registry.addComponent<Garnet::Components::Transform>(player, { Garnet::vec2(400, 300) });
    app->registry.addComponent<velocity>(player, { 0, 10 });
    
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetFloatProperty(props, GARNET_SVG_RASTER_WIDTH, 64);
    app->registry.addComponent<Garnet::TextureID>(player, app->textureManager.Load("Eagle.svg", props));
    SDL_DestroyProperties(props);


    app->scene.bind(updatePos);
    app->scene.getInitRegistry() = app->registry;
    app->manager.setActiveScene(&app->scene);
    app->manager.start();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState& app = *static_cast<AppState*>(appstate);
    
    app.Renderer.update(app.manager.getRenderRegistry());

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