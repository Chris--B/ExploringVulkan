#include "00-Prelude.hpp"

#include "Renderer.hpp"

#include "tiny_obj_loader.h"

#include <algorithm>
#include <string>
#include <vector>

struct UberContext
{
    bool fullscreen = false;
    uint32_t height = 0;
    uint32_t width  = 0;
};

struct Vertex
{
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    float w = 1.f;

    Vertex() = default;
    Vertex(float x, float y, float z, float w = 1.f)
            : x(x), y(y), z(z), w(w) {}
};

// ==== GLFW Callbacks ==========================================================

void glfwReportError(int error, const char* pMsg)
{
    const char* pErrorStr = [&error]() -> const char* {
        switch (error) {
            TO_CSTR_CASE(GLFW_NO_ERROR)
            TO_CSTR_CASE(GLFW_NOT_INITIALIZED)
            TO_CSTR_CASE(GLFW_NO_CURRENT_CONTEXT)
            TO_CSTR_CASE(GLFW_INVALID_ENUM)
            TO_CSTR_CASE(GLFW_INVALID_VALUE)
            TO_CSTR_CASE(GLFW_OUT_OF_MEMORY)
            TO_CSTR_CASE(GLFW_API_UNAVAILABLE)
            TO_CSTR_CASE(GLFW_VERSION_UNAVAILABLE)
            TO_CSTR_CASE(GLFW_PLATFORM_ERROR)
            TO_CSTR_CASE(GLFW_FORMAT_UNAVAILABLE)
            TO_CSTR_CASE(GLFW_NO_WINDOW_CONTEXT)
            default: return nullptr; // Unknown
        }
    }();
    if (pErrorStr != nullptr) {
        Bug("GLFW-ERROR: %s: %s", pErrorStr, pMsg);
    } else {
        Bug("GLFW-ERROR: UNKNOWN_ERROR (0x%x) %s", error, pMsg);
    }
}

void glfwKeyCallback(GLFWwindow* pWindow,
                     int         key,
                     int         scancode,
                     int         action,
                     int         mods)
{
    UNUSED(scancode);
    UNUSED(action);
    UNUSED(mods);

    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
        case GLFW_KEY_Q:
            glfwSetWindowShouldClose(pWindow, 1);
            break;
        case GLFW_KEY_F:
            Debug("TODO: Toggle fullscreen");
            break;

        default:
            UNUSED("Ignore other keys");
            break;
        }
    }
}

void glfwFramebufferSizeCallback(GLFWwindow* pWindow,
                                 int         width,
                                 int         height)
{
    Info("GLFW Framebuffer resized -> (%d, %d)", width, height);
}

const char* getEnvVarOr(const char* pEnvName, const char* pDefaultStr = "")
{
    const char* pValue = getenv(pEnvName);
    if (pValue != nullptr) {
        return pValue;
    }
    return pDefaultStr;
}

