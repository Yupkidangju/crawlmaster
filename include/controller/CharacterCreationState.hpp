#ifndef CHARACTER_CREATION_STATE_HPP
#define CHARACTER_CREATION_STATE_HPP

#include "core/GameState.hpp"
#include "model/RecruitmentDraft.hpp"
#include <array>
#include <string>
#include <vector>

namespace crawl {

class Game;

class CharacterCreationState : public GameState {
public:
    explicit CharacterCreationState(Game& game);
    ~CharacterCreationState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    enum class FocusRow {
        NAME = 0, AGE = 1, GENDER = 2, CLASS = 3,
        STR = 4, DEX = 5, CON = 6, INT = 7, WIS = 8, CHA = 9,
        REROLL = 10, CONFIRM = 11, CANCEL = 12
    };

    void adjustFocused(int delta);
    void attemptConfirm();

    Game& m_game;
    RecruitmentDraft m_draft;
    std::string m_nameBuffer;
    int m_focusIndex = 0;
    bool m_confirmingSave = false;
    std::string m_statusMessage;

    std::array<sf::FloatRect, 4> m_classRects{};
    std::array<sf::FloatRect, 6> m_statMinusRects{};
    std::array<sf::FloatRect, 6> m_statPlusRects{};
    sf::FloatRect m_ageMinusRect{};
    sf::FloatRect m_agePlusRect{};
    sf::FloatRect m_genderRect{};
    sf::FloatRect m_rerollRect{};
    sf::FloatRect m_confirmRect{};
    sf::FloatRect m_cancelRect{};
};

} // namespace crawl

#endif // CHARACTER_CREATION_STATE_HPP
