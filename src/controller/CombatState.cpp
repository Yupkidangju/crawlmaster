// [v0.4.0] CombatState.cpp 신규 작성
// 턴 선제권 롤러, d20 주사위 판정, 클래스 특화 스킬 및 TPK 전멸 시 세이브 하드코어 포맷 리셋 전투 엔진을 상세 구현한다.

#include "controller/CombatState.hpp"
#include "controller/DungeonState.hpp"
#include "controller/GameOverState.hpp"
#include "controller/VictoryState.hpp"
#include "controller/SettingsState.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/CombatRules.hpp"
#include "model/CombatActionRules.hpp"
#include "model/MonsterFactory.hpp"
#include "model/ConcreteSkills.hpp"
#include "model/SkillFactory.hpp"
#include "model/ConcreteItems.hpp"
#include "model/ItemFactory.hpp"
#include <random>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace crawl {

CombatState::CombatState(Game& game, EncounterTier tier, bool bossBattle)
    : CombatState(game, EncounterSpec{tier, bossBattle ? "mon_dragon_whelp" : "", "", "",
                                      bossBattle, bossBattle}) {}

CombatState::CombatState(Game& game, EncounterSpec encounter)
    : m_game(game), m_currentTurnIdx(0), m_selectedTargetIdx(0),
      m_encounterTier(encounter.tier), m_isBossBattle(encounter.bossBattle),
      m_isCampaignFinal(encounter.campaignFinal), m_questId(std::move(encounter.questId)),
      m_worldObjectId(std::move(encounter.worldObjectId)) {
    initTexts();
    
    // 1. 무작위 몬스터 무리 스폰 (1~3마리)
    if (!encounter.fixedMonsterId.empty()) {
        m_foes.push_back(MonsterFactory::createMonster(encounter.fixedMonsterId));
    } else {
        spawnMonsters();
    }

    // 2. 우선권 롤링 및 턴 리스트 정렬
    rollInitiatives();

    addLog(LocalizationManager::getInstance().get("COMBAT_LOG_START"));
    addLog(LocalizationManager::getInstance().get("COMBAT_LOG_COMMANDS"));

    // 3. 최초 턴 상황 갱신
    updateTuiContent();

}

