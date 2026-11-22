#include "core/Game.hpp"
#include "core/AudioManager.hpp"
#include "core/LocalizationManager.hpp"
#include "core/ResourceLocator.hpp"
#include "core/SessionRng.hpp"
#include "controller/ShutdownState.hpp"
#include "controller/TitleState.hpp"

#include <algorithm>
#include <iostream>

namespace crawl {

Game::Game(bool headless) : m_headless(headless) {
    std::cout << "[Session] RNG seed: " << SessionRng::global().seed() << std::endl;
    LocalizationManager::getInstance().loadConfig();
    AudioManager::getInstance().initialize(headless);
    const auto& lm = LocalizationManager::getInstance();
    AudioManager::getInstance().applyVolumeConfig(
        lm.getMasterVolume(), lm.getBgmVolume(), lm.getSfxVolume(), lm.isMuted());

    if (headless) {
        loadResources();
        return;
    }

    createWindowFromConfig();
    loadResources();
    m_assets.initializeShaders();
    triggerFadeIn(0.35f);
    m_stateManager.changeState(std::make_unique<TitleState>(*this));
}

Game::~Game() {
    m_assets.clear();
}

void Game::createWindowFromConfig() {
    const auto& lm = LocalizationManager::getInstance();
    const unsigned int w = static_cast<unsigned int>(lm.getWindowWidth());
    const unsigned int h = static_cast<unsigned int>(lm.getWindowHeight());

    sf::ContextSettings ctx;
    ctx.depthBits = 24;
    ctx.antialiasingLevel = lm.getGraphicsQuality() > 0 ? 4 : 0;

    if (lm.isFullscreen()) {
        const auto desktop = sf::VideoMode::getDesktopMode();
        m_window.create(desktop, "Crawlmaster", sf::Style::Fullscreen, ctx);
    } else {
        m_window.create(sf::VideoMode(w, h), "Crawlmaster", sf::Style::Default, ctx);
    }
    m_window.setFramerateLimit(60);
    m_window.setKeyRepeatEnabled(true);
    const auto sz = m_window.getSize();
    m_window.setView(computeLetterboxView(sz.x, sz.y));
}

void Game::applyDisplaySettings() {
    if (m_headless) return;
    createWindowFromConfig();
    m_assets.clear();
    m_assets.initializeShaders();
}

void Game::toggleFullscreen() {
    auto& lm = LocalizationManager::getInstance();
    lm.setFullscreen(!lm.isFullscreen());
    lm.saveConfig();
    applyDisplaySettings();
}

sf::View Game::computeLetterboxView(unsigned int width, unsigned int height) const {
    sf::View view(sf::FloatRect(0.0f, 0.0f, LOGICAL_WIDTH, LOGICAL_HEIGHT));
    if (width == 0 || height == 0) return view;

    const float windowRatio = static_cast<float>(width) / static_cast<float>(height);
    const float targetRatio = LOGICAL_WIDTH / LOGICAL_HEIGHT;
    float sizeX = 1.0f;
    float sizeY = 1.0f;
    float posX = 0.0f;
    float posY = 0.0f;

    if (windowRatio > targetRatio) {
        sizeX = targetRatio / windowRatio;
        posX = (1.0f - sizeX) * 0.5f;
    } else if (windowRatio < targetRatio) {
        sizeY = windowRatio / targetRatio;
        posY = (1.0f - sizeY) * 0.5f;
    }
    view.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));
    return view;
}

sf::Vector2f Game::mapPixelToLogical(sf::Vector2i pixel) const {
    if (!m_window.isOpen()) {
        return {static_cast<float>(pixel.x), static_cast<float>(pixel.y)};
    }
    return m_window.mapPixelToCoords(pixel, m_window.getView());
}

sf::Vector2f Game::getMouseLogicalPosition() const {
    if (!m_window.isOpen()) return {-1000.0f, -1000.0f};
    return mapPixelToLogical(sf::Mouse::getPosition(m_window));
}

void Game::triggerFadeIn(float durationSec) {
    m_fadeDuration = std::max(0.05f, durationSec);
    m_fadeAlpha = 1.0f;
}

void Game::pushToast(const std::string& message, sf::Color accent) {
    m_toastMessage = message;
    m_toastAccent = accent;
    m_toastTimer = 2.8f;
}

void Game::run() {
    sf::Clock clock;
    while (m_window.isOpen() && !m_stateManager.isEmpty()) {
        sf::Time deltaTime = clock.restart();
        // 긴 프레임 스파이크(윈도우 드래그 등) 클램핑으로 애니메이션 안정성 확보
        if (deltaTime.asSeconds() > 0.1f) deltaTime = sf::seconds(0.1f);

        processEvents();
        update(deltaTime);
        render();
    }
}

const sf::Font& Game::getFont() const {
    const auto lang = LocalizationManager::getInstance().getLanguage();
    if (lang == Language::JA || lang == Language::ZH_TW || lang == Language::ZH_CN) {
        return m_cjkFont;
    }
    // 고해상도 다크 판타지 UI 가독성을 위해 CJK 폰트가 로드된 경우 선명한 NotoSansCJK를 우선 사용한다.
    if (!m_cjkFont.getInfo().family.empty()) {
        return m_cjkFont;
    }
    return m_font;
}

const sf::Font& Game::getMonoFont() const {
    if (!m_monoFont.getInfo().family.empty()) return m_monoFont;
    return getFont();
}

