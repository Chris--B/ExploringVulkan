#include "Renderer.hpp"

#include <algorithm>

Renderer::~Renderer()
{
    deInit();
}

void Renderer::deInit()
{
    // TODO: Vulkan tear down
}

VkResult Renderer::init(RendererInfo const& info)
{
    m_logger.reserve(255);
    m_pGflwWindow = info.pWindow;

    VkResult result;

    Info("Built with Vulkan SDK %d", VK_HEADER_VERSION);

    // Init m_layers
    result = createLayers();

    // Init m_vkIntance
    result = createInstance();

    // Init m_vkPhysicalDevice
    result = createPhysicalDevice();

    // Init m_vkDevice
    result = createDevice();

    // Init m_vkSurface
    result = glfwCreateWindowSurface(m_vkInstance,
                                     m_pGflwWindow,
                                     nullptr,
                                     &m_vkSurface);

    // Create a swapchain
    result = createSwapChain();

    return result;
}

VkResult Renderer::createLayers()
{
    VkResult result;

    uint32_t countLayers = 0;
    result = vkEnumerateInstanceLayerProperties(&countLayers, nullptr);
    AssertVk(result);

    auto& availableLayers = m_queriedInfo.availableLayers;
    availableLayers.resize(countLayers);
    result = vkEnumerateInstanceLayerProperties(&countLayers,
                                                availableLayers.data());
    AssertVk(result);

    // Log the available layers
    for (auto const& prop : availableLayers) {
        m_logger.append("\n    ");
        m_logger.append(prop.layerName);
    }
    Info("Found %d layers:%s", availableLayers.size(), m_logger.c_str());
    m_logger.clear();

    static constexpr const char* wantedLayers[] = {
        "VK_LAYER_LUNARG_standard_validation",
        #if !OS_MACOS
            "VK_LAYER_LUNARG_monitor",
        #endif
    };

    auto begin = std::begin(availableLayers);
    auto end   = std::end(availableLayers);
    for (const char* const pLayer : wantedLayers) {
        auto it = std::find_if(begin, end,
                              [&pLayer](VkLayerProperties const& props) -> bool {
                                    return (strcmp(props.layerName, pLayer) == 0);
                              });
        if (it != end) {
            m_layers.push_back(pLayer);
        } else {
            Info("Unable to find vulkan layer \"%s\"", pLayer);
        }
    }

    // Log the enabled layers
    for (const char* pLayerName : m_layers) {
        m_logger.append("\n    ");
        m_logger.append(pLayerName);
    }
    Info("Using %d layers:%s", m_layers.size(), m_logger.c_str());
    m_logger.clear();

    return result;
}

