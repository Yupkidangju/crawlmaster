// [v0.9.0] SettingsState.cpp 신규 작성
// 설정 TUI 화면을 렌더링하고 키 입력을 수집하여 언어 및 볼륨 설정을 조율, config.json에 반영하는 상태 컨트롤러 클래스 구현.

#include "controller/SettingsState.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include <iostream>

namespace crawl {

SettingsState::SettingsState(Game& game)
    : m_game(game), m_selectedItemIndex(0) {
    // 진입 시점에 현재 설정된 수치들을 재 정렬하기 위해 로컬라이제이션 설정 자동 로딩
    LocalizationManager::getInstance().loadConfig();
}

void SettingsState::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        auto& lm = LocalizationManager::getInstance();
        
        switch (event.key.code) {
            case sf::Keyboard::Escape:
            case sf::Keyboard::O:
                saveAndClose();
                break;

            case sf::Keyboard::Up:
                m_selectedItemIndex = (m_selectedItemIndex + 3) % 4; // 0, 1, 2, 3 순환
                break;

            case sf::Keyboard::Down:
                m_selectedItemIndex = (m_selectedItemIndex + 1) % 4;
                break;

            case sf::Keyboard::Left:
                if (m_selectedItemIndex == 0) { // Language 변경
                    int current = static_cast<int>(lm.getLanguage());
                    int next = (current + 4) % 5; // 이전 언어 (KO->ZH_CN->ZH_TW->JA->EN->KO)
                    lm.setLanguage(static_cast<Language>(next));
                } else if (m_selectedItemIndex == 1) {
                    lm.setTextScale(lm.getTextScale() - 25);
                } else if (m_selectedItemIndex == 2) {
                    lm.setHighContrast(!lm.getHighContrast());
                }
                break;

            case sf::Keyboard::Right:
                if (m_selectedItemIndex == 0) { // Language 변경
                    int current = static_cast<int>(lm.getLanguage());
                    int next = (current + 1) % 5; // 다음 언어
                    lm.setLanguage(static_cast<Language>(next));
                } else if (m_selectedItemIndex == 1) {
                    lm.setTextScale(lm.getTextScale() + 25);
                } else if (m_selectedItemIndex == 2) {
                    lm.setHighContrast(!lm.getHighContrast());
                }
                break;

            case sf::Keyboard::Enter:
                if (m_selectedItemIndex == 2) lm.setHighContrast(!lm.getHighContrast());
                if (m_selectedItemIndex == 3) saveAndClose();
                break;

            default:
                break;
        }
    }
}

void SettingsState::update(sf::Time /*deltaTime*/) {
    // 실시간 비즈니스 애니메이션 없음
}

bool SettingsState::saveAndClose() {
    const auto result = LocalizationManager::getInstance().saveConfig();
    if (!result.durabilityConfirmed()) {
        m_statusMessage = LocalizationManager::getInstance().get("SETTINGS_DURABILITY_UNKNOWN");
        return false;
    }
    if (!result) {
        m_statusMessage = LocalizationManager::getInstance().get("SETTINGS_SAVE_ERROR");
        return false;
    }
    m_game.getStates().popState();
    return true;
}

