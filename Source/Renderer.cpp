#include "Renderer.hpp"

#include <algorithm>

Renderer::~Renderer()
{
    deInit();
}

void Renderer::deInit()
{
    vkDeviceWaitIdle(m_vkDevice);
    // TODO: Vulkan tear down
}

VkResult Renderer::init(RendererInfo const& info)
{
    m_logger.reserve(255);
    m_pGlfwWindow = info.pWindow;
    glfwSetWindowUserPointer(m_pGlfwWindow, this);

    Info("sizeof(Renderer) == %zu", sizeof(*this));

    VkResult result;

    VkExtent2D extent2d = {
        /*width*/  as<uint32_t>(info.framebufferWidth),
        /*height*/ as<uint32_t>(info.framebufferHeight),
    };
    VkExtent3D extent3d = {
        /*width*/  as<uint32_t>(info.framebufferWidth),
        /*height*/ as<uint32_t>(info.framebufferHeight),
        /*depth*/  1
    };

    Info("Built with Vulkan SDK %d", VK_HEADER_VERSION);

    // Init m_layers
    result = createLayers();

    // Init m_vkIntance
    result = createInstance();

    // Init m_vkPhysicalDevice
    result = createPhysicalDevice();

    // Init m_vkDevice and m_vkGraphicsQueue
    result = createDevice();

    // Init Semaphores and Fences.
    {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreInfo.flags = 0;
        result = vkCreateSemaphore(m_vkDevice, &semaphoreInfo, getVkAlloc(),
                                   &m_vkRenderSemaphore);
        AssertVk(result);
    }

    // Init m_vkSurface
    result = glfwCreateWindowSurface(m_vkInstance,
                                     m_pGlfwWindow,
                                     nullptr,
                                     &m_vkSurface);

    // Create a swapchain
    result = createSwapChain();

    // Init m_vkPresentImages, and m_vkPresentImageViews
    result = createPresentImages();

    // Init m_vkCommandPool
    result = createCommandPool();

    // Init m_vkDepthImage, m_vkDepthImageView, and m_vkDepthDeviceMemory
    result = createDepthBuffer(extent3d);

    // Init m_Framebuffer, and ???
    result = createRenderPassAndFramebuffer(extent3d);

    return result;
}

void Renderer::doOneFrame()
{
    constexpr uint32_t frameId = 0;
    VkResult result;

    VkExtent2D extent2d = {};
    glfwGetFramebufferSize(m_pGlfwWindow,
                           ptr_as<int>(&extent2d.width),
                           ptr_as<int>(&extent2d.height));

    VkCommandBufferAllocateInfo cmdBufAllocInfo;
    cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocInfo.commandPool        = m_vkCommandPool;
    cmdBufAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocInfo.commandBufferCount = 1;

    VkCommandBuffer simpleDraw;
    result = vkAllocateCommandBuffers(m_vkDevice, &cmdBufAllocInfo, &simpleDraw);
    AssertVk(result);

    // Record CmdBuffer
    VkCommandBufferBeginInfo cmdInfo = {};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    result = vkBeginCommandBuffer(simpleDraw, &cmdInfo);
    AssertVk(result);
    {

    }
    // End
    result = vkEndCommandBuffer(simpleDraw);
    AssertVk(result);

    // Submit
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 0;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &simpleDraw;
    submitInfo.signalSemaphoreCount = 0;

    result = vkQueueSubmit(m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

    // Wait - Who needs synchronization anyway?
    vkDeviceWaitIdle(m_vkDevice);

    vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &simpleDraw);
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
        auto preditcate = [pLayer](VkLayerProperties const& props) -> bool {
            return (strcmp(props.layerName, pLayer) == 0);
        };
        auto it = std::find_if(begin, end, preditcate);

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

    for (auto const& device : physicalDevices) {
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
        vkGetPhysicalDeviceMemoryProperties(device,
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

    // Pretend we did something more involved here.
    m_queriedInfo.physicalDeviceIndex = 0;

    m_vkPhysicalDevice = physicalDevices[m_queriedInfo.physicalDeviceIndex];
    Assert(m_vkPhysicalDevice != nullptr);

    VkPhysicalDeviceProperties deviceProperties = {};
    vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &deviceProperties);

    Info("Found %d physical device(s)", physicalDevices.size());
    Info("Using '%s' with Vulkan %d.%d.%d",
         deviceProperties.deviceName,
         VK_VERSION_MAJOR(deviceProperties.apiVersion),
         VK_VERSION_MINOR(deviceProperties.apiVersion),
         VK_VERSION_PATCH(deviceProperties.apiVersion));

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
    for (uint32_t i = 0; i < queueFamilies.size(); i += 1) {
        // Take the first graphics queue that we find.
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            m_vkGraphicsQueueIndex = i;
            break;
        }
    }
    Info("Using queue family #%d for graphics", m_vkGraphicsQueueIndex);

    // Actually create the device
    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = m_vkGraphicsQueueIndex;
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

    vkGetDeviceQueue(m_vkDevice,
                     m_vkGraphicsQueueIndex,
                     0, // We only made 1
                     &m_vkGraphicsQueue);
    Assert(m_vkGraphicsQueue != nullptr);

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
    Info("Surface Image Count: (%d, %d)", surfaceCapabilities.minImageCount,
                                          surfaceCapabilities.maxImageCount);

    // Is it even supported? lol
    VkBool32 surfaceIsSupported = VK_FALSE;
    result = vkGetPhysicalDeviceSurfaceSupportKHR(m_vkPhysicalDevice,
                                                  m_vkGraphicsQueueIndex,
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
    swapchainInfo.minImageCount    = 3;
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

VkResult Renderer::createPresentImages()
{
    VkResult result;

    // VkImage
    uint32_t n_images = 0;
    result = vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &n_images,
                                     nullptr);
    AssertMsg(n_images == Renderer::N,
              "Expected %d present images, but got %d", Renderer::N, n_images);
    AssertVk(result);
    result = vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &n_images,
                                     &m_vkPresentImages[0]);
    AssertVk(result);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.flags                           = 0;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = VK_FORMAT_B8G8R8A8_UNORM;
    viewInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    for (uint32_t i = 0; i < array_size(m_vkPresentImages); i += 1) {
        viewInfo.image = m_vkPresentImages[i];
        result = vkCreateImageView(m_vkDevice, &viewInfo, nullptr,
                                   &m_vkPresentImageViews[i]);
        AssertVk(result);
    }

    return result;
}

