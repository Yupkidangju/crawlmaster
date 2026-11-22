#ifndef TITLE_STATE_HPP
#define TITLE_STATE_HPP

#include "core/GameState.hpp"
#include "model/Party.hpp"
#include <array>
#include <string>

namespace crawl {

class Game;

class TitleState : public GameState {
public:
    explicit TitleState(Game& game);
    ~TitleState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    void activateMenu(int index);
    void startConfirmedNewGame();
    void attemptContinue();

    Game& m_game;
    int m_selectedMenuIndex = 0;
    int m_hoveredMenuIndex = -1;
    bool m_confirmingNewGame = false;
    std::string m_statusMessage;
    SavePreviewInfo m_preview;
    float m_time = 0.0f;
    std::array<sf::FloatRect, 4> m_buttonRects{};
    sf::FloatRect m_confirmYesRect{};
    sf::FloatRect m_confirmNoRect{};
};

} // namespace crawl

#endif // TITLE_STATE_HPP