void CombatState::handleInput(const sf::Event& event) {
    if (event.type != sf::Event::KeyPressed) return;

    // 현재 턴의 주체가 몬스터인 경우 아군 키 입력을 제한함
    if (m_turnOrder.empty()) return;
    const auto& currentEntity = m_turnOrder[m_currentTurnIdx];
    if (currentEntity.isMonster) return;

    sf::Keyboard::Key key = event.key.code;

    // 플레이어 overlay는 state stack에 보존되므로 설정에서 복귀해도 선택을 이어갈 수 있다.
    if (key == sf::Keyboard::O) {
        m_game.getStates().pushState(std::make_unique<SettingsState>(m_game));
        return;
    }

    auto selectAllyByKey = [&](sf::Keyboard::Key pressedKey) {
        const int memberCount = m_game.getParty().getMemberCount();
        if (memberCount <= 0) return;
        if (pressedKey == sf::Keyboard::Left || pressedKey == sf::Keyboard::Up) {
            m_selectedAllyIndex = (m_selectedAllyIndex + memberCount - 1) % memberCount;
        } else if (pressedKey == sf::Keyboard::Right || pressedKey == sf::Keyboard::Down) {
            m_selectedAllyIndex = (m_selectedAllyIndex + 1) % memberCount;
        } else if (pressedKey >= sf::Keyboard::Num1 && pressedKey <= sf::Keyboard::Num4) {
            const int requested = pressedKey - sf::Keyboard::Num1;
            if (requested < memberCount) m_selectedAllyIndex = requested;
        }
    };

    if (m_isSelectingItem || m_isSelectingItemTarget || m_isConfirmingItem) {
        if (key == sf::Keyboard::Escape) {
            if (m_isConfirmingItem) {
                m_isConfirmingItem = false;
                m_isSelectingItemTarget = true;
            } else if (m_isSelectingItemTarget) {
                m_isSelectingItemTarget = false;
                m_isSelectingItem = true;
            } else {
                m_isSelectingItem = false;
                m_selectedInventoryIndex = -1;
            }
            updateTuiContent();
            return;
        }

        if (m_isSelectingItem) {
            int selected = -1;
            if (key >= sf::Keyboard::Num1 && key <= sf::Keyboard::Num9) selected = key - sf::Keyboard::Num1;
            const auto indices = getConsumableInventoryIndices();
            if (selected >= 0 && selected < static_cast<int>(indices.size())) {
                m_selectedInventoryIndex = indices[static_cast<std::size_t>(selected)];
                m_selectedAllyIndex = currentEntity.index;
                m_isSelectingItem = false;
                m_isSelectingItemTarget = true;
            }
            updateTuiContent();
            return;
        }

        if (m_isSelectingItemTarget) {
            selectAllyByKey(key);
            if (key == sf::Keyboard::Enter) {
                const auto& inventory = m_game.getParty().getInventory();
                auto target = m_game.getParty().getMember(m_selectedAllyIndex);
                if (m_selectedInventoryIndex >= 0 &&
                    m_selectedInventoryIndex < static_cast<int>(inventory.size()) && target) {
                    auto consumable = std::dynamic_pointer_cast<ConsumableItem>(
                        inventory[static_cast<std::size_t>(m_selectedInventoryIndex)]);
                    if (consumable && CombatActionRules::canUseConsumable(*consumable, *target)) {
                        m_isSelectingItemTarget = false;
                        m_isConfirmingItem = true;
                    } else {
                        addLog(LocalizationManager::getInstance().get("COMBAT_LOG_ITEM_NO_EFFECT"));
                    }
                }
            }
            updateTuiContent();
            return;
        }

        if (m_isConfirmingItem && key == sf::Keyboard::Enter) {
            confirmSelectedItem();
            return;
        }
        return;
    }

    if (m_isSelectingSkillTarget) {
        if (key == sf::Keyboard::Escape) {
            if (m_isConfirmingSkillTarget) {
                m_isConfirmingSkillTarget = false;
                updateTuiContent();
                return;
            }
            m_isSelectingSkillTarget = false;
            m_pendingSkillIndex = -1;
            updateTuiContent();
            return;
        }
        if (!m_isConfirmingSkillTarget) selectAllyByKey(key);
        if (key == sf::Keyboard::Enter) {
            if (!m_isConfirmingSkillTarget) {
                m_isConfirmingSkillTarget = true;
            } else if (executePendingAllySkill()) {
                nextTurn();
            }
            updateTuiContent();
        }
        return;
    }

    // [v0.8.0] 스킬/주문 선택 팝업 모드 처리
    if (m_isSelectingSkill) {
        if (key == sf::Keyboard::Escape) {
            m_isSelectingSkill = false;
            updateTuiContent();
            return;
        }

        int skillIdx = -1;
        if (key >= sf::Keyboard::Num1 && key <= sf::Keyboard::Num9) {
            skillIdx = key - sf::Keyboard::Num1;
        } else if (key >= sf::Keyboard::Numpad1 && key <= sf::Keyboard::Numpad9) {
            skillIdx = key - sf::Keyboard::Numpad1;
        }

        if (skillIdx >= 0) {
            auto actor = m_game.getParty().getMember(currentEntity.index);
            const auto& skills = actor->getSkills();
            if (skillIdx < static_cast<int>(skills.size())) {
                auto skill = skills[skillIdx];

                // 마나 주문 슬롯 검사
                if (skill->isSpell() && actor->getSpellSlots() <= 0) {
                    addLog(LocalizationManager::getInstance().get("SKILL_LOG_NO_SLOTS"));
                    updateTuiContent();
                    return;
                }

                // 롱소드 등의 양손 그레이트소드 착용 시 방패를 들 수 없는 규칙과 유사하게, 
                // Shield Bash 스킬 등 장비 상황 검증
                if (skill->getId() == "skl_shield_bash" && !actor->getEquippedItem(EquipSlot::SHIELD)) {
                    addLog(LocalizationManager::getInstance().get("SKILL_LOG_SHIELD_REQUIRED"));
                    updateTuiContent();
                    return;
                }

                if (skill->getTargetType() == SkillTargetType::SINGLE_ALLY) {
                    m_pendingSkillIndex = skillIdx;
                    m_selectedAllyIndex = currentEntity.index;
                    m_isSelectingSkill = false;
                    m_isSelectingSkillTarget = true;
                    m_isConfirmingSkillTarget = false;
                    updateTuiContent();
                    return;
                }

                std::vector<std::string> logMsgs;
                std::vector<std::shared_ptr<Character>> allies;
                for (int i = 0; i < m_game.getParty().getMemberCount(); ++i) {
                    allies.push_back(m_game.getParty().getMember(i));
                }

                // 스킬 실행
                const bool committed = skill->execute(*actor, allies, m_foes, m_selectedTargetIdx, logMsgs);

                for (const auto& msg : logMsgs) {
                    addLog(msg);
                }

                m_isSelectingSkill = false;

                if (!committed) {
                    addLog(LocalizationManager::getInstance().get("COMBAT_LOG_NO_VALID_ACTION_TARGET"));
                    updateTuiContent();
                    return;
                }

                if (checkVictory()) {
                    nextTurn();
                    return;
                }
                nextTurn();
            }
        }
        return;
    }

    // 타겟팅 조절 (좌/우 방향키를 누르면 살아있는 다른 몬스터 조준)
    const int foeCount = static_cast<int>(m_foes.size());
    if (key == sf::Keyboard::Left) {
        int original = m_selectedTargetIdx;
        do {
            m_selectedTargetIdx = (m_selectedTargetIdx + foeCount - 1) % foeCount;
        } while (m_foes[m_selectedTargetIdx]->isDead() && m_selectedTargetIdx != original);
        updateTuiContent();
        return;
    } else if (key == sf::Keyboard::Right) {
        int original = m_selectedTargetIdx;
        do {
            m_selectedTargetIdx = (m_selectedTargetIdx + 1) % foeCount;
        } while (m_foes[m_selectedTargetIdx]->isDead() && m_selectedTargetIdx != original);
        updateTuiContent();
        return;
    }

    // 행동 번호 선택 단축키 처리
    if (key == sf::Keyboard::Num1) {
        performPlayerAttack();
    } else if (key == sf::Keyboard::Num2) {
        performPlayerSkill();
    } else if (key == sf::Keyboard::Num3) {
        performUseItem();
    } else if (key == sf::Keyboard::Num4) {
        performEscapeAttempt();
    }
}

void CombatState::update(sf::Time /*deltaTime*/) {
    if (m_hasStarted || m_turnOrder.empty()) return;
    m_hasStarted = true;
    const auto firstEntity = m_turnOrder[m_currentTurnIdx];
    if (firstEntity.isMonster) {
        handleMonsterTurn(firstEntity);
        nextTurn();
    }
}

void CombatState::draw(sf::RenderWindow& window) {
    // [v0.9.0] 설정 복귀 후 즉시 실시간 다국어 변경을 갱신하기 위해 강제 호출
    updateTuiContent();

    window.draw(m_headerText);
    window.draw(m_monsterAsciiText);
    window.draw(m_monsterListText);
    window.draw(m_actionMenuText);
    window.draw(m_logText);
}

