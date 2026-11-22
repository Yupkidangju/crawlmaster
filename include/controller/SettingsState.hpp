#ifndef SETTINGS_STATE_HPP
#define SETTINGS_STATE_HPP

#include "core/GameState.hpp"
#include <array>
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
    static constexpr int ROW_COUNT = 10;

    void adjustSetting(int row, int delta);
    void saveAndApply(bool recreateWindow);

    Game& m_game;
    int m_selectedRow = 0;
    std::string m_statusMessage;
    std::array<sf::FloatRect, ROW_COUNT> m_rowRects{};
    std::array<sf::FloatRect, ROW_COUNT> m_minusRects{};
    std::array<sf::FloatRect, ROW_COUNT> m_plusRects{};
    sf::FloatRect m_closeRect{};
};

} // namespace crawl

#endif // SETTINGS_STATE_HPP