VkResult Renderer::createInstance()
{
    VkResult result = VK_SUCCESS;

    // Application description
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext            = nullptr;
    appInfo.pApplicationName = "Hello Vulkan";
    appInfo.pEngineName      = nullptr;
    appInfo.engineVersion    = 1;
    appInfo.apiVersion       = VK_API_VERSION_1_0;

    // Instance description
    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext               = nullptr;
    instanceInfo.flags               = 0;
    instanceInfo.pApplicationInfo    = &appInfo;
    instanceInfo.enabledLayerCount   = as<uint32_t>(m_layers.size());
    instanceInfo.ppEnabledLayerNames = m_layers.data();

    // List all instance extensions
    uint32_t instExtsCount = 0;
    result = vkEnumerateInstanceExtensionProperties(nullptr,
                                                    &instExtsCount,
                                                    nullptr);
    AssertVk(result);

    auto& availableInstExts = m_queriedInfo.availableInstExts;
    availableInstExts.resize(instExtsCount);
    result = vkEnumerateInstanceExtensionProperties(nullptr,
                                                    &instExtsCount,
                                                    &availableInstExts[0]);
    AssertVk(result);

    for (const auto& properties : availableInstExts) {
        m_logger.append("\n    ");
        m_logger.append(properties.extensionName);
    }
    Info("Found %d instance extensions", availableInstExts.size());
    Verbose("Instance extensions found:%s", m_logger.c_str());
    m_logger.clear();

    // Pick our instance extensions
    instExtsCount = 0;
    const char** glfwInstExts = glfwGetRequiredInstanceExtensions(&instExtsCount);
    Assert(glfwInstExts != nullptr);

    auto& enabledInstExts = m_queriedInfo.enabledInstExts;
    std::copy(&glfwInstExts[0],
              &glfwInstExts[instExtsCount],
              std::back_inserter(enabledInstExts));

    // Any extensions we want to add go here.
    // Right now, it's just Glfw's required extensions.

    // Verify that our extension is available!
    for (const char* pExtName : enabledInstExts) {
        auto found = std::find_if(availableInstExts.begin(),
            availableInstExts.end(),
            [pExtName](const VkExtensionProperties& extProps) {
                return (strcmp(extProps.extensionName, pExtName) == 0);
            });
        if (found == availableInstExts.end()) {
            Bug("Unable to find instance extension '%s'", pExtName);
            // TODO: User-friendly error message + abort
            DebugBreak();
        }
    }

    instanceInfo.enabledExtensionCount = as<uint32_t>(enabledInstExts.size());
    instanceInfo.ppEnabledExtensionNames = &enabledInstExts[0];

    for (const char* pInstExtName : enabledInstExts) {
        m_logger.append("\n    ");
        m_logger.append(pInstExtName);
        m_logger.append("");
    }
    Info("Using %d instance extensions: %s", enabledInstExts.size(),
                                             m_logger.c_str());
    m_logger.clear();

    result = vkCreateInstance(&instanceInfo, nullptr, &m_vkInstance);
    AssertVk(result);
    Assert(m_vkInstance!= nullptr);

    return result;
}

VkResult Renderer::createPhysicalDevice()
{
    VkResult result;

    uint32_t deviceCount;
    result = vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);
    AssertVk(result);
    Assert(deviceCount != 0);

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    result = vkEnumeratePhysicalDevices(m_vkInstance,
                                        &deviceCount,
                                        &physicalDevices[0]);
    AssertVk(result);

    Info("Found %d physical device(s)", deviceCount);
    m_vkPhysicalDevice = physicalDevices[0];
    Assert(m_vkPhysicalDevice != nullptr);

    for (const auto& device : physicalDevices) {
        m_queriedInfo.physicalDeviceInfos.emplace_back();
        auto& deviceInfo = m_queriedInfo.physicalDeviceInfos.back();
        deviceInfo.device = device;

        VkPhysicalDeviceProperties& deviceProperties = deviceInfo.properties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        Verbose("%s Info\n"
            "    Driver Version: %d\n"
            "    Device Type:    %s\n"
            "    API Version:    %d.%d.%d",
            deviceProperties.deviceName,
            deviceProperties.driverVersion,
            ToCStr(deviceProperties.deviceType),
            VK_VERSION_MAJOR(deviceProperties.apiVersion),
            VK_VERSION_MINOR(deviceProperties.apiVersion),
            VK_VERSION_PATCH(deviceProperties.apiVersion));

        auto& memoryProperties = deviceInfo.memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice,
                                            &memoryProperties);

        // Anything about memoryProperties to print out?

        VkPhysicalDeviceFeatures& features = deviceInfo.features;
        vkGetPhysicalDeviceFeatures(device, &features);
        // Print all of the properties!
        std::string dev_features_str = ToStr(features);
        Verbose("%s Physical Device Features:\n%s",
            deviceProperties.deviceName,
            dev_features_str.c_str());

        uint32_t deviceExtCount = 0;
        vkEnumerateDeviceExtensionProperties(device,
                                             nullptr,
                                             &deviceExtCount,
                                             nullptr);
        Assert(deviceExtCount > 0);

        auto& availableDeviceExts = deviceInfo.availableDeviceExts;
        availableDeviceExts.resize(deviceExtCount);
        vkEnumerateDeviceExtensionProperties(device,
                                             nullptr,
                                             &deviceExtCount,
                                             &availableDeviceExts[0]);
        sort(availableDeviceExts.begin(), availableDeviceExts.end(),
            [](const auto& lhs, const auto& rhs) {
            // Because of course we can't standardize this name.
            #if OS_WINDOWS
                return strcmpi(lhs.extensionName, rhs.extensionName) < 0;
            #else
                return strcasecmp(lhs.extensionName, rhs.extensionName) < 0;
            #endif
        });

        for (const auto& extension : availableDeviceExts) {
            m_logger.append("\n    ");
            m_logger.append(extension.extensionName);
        }
        Info("(%s) Found %d available extensions",
            deviceProperties.deviceName,
            deviceExtCount);
        Verbose("(%s) Available extensions: %s",
            deviceProperties.deviceName,
            m_logger.c_str());
        m_logger.clear();
    }

    VkPhysicalDeviceProperties deviceProperties = {};
    vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &deviceProperties);

    uint32_t apiMajor = VK_VERSION_MAJOR(deviceProperties.apiVersion);
    uint32_t apiMinor = VK_VERSION_MINOR(deviceProperties.apiVersion);
    uint32_t apiPatch = VK_VERSION_PATCH(deviceProperties.apiVersion);
    Info("Using '%s' and Vulkan %d.%d.%d", deviceProperties.deviceName,
                                           apiMajor,
                                           apiMinor,
                                           apiPatch);

    return result;
}

