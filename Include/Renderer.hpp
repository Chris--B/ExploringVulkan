#pragma once

#include "00-Prelude.hpp"

// Information that the graphic system needs to know from other systems
// e.g. Anything from GLFW
struct RendererInfo
{
    RendererInfo() = default;

    GLFWwindow* pWindow             = nullptr;
    int         framebufferWidth    = 0;
    int         framebufferHeight   = 0;
};

// Information queried from Vulkan about devices, capabilities, formats. etc.
struct QueriedVulkanInfo
{
    QueriedVulkanInfo() = default;

    // These are ordered roughly in the order that they are initialized.

    std::vector<VkLayerProperties>          availableLayers;
    std::vector<VkExtensionProperties>      availableInstExts;
    std::vector<const char*>                enabledInstExts;

    struct PhysicalDeviceInfo
    {
        PhysicalDeviceInfo() = default;

        VkPhysicalDevice                    device                  = nullptr;
        VkPhysicalDeviceProperties          properties              = {};
        VkPhysicalDeviceFeatures            features                = {};
        VkPhysicalDeviceMemoryProperties    memoryProperties        = {};
        std::vector<VkExtensionProperties>  availableDeviceExts;
    };
    std::vector<PhysicalDeviceInfo>         physicalDeviceInfos;
    size_t                                  physicalDeviceIndex     = 0;

    std::vector<const char*>                enabledDeviceExts;
    std::vector<VkQueueFamilyProperties>    queueFamilies;

    VkSurfaceCapabilitiesKHR                surfaceCapabilities     = {};
    std::vector<VkSurfaceFormatKHR>         surfaceFormats;
    std::vector<VkPresentModeKHR>           surfacePresentModes;
};

class Renderer
{
    public:
        Renderer() = default;
        ~Renderer();

        VkResult init(RendererInfo const& info);
        void     deInit();

        void     doOneFrame();

        #define USE_CUSTOM_VK_ALLOC 0
        #if USE_CUSTOM_VK_ALLOC
        VkAllocationCallbacks const* getVkAlloc() const { return &m_vkAlloc; }
        #else
        VkAllocationCallbacks const* getVkAlloc() const { return nullptr; }
        #endif

    private:

        // Windowing object
        GLFWwindow*                 m_pGlfwWindow               = nullptr;

        // ---- Vulkan objects --------------------------------------------------

        // Core objects
        VkInstance                  m_vkInstance                = nullptr;
        VkPhysicalDevice            m_vkPhysicalDevice          = nullptr;
        VkDevice                    m_vkDevice                  = nullptr;
        VkAllocationCallbacks       m_vkAlloc                   = {}; // TODO

        // Presentation objects
        static constexpr uint32_t   N                           = 2;
        VkSurfaceKHR                m_vkSurface                 = nullptr;
        VkSwapchainKHR              m_vkSwapchain               = nullptr;
        VkImage                     m_vkPresentImages[N]        = {};
        VkImageView                 m_vkPresentImageViews[N]    = {};
        VkFramebuffer               m_vkFramebuffers[N]         = {};
        VkQueue                     m_vkPresentQueue            = nullptr;
        VkSemaphore                 m_vkRenderSemaphore         = nullptr;

        // Rendering objects
        VkCommandBuffer             m_vkDrawCmdBuffer           = nullptr;
        VkRenderPass                m_vkRenderPass              = nullptr;

        // Pools
        VkCommandPool               m_vkCommandPool             = nullptr;
        VkDescriptorPool            m_vkDescriptorPool          = nullptr;

        // Depth Buffer objects
        VkImage                     m_vkDepthImage              = nullptr;
        VkImageView                 m_vkDepthImageView          = nullptr;
        VkDeviceMemory              m_vkDepthDeviceMemory       = nullptr;

        // Pipeline objects
        VkShaderModule              m_vkShaderModule            = nullptr;
        VkDescriptorSet             m_vkDescriptorSet           = nullptr;
        VkDescriptorSetLayout       m_vkDescriptorSetLayout     = nullptr;

        VkPipeline                  m_vkPipeline                = nullptr;
        VkPipelineLayout            m_vkPipelineLayout          = nullptr;

        // Shader Uniforms
        VkBuffer                    m_vkUniformBuffer           = nullptr;
        VkDeviceMemory              m_vkUniformDeviceMemory     = nullptr;

        // ???
        VkFence                     m_vkUnknownFence            = nullptr;

        QueriedVulkanInfo           m_queriedInfo;

        std::vector<const char*>    m_layers;
        std::string                 m_logger;

        VkResult createLayers();
        VkResult createInstance();
        VkResult createPhysicalDevice();
        VkResult createDevice();
        VkResult createSwapChain();
        VkResult createPresentImages();
        VkResult createCommandPool();
        VkResult createDepthBuffer(VkExtent3D const& extent);
        VkResult createRenderPassAndFramebuffer(VkExtent3D const& extent);
};
