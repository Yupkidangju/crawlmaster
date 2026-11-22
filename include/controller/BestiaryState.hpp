#ifndef BESTIARY_STATE_HPP
#define BESTIARY_STATE_HPP

#include "core/GameState.hpp"
#include <string>
#include <vector>

namespace crawl {

class Game;

class BestiaryState : public GameState {
public:
    explicit BestiaryState(Game& game);
    ~BestiaryState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    Game& m_game;
    std::vector<std::string> m_monsterIds;
    int m_selectedIndex = 0;
    int m_scrollOffset = 0;
    std::vector<sf::FloatRect> m_rowRects;
    sf::FloatRect m_closeRect{};
};

} // namespace crawl

#endif // BESTIARY_STATE_HPP