VkResult Renderer::createDevice()
{
    VkResult result = VK_SUCCESS;

    // Choose device extensions
    auto& enabledExts = m_queriedInfo.enabledDeviceExts;
    enabledExts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    if (OS_MACOS) {
        // TODO: Runtime vendor checks
        //       For now, we only run this on macOS with AMD.
        enabledExts.push_back("VK_AMD_negative_viewport_height");
    }

    if (OS_WINDOWS) {
         enabledExts.push_back("VK_KHR_maintenance1");
    }

    Info("Using %d device extensions", enabledExts.size());
    for (const char *pDeviceExtName : enabledExts) {
        m_logger.append("\n    ");
        m_logger.append(pDeviceExtName);
    }
    Info("Enabled device extensions:%s", m_logger.c_str());
    m_logger.clear();

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice,
                                             &queueFamilyCount,
                                             nullptr);
    auto& queueFamilies = m_queriedInfo.queueFamilies;
    queueFamilies.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice,
                                             &queueFamilyCount,
                                             queueFamilies.data());

    // Log all available queues
    for (uint32_t i = 0; i < queueFamilies.size(); i += 1) {
        if (queueFamilies[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT)) {
            m_logger.append("\n    VK_QUEUE_GRAPHICS_BIT");
        }
        if (queueFamilies[i].queueFlags & (VK_QUEUE_COMPUTE_BIT)) {
            m_logger.append("\n    VK_QUEUE_COMPUTE_BIT");
        }
        if (queueFamilies[i].queueFlags & (VK_QUEUE_TRANSFER_BIT)) {
            m_logger.append("\n    VK_QUEUE_TRANSFER_BIT");
        }
        if (queueFamilies[i].queueFlags & (VK_QUEUE_SPARSE_BINDING_BIT)) {
            m_logger.append("\n    VK_QUEUE_SPARSE_BINDING_BIT");
        }

        Verbose("Queue Family #%d (count=%d): %s", i,
                                                   queueFamilies[i].queueCount,
                                                   m_logger.c_str());
        m_logger.clear();
    }

    // Select a graphics queue (by index)
    uint32_t queueFamilyIndex = 0;
    for (uint32_t i = 0; i < queueFamilies.size(); i += 1) {
        // Take the last graphics queue that we find.
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndex = i;
        }
    }

    // Actually create the device
    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = queueFamilyIndex;
    queueInfo.queueCount       = 1;
    const float queuePriority  = 1.0f;
    queueInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures features = {};
    features.shaderClipDistance = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount    = 1;
    deviceCreateInfo.pQueueCreateInfos       = &queueInfo;
    deviceCreateInfo.enabledLayerCount       = as<uint32_t>(m_layers.size());
    deviceCreateInfo.ppEnabledLayerNames     = m_layers.data();
    deviceCreateInfo.enabledExtensionCount   = as<uint32_t>(enabledExts.size());
    deviceCreateInfo.ppEnabledExtensionNames = enabledExts.data();
    deviceCreateInfo.pEnabledFeatures        = &features;

    result = vkCreateDevice(m_vkPhysicalDevice,
                            &deviceCreateInfo,
                            nullptr,
                            &m_vkDevice);
    AssertVk(result);
    Assert(m_vkDevice != nullptr);

    return result;
}

