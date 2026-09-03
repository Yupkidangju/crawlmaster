#ifndef VICTORY_STATE_HPP
#define VICTORY_STATE_HPP

#include "core/GameState.hpp"

namespace crawl {

class Game;

class VictoryState : public GameState {
public:
    VictoryState(Game& game, bool durabilityUnknown = false);
    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    Game& m_game;
    bool m_durabilityUnknown;
};

} // namespace crawl

#endif // VICTORY_STATE_HPP
