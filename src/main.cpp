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
    SDL_Thread* updateThread = nullptr;

    Garnet::TextureManager textureManager;
    Garnet::Renderer Renderer;

    ThreadData threadData;
    Garnet::Registry* regA;
    Garnet::Registry* regB;
    SDL_Mutex* mutexA = nullptr;
    SDL_Mutex* mutexB = nullptr;
    SDL_AtomicInt renderReg;
    SDL_AtomicInt running;

    AppState(SDL_Window* w, SDL_Renderer* r) :
        window(w), renderer(r), textureManager(r), Renderer(r, textureManager) {}
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
        // Get the renderRegly rendering buffer
        int activeBuff = SDL_GetAtomicInt(data.renderReg);
        if (activeBuff == -1) { break; };
        Garnet::Registry& buff = *data.registries[!activeBuff];
        // Calculate dt
        Uint64 current_ticks = SDL_GetTicksNS();
        float dt = (current_ticks - last_ticks) / 1'000'000'000.f;
        last_ticks = current_ticks;

        lastBuffer.each<Garnet::Components::Transform, velocity>([&](Garnet::Entity entity, Garnet::Components::Transform& transform, velocity& vel) {
            transform.position.x -= vel.x * dt;
            transform.position.y -= vel.y * dt;
        });
        
        SDL_LockMutex(data.mutexes[!activeBuff]);
        *data.registries[!activeBuff] = lastBuffer;
        SDL_UnlockMutex(data.mutexes[!activeBuff]);

        // DO NOT WRITE TO THE BUFFER AFTER THIS POINT
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
    
    app->regA = new Garnet::Registry();
    app->regB = new Garnet::Registry();
    app->mutexA = SDL_CreateMutex();
    app->mutexB = SDL_CreateMutex();
    SDL_SetAtomicInt(&app->renderReg, 0);
    SDL_SetAtomicInt(&app->running, 1);
    

    Garnet::Entity player = app->regA->createEntity();
    app->regA->addComponent<Garnet::Components::Transform>(player, { Garnet::vec2(400, 300) });
    app->regA->addComponent<velocity>(player, { 0, 10 });
    
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetFloatProperty(props, GARNET_SVG_RASTER_WIDTH, 64);
    app->regA->addComponent<Garnet::TextureID>(player, app->textureManager.Load("Eagle.svg", props));
    SDL_DestroyProperties(props);
    
    *app->regB = *app->regA;
    app->threadData = { { app->regA, app->regB }, { app->mutexA, app->mutexB }, &app->renderReg, &app->running };

    //app->scene.bind(updatePos);
    SDL_Log("Load thread");
    app->updateThread = SDL_CreateThread(ThreadLogic, "Update Logic", &app->threadData);
    if (!app->updateThread) {
        SDL_Log("SDL_CreateThread failed: %s", SDL_GetError());
    }
    SDL_Log("Created Thread");

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState& app = *static_cast<AppState*>(appstate);
    

    int index = SDL_GetAtomicInt(&app.renderReg);
    if (index == 0) {
        SDL_LockMutex(app.mutexA);
        app.Renderer.update(*app.regA);
        SDL_UnlockMutex(app.mutexA);
    }
    else {
        SDL_LockMutex(app.mutexB);
        app.Renderer.update(*app.regB);
        SDL_UnlockMutex(app.mutexB);
    }

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

    if (app.updateThread) {
        SDL_SetAtomicInt(&app.running, 0);
        SDL_WaitThread(app.updateThread, nullptr);
    }
    SDL_DestroyMutex(app.mutexA);
    SDL_DestroyMutex(app.mutexB);

    if (app.renderer)          { SDL_DestroyRenderer(app.renderer); }
    if (app.window)            { SDL_DestroyWindow(app.window); }
    delete app.regA;
    delete app.regB;
    delete &app;

    return;
}