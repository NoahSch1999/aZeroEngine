#pragma once

#include <flecs.h>
#include <functional>

namespace aZero::Flecs_Helper
{
    inline void ItterateHierarchy(flecs::entity e, const std::function<void()>& before, const std::function<void()>& after) {
        before();
        // Iterate children recursively
        e.children([&](flecs::entity child) {
            ItterateHierarchy(child, before, after);
        });
        after();
    }
}