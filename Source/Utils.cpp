#include <00-Prelude.hpp>
#include <fstream>

std::vector<uint8_t> loadBytesFrom(const char *const filename)
{
    // Open the file for binary reading, starting at the end.
    std::ifstream file(filename, ::std::ios::binary | ::std::ios::ate);
    if (!file.is_open()) {
        AssertMsg(file.is_open(), "Unable to open file '%s'", filename);
    }

    auto size = as<uint64_t>(file.tellg());
    Info("Loading %zu bytes from %s", size, filename);
    Assert(size != 0);

    std::vector<uint8_t> bytes(size, '\0');
    file.seekg(0, std::ios::beg);
    Assert(file);
    file.read((char*)&bytes[0], as<std::streamsize>(size));
    Assert(file);

    return bytes;
}