void CombatState::spawnMonsters() {
    int minimum = m_encounterTier == EncounterTier::LATE ? 2 : 1;
    int maximum = m_encounterTier == EncounterTier::EARLY ? 1 :
                  (m_encounterTier == EncounterTier::MIDDLE ? 2 : 3);
    int count = SessionRng::global().rollRange(minimum, maximum);
    for (int i = 0; i < count; ++i) {
        m_foes.push_back(MonsterFactory::createRandomMonster(m_encounterTier, SessionRng::global()));
    }

    // 살아있는 몬스터 첫 번째 타겟팅 자동 선점
    m_selectedTargetIdx = 0;
}

void CombatState::rollInitiatives() {
    m_turnOrder.clear();

    // 1. 아군 파티 멤버 우선권 굴림
    const auto& members = m_game.getParty().getMembers();
    for (size_t i = 0; i < members.size(); ++i) {
        if (members[i] && !members[i]->isDead()) {
            int dexMod = members[i]->getAbilities().getModifier(members[i]->getAbilities().dexterity);
            int roll = SessionRng::global().rollDie(20) + dexMod +
                       CombatRules::initiativeBonus(*members[i]);
            m_turnOrder.push_back({false, static_cast<int>(i), roll});
        }
    }

    // 2. 몬스터 우선권 굴림
    for (size_t i = 0; i < m_foes.size(); ++i) {
        if (m_foes[i] && !m_foes[i]->isDead()) {
            int roll = SessionRng::global().rollDie(20) + 1;
            m_turnOrder.push_back({true, static_cast<int>(i), roll});
        }
    }

    // 3. 주사위 값이 높은 순으로 내림차순 정렬
    std::sort(m_turnOrder.begin(), m_turnOrder.end(), [](const TurnEntity& a, const TurnEntity& b) {
        return a.initiativeRoll > b.initiativeRoll;
    });

    m_currentTurnIdx = 0;
}

void CombatState::nextTurn() {
    if (!m_turnOrder.empty()) {
        const auto& completedTurn = m_turnOrder[m_currentTurnIdx];
        if (!completedTurn.isMonster) {
            if (auto member = m_game.getParty().getMember(completedTurn.index)) {
                std::vector<std::string> expiryLogs;
                member->advanceCombatBuffDurations(expiryLogs);
                for (const auto& log : expiryLogs) addLog(log);
            }
        }
    }

    // 1. 전투 승리 여부 검사
    if (checkVictory()) {
        if (!distributeRewards()) return;
        clearPartyCombatBuffs();
        if (m_isCampaignFinal) {
            m_game.getStates().replaceAll(
                std::make_unique<VictoryState>(m_game, m_victoryDurabilityUnknown));
        } else {
            m_game.getStates().popState();
        }
        return;
    }

    // 2. 아군 파티 완전 전멸 (TPK) 검사
    if (checkDefeat()) {
        addLog(LocalizationManager::getInstance().get("COMBAT_DEFEAT"));
        addLog(LocalizationManager::getInstance().get("COMBAT_LOG_TPK_CHECKPOINT"));
        const auto restoreResult = m_game.getParty().loadFromFile();
        if (!restoreResult.succeeded()) m_game.getParty().markRecoveryPending();
        m_game.getStates().replaceAll(std::make_unique<GameOverState>(m_game, restoreResult.succeeded()));
        return;
    }

    // 3. 턴 인덱스 순환
    m_currentTurnIdx = (m_currentTurnIdx + 1) % static_cast<int>(m_turnOrder.size());
    const auto& nextEntity = m_turnOrder[m_currentTurnIdx];

    // 4. 차례 대상이 이미 죽은 대상인지 스킵 필터링
    if (nextEntity.isMonster) {
        auto monster = m_foes[nextEntity.index];
        if (monster->isDead()) {
            nextTurn(); // 재귀 스킵
            return;
        }

        // --- [v0.8.0] 몬스터 턴 시작 시 독 대미지 정산 ---
        const bool wasParalyzed = monster->getParalysisTurns() > 0;
        std::vector<std::string> turnLogs;
        monster->processTurnEffects(turnLogs);
        for (const auto& log : turnLogs) {
            addLog(log);
        }

        if (monster->isDead()) {
            nextTurn();
            return;
        }

        // --- [v0.8.0] 몬스터 마비(Paralysis) 체크 및 행동 스킵 ---
        if (wasParalyzed) {
            addLog(LocalizationManager::getInstance().format("COMBAT_LOG_PARALYZED_SKIP", {{"target", monster->getName()}}));
            nextTurn();
            return;
        }

        // [v0.5.0] 몬스터 공격 행동(handleMonsterTurn) 완료 후, 다음 턴(nextTurn)을 연쇄 호출하고 복귀하여 전투 대기 상태를 풀지 않는 루프 멈춤 버그 수정.
        handleMonsterTurn(nextEntity);
        nextTurn(); // 몬스터 공격 종료 후 다음 턴 연쇄 기동
        return;
    } else {
        auto member = m_game.getParty().getMember(nextEntity.index);
        if (!member || member->isDead()) {
            nextTurn();
            return;
        }

        // --- [v0.8.0] 아군 턴 시작 시 독 대미지 및 임시 버프 차감 정산 ---
        const bool wasParalyzed = member->isParalyzed();
        std::vector<std::string> turnLogs;
        member->processTurnEffects(turnLogs);
        for (const auto& log : turnLogs) {
            addLog(log);
        }

        if (member->isDead()) {
            nextTurn();
            return;
        }

        // --- [v0.8.0] 아군 마비(Paralysis) 체크 및 행동 스킵 ---
        if (wasParalyzed) {
            addLog(LocalizationManager::getInstance().format("COMBAT_LOG_PARALYZED_SKIP", {{"target", member->getName()}}));
            nextTurn();
            return;
        }
    }

    updateTuiContent();
}

