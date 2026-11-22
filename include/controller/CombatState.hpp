#ifndef COMBAT_STATE_HPP
#define COMBAT_STATE_HPP

#include "core/GameState.hpp"
#include "model/Character.hpp"
#include "model/Monster.hpp"
#include "model/MonsterFactory.hpp"
#include "view/DungeonRenderer.hpp"
#include "view/UiSystem.hpp"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace crawl {

class Game;

struct EncounterSpec {
    EncounterTier tier = EncounterTier::EARLY;
    std::string fixedMonsterId;
    std::string questId;
    std::string worldObjectId;
    bool bossBattle = false;
    bool campaignFinal = false;
    int floorNumber = 1;
};

struct TurnEntity {
    bool isMonster = false;
    int index = 0;
    int initiative = 0;
};

class CombatState : public GameState {
public:
    CombatState(Game& game, EncounterTier tier, bool bossBattle = false);
    CombatState(Game& game, EncounterSpec encounter);
    ~CombatState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    enum class SubMenu {
        NONE = 0,
        SKILL_LIST = 1,
        SKILL_ALLY_TARGET = 2,
        ITEM_LIST = 3,
        ITEM_TARGET = 4
    };

    void spawnMonsters();
    void rollInitiatives();
    void startCurrentTurn();
    void nextTurn();
    void selectFirstLivingFoe();
    void addLog(const std::string& message);

    void executeBasicAttack(Character& actor);
    void executeGuard(Character& actor);
    void executeRowSwap(Character& actor);
    void executeSelectedSkill(int skillIndex);
    void executePendingAllySkill();
    void executeSelectedItem(int invIdx);
    void attemptFlee();
    void executeMonsterTurn(Monster& monster, int monsterIndex);

    void checkCombatResolution();
    void handleVictory();
    void handleDefeat();

    std::vector<int> getUsableConsumableIndices() const;
    sf::Vector2f monsterScreenCenter(std::size_t idx) const;
    sf::Vector2f partyCardCenter(std::size_t idx) const;

    Game& m_game;
    EncounterSpec m_encounter;
    std::unique_ptr<DungeonRenderer> m_backdropRenderer;
    CombatVfxSystem m_vfx;

    std::vector<std::shared_ptr<Monster>> m_foes;
    std::vector<TurnEntity> m_turnOrder;
    int m_currentTurnIdx = 0;
    int m_selectedTargetIdx = 0;
    int m_selectedAllyIdx = 0;
    bool m_rowSwappedThisTurn = false;

    SubMenu m_subMenu = SubMenu::NONE;
    int m_pendingSkillIndex = -1;
    int m_pendingItemInvIndex = -1;
    int m_subSelection = 0;

    bool m_combatEnded = false;
    bool m_victoryResult = false;
    float m_monsterActionDelay = 0.0f;
    float m_time = 0.0f;
    float m_hitStopTimer = 0.0f;

    // 시각 애니메이션 추적 (돌진, 피격 플래시, 지연 HP 잔상)
    std::vector<float> m_foeHitFlash;
    std::vector<float> m_foeLunge;
    std::vector<float> m_foeHpTrail;
    std::array<float, 4> m_partyHpTrail{1.0f, 1.0f, 1.0f, 1.0f};

    std::vector<std::string> m_combatLog;

    // 마우스 히트박스
    std::vector<sf::FloatRect> m_foeRects;
    std::array<sf::FloatRect, 4> m_partyRects{};
    std::array<sf::FloatRect, 6> m_cmdRects{}; // Attack, Skill, Guard, Row, Item, Flee
    std::vector<sf::FloatRect> m_subItemRects;
};

} // namespace crawl

#endif // COMBAT_STATE_HPP
