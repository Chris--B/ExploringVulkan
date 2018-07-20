#include "00-Prelude.hpp"

#include <cstdarg>
#include <cctype>

using namespace Logging;

// TODO: Expose this somewhere. CMake? Commandline args?
static const bool kEnableVerboseLogging = false;

// Sanitize a filename by removing a fixed prefix, set by CMake.
// TODO: Sometimes full paths are nice, so it would be nice if
//       we could toggle this.
static const char* DePrefixFilename(const char* pFilename)
{
    const char* pPrefix = DEMO_SOURCE_DIR;

    // We normalize the paths on Windows.
    // CMake likes doing: C:/Code/Graphics/vgfx
    // While MSVC does:   c:\code\Graphics\vgfx\Source\main.cpp
    #if OS_WINDOWS
    static constexpr uint32_t kPathSize = 512;
    // Normalize the prefix
    char normPrefix[kPathSize];
    GetFullPathNameA(pPrefix, kPathSize, normPrefix, nullptr);
    pPrefix = normPrefix;

    // Normalize the filename
    char normFilename[kPathSize];
    GetFullPathNameA(pFilename, kPathSize, normFilename, nullptr);
    pFilename = normFilename;
    #endif

    uint32_t i = 0;
    while (pPrefix[i]   != 0 &&
           pFilename[i] != 0 &&
           // Again, Windows is funny so we lower-case everything for compares.
           tolower(pFilename[i]) == tolower(pPrefix[i]))
    {
        i += 1;
    }
    return &pFilename[i];
}

// This exists because we briefly had two paths for logging and needed to reuse
// code between them. Now this is here (and not inlined) because it may be
// useful someday.
// e.g. MessageBox can call Impl, then display the text!
// Returns the number of characters written into 'pBuffer',
// including newline and NUL.
static uint32_t LogMsgFormat(char* pBuffer,
                        uint32_t bufferSize,
                        const char* pTag,
                        const Location& location,
                        const char* pFormat,
                        va_list args)
{
    int32_t written    = 0;
    int32_t remaining  = bufferSize - 1 /*'\n'*/ - 1 /*'\0'*/;
    int32_t lastLength = 0;

    lastLength = snprintf(&pBuffer[written], remaining,
                          "%-5s :: ",
                          pTag);
    written   += lastLength;
    remaining -= lastLength;
    if (remaining <= 2) { return written; }

    if (location.pFilename != nullptr) {
        const char* pFilename = DePrefixFilename(location.pFilename);
        lastLength = snprintf(&pBuffer[written], remaining,
                               "%s:%d ",
                              pFilename,
                              location.line);
        written    += lastLength;
        remaining  -= lastLength;
        if (remaining <= 2) { return written; }
    }

    lastLength = vsnprintf(&pBuffer[written], remaining, pFormat, args);
    written    += lastLength;
    remaining  -= lastLength;
    if (remaining <= 2) { return written; }

    pBuffer[written]   = '\n';
    pBuffer[written+1] = '\0';
    written += 2;

    return written;
}

Location::Location(const char* pFilename,
                   uint32_t    line,
                   const char* pFunction)
    :  pFilename(pFilename),
       line(line),
       pFunction(pFunction)
{
}

void Logging::_LogMsg(const char* pTag,
                      Location&& location,
                      const char* pFormat,
                      ...)
{
    va_list args;
    va_start(args, pFormat);

    if (!kEnableVerboseLogging) {
        uint32_t length = as<uint32_t>(strlen(pTag));
        // Ends with '+' -> Verbose only
        if (length - 1 < length && pTag[length - 1] == '+') {
            return;
        }
    }

    constexpr uint32_t bufferSize = 3072;
    char buffer[bufferSize];

    uint32_t written = LogMsgFormat(buffer,
                                    bufferSize,
                                    pTag,
                                    location,
                                    pFormat,
                                    args);
    va_end(args);
    if (written >= bufferSize) {
        snprintf(&buffer[bufferSize - 32], 32, "... TRUNCATED OH SHIT1\n");

        #if defined(_WIN64)
        // Windows Debug Output
        OutputDebugString(buffer);
        #endif

        // Terminal
        printf("%s", buffer);

        return;
    }

    char *pFinalMsg = buffer;

    uint32_t newlineCount = 0;
    for (uint32_t i = 0; buffer[i] != '\0'; i += 1)
    {
        if (buffer[i] == '\n' ) {
            newlineCount += 1;
        }
    }

    char post[bufferSize];
    if (newlineCount > 1) {
        uint32_t i      = 0;
        uint32_t offset = 0;
        for (; buffer[i] != '\0'; i += 1) {
            if ((i + offset) >= bufferSize) {
                snprintf(&post[bufferSize - 32], 32, "... TRUNCATED OH SHIT2\n");
                DebugBreak();
                break;
            }
            post[i + offset] = buffer[i];
            if (buffer[i] == '\n' && buffer[i + 1] != '\0') {
                offset += sprintf(&post[i + offset + 1], "%-5s :: ", "");
            }
        }
        if ((i + offset) < bufferSize) {
            post[i + offset] = '\0';
        }
        pFinalMsg = post;
    }

    #if defined(_WIN64)
    // Windows Debug Output
    OutputDebugString(pFinalMsg);
    #endif

    // Terminal
    printf("%s", pFinalMsg);

    // TODO: A log file would be nice
}