void CombatState::performPlayerAttack() {
    // 유효한 타겟팅 몬스터 획득
    if (m_foes[m_selectedTargetIdx]->isDead()) {
        // 살아있는 첫 몬스터 찾기 강제 보정
        for (size_t i = 0; i < m_foes.size(); ++i) {
            if (!m_foes[i]->isDead()) {
                m_selectedTargetIdx = static_cast<int>(i);
                break;
            }
        }
    }

    auto target = m_foes[m_selectedTargetIdx];
    const auto& currentEntity = m_turnOrder[m_currentTurnIdx];
    auto actor = m_game.getParty().getMember(currentEntity.index);

    int targetAc = target->getAc();
    const int naturalRoll = SessionRng::global().rollDie(20);
    const auto resolution = CombatRules::resolveAttack(*actor, naturalRoll, targetAc);
    const auto abilities = actor->getAbilities();
    const int abilityScore = actor->getClass() == CharacterClass::ROGUE
        ? abilities.dexterity : abilities.strength;
    const int abilityModifier = abilities.getModifier(abilityScore);
    const auto weapon = actor->getEquippedItem(EquipSlot::WEAPON);

    std::string actorName = actor->getName();
    std::string targetName = target->getName();

    if (resolution.outcome == HitOutcome::MISS) {
        if (naturalRoll == 1) {
            addLog(LocalizationManager::getInstance().get("COMBAT_LOG_FUMBLE"));
        } else {
            addLog(LocalizationManager::getInstance().format("COMBAT_LOG_ATTACK_MISS", {
                {"actor", actorName}, {"roll", std::to_string(resolution.attackTotal)},
                {"ac", std::to_string(targetAc)}}));
        }
    } else {
        const int diceCount = weapon ? weapon->getDamageDiceCount() : 1;
        const int diceSides = weapon ? weapon->getDamageDiceSides() : 4;
        int damage = CombatRules::rollAttackDamage(
            diceCount, diceSides, abilityModifier, 0, 0, 0,
            resolution.outcome == HitOutcome::CRITICAL, SessionRng::global());
        const DamageType damageType = weapon ? weapon->getDamageType() : DamageType::BLUDGEONING;
        damage = CombatRules::mitigateDamage(*target, damageType, damage);
        target->takeDamage(damage);
        addLog(LocalizationManager::getInstance().format("COMBAT_LOG_ATTACK_HIT", {
            {"actor", actorName}, {"roll", std::to_string(resolution.attackTotal)},
            {"ac", std::to_string(targetAc)}}));
        if (resolution.outcome == HitOutcome::CRITICAL) {
            addLog(LocalizationManager::getInstance().get("COMBAT_LOG_CRITICAL"));
        }
        addLog(LocalizationManager::getInstance().format("COMBAT_LOG_DAMAGE", {
            {"target", targetName}, {"damage", std::to_string(damage)}}));
    }

    if (target->isDead()) {
        addLog(LocalizationManager::getInstance().format("COMBAT_LOG_DEFEATED", {{"target", targetName}}));
    }

    nextTurn();
}

void CombatState::performPlayerSkill() {
    const auto& currentEntity = m_turnOrder[m_currentTurnIdx];
    auto actor = m_game.getParty().getMember(currentEntity.index);

    if (actor->getSkills().empty()) {
        addLog(LocalizationManager::getInstance().format("COMBAT_LOG_NO_SKILLS", {{"actor", actor->getName()}}));
        updateTuiContent();
        return;
    }

    m_isSelectingSkill = true;
    updateTuiContent();
}

void CombatState::performUseItem() {
    if (getConsumableInventoryIndices().empty()) {
        addLog(LocalizationManager::getInstance().get("COMBAT_LOG_NO_ITEMS"));
        updateTuiContent();
        return;
    }
    m_isSelectingItem = true;
    m_selectedInventoryIndex = -1;
    updateTuiContent();
}


void CombatState::performEscapeAttempt() {
    const auto& currentEntity = m_turnOrder[m_currentTurnIdx];
    auto actor = m_game.getParty().getMember(currentEntity.index);

    if (m_isBossBattle) {
        addLog(LocalizationManager::getInstance().get("COMBAT_LOG_BOSS_NO_ESCAPE"));
        nextTurn();
        return;
    }

    int dexMod = actor->getAbilities().getModifier(actor->getAbilities().dexterity);
    int roll = SessionRng::global().rollDie(20) + dexMod;

    // 도망 DC 난이도는 12 고정
    if (roll >= 12) {
        addLog(LocalizationManager::getInstance().get("COMBAT_ESCAPE_SUCCESS"));
        std::cout << "[FSM] CombatState에서 탈출 성공하여 DungeonState로 복귀합니다." << std::endl;
        clearPartyCombatBuffs();
        
        // [v0.5.0] changeState 호출 시 기존 DungeonState의 맵과 플레이어 위치가 날아가는 버그 수정.
        // 스택에 보존되어 있던 기존 DungeonState로 복귀하기 위해 popState 호출.
        m_game.getStates().popState();
    } else {
        addLog(LocalizationManager::getInstance().get("COMBAT_ESCAPE_FAIL"));
        nextTurn();
    }
}

void CombatState::clearPartyCombatBuffs() {
    for (int index = 0; index < m_game.getParty().getMemberCount(); ++index) {
        if (auto member = m_game.getParty().getMember(index)) member->clearCombatBuffs();
    }
}