GameStateManager& Game::getStates() { return m_stateManager; }
Party& Game::getParty() { return m_party; }
AssetManager& Game::getAssets() { return m_assets; }
sf::RenderWindow& Game::getWindow() { return m_window; }

void Game::requestShutdown() {
    if (dynamic_cast<ShutdownState*>(m_stateManager.getCurrentState()) != nullptr) return;
    if (m_party.isRecoveryPending()) {
        m_stateManager.pushState(
            std::make_unique<ShutdownState>(*this, PersistenceStatus::RecoveryPending));
        return;
    }
    if (!m_party.hasActiveSaveSession()) {
        completeShutdown();
        return;
    }
    const auto result = m_party.saveToFile();
    if (result.status == PersistenceStatus::Saved) {
        completeShutdown();
        return;
    }
    m_stateManager.pushState(std::make_unique<ShutdownState>(*this, result.status));
}

void Game::completeShutdown() {
    m_shutdownApproved = true;
    AudioManager::getInstance().shutdown();
    m_window.close();
}

bool Game::isShutdownApproved() const { return m_shutdownApproved; }

void Game::processEvents() {
    sf::Event event{};
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            requestShutdown();
            return;
        }
        if (event.type == sf::Event::Resized) {
            m_window.setView(computeLetterboxView(event.size.width, event.size.height));
            continue;
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F11) {
            toggleFullscreen();
            continue;
        }

        // 마우스 이벤트의 픽셀 좌표를 논리 1280x720 좌표계로 정규화해 전달하지 않고
        // 각 상태가 mapPixelToLogical()을 호출할 수 있도록 원본 이벤트를 그대로 넘긴다.
        if (GameState* currentState = m_stateManager.getCurrentState()) {
            currentState->handleInput(event);
        }
    }
}

void Game::update(sf::Time deltaTime) {
    const float dt = deltaTime.asSeconds();
    if (m_fadeAlpha > 0.0f) {
        m_fadeAlpha = std::max(0.0f, m_fadeAlpha - dt / m_fadeDuration);
    }
    if (m_toastTimer > 0.0f) {
        m_toastTimer = std::max(0.0f, m_toastTimer - dt);
    }

    if (GameState* currentState = m_stateManager.getCurrentState()) {
        currentState->update(deltaTime);
    }
}

void Game::render() {
    m_window.clear(sf::Color(9, 9, 13));
    const auto sz = m_window.getSize();
    m_window.setView(computeLetterboxView(sz.x, sz.y));

    if (GameState* currentState = m_stateManager.getCurrentState()) {
        currentState->draw(m_window);
    }

    // 전역 토스트 알림 오버레이
    if (m_toastTimer > 0.0f && !m_toastMessage.empty()) {
        const float alpha = std::min(1.0f, m_toastTimer * 1.6f);
        sf::Text toastText;
        toastText.setFont(getFont());
        toastText.setCharacterSize(15);
        toastText.setString(sf::String::fromUtf8(m_toastMessage.begin(), m_toastMessage.end()));
        const auto b = toastText.getLocalBounds();
        const float boxW = std::clamp(b.width + 44.0f, 240.0f, 680.0f);
        const float boxH = 38.0f;
        const float bx = (LOGICAL_WIDTH - boxW) * 0.5f;
        const float by = 18.0f;

        sf::RectangleShape bg({boxW, boxH});
        bg.setPosition(bx, by);
        bg.setFillColor(sf::Color(16, 17, 24, static_cast<sf::Uint8>(230.0f * alpha)));
        bg.setOutlineThickness(2.0f);
        bg.setOutlineColor(sf::Color(m_toastAccent.r, m_toastAccent.g, m_toastAccent.b,
                                     static_cast<sf::Uint8>(240.0f * alpha)));
        m_window.draw(bg);

        toastText.setFillColor(sf::Color(242, 236, 224, static_cast<sf::Uint8>(255.0f * alpha)));
        toastText.setPosition(bx + (boxW - b.width) * 0.5f - b.left,
                              by + (boxH - b.height) * 0.5f - b.top - 1.0f);
        m_window.draw(toastText);
    }

    // 부드러운 화면 페이드 인 오버레이
    if (m_fadeAlpha > 0.005f) {
        sf::RectangleShape fade({LOGICAL_WIDTH, LOGICAL_HEIGHT});
        fade.setFillColor(sf::Color(5, 5, 8, static_cast<sf::Uint8>(255.0f * m_fadeAlpha)));
        m_window.draw(fade);
    }

    m_window.display();
}

void Game::loadResources() {
    if (!m_cjkFont.loadFromFile(ResourceLocator::assetPath("fonts/NotoSansCJK-Regular.ttc").string())) {
        std::cerr << "[Critical] Failed to load NotoSansCJK-Regular.ttc" << std::endl;
    }
    if (!m_monoFont.loadFromFile(ResourceLocator::assetPath("fonts/UbuntuMono[wght].ttf").string())) {
        std::cerr << "[Warning] Failed to load UbuntuMono[wght].ttf" << std::endl;
        m_monoFont = m_cjkFont;
    }
    if (!m_font.loadFromFile(ResourceLocator::assetPath("fonts/neodgm.ttf").string())) {
        m_font = m_cjkFont;
    }
}

} // namespace crawl
