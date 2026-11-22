#ifndef GAME_OVER_STATE_HPP
#define GAME_OVER_STATE_HPP

#include "core/GameState.hpp"
#include <array>
#include <string>

namespace crawl {

class Game;

class GameOverState : public GameState {
public:
    explicit GameOverState(Game& game, bool checkpointRecovered = true, std::string statusMessage = {});
    ~GameOverState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    void activate(int idx);

    Game& m_game;
    bool m_checkpointRecovered = true;
    std::string m_statusMessage;
    int m_selectedIndex = 0;
    std::array<sf::FloatRect, 2> m_buttonRects{};
};

} // namespace crawl

#endif // GAME_OVER_STATE_HPP