void CombatState::handleMonsterTurn(const TurnEntity& entity) {
    auto monster = m_foes[entity.index];
    Party& party = m_game.getParty();

    // 몬스터 공격 대상 무작위 지정 (생존 파티원 중 선별)
    std::vector<int> aliveIdxs;
    for (int i = 0; i < party.getMemberCount(); ++i) {
        if (party.getMember(i) && !party.getMember(i)->isDead()) {
            aliveIdxs.push_back(i);
        }
    }

    if (aliveIdxs.empty()) return; // 파티 전멸 상태면 AI 처리 무시

    int targetIdx = aliveIdxs[static_cast<std::size_t>(
        SessionRng::global().rollRange(0, static_cast<int>(aliveIdxs.size()) - 1))];
    auto targetChar = party.getMember(targetIdx);

    std::string monId = monster->getId();
    std::string monName = monster->getName();
    std::string tarName = targetChar->getName();

    // --- [v0.8.0] 몬스터 종류별 특수 행동 처리 ---

    // 1. 고블린 주술사 (mon_goblin_shaman) : 35% 확률로 매직 미사일 시전 (1d4+1 필중 마법 피해)
    if (monId == "mon_goblin_shaman") {
        if (SessionRng::global().rollRange(1, 100) <= 35) {
            int damage = SessionRng::global().rollDie(4) + 1;
            targetChar->takeDamage(damage);
            addLog(LocalizationManager::getInstance().format("COMBAT_LOG_MONSTER_ABILITY", {
                {"actor", monName}, {"ability", LocalizationManager::getInstance().get("ABILITY_MAGIC_MISSILE")}}));
            addLog(LocalizationManager::getInstance().format("COMBAT_LOG_DAMAGE", {
                {"target", tarName}, {"damage", std::to_string(damage)}}));
            if (targetChar->isDead()) {
                addLog(LocalizationManager::getInstance().format("COMBAT_LOG_DEFEATED", {{"target", tarName}}));
            }
            return;
        }
    }

    // 2. 새끼 용 (mon_dragon_whelp) : 3턴마다 화염 브레스 발사 (광역 공격, 민첩 구원투사 DC 12)
    if (monId == "mon_dragon_whelp") {
        m_monsterActionTurns[monster]++;
        if (m_monsterActionTurns[monster] % 3 == 0) {
            addLog(LocalizationManager::getInstance().format("COMBAT_LOG_MONSTER_ABILITY", {
                {"actor", monName}, {"ability", LocalizationManager::getInstance().get("ABILITY_FIRE_BREATH")}}));
            
            for (int i = 0; i < party.getMemberCount(); ++i) {
                auto member = party.getMember(i);
                if (member && !member->isDead()) {
                    int baseDmg = SessionRng::global().rollDie(4) + SessionRng::global().rollDie(4);
                    
                    // 민첩 구원 투사 d20
                    int dexMod = member->getAbilities().getModifier(member->getAbilities().dexterity);
                    int saveRoll = SessionRng::global().rollDie(20) + dexMod;
                    
                    int finalDmg = baseDmg;
                    if (saveRoll >= 12) {
                        finalDmg /= 2; // 절반으로 감쇄
                        addLog(LocalizationManager::getInstance().format("COMBAT_LOG_SAVE_SUCCESS", {
                            {"target", member->getName()}, {"roll", std::to_string(saveRoll)}, {"dc", "12"}}));
                    } else {
                        addLog(LocalizationManager::getInstance().format("COMBAT_LOG_SAVE_FAIL", {
                            {"target", member->getName()}, {"roll", std::to_string(saveRoll)}, {"dc", "12"}}));
                    }
                    finalDmg = std::max(1, finalDmg);
                    member->takeDamage(finalDmg);
                    addLog(LocalizationManager::getInstance().format("COMBAT_LOG_DAMAGE", {
                        {"target", member->getName()}, {"damage", std::to_string(finalDmg)}}));
                    if (member->isDead()) {
                        addLog(LocalizationManager::getInstance().format("COMBAT_LOG_DEFEATED", {{"target", member->getName()}}));
                    }
                }
            }
            return;
        }
    }

    // 기본 공격 명중 주사위 굴림
    const int naturalRoll = SessionRng::global().rollDie(20);
    int attackRoll = naturalRoll + 3;
    int targetAc = targetChar->getAc();

    if (naturalRoll != 1 && (naturalRoll == 20 || attackRoll >= targetAc)) {
        int damage = monster->getDamageRoll();
        if (naturalRoll == 20) damage += monster->getDamageRoll();
        
        // 3. 오크 (mon_orc) : 체력 절반 이하 시 분노로 추가 물리 피해 +2
        if (monId == "mon_orc" && monster->getHp() <= monster->getMaxHp() / 2) {
            damage += 2;
            addLog(LocalizationManager::getInstance().format("COMBAT_LOG_MONSTER_ABILITY", {
                {"actor", monName}, {"ability", LocalizationManager::getInstance().get("ABILITY_RAGE")}}));
        }

        targetChar->takeDamage(damage);
        addLog(LocalizationManager::getInstance().format("COMBAT_LOG_MONSTER_HIT", {
            {"actor", monName}, {"target", tarName}, {"roll", std::to_string(attackRoll)},
            {"ac", std::to_string(targetAc)}}));
        addLog(LocalizationManager::getInstance().format("COMBAT_LOG_DAMAGE", {
            {"target", tarName}, {"damage", std::to_string(damage)}}));

        // 4. 거대 거미 (mon_giant_spider) : 25% 확률로 독 감염 (3턴 지속)
        if (monId == "mon_giant_spider") {
            if (SessionRng::global().rollRange(1, 100) <= 25) {
                targetChar->setPoison(3);
                addLog(LocalizationManager::getInstance().format("COMBAT_LOG_POISONED", {{"target", tarName}, {"turns", "3"}}));
            }
        }

        // 5. 구울 (mon_ghoul) : 20% 확률로 마비 감염 (1턴 지속)
        if (monId == "mon_ghoul") {
            if (SessionRng::global().rollRange(1, 100) <= 20) {
                targetChar->setParalysis(1);
                addLog(LocalizationManager::getInstance().format("COMBAT_LOG_PARALYZED", {{"target", tarName}, {"turns", "1"}}));
            }
        }

        if (targetChar->isDead()) {
            addLog(LocalizationManager::getInstance().format("COMBAT_LOG_DEFEATED", {{"target", tarName}}));
        }
    } else {
        addLog(LocalizationManager::getInstance().format("COMBAT_LOG_ATTACK_MISS", {
            {"actor", monName}, {"roll", std::to_string(attackRoll)}, {"ac", std::to_string(targetAc)}}));
    }
}

