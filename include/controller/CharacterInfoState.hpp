#ifndef CHARACTER_INFO_STATE_HPP
#define CHARACTER_INFO_STATE_HPP

#include "core/GameState.hpp"
#include "model/Equipment.hpp"
#include <array>
#include <string>
#include <vector>

namespace crawl {

class Game;

class CharacterInfoState : public GameState {
public:
    explicit CharacterInfoState(Game& game, int initialMemberIndex = 0);
    ~CharacterInfoState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    enum class RightTab { INVENTORY = 0, TALENTS = 1, SKILLS = 2 };

    void equipOrUseInventoryItem(int invIndex);
    void unequipSlot(EquipSlot slot);

    Game& m_game;
    int m_selectedMemberIdx = 0;
    int m_selectedInvIdx = 0;
    RightTab m_rightTab = RightTab::INVENTORY;
    std::string m_statusMessage;

    std::array<sf::FloatRect, 4> m_memberTabRects{};
    std::array<sf::FloatRect, 4> m_equipSlotRects{};
    std::array<sf::FloatRect, 3> m_rightTabRects{};
    std::array<sf::FloatRect, 4> m_talentRects{};
    std::array<sf::FloatRect, 2> m_specRects{};
    sf::FloatRect m_rowToggleRect{};
    sf::FloatRect m_closeRect{};
    std::vector<sf::FloatRect> m_invRowRects;
};

} // namespace crawl

#endif // CHARACTER_INFO_STATE_HPP
