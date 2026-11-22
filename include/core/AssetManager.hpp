#ifndef ASSET_MANAGER_HPP
#define ASSET_MANAGER_HPP

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace crawl {

// 텍스처·노멀맵·몬스터·초상화·UI·셰이더 리소스를 중앙 캐싱하고 안전하게 제공하는 매니저.
class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager() = default;

    // OpenGL 컨텍스트 생성 후 호출: 셰이더 가용성 판별 및 사전 로드
    void initializeShaders();

    // 상대 경로(assets/ 기준, 예: "textures/biomes/wall_catacombs.png")로 텍스처 로드/캐시
    const sf::Texture* getTexture(const std::string& relativePath,
                                  bool repeated = false,
                                  bool smooth = true);

    const sf::Texture* getMonsterTexture(const std::string& monsterId);
    const sf::Texture* getPortraitTexture(const std::string& classKey);
    const sf::Texture* getPropTexture(const std::string& propName);
    const sf::Texture* getUiTexture(const std::string& uiName);
    const sf::Texture* getBackgroundTexture(const std::string& bgName);
    const sf::Texture* getBiomeTexture(const std::string& biomeSlug, const std::string& kind);

    sf::Shader* getDungeonShader();
    sf::Shader* getPostShader();
    [[nodiscard]] bool shadersAvailable() const { return m_shadersReady; }

    void clear();

private:
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> m_textures;
    std::unique_ptr<sf::Shader> m_dungeonShader;
    std::unique_ptr<sf::Shader> m_postShader;
    bool m_shadersAttempted = false;
    bool m_shadersReady = false;
};

} // namespace crawl

#endif // ASSET_MANAGER_HPP