bool CombatState::checkVictory() {
    // 몬스터가 모두 죽었는지 판정
    return std::all_of(m_foes.begin(), m_foes.end(), [](const auto& m) {
        return m && m->isDead();
    });
}

bool CombatState::checkDefeat() {
    // 살아있는 파티원이 0명이면 전멸로 판정
    Party& party = m_game.getParty();
    if (party.getMemberCount() == 0) return true;

    for (int i = 0; i < party.getMemberCount(); ++i) {
        if (party.getMember(i) && !party.getMember(i)->isDead()) {
            return false;
        }
    }
    return true;
}

bool CombatState::distributeRewards() {
    Party& party = m_game.getParty();
    const PartyCheckpoint checkpoint = party.captureCheckpoint();
    
    // 1. 총 보상 산정
    int totalXp = 0;
    for (const auto& monster : m_foes) {
        if (monster) {
            totalXp += monster->getXpReward();
        }
    }

    int totalGold = CombatRules::rollGoldReward(m_foes, SessionRng::global());

    // 2. 살아있는 멤버 수 집계 및 균등 경험치 배분
    int aliveCount = 0;
    for (int i = 0; i < party.getMemberCount(); ++i) {
        if (party.getMember(i) && !party.getMember(i)->isDead()) aliveCount++;
    }

    addLog(LocalizationManager::getInstance().get("COMBAT_VICTORY"));
    addLog(LocalizationManager::getInstance().format("COMBAT_LOG_GOLD", {{"amount", std::to_string(totalGold)}}));

    party.addGold(totalGold);

    if (aliveCount > 0) {
        int xpShare = totalXp / aliveCount;
        addLog(LocalizationManager::getInstance().format("COMBAT_LOG_XP", {
            {"total", std::to_string(totalXp)}, {"share", std::to_string(xpShare)}}));
        for (int i = 0; i < party.getMemberCount(); ++i) {
            auto member = party.getMember(i);
            if (member && !member->isDead()) {
                member->addXp(xpShare);
            }
        }
    }

    // 3. 처치한 몬스터 ID를 순회하여 활성 퀘스트 진행도 동기화
    for (const auto& monster : m_foes) {
        if (monster) {
            party.updateQuestKillProgress(monster->getId(), 1);
            const auto dropIds = MonsterFactory::getDropItemIds(monster->getId());
            if (!dropIds.empty()) {
                if (m_isCampaignFinal) {
                    for (const auto& dropId : dropIds) {
                        if (auto item = ItemFactory::createItem(dropId)) {
                            party.addItem(item);
                            addLog(LocalizationManager::getInstance().format("COMBAT_LOG_LOOT", {{"item", item->getName()}}));
                        }
                    }
                } else if (SessionRng::global().rollRange(1, 100) <= 35) {
                    const int dropIndex = SessionRng::global().rollRange(0, static_cast<int>(dropIds.size()) - 1);
                    if (auto item = ItemFactory::createItem(dropIds[static_cast<std::size_t>(dropIndex)])) {
                        party.addItem(item);
                        addLog(LocalizationManager::getInstance().format("COMBAT_LOG_LOOT", {{"item", item->getName()}}));
                    }
                }
            }
        }
    }

    if (!m_questId.empty()) {
        party.markQuestObjectiveComplete(m_questId);
        if (auto* object = party.getWorld().findObject(m_worldObjectId)) {
            object->state = WorldObjectState::RESOLVED;
        }
    }
    if (m_isCampaignFinal) {
        party.setCampaignCompleted(true);
    }
    const auto saveResult = party.saveToFile();
    if (!saveResult.durabilityConfirmed()) {
        m_victoryDurabilityUnknown = true;
        addLog(LocalizationManager::getInstance().get("VICTORY_DURABILITY_UNKNOWN"));
        return true;
    }
    if (!saveResult) {
        party.restoreCheckpoint(checkpoint);
        addLog(LocalizationManager::getInstance().get("COMBAT_LOG_REWARD_ROLLBACK"));
        return false;
    }
    return true;
}

void CombatState::addLog(const std::string& msg) {
    m_battleLog.push_back(msg);
    if (m_battleLog.size() > 25) {
        m_battleLog.erase(m_battleLog.begin());
    }
}

void CombatState::initTexts() {
    const sf::Font& font = m_game.getFont();
    sf::Color neonGreen = sf::Color(51, 255, 51);

    // 헤더 상태 정보
    m_headerText.setFont(font);
    m_headerText.setCharacterSize(20);
    m_headerText.setFillColor(neonGreen);
    m_headerText.setPosition(50.0f, 40.0f);

    // 몬스터 데코레이션 아스키 아트 텍스트
    m_monsterAsciiText.setFont(font);
    m_monsterAsciiText.setCharacterSize(14);
    m_monsterAsciiText.setFillColor(sf::Color(102, 255, 102));
    m_monsterAsciiText.setPosition(100.0f, 150.0f);

    // 우측 상단 몬스터 라이브 목록
    m_monsterListText.setFont(font);
    m_monsterListText.setCharacterSize(14);
    m_monsterListText.setFillColor(neonGreen);
    m_monsterListText.setPosition(650.0f, 40.0f);

    // 우측 하단 아군 턴 행동 메뉴
    m_actionMenuText.setFont(font);
    m_actionMenuText.setCharacterSize(14);
    m_actionMenuText.setFillColor(sf::Color(102, 255, 102));
    m_actionMenuText.setPosition(650.0f, 320.0f);

    // 하단 배틀 로그창
    m_logText.setFont(font);
    m_logText.setCharacterSize(14);
    m_logText.setFillColor(sf::Color(255, 176, 0)); // Amber
    m_logText.setPosition(50.0f, 540.0f);
}

