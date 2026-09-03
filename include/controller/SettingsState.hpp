// [v0.9.0] SettingsState.hpp 신규 작성
// 설정 메뉴를 구성하고 렌더링하며, 키 입력을 처리해 다국어 전환 및 가상 볼륨 수치를 조정하는 설정 상태 헤더 정의.

#ifndef SETTINGS_STATE_HPP
#define SETTINGS_STATE_HPP

#include "core/GameState.hpp"
#include <string>

namespace crawl {

class Game;

class SettingsState : public GameState {
public:
    explicit SettingsState(Game& game);
    ~SettingsState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    Game& m_game;

    // 설정 항목 인덱스 (0: Language, 1: Text Scale, 2: High Contrast, 3: Back)
    int m_selectedItemIndex;
    std::string m_statusMessage;

    bool saveAndClose();

    // UI 그리기 보조 유틸리티
    void drawBox(sf::RenderWindow& window, float x, float y, float w, float h, sf::Color color, float thickness = 1.0f);
    void drawText(sf::RenderWindow& window, const std::string& str, float x, float y, int size, sf::Color color, bool center = false);
};

} // namespace crawl

#endif // SETTINGS_STATE_HPP
