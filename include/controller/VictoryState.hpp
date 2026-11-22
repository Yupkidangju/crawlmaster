#ifndef VICTORY_STATE_HPP
#define VICTORY_STATE_HPP

#include "core/GameState.hpp"
#include <array>

namespace crawl {

class Game;

class VictoryState : public GameState {
public:
    explicit VictoryState(Game& game);
    ~VictoryState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    void activate(int idx);

    Game& m_game;
    int m_selectedIndex = 0;
    float m_time = 0.0f;
    std::array<sf::FloatRect, 2> m_buttonRects{};
};

} // namespace crawl

#endif // VICTORY_STATE_HPP