void CombatState::updateTuiContent() {
    // [v0.9.4] 언어 전환 시 지속 Text가 이전 State 생성 시점의 폰트를 유지하지 않도록 갱신한다.
    const sf::Font& font = m_game.getFont();
    m_headerText.setFont(font);
    m_monsterAsciiText.setFont(font);
    m_monsterListText.setFont(font);
    m_actionMenuText.setFont(font);
    m_logText.setFont(font);
    auto& textLocalization = LocalizationManager::getInstance();
    const bool largeText = textLocalization.getTextScale() > 125;
    m_headerText.setCharacterSize(textLocalization.getScaledTextSize(20));
    m_monsterAsciiText.setCharacterSize(textLocalization.getScaledTextSize(14));
    m_monsterListText.setCharacterSize(textLocalization.getScaledTextSize(14));
    m_actionMenuText.setCharacterSize(textLocalization.getScaledTextSize(14));
    m_logText.setCharacterSize(textLocalization.getScaledTextSize(14));
    m_monsterAsciiText.setPosition(largeText ? 100.0f : 100.0f, largeText ? 155.0f : 150.0f);
    m_actionMenuText.setPosition(largeText ? 680.0f : 650.0f, largeText ? 300.0f : 320.0f);
    // [v0.9.2] COMBAT_TITLE 헤더 갱신 시 ANSI 변환 경로를 제거하고 getSf() 헬퍼를 적용하여 UTF-8 안전 보장
    // 1. 헤더 타이틀 갱신
    m_headerText.setString(LocalizationManager::getInstance().getSf("COMBAT_TITLE"));

    // 2. 몬스터 아스키 그래픽 갱신
    const std::string monsterArt = getMonsterAsciiArt();
    m_monsterAsciiText.setString(sf::String::fromUtf8(monsterArt.begin(), monsterArt.end()));

    // 3. 우측 몬스터 목록 갱신
    std::ostringstream foeOss;
    foeOss << LocalizationManager::getInstance().get("COMBAT_MONSTER_LIST") << "\n";
    for (size_t i = 0; i < m_foes.size(); ++i) {
        if (m_foes[i]) {
            std::string indicator = (i == static_cast<size_t>(m_selectedTargetIdx)) ? " > " : "   ";
            foeOss << indicator << static_cast<char>('A' + i) << ". " << m_foes[i]->getName();
            if (m_foes[i]->isDead()) {
                foeOss << " (" << LocalizationManager::getInstance().get("COMBAT_DEFEATED") << ")\n";
            } else {
                foeOss << " (" << LocalizationManager::getInstance().get("COMBAT_HP_SHORT") << ": "
                       << m_foes[i]->getHp() << "/" << m_foes[i]->getMaxHp() << ")\n";
            }
        }
    }
    foeOss << "\n " << LocalizationManager::getInstance().get(
        largeText ? "COMBAT_SELECT_TARGET_SHORT" : "COMBAT_SELECT_TARGET");
    std::string foeStr = foeOss.str();
    m_monsterListText.setString(sf::String::fromUtf8(foeStr.begin(), foeStr.end()));

    // 4. 우측 하단 행동 메뉴 갱신
    std::ostringstream actOss;
    const auto& currentEntity = m_turnOrder[m_currentTurnIdx];

    if (currentEntity.isMonster) {
        actOss << LocalizationManager::getInstance().get("COMBAT_ENEMY_TURN") << "\n";
        actOss << LocalizationManager::getInstance().get("COMBAT_ENEMY_PREPARE");
    } else {
        auto member = m_game.getParty().getMember(currentEntity.index);
        
        std::string classStr = "";
        switch (member->getClass()) {
            case CharacterClass::WARRIOR: classStr = LocalizationManager::getInstance().get("CLASS_WARRIOR"); break;
            case CharacterClass::MAGE:    classStr = LocalizationManager::getInstance().get("CLASS_MAGE"); break;
            case CharacterClass::ROGUE:   classStr = LocalizationManager::getInstance().get("CLASS_ROGUE"); break;
            case CharacterClass::CLERIC:  classStr = LocalizationManager::getInstance().get("CLASS_CLERIC"); break;
        }

        actOss << LocalizationManager::getInstance().get("COMBAT_ACTOR_TURN") << "\n";
        actOss << LocalizationManager::getInstance().format(
                      largeText ? "COMBAT_TURN_LINE_SHORT" : "COMBAT_TURN_LINE", {
                      {"name", member->getName()}, {"class", classStr}}) << "\n"
               << LocalizationManager::getInstance().format(
                      largeText ? "COMBAT_STATS_LINE_SHORT" : "COMBAT_STATS_LINE", {
                      {"hp", std::to_string(member->getHp())}, {"maxHp", std::to_string(member->getMaxHp())},
                      {"slots", std::to_string(member->getSpellSlots())}}) << "\n\n";

        if (m_isSelectingItem) {
            actOss << LocalizationManager::getInstance().get("COMBAT_SELECT_ITEM") << "\n";
            const auto indices = getConsumableInventoryIndices();
            const auto& inventory = m_game.getParty().getInventory();
            for (std::size_t index = 0; index < indices.size() && index < 9; ++index) {
                actOss << (index + 1) << ". "
                       << inventory[static_cast<std::size_t>(indices[index])]->getName() << "\n";
            }
            actOss << LocalizationManager::getInstance().get("COMBAT_CANCEL_RETURN");
        } else if (m_isSelectingItemTarget || m_isConfirmingItem || m_isSelectingSkillTarget) {
            actOss << LocalizationManager::getInstance().get(
                largeText ? "COMBAT_SELECT_ALLY_SHORT" : "COMBAT_SELECT_ALLY") << "\n";
            for (int index = 0; index < m_game.getParty().getMemberCount(); ++index) {
                const auto ally = m_game.getParty().getMember(index);
                actOss << (index == m_selectedAllyIndex ? "> " : "  ") << (index + 1) << ". "
                       << ally->getName() << " HP " << ally->getHp() << "/" << ally->getMaxHp() << "\n";
            }
            if (m_isConfirmingItem) {
                const auto& inventory = m_game.getParty().getInventory();
                if (m_selectedInventoryIndex >= 0 &&
                    m_selectedInventoryIndex < static_cast<int>(inventory.size())) {
                    actOss << "\n" << LocalizationManager::getInstance().get(
                        largeText ? "COMBAT_CONFIRM_ITEM_SHORT" : "COMBAT_CONFIRM_ITEM")
                           << ": " << inventory[static_cast<std::size_t>(m_selectedInventoryIndex)]->getName();
                }
            } else if (m_isConfirmingSkillTarget) {
                actOss << "\n" << LocalizationManager::getInstance().get(
                    largeText ? "COMBAT_CONFIRM_SKILL_TARGET_SHORT" : "COMBAT_CONFIRM_SKILL_TARGET");
            }
            actOss << "\n" << LocalizationManager::getInstance().get(
                largeText ? "COMBAT_CONFIRM_CANCEL_SHORT" : "COMBAT_CONFIRM_CANCEL");
        } else if (m_isSelectingSkill) {
            actOss << LocalizationManager::getInstance().get("COMBAT_SELECT_SKILL_SPELL") << "\n";
            const auto& skills = member->getSkills();
            for (size_t i = 0; i < skills.size(); ++i) {
                actOss << std::to_string(i + 1) << ". " << skills[i]->getName();
                if (skills[i]->isSpell()) {
                    actOss << " " << LocalizationManager::getInstance().format("COMBAT_SKILL_SLOT_COST", {
                        {"level", std::to_string(skills[i]->getSpellLevel())}});
                } else {
                    actOss << " " << LocalizationManager::getInstance().get("COMBAT_SKILL_NO_COST");
                }
                actOss << "\n";
            }
            actOss << LocalizationManager::getInstance().get("COMBAT_CANCEL_RETURN");
        } else {
            actOss << LocalizationManager::getInstance().get("COMBAT_OPTIONS") << "\n"
                   << LocalizationManager::getInstance().get(largeText ? "COMBAT_ATTACK_SHORT" : "COMBAT_ATTACK") << "\n"
                   << LocalizationManager::getInstance().get(largeText ? "COMBAT_SKILL_SHORT" : "COMBAT_SKILL") << "\n"
                   << LocalizationManager::getInstance().get(largeText ? "COMBAT_ITEM_SHORT" : "COMBAT_ITEM") << "\n"
                   << LocalizationManager::getInstance().get(largeText ? "COMBAT_RUN_SHORT" : "COMBAT_RUN");
        }
    }
    std::string actStr = actOss.str();
    m_actionMenuText.setString(sf::String::fromUtf8(actStr.begin(), actStr.end()));

    // 5. 하단 로그 갱신 (최대 8줄 출력)
    std::ostringstream logOss;
    logOss << LocalizationManager::getInstance().get("COMBAT_LOG_TITLE") << "\n";
    const std::size_t visibleLogCount = largeText ? 3U : 8U;
    size_t start = (m_battleLog.size() > visibleLogCount)
        ? (m_battleLog.size() - visibleLogCount) : 0;
    for (size_t i = start; i < m_battleLog.size(); ++i) {
        logOss << m_battleLog[i] << "\n";
    }
    std::string logStr = logOss.str();
    m_logText.setString(sf::String::fromUtf8(logStr.begin(), logStr.end()));
}

