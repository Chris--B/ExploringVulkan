#include <00-Prelude.hpp>

static std::vector<RegistryLayerValue> loadLayersFrom(const char* pKeyName)
{
    std::vector<RegistryLayerValue> layers;

    HKEY hkey;
    LONG status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               pKeyName,
                               0, // ulOptions
                               KEY_READ,
                               &hkey);
    Assert(status != ERROR_FILE_NOT_FOUND);
    Assert(status == ERROR_SUCCESS);

    DWORD index = 0;
    while (true) {
        char buffer[256] = "";
        DWORD bufferLength = array_size(buffer);
        DWORD dataType = REG_NONE;
        DWORD data     = -1;
        DWORD dataSize = sizeof(data);
        status = RegEnumValueA(hkey,
                               index,
                               buffer,
                               &bufferLength,
                               NULL, // reserved
                               &dataType,
                               ptr_as<BYTE>(&data),
                               &dataSize);
        index += 1;
        if (status == ERROR_NO_MORE_ITEMS) {
            break;
        }
        // We don't expect any other keys here.
        Assert(dataType == REG_DWORD);
        layers.emplace_back(buffer, data);
    }

    return layers;
}

std::vector<RegistryLayerValue> queryExplicitLayers()
{
    return loadLayersFrom("SOFTWARE\\Khronos\\Vulkan\\ExplicitLayers");
}

std::vector<RegistryLayerValue> queryImplicitLayers()
{
    return loadLayersFrom("SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers");
}
