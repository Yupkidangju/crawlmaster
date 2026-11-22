#ifndef GAME_HPP
#define GAME_HPP

#include <SFML/Graphics.hpp>
#include <string>
#include "core/AssetManager.hpp"
#include "core/GameStateManager.hpp"
#include "model/Party.hpp"

namespace crawl {

// Crawlmaster v1.0.0 메인 게임 엔진:
// 16:9 반응형 논리 좌표(1280x720), 마우스 좌표 투영, AssetManager, AudioManager,
// 화면 페이드 전환 및 토스트 알림, 상태 머신 루프를 총괄한다.
class Game {
public:
    static constexpr float LOGICAL_WIDTH = 1280.0f;
    static constexpr float LOGICAL_HEIGHT = 720.0f;

    explicit Game(bool headless = false);
    ~Game();

    void run();

    [[nodiscard]] const sf::Font& getFont() const;
    [[nodiscard]] const sf::Font& getMonoFont() const;
    [[nodiscard]] GameStateManager& getStates();
    [[nodiscard]] Party& getParty();
    [[nodiscard]] AssetManager& getAssets();
    [[nodiscard]] sf::RenderWindow& getWindow();

    // 반응형 16:9 뷰포트 적용 및 마우스 픽셀 -> 논리 좌표(1280x720) 변환
    void applyDisplaySettings();
    void toggleFullscreen();
    [[nodiscard]] sf::Vector2f mapPixelToLogical(sf::Vector2i pixel) const;
    [[nodiscard]] sf::Vector2f getMouseLogicalPosition() const;
    [[nodiscard]] sf::View computeLetterboxView(unsigned int width, unsigned int height) const;

    // 시각 전환/토스트/피드백
    void triggerFadeIn(float durationSec = 0.22f);
    void pushToast(const std::string& message, sf::Color accent = sf::Color(212, 175, 95));

    void requestShutdown();
    void completeShutdown();
    [[nodiscard]] bool isShutdownApproved() const;
    [[nodiscard]] bool isHeadless() const { return m_headless; }

private:
    void createWindowFromConfig();
    void processEvents();
    void update(sf::Time deltaTime);
    void render();
    void loadResources();

    sf::RenderWindow m_window;
    sf::Font m_font;
    sf::Font m_cjkFont;
    sf::Font m_monoFont;
    AssetManager m_assets;
    GameStateManager m_stateManager;
    Party m_party;

    bool m_headless = false;
    bool m_shutdownApproved = false;

    float m_fadeAlpha = 0.0f;
    float m_fadeDuration = 0.22f;
    std::string m_toastMessage;
    sf::Color m_toastAccent = sf::Color(212, 175, 95);
    float m_toastTimer = 0.0f;
};

} // namespace crawl

#endif // GAME_HPP
