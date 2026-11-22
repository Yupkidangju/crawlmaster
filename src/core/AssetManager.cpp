#include "core/AssetManager.hpp"
#include "core/ResourceLocator.hpp"

#include <algorithm>
#include <iostream>

namespace crawl {
namespace {

// OpenGL 1.1(GL_MAX_TEXTURE_SIZE=1024 등) 환경에서도 대형 텍스처가 절대 로드 실패하지 않도록 CPU 안전 축소
sf::Image downscaleIfNeeded(const sf::Image& src, unsigned int maxDim) {
    const auto sz = src.getSize();
    if (maxDim == 0 || (sz.x <= maxDim && sz.y <= maxDim)) {
        return src;
    }
    const double scale = std::min(static_cast<double>(maxDim) / static_cast<double>(sz.x),
                                  static_cast<double>(maxDim) / static_cast<double>(sz.y));
    const unsigned int nw = std::max(1U, static_cast<unsigned int>(static_cast<double>(sz.x) * scale));
    const unsigned int nh = std::max(1U, static_cast<unsigned int>(static_cast<double>(sz.y) * scale));
    sf::Image out;
    out.create(nw, nh);
    for (unsigned int y = 0; y < nh; ++y) {
        const unsigned int sy = std::min(sz.y - 1, static_cast<unsigned int>((static_cast<double>(y) + 0.5) / scale));
        for (unsigned int x = 0; x < nw; ++x) {
            const unsigned int sx = std::min(sz.x - 1, static_cast<unsigned int>((static_cast<double>(x) + 0.5) / scale));
            out.setPixel(x, y, src.getPixel(sx, sy));
        }
    }
    return out;
}

} // namespace

void AssetManager::initializeShaders() {
    if (m_shadersAttempted) return;
    m_shadersAttempted = true;
    m_shadersReady = false;

    if (!sf::Shader::isAvailable()) {
        std::cout << "[Graphics] GLSL shaders unavailable on this context; using software-textured 3D path."
                  << std::endl;
        return;
    }

    auto dungeonShader = std::make_unique<sf::Shader>();
    auto postShader = std::make_unique<sf::Shader>();

    const auto vertPath = ResourceLocator::assetPath("shaders/dungeon_lit.vert");
    const auto fragPath = ResourceLocator::assetPath("shaders/dungeon_lit.frag");
    const auto postPath = ResourceLocator::assetPath("shaders/post_bloom.frag");

    if (dungeonShader->loadFromFile(vertPath.string(), fragPath.string()) &&
        postShader->loadFromFile(postPath.string(), sf::Shader::Fragment)) {
        m_dungeonShader = std::move(dungeonShader);
        m_postShader = std::move(postShader);
        m_shadersReady = true;
        std::cout << "[Graphics] OpenGL GLSL 3D lighting & post-processing shaders initialized." << std::endl;
    } else {
        std::cerr << "[Graphics] Shader compile failed; falling back to textured 3D path." << std::endl;
    }
}

const sf::Texture* AssetManager::getTexture(const std::string& relativePath,
                                            bool repeated,
                                            bool smooth) {
    const auto it = m_textures.find(relativePath);
    if (it != m_textures.end()) {
        return it->second.get();
    }

    auto tex = std::make_unique<sf::Texture>();
    const auto fullPath = ResourceLocator::assetPath(relativePath);
    sf::Image img;
    if (!img.loadFromFile(fullPath.string())) {
        std::cerr << "[Asset Warning] Failed to load image: " << fullPath << std::endl;
        m_textures.emplace(relativePath, nullptr);
        return nullptr;
    }
    const sf::Image safeImg = downscaleIfNeeded(img, sf::Texture::getMaximumSize());
    if (!tex->loadFromImage(safeImg)) {
        std::cerr << "[Asset Warning] Failed to upload texture: " << fullPath << std::endl;
        m_textures.emplace(relativePath, nullptr);
        return nullptr;
    }
    tex->setRepeated(repeated);
    tex->setSmooth(smooth);
    const sf::Texture* ptr = tex.get();
    m_textures.emplace(relativePath, std::move(tex));
    return ptr;
}

const sf::Texture* AssetManager::getMonsterTexture(const std::string& monsterId) {
    return getTexture("textures/monsters/" + monsterId + ".png", false, true);
}

const sf::Texture* AssetManager::getPortraitTexture(const std::string& classKey) {
    return getTexture("textures/portraits/hero_" + classKey + ".png", false, true);
}

const sf::Texture* AssetManager::getPropTexture(const std::string& propName) {
    return getTexture("textures/props/" + propName + ".png", false, true);
}

const sf::Texture* AssetManager::getUiTexture(const std::string& uiName) {
    return getTexture("textures/ui/" + uiName + ".png", false, true);
}

const sf::Texture* AssetManager::getBackgroundTexture(const std::string& bgName) {
    return getTexture("textures/backgrounds/" + bgName + ".png", false, true);
}

const sf::Texture* AssetManager::getBiomeTexture(const std::string& biomeSlug, const std::string& kind) {
    return getTexture("textures/biomes/" + kind + "_" + biomeSlug + ".png", true, true);
}

sf::Shader* AssetManager::getDungeonShader() {
    return m_shadersReady ? m_dungeonShader.get() : nullptr;
}

sf::Shader* AssetManager::getPostShader() {
    return m_shadersReady ? m_postShader.get() : nullptr;
}

void AssetManager::clear() {
    m_textures.clear();
    m_dungeonShader.reset();
    m_postShader.reset();
    m_shadersAttempted = false;
    m_shadersReady = false;
}

} // namespace crawl
