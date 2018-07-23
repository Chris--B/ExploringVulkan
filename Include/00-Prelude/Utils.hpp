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
const U as(T const& other)    { return static_cast<U>(other); }

// Shorthand for `reinterpret_cast`
template<typename U, typename T>
[[nodiscard]]
U* ptr_as(const T* other) { return reinterpret_cast<U*>(other); }


[[nodiscard]]
std::vector<uint8_t> loadBytesFrom(const char *const filename);

// ==== ToCStr Overloads ========================================================
// Leave this defined because MSVC's IntelliSense cannot handle re-used macros.
#define TO_CSTR_CASE(VAL) case VAL: return #VAL;

constexpr const char* ToCStr(VkBool32 condition)
{
    return condition ? "+" : "-";
}

constexpr const char* ToCStr(VkResult result)
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
        default:
            return "Unknown VkResult Value";
    }
}

constexpr const char* ToCStr(VkPhysicalDeviceType const& value)
{
    switch (value) {
        TO_CSTR_CASE(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        TO_CSTR_CASE(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        TO_CSTR_CASE(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
        TO_CSTR_CASE(VK_PHYSICAL_DEVICE_TYPE_CPU)
        default:
            return "Other";
    }
}

inline std::string ToStr(VkPhysicalDeviceFeatures const& value)
{
    auto doPrintf = [&value](char* pBuffer, size_t size) {
        return snprintf(pBuffer, size,
                        "    %s robustBufferAccess\n"
                        "    %s fullDrawIndexUint32\n"
                        "    %s imageCubeArray\n"
                        "    %s independentBlend\n"
                        "    %s geometryShader\n"
                        "    %s tessellationShader\n"
                        "    %s sampleRateShading\n"
                        "    %s dualSrcBlend\n"
                        "    %s logicOp\n"
                        "    %s multiDrawIndirect\n"
                        "    %s drawIndirectFirstInstance\n"
                        "    %s depthClamp\n"
                        "    %s depthBiasClamp\n"
                        "    %s fillModeNonSolid\n"
                        "    %s depthBounds\n"
                        "    %s wideLines\n"
                        "    %s largePoints\n"
                        "    %s alphaToOne\n"
                        "    %s multiViewport\n"
                        "    %s samplerAnisotropy\n"
                        "    %s textureCompressionETC2\n"
                        "    %s textureCompressionASTC_LDR\n"
                        "    %s textureCompressionBC\n"
                        "    %s occlusionQueryPrecise\n"
                        "    %s pipelineStatisticsQuery\n"
                        "    %s vertexPipelineStoresAndAtomics\n"
                        "    %s fragmentStoresAndAtomics\n"
                        "    %s shaderTessellationAndGeometryPointSize\n"
                        "    %s shaderImageGatherExtended\n"
                        "    %s shaderStorageImageExtendedFormats\n"
                        "    %s shaderStorageImageMultisample\n"
                        "    %s shaderStorageImageReadWithoutFormat\n"
                        "    %s shaderStorageImageWriteWithoutFormat\n"
                        "    %s shaderUniformBufferArrayDynamicIndexing\n"
                        "    %s shaderSampledImageArrayDynamicIndexing\n"
                        "    %s shaderStorageBufferArrayDynamicIndexing\n"
                        "    %s shaderStorageImageArrayDynamicIndexing\n"
                        "    %s shaderClipDistance\n"
                        "    %s shaderCullDistance\n"
                        "    %s shaderFloat64\n"
                        "    %s shaderInt64\n"
                        "    %s shaderInt16\n"
                        "    %s shaderResourceResidency\n"
                        "    %s shaderResourceMinLod\n"
                        "    %s sparseBinding\n"
                        "    %s sparseResidencyBuffer\n"
                        "    %s sparseResidencyImage2D\n"
                        "    %s sparseResidencyImage3D\n"
                        "    %s sparseResidency2Samples\n"
                        "    %s sparseResidency4Samples\n"
                        "    %s sparseResidency8Samples\n"
                        "    %s sparseResidency16Samples\n"
                        "    %s sparseResidencyAliased\n"
                        "    %s variableMultisampleRate\n"
                        "    %s inheritedQueries",
                        ToCStr(value.robustBufferAccess),
                        ToCStr(value.fullDrawIndexUint32),
                        ToCStr(value.imageCubeArray),
                        ToCStr(value.independentBlend),
                        ToCStr(value.geometryShader),
                        ToCStr(value.tessellationShader),
                        ToCStr(value.sampleRateShading),
                        ToCStr(value.dualSrcBlend),
                        ToCStr(value.logicOp),
                        ToCStr(value.multiDrawIndirect),
                        ToCStr(value.drawIndirectFirstInstance),
                        ToCStr(value.depthClamp),
                        ToCStr(value.depthBiasClamp),
                        ToCStr(value.fillModeNonSolid),
                        ToCStr(value.depthBounds),
                        ToCStr(value.wideLines),
                        ToCStr(value.largePoints),
                        ToCStr(value.alphaToOne),
                        ToCStr(value.multiViewport),
                        ToCStr(value.samplerAnisotropy),
                        ToCStr(value.textureCompressionETC2),
                        ToCStr(value.textureCompressionASTC_LDR),
                        ToCStr(value.textureCompressionBC),
                        ToCStr(value.occlusionQueryPrecise),
                        ToCStr(value.pipelineStatisticsQuery),
                        ToCStr(value.vertexPipelineStoresAndAtomics),
                        ToCStr(value.fragmentStoresAndAtomics),
                        ToCStr(value.shaderTessellationAndGeometryPointSize),
                        ToCStr(value.shaderImageGatherExtended),
                        ToCStr(value.shaderStorageImageExtendedFormats),
                        ToCStr(value.shaderStorageImageMultisample),
                        ToCStr(value.shaderStorageImageReadWithoutFormat),
                        ToCStr(value.shaderStorageImageWriteWithoutFormat),
                        ToCStr(value.shaderUniformBufferArrayDynamicIndexing),
                        ToCStr(value.shaderSampledImageArrayDynamicIndexing),
                        ToCStr(value.shaderStorageBufferArrayDynamicIndexing),
                        ToCStr(value.shaderStorageImageArrayDynamicIndexing),
                        ToCStr(value.shaderClipDistance),
                        ToCStr(value.shaderCullDistance),
                        ToCStr(value.shaderFloat64),
                        ToCStr(value.shaderInt64),
                        ToCStr(value.shaderInt16),
                        ToCStr(value.shaderResourceResidency),
                        ToCStr(value.shaderResourceMinLod),
                        ToCStr(value.sparseBinding),
                        ToCStr(value.sparseResidencyBuffer),
                        ToCStr(value.sparseResidencyImage2D),
                        ToCStr(value.sparseResidencyImage3D),
                        ToCStr(value.sparseResidency2Samples),
                        ToCStr(value.sparseResidency4Samples),
                        ToCStr(value.sparseResidency8Samples),
                        ToCStr(value.sparseResidency16Samples),
                        ToCStr(value.sparseResidencyAliased),
                        ToCStr(value.variableMultisampleRate),
                        ToCStr(value.inheritedQueries));
    };

    uint32_t sizeNeeded = doPrintf(nullptr, 0);
    std::string result(sizeNeeded, '\0');
    doPrintf(result.data(), result.size());

    return result;
}
