#ifndef GAME_OVER_STATE_HPP
#define GAME_OVER_STATE_HPP

#include "core/GameState.hpp"

namespace crawl {

class Game;

class GameOverState : public GameState {
public:
    GameOverState(Game& game, bool checkpointRestored = true);

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    Game& m_game;
    bool m_checkpointRestored;
};

} // namespace crawl

#endif // GAME_OVER_STATE_HPP
