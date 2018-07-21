#pragma once

#ifndef WE_HAVE_PRELUDE
static_assert(false, "WE_HAVE_PRELUDE wasn't defined by the build system");
#endif
#undef  WE_HAVE_PRELUDE
#define WE_HAVE_PRELUDE 1

// This file includes headers that are used so ubiquitously that it's not
// practical to include them everywhere they're used, when they're used.
// Instead, we include everything here once, and everywhere gets it.

// Project header that wraps platform specific things
#include "00-Prelude/Os.hpp"

#include <vulkan/vulkan.h>
#define DeclLoadVkPFN(instance, FnName)                                         \
    auto FnName = ptr_as<PFN_ ## FnName>(vkGetInstanceProcAddr(instance,        \
                                                               STR(FnName)));   \
    AssertMsg((FnName) != nullptr, STR(FnName) " couldn't be loaded")

// Include this after our OS header.
#include <GLFW/glfw3.h>

// ==== C Libs ==================================================================
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>

// MSVC missed the memo on including this.
static constexpr float PI = 3.14159265358979323846f;

// === STL ======================================================================
#include <string>
#include <vector>

// ==== Math Includes ===========================================================
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

// Math Typedefs
using Vec3 = glm::vec3;
using Mat4 = glm::mat4;

// ==== Misc Macros ============================================================
#define IMPL_STR(thing) # thing
#define STR(thing)      IMPL_STR(thing)

// Evaluates an expression, and suppresses unused warnings from the compiler.
#define UNUSED(var)     (void)(var)

// ==== Project Includes ========================================================
// These headers assume everything in the prelude exists, so don't do anything
// after this section.
// Be careful when re-ordering things here.
#include "00-Prelude/Debug.hpp"
#include "00-Prelude/Utils.hpp"
