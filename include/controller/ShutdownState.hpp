#ifndef SHUTDOWN_STATE_HPP
#define SHUTDOWN_STATE_HPP

#include "core/GameState.hpp"
#include "core/Persistence.hpp"

namespace crawl {

class Game;

class ShutdownState : public GameState {
public:
    ShutdownState(Game& game, PersistenceStatus status);
    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    Game& m_game;
    PersistenceStatus m_status;
    sf::Text m_title;
    sf::Text m_body;
    sf::Text m_guide;
    void refresh();
};

} // namespace crawl

#endif
