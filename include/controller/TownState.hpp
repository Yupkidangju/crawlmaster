#ifndef TOWN_STATE_HPP
#define TOWN_STATE_HPP

#include "core/GameState.hpp"
#include <memory>
#include <string>
#include <vector>

namespace crawl {

class Game;
class Equipment;

enum class TownLocation {
    HUB = 0,
    GUILD = 1,
    SHOP_BUY = 2,
    SHOP_SELL = 3,
    BLACKSMITH = 4,
    CHURCH = 5,
    QUEST_BOARD = 6,
    DUNGEON_GATE = 7
};

class TownState : public GameState {
public:
    explicit TownState(Game& game);
    ~TownState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    struct ClickItem {
        sf::FloatRect rect;
        int actionId = -1;
    };

    struct ForgeEntry {
        std::shared_ptr<Equipment> equip;
        std::string ownerLabel;
    };

    void handleHubAction(int actionId);
    void handleSubAction(int actionId);
    std::vector<ForgeEntry> collectForgeEntries() const;
    void setNotice(const std::string& msg);

    Game& m_game;
    TownLocation m_location = TownLocation::HUB;
    int m_selectedIndex = 0;
    int m_hoveredIndex = -1;
    int m_selectedFloor = 1;
    int m_pendingConfirmIndex = -1;
    bool m_confirmingDismiss = false;
    bool m_confirmingSell = false;
    std::string m_statusMessage;
    float m_time = 0.0f;

    std::vector<ClickItem> m_navButtons;
    std::vector<ClickItem> m_listButtons;
    std::vector<ClickItem> m_actionButtons;
};

} // namespace crawl

#endif // TOWN_STATE_HPP
