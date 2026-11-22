#include "controller/TitleState.hpp"
#include "controller/SettingsState.hpp"
#include "controller/TownState.hpp"
#include "controller/VictoryState.hpp"
#include "core/AudioManager.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "view/UiSystem.hpp"
#include "view/UiTheme.hpp"

#include <cmath>

namespace crawl {

TitleState::TitleState(Game& game) : m_game(game) {
    m_preview = Party::inspectSavePreview();
    AudioManager::getInstance().stopAmbience();
    AudioManager::getInstance().playBgm("bgm_title");
}

void TitleState::startConfirmedNewGame() {
    const auto& lm = LocalizationManager::getInstance();
    const PartyCheckpoint cp = m_game.getParty().captureCheckpoint();
    SessionRng::startNewGlobalSession();
    const auto res = m_game.getParty().startNewGame();
    if (!res.durabilityConfirmed()) {
        m_statusMessage = lm.get("TITLE_DURABILITY_UNKNOWN");
        AudioManager::getInstance().playSfx("sfx_ui_error");
        return;
    }
    if (!res) {
        m_game.getParty().restoreCheckpoint(cp);
        m_statusMessage = lm.get("TITLE_SAVE_ERROR");
        AudioManager::getInstance().playSfx("sfx_ui_error");
        return;
    }
    AudioManager::getInstance().playSfx("sfx_ui_click");
    m_game.triggerFadeIn(0.28f);
    m_game.getStates().replaceAll(std::make_unique<TownState>(m_game));
}

void TitleState::attemptContinue() {
    const auto& lm = LocalizationManager::getInstance();
    const bool recoveryContext = m_game.getParty().hasActiveSaveSession() || Party::hasRecoverableSave();
    const auto res = m_game.getParty().loadFromFile();
    if (res) {
        const auto savedSeed = m_game.getParty().getLastSessionSeed();
        if (savedSeed != 0U) {
            SessionRng::global() = SessionRng(savedSeed, m_game.getParty().getSessionRngDrawCount());
        } else {
            SessionRng::startNewGlobalSession();
        }
        AudioManager::getInstance().playSfx("sfx_ui_click");
        m_game.triggerFadeIn(0.28f);
        if (m_game.getParty().isCampaignCompleted()) {
            m_game.getStates().replaceAll(std::make_unique<VictoryState>(m_game));
        } else {
            m_game.getStates().replaceAll(std::make_unique<TownState>(m_game));
        }
        return;
    }

    AudioManager::getInstance().playSfx("sfx_ui_error");
    if (res.status == PersistenceStatus::UnsupportedVersion) {
        m_statusMessage = lm.get("TITLE_LEGACY_QUARANTINED");
        m_preview = Party::inspectSavePreview();
        return;
    }
    if (recoveryContext) m_game.getParty().markRecoveryPending();
    m_statusMessage = lm.get(res.status == PersistenceStatus::NotFound
                                 ? "TITLE_NO_SAVE" : "TITLE_CORRUPT_SAVE");
}

void TitleState::activateMenu(int index) {
    if (index == 0) {
        if (m_preview.valid) {
            m_confirmingNewGame = true;
            m_statusMessage = LocalizationManager::getInstance().get("TITLE_NEW_CONFIRM");
            AudioManager::getInstance().playSfx("sfx_ui_click");
        } else {
            startConfirmedNewGame();
        }
    } else if (index == 1) {
        attemptContinue();
    } else if (index == 2) {
        AudioManager::getInstance().playSfx("sfx_ui_click");
        m_game.getStates().pushState(std::make_unique<SettingsState>(m_game));
    } else if (index == 3) {
        AudioManager::getInstance().playSfx("sfx_ui_click");
        m_game.completeShutdown();
    }
}

void TitleState::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::MouseMoved) {
        const auto pos = m_game.mapPixelToLogical({event.mouseMove.x, event.mouseMove.y});
        int prevHover = m_hoveredMenuIndex;
        m_hoveredMenuIndex = -1;
        for (int i = 0; i < 4; ++i) {
            if (m_buttonRects[static_cast<std::size_t>(i)].contains(pos)) {
                m_hoveredMenuIndex = i;
                m_selectedMenuIndex = i;
            }
        }
        if (m_hoveredMenuIndex != -1 && m_hoveredMenuIndex != prevHover) {
            AudioManager::getInstance().playSfx("sfx_ui_hover");
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const auto pos = m_game.mapPixelToLogical({event.mouseButton.x, event.mouseButton.y});
        if (m_confirmingNewGame) {
            if (m_confirmYesRect.contains(pos)) {
                startConfirmedNewGame();
            } else if (m_confirmNoRect.contains(pos)) {
                m_confirmingNewGame = false;
                m_statusMessage.clear();
                AudioManager::getInstance().playSfx("sfx_ui_click");
            }
            return;
        }
        for (int i = 0; i < 4; ++i) {
            if (m_buttonRects[static_cast<std::size_t>(i)].contains(pos)) {
                m_selectedMenuIndex = i;
                activateMenu(i);
                return;
            }
        }
        return;
    }

    if (event.type != sf::Event::KeyPressed) return;
    const auto key = event.key.code;

    if (m_confirmingNewGame) {
        if (key == sf::Keyboard::Enter || key == sf::Keyboard::Y) {
            startConfirmedNewGame();
        } else if (key == sf::Keyboard::Escape || key == sf::Keyboard::N) {
            m_confirmingNewGame = false;
            m_statusMessage.clear();
            AudioManager::getInstance().playSfx("sfx_ui_click");
        }
        return;
    }

    if (key == sf::Keyboard::Up || key == sf::Keyboard::W) {
        m_selectedMenuIndex = (m_selectedMenuIndex + 3) % 4;
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (key == sf::Keyboard::Down || key == sf::Keyboard::S) {
        m_selectedMenuIndex = (m_selectedMenuIndex + 1) % 4;
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (key == sf::Keyboard::O) {
        activateMenu(2);
    } else if (key == sf::Keyboard::Enter || key == sf::Keyboard::Space) {
        activateMenu(m_selectedMenuIndex);
    }
}

void TitleState::update(sf::Time deltaTime) {
    m_time += deltaTime.asSeconds();
    m_preview = Party::inspectSavePreview();
}

void TitleState::draw(sf::RenderWindow& window) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    const auto& lm = LocalizationManager::getInstance();
    const auto mouse = m_game.getMouseLogicalPosition();

    // 1. 풀스크린 다크 판타지 키아트 배경 + 부유 불씨 파티클
    UiSystem::drawBackdrop(window, assets, "title_bg", 72);

    if (const sf::Texture* glow = assets.getUiTexture("particle_glow")) {
        const auto gsz = glow->getSize();
        for (int i = 0; i < 24; ++i) {
            const float phase = static_cast<float>(i) * 1.37f;
            const float px = std::fmod(static_cast<float>(i * 73) + std::sin(m_time * 0.7f + phase) * 28.0f + 1280.0f, 1280.0f);
            const float py = 740.0f - std::fmod(m_time * (28.0f + static_cast<float>(i % 5) * 8.0f) + static_cast<float>(i * 41), 760.0f);
            const float pulse = 0.45f + 0.55f * std::sin(m_time * 3.0f + phase);
            sf::Sprite sp(*glow);
            sp.setOrigin(static_cast<float>(gsz.x) * 0.5f, static_cast<float>(gsz.y) * 0.5f);
            const float sc = (10.0f + static_cast<float>(i % 4) * 4.0f) / static_cast<float>(gsz.x);
            sp.setScale(sc, sc);
            sp.setPosition(px, py);
            sp.setColor(sf::Color(255, 162, 62, static_cast<sf::Uint8>(155.0f * pulse)));
            window.draw(sp, sf::BlendAdd);
        }
    }

    // 2. 장엄한 타이틀 문장 헤더
    const float bannerW = 680.0f;
    const float bannerX = (Game::LOGICAL_WIDTH - bannerW) * 0.5f;
    UiSystem::drawPanel(window, assets, {bannerX, 54.0f, bannerW, 134.0f}, true, 212);
    UiSystem::drawCenteredText(window, font, "C R A W L M A S T E R",
                               {bannerX, 70.0f, bannerW, 56.0f}, 42, UiTheme::textGold(), true);
    UiSystem::drawCenteredText(window, font, lm.get("TITLE_SUBTITLE"),
                               {bannerX, 132.0f, bannerW, 34.0f}, 16, UiTheme::textPrimary(), true);

    // 3. 중앙 메뉴 카드 + 세이브 미리보기
    const float menuW = 460.0f;
    const float menuX = (Game::LOGICAL_WIDTH - menuW) * 0.5f;
    const float menuY = 330.0f;
    UiSystem::drawPanel(window, assets, {menuX - 24.0f, menuY - 22.0f, menuW + 48.0f, 276.0f}, false, 224);

    std::string contLabel = lm.get("TITLE_CONTINUE");
    if (m_preview.valid) {
        contLabel += "  (" + std::to_string(m_preview.deepestFloor) + "F · " +
                     std::to_string(m_preview.memberCount) + "P · " +
                     std::to_string(m_preview.gold) + "G)";
    }

    const std::array<std::pair<std::string, std::string>, 4> items = {{
        {lm.get("TITLE_NEW_GAME"), "1"},
        {contLabel, "2"},
        {lm.get("TITLE_SETTINGS"), "O"},
        {lm.get("TITLE_EXIT"), "Esc"}
    }};

    for (int i = 0; i < 4; ++i) {
        const sf::FloatRect r(menuX, menuY + static_cast<float>(i) * 58.0f, menuW, 48.0f);
        m_buttonRects[static_cast<std::size_t>(i)] = r;
        UiSystem::drawButton(window, assets, font, {
            r, items[static_cast<std::size_t>(i)].first, items[static_cast<std::size_t>(i)].second,
            m_selectedMenuIndex == i, r.contains(mouse), true, i == 0
        });
    }

    // 4. 상태/경고 메시지 또는 구버전 세이브 격리 안내
    if (!m_statusMessage.empty()) {
        UiSystem::drawPanel(window, assets, {260.0f, 616.0f, 760.0f, 44.0f}, true, 236);
        UiSystem::drawCenteredText(window, font, m_statusMessage,
                                   {272.0f, 616.0f, 736.0f, 44.0f}, 14, UiTheme::textGold(), true);
    }

    // 5. 하단 버전 및 조작 힌트
    UiSystem::drawText(window, font, "Crawlmaster v1.0.0 AAA Edition  ·  5-Floor Dark Fantasy Tactical RPG",
                       {22.0f, 686.0f}, 13, UiTheme::textMuted(), true);
    UiSystem::drawText(window, font, lm.get("TITLE_PRESS_KEY"),
                       {820.0f, 686.0f}, 13, UiTheme::textMuted(), true, 440.0f);

    // 6. 새 게임 덮어쓰기 모달 다이얼로그
    if (m_confirmingNewGame) {
        sf::RectangleShape dim({Game::LOGICAL_WIDTH, Game::LOGICAL_HEIGHT});
        dim.setFillColor(sf::Color(0, 0, 0, 170));
        window.draw(dim);

        const sf::FloatRect modal(360.0f, 250.0f, 560.0f, 210.0f);
        UiSystem::drawPanel(window, assets, modal, true, 250);
        UiSystem::drawCenteredText(window, font, lm.get("TITLE_NEW_GAME"),
                                   {modal.left, modal.top + 18.0f, modal.width, 32.0f}, 22, UiTheme::textGold(), true);
        UiSystem::drawCenteredText(window, font, lm.get("TITLE_NEW_CONFIRM"),
                                   {modal.left + 24.0f, modal.top + 66.0f, modal.width - 48.0f, 54.0f},
                                   15, UiTheme::textPrimary(), true);
        m_confirmYesRect = {modal.left + 58.0f, modal.top + 138.0f, 200.0f, 46.0f};
        m_confirmNoRect = {modal.left + 302.0f, modal.top + 138.0f, 200.0f, 46.0f};
        UiSystem::drawButton(window, assets, font, {
            m_confirmYesRect, lm.get("COMMON_CONFIRM"), "Enter", true, m_confirmYesRect.contains(mouse), true, true
        });
        UiSystem::drawButton(window, assets, font, {
            m_confirmNoRect, lm.get("COMMON_CANCEL"), "Esc", false, m_confirmNoRect.contains(mouse), true, false
        });
    }
}

} // namespace crawl