VkResult Renderer::createCommandPool()
{
    VkResult result;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
                     VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = m_vkGraphicsQueueIndex;

    result = vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr,
                                 &m_vkCommandPool);

    return result;
}

VkResult Renderer::createDepthBuffer(VkExtent3D const& extent)
{
    VkResult result;

    // m_vkDepthImage
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent        = extent;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateImage(m_vkDevice, &imageInfo, nullptr, &m_vkDepthImage);
    AssertVk(result);

    // Allocate
    VkMemoryRequirements memReq = {};
    vkGetImageMemoryRequirements(m_vkDevice, m_vkDepthImage, &memReq);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memReq.size;
    allocInfo.memoryTypeIndex = VK_MAX_MEMORY_TYPES;

    // TODO: This should be a function.
    {
        VkPhysicalDeviceMemoryProperties memoryProperties = {};
        vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice,
                                            &memoryProperties);
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            // Use the first memory type that works.
            if (memReq.memoryTypeBits & (1u << i)) {
                allocInfo.memoryTypeIndex = i;
                break;
            }
        }
        AssertMsg(allocInfo.memoryTypeIndex < memoryProperties.memoryTypeCount,
                  "Couldn't find a valid memory type index");
    }
    result = vkAllocateMemory(m_vkDevice, &allocInfo, nullptr,
                              &m_vkDepthDeviceMemory);
    AssertVk(result);

    constexpr VkDeviceSize memoryOffset = 0;
    result = vkBindImageMemory(m_vkDevice, m_vkDepthImage, m_vkDepthDeviceMemory,
                               memoryOffset);
    AssertVk(result);

    // m_vkDepthImageView
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

    viewInfo.flags    = 0;
    viewInfo.image    = m_vkDepthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format   = VK_FORMAT_D32_SFLOAT;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    result = vkCreateImageView(m_vkDevice, &viewInfo, nullptr,
                               &m_vkDepthImageView);
    AssertVk(result);

    return result;
}

VkResult Renderer::createRenderPassAndFramebuffer(VkExtent3D const& extent)
{
    VkResult result;

    uint32_t attachmentCount = 0;

    // Presentable Color Attachment
    VkAttachmentDescription colorDesc = {};
    colorDesc.format         = VK_FORMAT_B8G8R8A8_UNORM;
    colorDesc.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorDesc.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorDesc.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorDesc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorDesc.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorDesc.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef = {};
    colorRef.attachment = attachmentCount;
    colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentCount += 1;

    // Depth Attachment
    VkAttachmentDescription depthDesc = {};
    depthDesc.format         = VK_FORMAT_D32_SFLOAT;
    depthDesc.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthDesc.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthDesc.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthDesc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthDesc.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthDesc.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef = {};
    depthRef.attachment = attachmentCount;
    depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachmentCount += 1;

    VkAttachmentReference colorRefs[] = {
        colorRef,
    };

    VkSubpassDescription subpassDesc = {};
    subpassDesc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount    = array_size(colorRefs);
    subpassDesc.pColorAttachments       = colorRefs;
    subpassDesc.pDepthStencilAttachment = &depthRef;

    VkAttachmentDescription attachmentDescs[] = {
        colorDesc,
        depthDesc,
    };
    Assert(attachmentCount == array_size(attachmentDescs));

    VkSubpassDescription subpasses[] = {
        subpassDesc,
    };

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = array_size(attachmentDescs);
    renderPassInfo.pAttachments    = attachmentDescs;
    renderPassInfo.subpassCount    = array_size(subpasses);
    renderPassInfo.pSubpasses      = subpasses;

    result = vkCreateRenderPass(m_vkDevice, &renderPassInfo, getVkAlloc(),
                                &m_vkRenderPass);
    AssertVk(result);

    VkFramebufferCreateInfo fbInfo = {};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = m_vkRenderPass;
    fbInfo.width      = extent.width;
    fbInfo.height     = extent.height;
    fbInfo.layers     = extent.depth;

    Info("VkFramebufferCreateInfo {\n"
         "    renderPass = 0x%016x,\n"
         "    width      = %u,\n"
         "    height     = %u,\n"
         "    depth      = %u\n"
         "}",
         m_vkRenderPass,
         extent.width,
         extent.height,
         extent.depth
    );

    for (uint32_t i = 0; i < array_size(m_vkFramebuffers); i += 1) {
        VkImageView attachmentsViews[2] = {
            m_vkPresentImageViews[i],
            m_vkDepthImageView,
        };
        fbInfo.attachmentCount = array_size(attachmentsViews);
        fbInfo.pAttachments    = attachmentsViews;

        Info("Creating framebuffer #%u", i);
        result = vkCreateFramebuffer(m_vkDevice, &fbInfo, getVkAlloc(),
                                     &m_vkFramebuffers[i]);
        AssertVk(result);
    }

    return result;
}
