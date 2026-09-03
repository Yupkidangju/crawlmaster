#include "core/ResourceLocator.hpp"

#include <array>
#include <cstdlib>
#include <string>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace crawl {
namespace {

std::string environmentValue(const char* name) {
#ifdef _WIN32
    char* rawValue = nullptr;
    std::size_t valueSize = 0;
    if (_dupenv_s(&rawValue, &valueSize, name) != 0 || rawValue == nullptr) {
        std::free(rawValue);
        return {};
    }
    const std::string value(rawValue);
    std::free(rawValue);
    return value;
#else
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string{};
#endif
}

} // namespace

std::filesystem::path ResourceLocator::executableDirectory() {
#ifdef _WIN32
    std::array<wchar_t, 32768> buffer{};
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length > 0 && length < buffer.size()) {
        return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
    }
#else
    std::array<char, 4096> buffer{};
    const ssize_t length = ::readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length > 0) {
        return std::filesystem::path(std::string(buffer.data(), static_cast<std::size_t>(length))).parent_path();
    }
#endif
    return std::filesystem::current_path();
}

std::filesystem::path ResourceLocator::assetDirectory() {
    if (const std::string overridePath = environmentValue("CRAWLMASTER_ASSET_DIR");
        !overridePath.empty()) {
        const std::filesystem::path path(overridePath);
        if (path.is_absolute() && std::filesystem::is_directory(path)) return path;
    }

    const auto executable = executableDirectory();
    const std::array candidates = {
        executable / "assets",
        executable / ".." / "assets",
        executable / ".." / "share" / "crawlmaster" / "assets"
    };
    for (const auto& candidate : candidates) {
        std::error_code error;
        const auto canonical = std::filesystem::weakly_canonical(candidate, error);
        if (!error && std::filesystem::is_directory(canonical)) return canonical;
    }
    return executable / "assets";
}

std::filesystem::path ResourceLocator::assetPath(const std::filesystem::path& relativePath) {
    return assetDirectory() / relativePath;
}

} // namespace crawl
