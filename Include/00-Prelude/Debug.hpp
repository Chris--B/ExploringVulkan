#pragma once

static_assert(WE_HAVE_PRELUDE, "Debug.hpp was included without the Prelude.");

namespace Logging {

// Describes the location of a log event.
struct Location {
    const char* pFilename;
    const char* pFunction;
    uint32_t    line;

    Location() = delete;
    Location(const char* pFilename,
             uint32_t    line,
             const char* pFunction);
};
#define NullLocation() ::Logging::Location(nullptr, 0, nullptr)
#define MakeLocation() ::Logging::Location(__FILE__,                            \
                                           __LINE__,                            \
                                           __func__)

// Generic method to log a message.
// TODO: Should this be in the source file?
//
// These messages may go to one or more of the following, if they're present:
//      Command line terminal
//      Debug window
//      Output file(s)
//
// Arguments:
//      'pTag'    - Human-readable tag (useful for filtering?)
//      'loc'     - Must be passed as a MakeLocation() macro call
//      'pFormat' - printf style format string
//      '...'     - printf style varargs
void _LogMsg(const char* pTag, Location&& location, const char* pFormat, ...);

// Each of these tags fits nicely within 5 characters.
#define LogMsg(tag, location, ...) _LogMsg((tag), (location), __VA_ARGS__)
#define Info(...)            LogMsg("Info",  NullLocation(), __VA_ARGS__)
#define Verbose(...)         LogMsg("Info+", NullLocation(), __VA_ARGS__)
#define Debug(...)           LogMsg("Debug", MakeLocation(), __VA_ARGS__)
#define Bug(...)             LogMsg("Bug",   MakeLocation(), __VA_ARGS__)

} // Logging

#if OS_WINDOWS
    #define DebugBreak() __debugbreak()
#else
    #define DebugBreak() __builtin_debugtrap()
#endif

#define Assert(f) do {                                                          \
            if (!(f)) {                                                         \
                Bug("'%s' failed", # f);                                        \
                DebugBreak();                                                   \
            }                                                                   \
        } while (false)

#define AssertMsg(val, ...) do {                                                \
        if (!(val)) {                                                           \
            Bug(__VA_ARGS__);                                                   \
            Assert(val);                                                        \
        }                                                                       \
    } while (false)

#define AssertVk(result) do {                                                   \
        /* Static typing on a text-replacement macro. I love C++. */            \
        static_assert(std::is_same<VkResult, decltype(result)>::value,          \
                      "This macro expects a VkResult");                         \
        if (result != VK_SUCCESS) {                                             \
            Bug("'%s' = %s", STR(result), ToCStr(result));                      \
            DebugBreak();                                                       \
        }                                                                       \
    } while (false)