VkResult Renderer::createSwapChain()
{
    VkResult result;

    // Query a bunch of stuff about the surface

    //
    auto& surfaceCapabilities = m_queriedInfo.surfaceCapabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysicalDevice,
                                                       m_vkSurface,
                                                       &surfaceCapabilities);
    AssertVk(result);

    // Is it even supported? lol
    VkBool32 surfaceIsSupported = VK_FALSE;
    uint32_t queueFamilyIndex = 0;
    result = vkGetPhysicalDeviceSurfaceSupportKHR(m_vkPhysicalDevice,
                                                  queueFamilyIndex,
                                                  m_vkSurface,
                                                  &surfaceIsSupported);
    AssertVk(result);
    Assert(surfaceIsSupported);

    // All of the formats
    uint32_t surfaceFormatsCount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice,
                                                  m_vkSurface,
                                                  &surfaceFormatsCount,
                                                  nullptr);
    AssertVk(result);
    auto& surfaceFormats = m_queriedInfo.surfaceFormats;
    surfaceFormats.resize(surfaceFormatsCount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice,
                                                  m_vkSurface,
                                                  &surfaceFormatsCount,
                                                  surfaceFormats.data());
    AssertVk(result);

    // All of the present modes
    uint32_t surfacePresentModesCount = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice,
                                                       m_vkSurface,
                                                       &surfacePresentModesCount,
                                                       nullptr);
    AssertVk(result);
    auto& surfacePresentModes = m_queriedInfo.surfacePresentModes;
    surfacePresentModes.resize(surfacePresentModesCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice,
                                                       m_vkSurface,
                                                       &surfacePresentModesCount,
                                                       surfacePresentModes.data());
    AssertVk(result);

    // And then use FIFO anyway.
    AssertMsg(std::find(std::begin(surfacePresentModes),
                        std::end(surfacePresentModes),
                        VK_PRESENT_MODE_FIFO_KHR)
                != std::end(surfacePresentModes),
             "Expected VK_PRESENT_MODE_FIFO_KHR to be supported.");
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

    // Actually create a swap chain
    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface          = m_vkSurface;
    swapchainInfo.minImageCount    = 2; // Double buffering
    swapchainInfo.imageFormat      = VK_FORMAT_B8G8R8A8_UNORM;
    swapchainInfo.imageColorSpace  = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapchainInfo.imageExtent      = surfaceCapabilities.currentExtent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode      = presentMode;
    swapchainInfo.clipped          = VK_TRUE;
    swapchainInfo.oldSwapchain     = nullptr;

    result = vkCreateSwapchainKHR(m_vkDevice,
                                  &swapchainInfo,
                                  nullptr,
                                  &m_vkSwapchain);
    AssertVk(result);
    Assert(m_vkSwapchain != nullptr);

    return result;
}