std::string CombatState::getMonsterAsciiArt() const {
    // 살아있는 몬스터 수에 맞춰 동적으로 다수/소수 아스키 그래픽 출력
    int aliveCount = 0;
    std::string monsterId = "mon_kobold";
    for (const auto& monster : m_foes) {
        if (monster && !monster->isDead()) {
            aliveCount++;
            monsterId = monster->getId();
        }
    }

    if (aliveCount == 0) {
        return "\n\n      " + LocalizationManager::getInstance().get("COMBAT_ALL_DEFEATED");
    }

    if (LocalizationManager::getInstance().getTextScale() > 125) {
        return "\n" + LocalizationManager::getInstance().format("COMBAT_MONSTERS_ENGAGED", {
            {"count", std::to_string(aliveCount)}});
    }

    // 몬스터 데코용 복고풍 외곽 몬스터 도트아트 형태 제공
    std::string monsterSymbol = 
        "           ,___,           \n"
        "           (o.o)   /\\_/\\  \n"
        "           /|_|\\  ( o.o ) \n"
        "           | | |   > ^ <  \n"
        "          _| |_|_   / \\   \n";

    if (monsterId == "mon_orc") {
        monsterSymbol =
            "         (\\  _  /)         \n"
            "         ( \\( )/ )         \n"
            "         (  / \\  )         \n"
            "         (  `-'  )         \n"
            "          \\_|_|_/          \n"
            "          [  ###  ]        \n";
    } else if (monsterId == "mon_skeleton") {
        monsterSymbol =
            "          .---.            \n"
            "         /  .  \\           \n"
            "        |  \\_/  |          \n"
            "        |   |   |          \n"
            "        |  / \\  |          \n"
            "        [  ++++  ]          \n";
    }

    std::ostringstream oss;
    oss << "\n       [ " << LocalizationManager::getInstance().get("COMBAT_FOE_SPOTTING") << " ]\n\n"
        << monsterSymbol << "\n"
        << "      " << LocalizationManager::getInstance().format("COMBAT_MONSTERS_ENGAGED", {
            {"count", std::to_string(aliveCount)}});
    return oss.str();
}

} // namespace crawl
