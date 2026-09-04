#ifndef QUEST_JOURNAL_STATE_HPP
#define QUEST_JOURNAL_STATE_HPP

#include "core/GameState.hpp"

namespace crawl {

class Game;

class QuestJournalState : public GameState {
public:
    explicit QuestJournalState(Game& game);
    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    Game& m_game;
    int m_selected = 0;
    sf::Text m_title;
    sf::Text m_body;
    sf::Text m_guide;

    void refresh();
};

} // namespace crawl

#endif
