#ifndef SHUTDOWN_STATE_HPP
#define SHUTDOWN_STATE_HPP

#include "core/GameState.hpp"
#include "core/Persistence.hpp"
#include <array>
#include <string>

namespace crawl {

class Game;

class ShutdownState : public GameState {
public:
    ShutdownState(Game& game, PersistenceStatus initialFailure);
    ~ShutdownState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    void retryPrimary();

    Game& m_game;
    PersistenceStatus m_failureStatus;
    std::string m_statusMessage;
    std::array<sf::FloatRect, 2> m_buttonRects{};
};

} // namespace crawl

#endif // SHUTDOWN_STATE_HPP