int main(int argc, char** argv)
{
    if (argc == 1) {
        printf("%d arg:\n", argc);
    } else {
        printf("%d args:\n", argc);
    }
    for (int i = 0; i < argc; i += 1) {
        printf("  \"%s\"\n", argv[i]);
    }

    // Resources are loaded with relative paths, so it's useful to make sure
    // the application started where we thought it did.
    // Different platforms start in slightly different directories, so this will
    // help keep us sane.
    // (e.g. MSVC adds a 'Debug/' folder between the binary and resources.)
    {
        char buffer[1024] = "";

        #if OS_WINDOWS
        GetCurrentDirectory(array_size(buffer), buffer);
        #else
        getcwd(buffer, array_size(buffer));
        #endif

        Info("Working directory: %s", buffer);
    }

    // ====

    UberContext ctx;

    // ==== GLFW Init ===========================================================
    GLFWwindow* pWindow = nullptr;
    {
        glfwSetErrorCallback(glfwReportError);
        bool glfwOk = glfwInit() == 0 ? false : true;
        // TODO: More user-friendly failure here would be nice.
        AssertMsg(glfwOk, "glfwInit() failed");
        AssertMsg(glfwVulkanSupported(), "GLFW reports Vulkan is unsupported");
    }

    {
        ctx.fullscreen = (strcmp(getEnvVarOr("FULLSCREEN", "0"), "1") == 0);
        if (ctx.fullscreen) {
            Info("Starting in fullscreen");
        }
    }

    // Pick resolution, etc.
    {
        GLFWmonitor* pMonitor = glfwGetPrimaryMonitor();
        const auto* pVidmode = glfwGetVideoMode(pMonitor);
        if (ctx.fullscreen) {
            ctx.height = pVidmode->height;
            ctx.width = pVidmode->width;
        } else {
            pMonitor = nullptr; // Tell Glfw to go windowed
            ctx.height = pVidmode->height / 2;
            ctx.width = std::min(as<uint32_t>(pVidmode->height / 1.6),
                                 as<uint32_t>(pVidmode->width));
        }
        Info("Window resolution: %d x %d", ctx.width, ctx.height);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, false);
        pWindow = glfwCreateWindow(ctx.width,
                                   ctx.height,
                                   "Hello Vulkan!",
                                   pMonitor,
                                   nullptr // GLFWwindow* share
        );
        AssertMsg(pWindow != nullptr, "Unable to create a GLFW window");
    }

    // Set Glfw Callbacks
    glfwSetKeyCallback(pWindow, glfwKeyCallback);
    glfwSetFramebufferSizeCallback(pWindow, glfwFramebufferSizeCallback);

    // The framebuffer might be larger than the window size, especially when
    // running on high DPI displays.
    // MacOS does this, and it's not by an easy to predict ratio.
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(pWindow, &framebufferWidth, &framebufferHeight);
    Info("Framebuffer resolution: %d x %d", framebufferWidth,
                                            framebufferHeight);

    // ==== Renderer Init =======================================================
    RendererInfo renderInfo;
    renderInfo.pWindow           = pWindow;
    renderInfo.framebufferWidth  = framebufferWidth;
    renderInfo.framebufferHeight = framebufferHeight;

    Renderer renderer;
    VkResult result = renderer.init(renderInfo);
    AssertVk(result);

    // Load a model!
    {
        using namespace tinyobj;

        const char* pFilename = "../External/tinyobjloader/models/"
                                "cornell_box.obj";

        attrib_t                attrib;
        std::vector<shape_t>    shapes;
        std::vector<material_t> materials;
        std::string errMsg;

        Info("Loading wavefront file \"%s\"", pFilename);
        bool okay = LoadObj(&attrib,
                            &shapes,
                            &materials,
                            &errMsg,
                            pFilename,
                            nullptr /*mtl base directroy*/);
        AssertMsg(okay, "tinyobj: %s", errMsg.c_str());

        Info("Loaded %u vertices", attrib.vertices.size());
        std::vector<Vertex> verts;
        verts.reserve(attrib.vertices.size());

        Vec3 min(0, 0, 0);
        Vec3 max(0, 0, 0);

        for (const auto& shape : shapes) {
            for (const index_t& index : shape.mesh.indices) {
                size_t i = index.vertex_index;
                Vec3& pos = *reinterpret_cast<Vec3*>(&attrib.vertices[3*i]);
                min = glm::min(min, pos);
                max = glm::max(max, pos);
            }
        }

        Info("min:    (% 6.1f, % 6.1f, % 6.1f)", min.x, min.y, min.z);
        Info("max:    (% 6.1f, % 6.1f, % 6.1f)", max.x, max.y, max.z);

        Vec3 offset = -0.5f * (max - min);
        offset.z = 0;
        Vec3 scale = 3.f / (max - min);

        Info("offset: (% 6.1f, % 6.1f, % 6.1f)", offset.x, offset.y, offset.z);
        Info("scale:  (% 6.1f, % 6.1f, % 6.1f)", scale.x, scale.y, scale.z);

        for (const auto& shape : shapes) {
            for (const index_t& index : shape.mesh.indices) {
                size_t i = index.vertex_index;
                Vec3 pos = *reinterpret_cast<Vec3*>(&attrib.vertices[3*i]);
                pos = scale * (pos + offset);
                verts.emplace_back(pos.x, pos.y, pos.z);
            }
        }
    }

    // Main loop
    while (!glfwWindowShouldClose(pWindow)) {
        glfwPollEvents();
    }

    glfwTerminate();
}

#if OS_WINDOWS
int WINAPI WinMain(HINSTANCE /* hInstance     */,
                   HINSTANCE /* hPrevInstance */,
                   LPSTR     /* lpCmdLine     */,
                   int       /* nCmdShow      */)
{
    main(__argc, __argv);
}
#endif
