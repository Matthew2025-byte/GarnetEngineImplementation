#pragma once
#include <SDL3/SDL.h>
#include "vec2.hpp"

namespace Garnet::Components {
    struct Transform {
        Garnet::vec2 position;
        float rotation;
    };
}