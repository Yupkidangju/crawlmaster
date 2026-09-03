#ifndef RESOURCE_LOCATOR_HPP
#define RESOURCE_LOCATOR_HPP

#include <filesystem>

namespace crawl {

class ResourceLocator {
public:
    static std::filesystem::path executableDirectory();
    static std::filesystem::path assetDirectory();
    static std::filesystem::path assetPath(const std::filesystem::path& relativePath);
};

} // namespace crawl

#endif // RESOURCE_LOCATOR_HPP
