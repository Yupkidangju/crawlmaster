#include "controller/SettingsState.hpp"
#include "core/AudioManager.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "view/UiSystem.hpp"
#include "view/UiTheme.hpp"

#include <array>

namespace crawl {
namespace {

constexpr std::array<std::pair<int, int>, 3> kResPresets = {{{1280, 720}, {1600, 900}, {1920, 1080}}};
constexpr std::array<int, 5> kScalePresets = {75, 100, 125, 150, 200};

} // namespace

SettingsState::SettingsState(Game& game) : m_game(game) {}

void SettingsState::saveAndApply(bool recreateWindow) {
    auto& lm = LocalizationManager::getInstance();
    AudioManager::getInstance().applyVolumeConfig(
        lm.getMasterVolume(), lm.getBgmVolume(), lm.getSfxVolume(), lm.isMuted());
    const auto res = lm.saveConfig();
    if (!res) {
        m_statusMessage = lm.get("SETTINGS_SAVE_ERROR");
    } else {
        m_statusMessage = lm.get("SETTINGS_SAVED");
    }
    if (recreateWindow) {
        m_game.applyDisplaySettings();
    }
}

void SettingsState::adjustSetting(int row, int delta) {
    auto& lm = LocalizationManager::getInstance();
    bool needWindow = false;
    switch (row) {
        case 0: { // Language
            const int next = (static_cast<int>(lm.getLanguage()) + delta + 5) % 5;
            lm.setLanguage(static_cast<Language>(next));
            break;
        }
        case 1: { // Text scale
            int idx = 1;
            for (int i = 0; i < 5; ++i) {
                if (kScalePresets[static_cast<std::size_t>(i)] == lm.getTextScale()) idx = i;
            }
            idx = std::clamp(idx + delta, 0, 4);
            lm.setTextScale(kScalePresets[static_cast<std::size_t>(idx)]);
            break;
        }
        case 2: // High contrast
            lm.setHighContrast(!lm.getHighContrast());
            break;
        case 3: { // Resolution
            int idx = 0;
            for (int i = 0; i < 3; ++i) {
                if (kResPresets[static_cast<std::size_t>(i)].first == lm.getWindowWidth()) idx = i;
            }
            idx = (idx + delta + 3) % 3;
            lm.setResolution(kResPresets[static_cast<std::size_t>(idx)].first,
                             kResPresets[static_cast<std::size_t>(idx)].second);
            needWindow = true;
            break;
        }
        case 4: // Fullscreen
            lm.setFullscreen(!lm.isFullscreen());
            needWindow = true;
            break;
        case 5: // Graphics Quality
            lm.setGraphicsQuality(lm.getGraphicsQuality() == 0 ? 1 : 0);
            break;
        case 6: // Master Vol
            lm.setMasterVolume(lm.getMasterVolume() + delta * 10);
            break;
        case 7: // BGM Vol
            lm.setBgmVolume(lm.getBgmVolume() + delta * 10);
            break;
        case 8: // SFX Vol
            lm.setSfxVolume(lm.getSfxVolume() + delta * 10);
            break;
        case 9: // Mute
            lm.setMuted(!lm.isMuted());
            break;
        default:
            break;
    }
    saveAndApply(needWindow);
    AudioManager::getInstance().playSfx("sfx_ui_click");
}

void SettingsState::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const auto pos = m_game.mapPixelToLogical({event.mouseButton.x, event.mouseButton.y});
        if (m_closeRect.contains(pos)) {
            AudioManager::getInstance().playSfx("sfx_ui_click");
            m_game.getStates().popState();
            return;
        }
        for (int i = 0; i < ROW_COUNT; ++i) {
            if (m_minusRects[static_cast<std::size_t>(i)].contains(pos)) {
                m_selectedRow = i;
                adjustSetting(i, -1);
                return;
            }
            if (m_plusRects[static_cast<std::size_t>(i)].contains(pos)) {
                m_selectedRow = i;
                adjustSetting(i, 1);
                return;
            }
            if (m_rowRects[static_cast<std::size_t>(i)].contains(pos)) {
                m_selectedRow = i;
                adjustSetting(i, 1);
                return;
            }
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
        AudioManager::getInstance().playSfx("sfx_ui_click");
        m_game.getStates().popState();
        return;
    }

