// [v0.4.0] CombatState.hpp 신규 작성
// 턴 순서(Initiative) 산정, d20 주사위 명중/피해 전투 룰러, 클래스 마법 주문 슬롯 제어 및 전멸 시 하드코어 리셋을 다루는 전투 상태 헤더 선언.

#ifndef COMBAT_STATE_HPP
#define COMBAT_STATE_HPP

#include "core/GameState.hpp"
#include "model/Monster.hpp"
#include "model/MonsterFactory.hpp"
#include <vector>
#include <string>
#include <variant>
#include <map>

namespace crawl {

class Game; // 전방 선언

struct EncounterSpec {
    EncounterTier tier = EncounterTier::EARLY;
    std::string fixedMonsterId;
    std::string questId;
    std::string worldObjectId;
    bool bossBattle = false;
    bool campaignFinal = false;
};

// 전투 턴 참여 엔티티 구조체 (캐릭터 또는 몬스터)
struct TurnEntity {
    bool isMonster;                 // 몬스터 여부 플래그
    int index;                      // 멤버 배열 혹은 몬스터 배열에서의 인덱스
    int initiativeRoll;             // 선제권 주사위 결과값
};

// CombatState 클래스: 턴제 전투 루프 및 배틀 TUI UI 핸들링
class CombatState : public GameState {
public:
    explicit CombatState(Game& game, EncounterTier tier = EncounterTier::EARLY,
                         bool bossBattle = false);
    CombatState(Game& game, EncounterSpec encounter);
    ~CombatState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    friend class ControllerTestAccess;
    Game& m_game;                                   // 전역 Game 참조
    std::vector<std::shared_ptr<Monster>> m_foes;   // 대적할 몬스터 리스트 (1~3마리)
    std::vector<TurnEntity> m_turnOrder;            // 우선권 순으로 정렬된 턴 트래커
    int m_currentTurnIdx;                           // 현재 차례의 turnOrder 인덱스

    std::vector<std::string> m_battleLog;           // 하단에 누적 표출될 전투 텍스트 로그
    int m_selectedTargetIdx;                        // 플레이어가 타겟팅한 몬스터 인덱스
    EncounterTier m_encounterTier;
    bool m_isBossBattle;
    bool m_isCampaignFinal = false;
    std::string m_questId;
    std::string m_worldObjectId;
    bool m_hasStarted = false;
    bool m_victoryDurabilityUnknown = false;

    // [v0.8.0] 스킬/주문 선택 서브 상태 변수
    bool m_isSelectingSkill = false;
    bool m_isSelectingItem = false;
    bool m_isSelectingItemTarget = false;
    bool m_isConfirmingItem = false;
    bool m_isSelectingSkillTarget = false;
    bool m_isConfirmingSkillTarget = false;
    int m_selectedInventoryIndex = -1;
    int m_selectedAllyIndex = 0;
    int m_pendingSkillIndex = -1;
    std::map<std::shared_ptr<Monster>, int> m_monsterActionTurns; // 새끼 용 브레스용 턴 트래커

    // 초기화
    void spawnMonsters();                           // 무작위 몬스터 무리 스폰
    void rollInitiatives();                         // 우선권 주사위 롤링 및 턴 정렬
    void nextTurn();                                // 다음 턴 엔티티로 차례 이전

    // 전투 액션 처리
    void performPlayerAttack();                     // 일반 물리 공격 실행
    void performPlayerSkill();                      // 클래스별 마법/특화 스킬 실행 (v0.8.0 다형성 적용)
    void performUseItem();                          // 아이템 사용 실행 (v0.8.0 마나/스탯 버프/해독 물약 분기)
    std::vector<int> getConsumableInventoryIndices() const;
    void confirmSelectedItem();
    bool executePendingAllySkill();
    void performEscapeAttempt();                    // 도망 확률 롤링 계산

    void handleMonsterTurn(const TurnEntity& entity); // 몬스터 AI 공격 실행 (v0.8.0 브레스, 독침, 마법화살 구현)

    // 상태 체크
    bool checkVictory();                            // 전투 승리 조건 충족 유무
    bool checkDefeat();                             // 아군 파티 전멸 유무

    bool distributeRewards();                       // 승리 보상 정산 및 원자 저장
    void clearPartyCombatBuffs();
    void addLog(const std::string& msg);            // 전투 로그 버퍼 기록

    // UI 텍스트 드로우 컴포넌트
    sf::Text m_headerText;
    sf::Text m_logText;
    sf::Text m_monsterListText;
    sf::Text m_actionMenuText;
    sf::Text m_monsterAsciiText;                    // 거대 아스키 몬스터 데코레이션

    void initTexts();
    void updateTuiContent();
    std::string getMonsterAsciiArt() const;
};

} // namespace crawl

#endif // COMBAT_STATE_HPP
