#ifndef QUEST_JOURNAL_STATE_HPP
#define QUEST_JOURNAL_STATE_HPP

#include "core/GameState.hpp"
#include <vector>

namespace crawl {

class Game;

class QuestJournalState : public GameState {
public:
    explicit QuestJournalState(Game& game);
    ~QuestJournalState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    Game& m_game;
    int m_selectedIndex = 0;
    std::vector<sf::FloatRect> m_rowRects;
    sf::FloatRect m_closeRect{};
};

} // namespace crawl

#endif // QUEST_JOURNAL_STATE_HPP