    if (event.type != sf::Event::KeyPressed) return;
    const auto key = event.key.code;
    if (key == sf::Keyboard::Escape || key == sf::Keyboard::O) {
        AudioManager::getInstance().playSfx("sfx_ui_click");
        m_game.getStates().popState();
    } else if (key == sf::Keyboard::Up || key == sf::Keyboard::W) {
        m_selectedRow = (m_selectedRow + ROW_COUNT - 1) % ROW_COUNT;
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (key == sf::Keyboard::Down || key == sf::Keyboard::S) {
        m_selectedRow = (m_selectedRow + 1) % ROW_COUNT;
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (key == sf::Keyboard::Left || key == sf::Keyboard::A) {
        adjustSetting(m_selectedRow, -1);
    } else if (key == sf::Keyboard::Right || key == sf::Keyboard::D || key == sf::Keyboard::Enter) {
        adjustSetting(m_selectedRow, 1);
    }
}

void SettingsState::update(sf::Time /*deltaTime*/) {}

void SettingsState::draw(sf::RenderWindow& window) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    const auto& lm = LocalizationManager::getInstance();
    const auto mouse = m_game.getMouseLogicalPosition();

    sf::RectangleShape dim({Game::LOGICAL_WIDTH, Game::LOGICAL_HEIGHT});
    dim.setFillColor(sf::Color(5, 6, 10, 200));
    window.draw(dim);

    const sf::FloatRect panel(180.0f, 44.0f, 920.0f, 632.0f);
    UiSystem::drawPanel(window, assets, panel, true, 246);
    UiSystem::drawCenteredText(window, font, lm.get("SETTINGS_TITLE"),
                               {panel.left, panel.top + 14.0f, panel.width, 36.0f}, 24, UiTheme::textGold(), true);

    const std::array<std::pair<std::string, std::string>, ROW_COUNT> rows = {{
        {lm.get("SETTINGS_LANGUAGE"), lm.getLanguageName(lm.getLanguage())},
        {lm.get("SETTINGS_TEXT_SCALE"), std::to_string(lm.getTextScale()) + "%"},
        {lm.get("SETTINGS_HIGH_CONTRAST"), lm.getHighContrast() ? lm.get("COMMON_ON") : lm.get("COMMON_OFF")},
        {lm.get("SETTINGS_RESOLUTION"), std::to_string(lm.getWindowWidth()) + " x " + std::to_string(lm.getWindowHeight())},
        {lm.get("SETTINGS_FULLSCREEN"), lm.isFullscreen() ? lm.get("COMMON_ON") : lm.get("COMMON_OFF")},
        {lm.get("SETTINGS_GRAPHICS"), lm.getGraphicsQuality() > 0 ? lm.get("SETTINGS_QUALITY_HIGH") : lm.get("SETTINGS_QUALITY_MED")},
        {lm.get("SETTINGS_MASTER_VOL"), std::to_string(lm.getMasterVolume()) + "%"},
        {lm.get("SETTINGS_BGM_VOL"), std::to_string(lm.getBgmVolume()) + "%"},
        {lm.get("SETTINGS_SFX_VOL"), std::to_string(lm.getSfxVolume()) + "%"},
        {lm.get("SETTINGS_MUTE"), lm.isMuted() ? lm.get("COMMON_ON") : lm.get("COMMON_OFF")}
    }};

    for (int i = 0; i < ROW_COUNT; ++i) {
        const float y = panel.top + 66.0f + static_cast<float>(i) * 48.0f;
        const sf::FloatRect rr(panel.left + 36.0f, y, panel.width - 72.0f, 40.0f);
        m_rowRects[static_cast<std::size_t>(i)] = rr;

        const bool sel = (m_selectedRow == i) || rr.contains(mouse);
        sf::RectangleShape bg({rr.width, rr.height});
        bg.setPosition(rr.left, rr.top);
        bg.setFillColor(sel ? UiTheme::panelHover() : UiTheme::panelAlt());
        bg.setOutlineThickness(sel ? 1.5f : 1.0f);
        bg.setOutlineColor(sel ? UiTheme::borderGold() : UiTheme::borderBronze());
        window.draw(bg);

        const unsigned int rowSz = std::min(19U, lm.getScaledTextSize(14));
        UiSystem::drawText(window, font, rows[static_cast<std::size_t>(i)].first,
                           {rr.left + 16.0f, rr.top + (rr.height - static_cast<float>(rowSz)) * 0.5f - 1.0f}, rowSz,
                           sel ? UiTheme::textGold() : UiTheme::textPrimary(), true, 360.0f);

        m_minusRects[static_cast<std::size_t>(i)] = {rr.left + rr.width - 376.0f, rr.top + 4.0f, 38.0f, 32.0f};
        m_plusRects[static_cast<std::size_t>(i)] = {rr.left + rr.width - 48.0f, rr.top + 4.0f, 38.0f, 32.0f};

        UiSystem::drawButton(window, assets, font, {
            m_minusRects[static_cast<std::size_t>(i)], "<", "", false,
            m_minusRects[static_cast<std::size_t>(i)].contains(mouse), true, false
        });
        UiSystem::drawCenteredText(window, font, rows[static_cast<std::size_t>(i)].second,
                                   {rr.left + rr.width - 332.0f, rr.top + 4.0f, 278.0f, 32.0f},
                                   rowSz, UiTheme::textPrimary(), true);
        UiSystem::drawButton(window, assets, font, {
            m_plusRects[static_cast<std::size_t>(i)], ">", "", false,
            m_plusRects[static_cast<std::size_t>(i)].contains(mouse), true, false
        });
    }

    if (!m_statusMessage.empty()) {
        UiSystem::drawText(window, font, m_statusMessage,
                           {panel.left + 38.0f, panel.top + panel.height - 54.0f}, 14, UiTheme::statPos(), true, 540.0f);
    }

    m_closeRect = {panel.left + panel.width - 220.0f, panel.top + panel.height - 62.0f, 184.0f, 44.0f};
    UiSystem::drawButton(window, assets, font, {
        m_closeRect, lm.get("COMMON_BACK"), "Esc", false, m_closeRect.contains(mouse), true, true
    });
}

} // namespace crawl
