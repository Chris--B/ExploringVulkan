#pragma once

static_assert(WE_HAVE_PRELUDE, "Utils.hpp was included without the Prelude.");

template<typename T, uint32_t N>
[[nodiscard]]
constexpr uint32_t array_size(T(&)[N]) { return N; }

// Shorthand for `static_cast`.
template<typename U, typename T>
[[nodiscard]]
U as(T &other)                { return static_cast<U>(other); }

// Shorthand for `static_cast`, but respects `const` input.
template<typename U, typename T>
[[nodiscard]]
const U as(const T &other)    { return static_cast<U>(other); }

// Shorthand for `reinterpret_cast`
template<typename U, typename T>
[[nodiscard]]
U* ptr_as(const T* other) { return reinterpret_cast<U*>(other); }


[[nodiscard]]
std::vector<uint8_t> loadBytesFrom(const char *const filename);

// ==== ToCStr Overloads ========================================================
// Leave this defined because MSVC cannot handle re-used macros.
#define TO_CSTR_CASE(VAL) case VAL: return #VAL;

constexpr const char* ToCStr(VkBool32 condition)
{
    return condition ? "+" : "-";
}

inline static const char* ToCStr(VkResult result)
{
    switch (result) {
        TO_CSTR_CASE(VK_SUCCESS)
        TO_CSTR_CASE(VK_NOT_READY)
        TO_CSTR_CASE(VK_TIMEOUT)
        TO_CSTR_CASE(VK_EVENT_SET)
        TO_CSTR_CASE(VK_EVENT_RESET)
        TO_CSTR_CASE(VK_INCOMPLETE)
        TO_CSTR_CASE(VK_ERROR_OUT_OF_HOST_MEMORY)
        TO_CSTR_CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY)
        TO_CSTR_CASE(VK_ERROR_INITIALIZATION_FAILED)
        TO_CSTR_CASE(VK_ERROR_DEVICE_LOST)
        TO_CSTR_CASE(VK_ERROR_MEMORY_MAP_FAILED)
        TO_CSTR_CASE(VK_ERROR_LAYER_NOT_PRESENT)
        TO_CSTR_CASE(VK_ERROR_EXTENSION_NOT_PRESENT)
        TO_CSTR_CASE(VK_ERROR_FEATURE_NOT_PRESENT)
        TO_CSTR_CASE(VK_ERROR_INCOMPATIBLE_DRIVER)
        TO_CSTR_CASE(VK_ERROR_TOO_MANY_OBJECTS)
        TO_CSTR_CASE(VK_ERROR_FORMAT_NOT_SUPPORTED)
        TO_CSTR_CASE(VK_ERROR_FRAGMENTED_POOL)
        TO_CSTR_CASE(VK_ERROR_SURFACE_LOST_KHR)
        TO_CSTR_CASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
        TO_CSTR_CASE(VK_SUBOPTIMAL_KHR)
        TO_CSTR_CASE(VK_ERROR_OUT_OF_DATE_KHR)
        TO_CSTR_CASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
        TO_CSTR_CASE(VK_ERROR_VALIDATION_FAILED_EXT)
        TO_CSTR_CASE(VK_ERROR_INVALID_SHADER_NV)
        TO_CSTR_CASE(VK_ERROR_OUT_OF_POOL_MEMORY_KHR)
        TO_CSTR_CASE(VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR)
        TO_CSTR_CASE(VK_ERROR_NOT_PERMITTED_EXT)
        default:
            return "Unknown VkResult Value";
    }
}