void SettingsState::draw(sf::RenderWindow& window) {
    auto& lm = LocalizationManager::getInstance();
    sf::Color neonGreen = sf::Color(51, 255, 51);
    sf::Color brightGreen = sf::Color(102, 255, 102);
    sf::Color amber = sf::Color(255, 176, 0);
    sf::Color mutedColor = lm.getHighContrast() ? sf::Color(170, 210, 170) : sf::Color(110, 155, 110);

    // 1. 설정 화면 타이틀
    drawText(window, lm.get("SETTINGS_TITLE"), 512.0f, 60.0f, 24, brightGreen, true);

    // 2. 바깥 메인 박스 (x: 100, y: 120, w: 824, h: 520)
    drawBox(window, 100.0f, 120.0f, 824.0f, 520.0f, neonGreen, 2.0f);

    float startY = 180.0f;
    float spacing = 50.0f;

    // --- 항목 0: Language ---
    {
        std::string label = lm.get("SETTINGS_LANGUAGE") + " : ";
        std::string valStr = "< " + lm.getLanguageName(lm.getLanguage()) + " >";
        std::string textLine = label + valStr;
        sf::Color itemColor = (m_selectedItemIndex == 0) ? brightGreen : mutedColor;
        std::string prefix = (m_selectedItemIndex == 0) ? "> " : "  ";
        drawText(window, prefix + textLine, 150.0f, startY, 16, itemColor);
    }

    // --- 항목 1: Text Scale ---
    {
        std::string textLine = lm.get("SETTINGS_TEXT_SCALE") + " : < " +
                               std::to_string(lm.getTextScale()) + "% >";
        sf::Color itemColor = (m_selectedItemIndex == 1) ? brightGreen : mutedColor;
        std::string prefix = (m_selectedItemIndex == 1) ? "> " : "  ";
        drawText(window, prefix + textLine, 150.0f, startY + spacing, 16, itemColor);
    }

    // --- 항목 2: High Contrast ---
    {
        std::string textLine = lm.get("SETTINGS_HIGH_CONTRAST") + " : < " +
            lm.get(lm.getHighContrast() ? "SETTINGS_ON" : "SETTINGS_OFF") + " >";
        sf::Color itemColor = (m_selectedItemIndex == 2) ? brightGreen : mutedColor;
        std::string prefix = (m_selectedItemIndex == 2) ? "> " : "  ";
        drawText(window, prefix + textLine, 150.0f, startY + spacing * 2.0f, 16, itemColor);
    }

    // --- 항목 3: Back ---
    {
        std::string textLine = lm.get("SETTINGS_BACK");
        sf::Color itemColor = (m_selectedItemIndex == 3) ? brightGreen : mutedColor;
        std::string prefix = (m_selectedItemIndex == 3) ? "> " : "  ";
        drawText(window, prefix + textLine, 150.0f, startY + spacing * 3.5f, 16, itemColor);
    }

    // 3. 하단 조작 키 가이드 보드
    const bool largeText = lm.getTextScale() > 125;
    drawBox(window, 150.0f, startY + spacing * 5.0f, 724.0f, 150.0f, neonGreen, 1.0f);
    drawText(window, lm.get("SETTINGS_KEY_GUIDE"), 512.0f, startY + spacing * 5.0f + 15.0f, 14, amber, true);
    if (largeText) {
        drawText(window, lm.get("SETTINGS_GUIDE_LARGE"), 180.0f,
                 startY + spacing * 5.0f + 48.0f, 14, brightGreen);
    } else {
        drawText(window, lm.get("SETTINGS_GUIDE_MOVE"), 180.0f, startY + spacing * 5.0f + 45.0f, 14, brightGreen);
        drawText(window, lm.get("SETTINGS_GUIDE_MENU"), 180.0f, startY + spacing * 5.0f + 70.0f, 14, brightGreen);
        drawText(window, lm.get("SETTINGS_GUIDE_ESC"), 180.0f, startY + spacing * 5.0f + 95.0f, 14, brightGreen);
        drawText(window, lm.get("SETTINGS_GUIDE_MAP"), 180.0f, startY + spacing * 5.0f + 120.0f, 14, brightGreen);
    }

    // 4. 맨 아래 안내 테두리선
    drawBox(window, 100.0f, 660.0f, 824.0f, 50.0f, neonGreen, 1.0f);
    drawText(window, lm.get(largeText ? "SETTINGS_GUIDE_BAR_LARGE" : "SETTINGS_GUIDE_BAR"),
             512.0f, 685.0f, 14, brightGreen, true);
    if (!m_statusMessage.empty()) {
        drawText(window, m_statusMessage, 512.0f, 730.0f, 14, sf::Color(255, 90, 90), true);
    }
}

void SettingsState::drawBox(sf::RenderWindow& window, float x, float y, float w, float h, sf::Color color, float thickness) {
    sf::RectangleShape box(sf::Vector2f(w, h));
    box.setPosition(x, y);
    box.setFillColor(sf::Color::Transparent);
    box.setOutlineColor(color);
    box.setOutlineThickness(thickness);
    window.draw(box);
}

void SettingsState::drawText(sf::RenderWindow& window, const std::string& str, float x, float y, int size, sf::Color color, bool center) {
    sf::Text text;
    text.setFont(m_game.getFont());

    std::string safeStr = str;
    text.setString(sf::String::fromUtf8(safeStr.begin(), safeStr.end()));

    text.setCharacterSize(LocalizationManager::getInstance().getScaledTextSize(
        static_cast<unsigned int>(std::max(1, size))));
    text.setFillColor(color);

    if (center) {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    }

    text.setPosition(x, y);
    window.draw(text);
}

} // namespace crawl
