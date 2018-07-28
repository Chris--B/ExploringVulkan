#pragma once

#include "00-Prelude.hpp"

static_assert(OS_WINDOWS, "Are you building on Windows?");

// Represents how Vulkan layers are presented in the Windows Registry.
struct RegistryLayerValue {
    std::string name = "";
    DWORD       data = 0;

    RegistryLayerValue() = default;
    RegistryLayerValue(std::string const& name, DWORD data)
        : name(name),
          data(data)
    {}
};

std::vector<RegistryLayerValue> queryExplicitLayers();
std::vector<RegistryLayerValue> queryImplicitLayers();
