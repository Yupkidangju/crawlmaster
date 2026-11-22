#include "controller/CombatState.hpp"
#include "controller/GameOverState.hpp"
#include "controller/VictoryState.hpp"
#include "core/AudioManager.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/CombatActionRules.hpp"
#include "model/CombatRules.hpp"
#include "model/ConcreteItems.hpp"
#include "model/ItemFactory.hpp"
#include "model/MonsterFactory.hpp"
#include "model/Skill.hpp"
#include "view/UiTheme.hpp"

#include <algorithm>
#include <cmath>

namespace crawl {

std::vector<int> CombatState::getUsableConsumableIndices() const {
    std::vector<int> out;
    const auto& inv = m_game.getParty().getInventory();
    const auto& members = m_game.getParty().getMembers();
    for (int i = 0; i < static_cast<int>(inv.size()); ++i) {
        auto cons = std::dynamic_pointer_cast<ConsumableItem>(inv[static_cast<std::size_t>(i)]);
        if (cons && CombatActionRules::canUseConsumableOnParty(*cons, members)) {
            out.push_back(i);
        }
    }
    return out;
}

void CombatState::executeBasicAttack(Character& actor) {
    if (m_selectedTargetIdx < 0 || m_selectedTargetIdx >= static_cast<int>(m_foes.size()) ||
        m_foes[static_cast<std::size_t>(m_selectedTargetIdx)]->isDead()) {
        selectFirstLivingFoe();
    }
    if (m_foes.empty() || m_foes[static_cast<std::size_t>(m_selectedTargetIdx)]->isDead()) return;

    auto& target = *m_foes[static_cast<std::size_t>(m_selectedTargetIdx)];
    const auto& lm = LocalizationManager::getInstance();
    auto& rng = SessionRng::global();
    const auto center = monsterScreenCenter(static_cast<std::size_t>(m_selectedTargetIdx));

    const auto wpn = actor.getEquippedItem(EquipSlot::WEAPON);
    const DamageType dmgType = wpn ? wpn->getDamageType() : DamageType::BLUDGEONING;
    const WeaponReach reach = wpn ? wpn->getReach() : WeaponReach::MELEE;

    const int nat = rng.rollDie(20);
    const auto res = CombatRules::resolveAttack(actor, nat, target.getAc());
    if (res.outcome == HitOutcome::MISS) {
        AudioManager::getInstance().playSfx("sfx_miss");
        m_vfx.spawnFloatingText(center, "MISS", UiTheme::textMuted(), 20);
        addLog(lm.format("COMBAT_LOG_MISS", {{"attacker", actor.getName()}, {"target", target.getName()}}));
        nextTurn();
        return;
    }

    const bool crit = (res.outcome == HitOutcome::CRITICAL);
    const int abMod = CombatRules::attackAbilityModifier(actor);
    const int diceC = wpn ? wpn->getDamageDiceCount() : 1;
    const int diceS = wpn ? wpn->getDamageDiceSides() : 4;
    const int flat = wpn ? wpn->getUpgradeDamageBonus() : 0;
    int raw = CombatRules::rollAttackDamage(diceC, diceS, abMod, flat, 0, 0, crit, rng);
    raw = std::max(1, static_cast<int>(std::round(
        static_cast<float>(raw) * CombatRules::rowAttackMultiplier(actor.getRow(), reach))));

    const auto tac = CombatRules::applyTacticalDamage(actor, target, dmgType, raw, 1, true);
    if (tac.hitWeakness) m_game.getParty().getBestiary().revealWeakness(target.getId());

    m_foeHitFlash[static_cast<std::size_t>(m_selectedTargetIdx)] = 1.0f;
    m_vfx.spawnSlash(center, UiTheme::elementColor(dmgType));
    m_vfx.triggerShake(crit || tac.triggeredBreak ? 11.0f : 5.0f, crit ? 0.22f : 0.13f);
    if (crit || tac.triggeredBreak) m_hitStopTimer = 0.055f;

    AudioManager::getInstance().playSfx(tac.triggeredBreak ? "sfx_break" : (crit ? "sfx_crit" : "sfx_sword_hit"));

    std::string pop = std::to_string(tac.finalDamage);
    if (crit) pop += " CRIT!";
    if (tac.hitWeakness) pop += " WEAK!";
    m_vfx.spawnFloatingText(center - sf::Vector2f(0.0f, 24.0f), pop,
                            crit ? UiTheme::textGold() : (tac.hitWeakness ? UiTheme::staggerAmber() : UiTheme::textPrimary()),
                            crit ? 26 : 22);
    if (tac.triggeredBreak) {
        m_vfx.spawnBurst(center, UiTheme::breakMagenta(), 28, 220.0f);
        m_vfx.spawnFloatingText(center - sf::Vector2f(0.0f, 58.0f), "BREAK!", UiTheme::breakMagenta(), 28);
    }

    addLog(lm.format("COMBAT_LOG_HIT", {
        {"attacker", actor.getName()}, {"target", target.getName()}, {"damage", pop}}));

    if (!target.isDead()) {
        std::vector<std::string> phaseLogs;
        if (target.checkPhaseTransition(phaseLogs)) {
            for (const auto& pl : phaseLogs) addLog(pl);
            m_vfx.spawnBurst(center, sf::Color(255, 84, 34), 36, 260.0f);
            m_vfx.triggerShake(14.0f, 0.35f);
            AudioManager::getInstance().playSfx("sfx_fire_spell");
        }
    } else {
        addLog(lm.format("COMBAT_LOG_DEFEATED", {{"target", target.getName()}}));
        m_game.getParty().getBestiary().recordDefeat(target.getId());
    }
    nextTurn();
}

void CombatState::executeGuard(Character& actor) {
    actor.setGuarding(true);
    AudioManager::getInstance().playSfx("sfx_shield_block");
    const auto idx = m_turnOrder[static_cast<std::size_t>(m_currentTurnIdx)].index;
    m_vfx.spawnFloatingText(partyCardCenter(static_cast<std::size_t>(idx)) - sf::Vector2f(0.0f, 50.0f),
                            "GUARD", UiTheme::spAzure(), 20);
    addLog(LocalizationManager::getInstance().format("COMBAT_LOG_GUARD", {{"actor", actor.getName()}}));
    nextTurn();
}

void CombatState::executeRowSwap(Character& actor) {
    if (m_rowSwappedThisTurn) return;
    actor.toggleRow();
    m_rowSwappedThisTurn = true;
    AudioManager::getInstance().playSfx("sfx_turn");
    addLog(LocalizationManager::getInstance().format("COMBAT_LOG_ROW_SWAP", {
        {"actor", actor.getName()},
        {"row", LocalizationManager::getInstance().get(actor.getRow() == CombatRow::FRONT ? "ROW_FRONT" : "ROW_BACK")}
    }));
}

void CombatState::executeSelectedSkill(int skillIndex) {
    const auto& cur = m_turnOrder[static_cast<std::size_t>(m_currentTurnIdx)];
    auto actor = m_game.getParty().getMember(cur.index);
    if (!actor) return;
    const auto& skills = actor->getSkills();
    if (skillIndex < 0 || skillIndex >= static_cast<int>(skills.size())) return;
    const auto& sk = skills[static_cast<std::size_t>(skillIndex)];

    if (sk->getTargetType() == SkillTargetType::SINGLE_ALLY) {
        m_pendingSkillIndex = skillIndex;
        m_subMenu = SubMenu::SKILL_ALLY_TARGET;
        AudioManager::getInstance().playSfx("sfx_ui_click");
        return;
    }

    std::vector<std::string> logs;
    auto allies = m_game.getParty().getMembers();
    const bool ok = sk->execute(*actor, allies, m_foes, m_selectedTargetIdx, logs);
    for (const auto& l : logs) addLog(l);
    if (!ok) {
        AudioManager::getInstance().playSfx("sfx_ui_error");
        return;
    }

    // 시각 및 오디오 피드백
    const auto elemCol = UiTheme::elementColor(sk->getDamageType());
    if (sk->getTargetType() == SkillTargetType::ALL_FOES) {
        for (std::size_t i = 0; i < m_foes.size(); ++i) {
            if (m_foes[i]->isDead()) m_game.getParty().getBestiary().recordDefeat(m_foes[i]->getId());
            m_foeHitFlash[i] = 1.0f;
            m_vfx.spawnBurst(monsterScreenCenter(i), elemCol, 20, 180.0f);
        }
        m_vfx.triggerShake(9.0f, 0.22f);
    } else if (sk->getTargetType() == SkillTargetType::SINGLE_FOE &&
               m_selectedTargetIdx < static_cast<int>(m_foes.size())) {
        const auto idx = static_cast<std::size_t>(m_selectedTargetIdx);
        if (m_foes[idx]->isDead()) m_game.getParty().getBestiary().recordDefeat(m_foes[idx]->getId());
        m_foeHitFlash[idx] = 1.0f;
        m_vfx.spawnBurst(monsterScreenCenter(idx), elemCol, 22, 190.0f);
        m_vfx.triggerShake(7.0f, 0.16f);
    }

    switch (sk->getDamageType()) {
        case DamageType::FIRE:      AudioManager::getInstance().playSfx("sfx_fire_spell"); break;
        case DamageType::ICE:       AudioManager::getInstance().playSfx("sfx_ice_spell"); break;
        case DamageType::LIGHTNING: AudioManager::getInstance().playSfx("sfx_lightning_spell"); break;
        case DamageType::POISON:    AudioManager::getInstance().playSfx("sfx_poison"); break;
        case DamageType::HOLY:      AudioManager::getInstance().playSfx("sfx_heal"); break;
        default:                    AudioManager::getInstance().playSfx("sfx_sword_hit"); break;
    }

    m_subMenu = SubMenu::NONE;
    nextTurn();
}

void CombatState::executePendingAllySkill() {
    const auto& cur = m_turnOrder[static_cast<std::size_t>(m_currentTurnIdx)];
    auto actor = m_game.getParty().getMember(cur.index);
    if (!actor || m_pendingSkillIndex < 0 || m_pendingSkillIndex >= static_cast<int>(actor->getSkills().size())) return;

    std::vector<std::string> logs;
    auto allies = m_game.getParty().getMembers();
    const bool ok = actor->getSkills()[static_cast<std::size_t>(m_pendingSkillIndex)]->execute(
        *actor, allies, m_foes, m_selectedAllyIdx, logs);
    for (const auto& l : logs) addLog(l);
    if (!ok) {
        AudioManager::getInstance().playSfx("sfx_ui_error");
        return;
    }

    AudioManager::getInstance().playSfx("sfx_heal");
    const auto pos = partyCardCenter(static_cast<std::size_t>(m_selectedAllyIdx)) - sf::Vector2f(0.0f, 54.0f);
    m_vfx.spawnBurst(pos, UiTheme::statPos(), 16, 120.0f);
    m_vfx.spawnFloatingText(pos, "+HEAL", UiTheme::statPos(), 22);
    m_subMenu = SubMenu::NONE;
    m_pendingSkillIndex = -1;
    nextTurn();
}

void CombatState::executeSelectedItem(int invIdx) {
    auto& party = m_game.getParty();
    const auto& inv = party.getInventory();
    if (invIdx < 0 || invIdx >= static_cast<int>(inv.size())) return;
    auto cons = std::dynamic_pointer_cast<ConsumableItem>(inv[static_cast<std::size_t>(invIdx)]);
    if (!cons) return;

    const auto& cur = m_turnOrder[static_cast<std::size_t>(m_currentTurnIdx)];
    auto actor = party.getMember(cur.index);
    if (!actor) return;

    if (cons->isOffensiveBomb()) {
        const auto elem = cons->getBombDamageType();
        for (std::size_t i = 0; i < m_foes.size(); ++i) {
            if (m_foes[i]->isDead()) continue;
            const auto tac = CombatRules::applyTacticalDamage(*actor, *m_foes[i], elem, cons->getBombPower(), 2, false);
            m_foeHitFlash[i] = 1.0f;
            m_vfx.spawnBurst(monsterScreenCenter(i), UiTheme::elementColor(elem), 22, 190.0f);
            m_vfx.spawnFloatingText(monsterScreenCenter(i), std::to_string(tac.finalDamage),
                                    UiTheme::elementColor(elem), 22);
            if (m_foes[i]->isDead()) party.getBestiary().recordDefeat(m_foes[i]->getId());
        }
        AudioManager::getInstance().playSfx(elem == DamageType::ICE ? "sfx_ice_spell" : "sfx_fire_spell");
        m_vfx.triggerShake(9.0f, 0.22f);
        addLog(actor->getName() + " -> " + cons->getName());
        party.removeItem(invIdx);
        m_subMenu = SubMenu::NONE;
        nextTurn();
        return;
    }

    auto target = party.getMember(m_selectedAllyIdx);
    if (!target || !CombatActionRules::canUseConsumable(*cons, *target)) {
        addLog(LocalizationManager::getInstance().get("COMBAT_LOG_ITEM_NO_EFFECT"));
        AudioManager::getInstance().playSfx("sfx_ui_error");
        return;
    }
    std::vector<std::string> logs;
    auto allies = party.getMembers();
    cons->applyEffect(*target, allies, logs);
    for (const auto& l : logs) addLog(l);
    party.removeItem(invIdx);

    AudioManager::getInstance().playSfx("sfx_heal");
    m_vfx.spawnBurst(partyCardCenter(static_cast<std::size_t>(m_selectedAllyIdx)) - sf::Vector2f(0.0f, 50.0f),
                     UiTheme::statPos(), 14, 110.0f);
    m_subMenu = SubMenu::NONE;
    nextTurn();
}

void CombatState::attemptFlee() {
    if (m_encounter.bossBattle) {
        addLog(LocalizationManager::getInstance().get("COMBAT_CANNOT_FLEE_BOSS"));
        AudioManager::getInstance().playSfx("sfx_ui_error");
        return;
    }
    if (SessionRng::global().rollRange(1, 100) <= 65) {
        AudioManager::getInstance().playSfx("sfx_turn");
        for (const auto& m : m_game.getParty().getMembers()) {
            if (m) m->clearCombatBuffs();
        }
        m_game.getStates().popState();
        return;
    }
    AudioManager::getInstance().playSfx("sfx_ui_error");
    addLog(LocalizationManager::getInstance().get("COMBAT_FLEE_FAILED"));
    nextTurn();
}

void CombatState::executeMonsterTurn(Monster& monster, int monsterIndex) {
    auto& party = m_game.getParty();
    const auto& lm = LocalizationManager::getInstance();
    auto& rng = SessionRng::global();

    std::vector<std::string> dotLogs;
    monster.processTurnEffects(dotLogs);
    for (const auto& l : dotLogs) addLog(l);
    if (monster.isDead()) {
        party.getBestiary().recordDefeat(monster.getId());
        return;
    }

    if (monster.isBroken()) {
        monster.recoverFromBreak();
        addLog(lm.format("COMBAT_LOG_MONSTER_BROKEN", {{"monster", monster.getName()}}));
        monster.rollNextIntent(rng);
        return;
    }
    if (monster.getParalysisTurns() > 0) {
        addLog(lm.format("STATUS_LOG_PARALYZED", {{"target", monster.getName()}}));
        monster.rollNextIntent(rng);
        return;
    }

    // 생존 아군 중 타겟 선택 (도발 최우선, 전열 75% 가중치)
    std::vector<int> living;
    int taunter = -1;
    for (int i = 0; i < party.getMemberCount(); ++i) {
        const auto m = party.getMember(i);
        if (m && !m->isDead()) {
            living.push_back(i);
            if (m->getTauntTurns() > 0) taunter = i;
        }
    }
    if (living.empty()) return;

    auto pickTarget = [&]() -> int {
        if (taunter >= 0) return taunter;
        std::vector<int> weighted;
        for (int idx : living) {
            const int w = (party.getMember(idx)->getRow() == CombatRow::FRONT) ? 3 : 1;
            for (int k = 0; k < w; ++k) weighted.push_back(idx);
        }
        return weighted[static_cast<std::size_t>(rng.rollRange(0, static_cast<int>(weighted.size()) - 1))];
    };

    const EnemyIntent intent = monster.getNextIntent();
    m_foeLunge[static_cast<std::size_t>(monsterIndex)] = 1.0f;

    if (intent.kind == EnemyIntentKind::CHARGING) {
        AudioManager::getInstance().playSfx("sfx_fire_spell");
        m_vfx.spawnBurst(monsterScreenCenter(static_cast<std::size_t>(monsterIndex)), UiTheme::staggerAmber(), 20, 140.0f);
        addLog(lm.format("COMBAT_LOG_CHARGING", {{"monster", monster.getName()}}));
        monster.rollNextIntent(rng);
        return;
    }

    if (intent.kind == EnemyIntentKind::DEFEND) {
        monster.setDefending(true);
        AudioManager::getInstance().playSfx("sfx_shield_block");
        addLog(lm.format("COMBAT_LOG_GUARD", {{"actor", monster.getName()}}));
        monster.rollNextIntent(rng);
        return;
    }

    if (intent.kind == EnemyIntentKind::AOE_BREATH) {
        AudioManager::getInstance().playSfx("sfx_fire_spell");
        m_vfx.triggerShake(12.0f, 0.28f);
        m_vfx.triggerDamageFlash(0.8f);
        for (int idx : living) {
            auto ally = party.getMember(idx);
            int dmg = std::max(2, monster.getDamageRoll() + monster.getTier() * 2);
            if (ally->isGuarding()) dmg = std::max(1, dmg / 2);
            if (ally->getWardTurns() > 0) dmg = std::max(1, dmg * 75 / 100);
            ally->takeDamage(dmg);
            if (intent.damageType == DamageType::FIRE && rng.rollRange(1, 100) <= 35) ally->setBurn(2);
            if (intent.damageType == DamageType::POISON && rng.rollRange(1, 100) <= 45) ally->setPoison(2);
            m_vfx.spawnFloatingText(partyCardCenter(static_cast<std::size_t>(idx)) - sf::Vector2f(0.0f, 54.0f),
                                    "-" + std::to_string(dmg), UiTheme::hpCrimson(), 22);
        }
        addLog(lm.format("COMBAT_LOG_AOE", {{"monster", monster.getName()}}));
        monster.rollNextIntent(rng);
        return;
    }

    // 단일 대상 공격 / 강타 / 주문 / 디버프
    const int targetIdx = pickTarget();
    auto ally = party.getMember(targetIdx);
    const bool isMelee = (intent.kind == EnemyIntentKind::ATTACK || intent.kind == EnemyIntentKind::HEAVY_STRIKE);
    const int targetAc = ally->getAc() + CombatRules::rowIncomingAcBonus(ally->getRow(), isMelee);
    const int atkRoll = monster.getAttackRoll() + (intent.kind == EnemyIntentKind::SPELL ? 3 : 0);

    if (intent.kind != EnemyIntentKind::SPELL && atkRoll < targetAc) {
        AudioManager::getInstance().playSfx("sfx_shield_block");
        m_vfx.spawnFloatingText(partyCardCenter(static_cast<std::size_t>(targetIdx)) - sf::Vector2f(0.0f, 50.0f),
                                "BLOCK", UiTheme::spAzure(), 18);
        addLog(lm.format("COMBAT_LOG_MISS", {{"attacker", monster.getName()}, {"target", ally->getName()}}));
        monster.rollNextIntent(rng);
        return;
    }

    int dmg = monster.getDamageRoll();
    if (intent.kind == EnemyIntentKind::HEAVY_STRIKE) dmg = static_cast<int>(std::round(dmg * 1.45f));
    if (ally->isGuarding()) dmg = std::max(1, dmg / 2);
    if (ally->getWardTurns() > 0) dmg = std::max(1, dmg * 75 / 100);
    ally->takeDamage(dmg);

    if (intent.kind == EnemyIntentKind::DEBUFF && !ally->isDead()) {
        if (monster.getId() == "mon_ghoul") ally->setParalysis(1);
        else ally->setPoison(3);
    }

    AudioManager::getInstance().playSfx(intent.kind == EnemyIntentKind::SPELL ? "sfx_fire_spell" : "sfx_sword_hit");
    m_vfx.triggerShake(7.0f, 0.18f);
    m_vfx.triggerDamageFlash(0.62f);
    m_vfx.spawnFloatingText(partyCardCenter(static_cast<std::size_t>(targetIdx)) - sf::Vector2f(0.0f, 52.0f),
                            "-" + std::to_string(dmg), UiTheme::hpCrimson(), 24);
    addLog(lm.format("COMBAT_LOG_HIT", {
        {"attacker", monster.getName()}, {"target", ally->getName()}, {"damage", std::to_string(dmg)}}));

    monster.rollNextIntent(rng);
}

void CombatState::checkCombatResolution() {
    if (m_combatEnded) return;

    bool anyFoeAlive = false;
    for (const auto& f : m_foes) {
        if (f && !f->isDead()) anyFoeAlive = true;
    }
    if (!anyFoeAlive) {
        m_combatEnded = true;
        m_victoryResult = true;
        AudioManager::getInstance().playSfx("sfx_victory");
        return;
    }

    bool anyHeroAlive = false;
    for (const auto& m : m_game.getParty().getMembers()) {
        if (m && !m->isDead()) anyHeroAlive = true;
    }
    if (!anyHeroAlive) {
        m_combatEnded = true;
        m_victoryResult = false;
        AudioManager::getInstance().playSfx("sfx_defeat");
    }
}

void CombatState::handleVictory() {
    auto& party = m_game.getParty();
    auto& rng = SessionRng::global();

    int totalXp = 0;
    for (const auto& f : m_foes) {
        if (!f) continue;
        totalXp += f->getXpReward();
        party.updateQuestKillProgress(f->getId(), 1);
        party.getBestiary().recordDefeat(f->getId());

        const auto drops = MonsterFactory::getDropItemIds(f->getId());
        if (!drops.empty() && (f->isBoss() || rng.rollRange(1, 100) <= 45)) {
            const std::string dropId = drops[static_cast<std::size_t>(
                rng.rollRange(0, static_cast<int>(drops.size()) - 1))];
            if (auto item = ItemFactory::createItem(dropId)) {
                if (auto eq = std::dynamic_pointer_cast<Equipment>(item)) {
                    if (f->isBoss() || rng.rollRange(1, 100) <= 35) {
                        const ItemRarity rar = f->isBoss() ? std::max(eq->getRarity(), ItemRarity::RARE) : eq->getRarity();
                        eq->setRarity(rar);
                        eq->setAffix(ItemFactory::rollAffix(rar, rng));
                    }
                }
                party.addItem(item);
            }
        }
    }

    const int gold = CombatRules::rollGoldReward(m_foes, rng);
    party.addGold(gold);

    bool anyLeveled = false;
    for (const auto& m : party.getMembers()) {
        if (!m) continue;
        m->clearCombatBuffs();
        if (!m->isDead() && m->addXp(totalXp)) anyLeveled = true;
    }
    if (anyLeveled) AudioManager::getInstance().playSfx("sfx_levelup");

    if (!m_encounter.worldObjectId.empty()) {
        if (auto* obj = party.getWorld().findObject(m_encounter.worldObjectId)) {
            obj->state = WorldObjectState::RESOLVED;
        }
    }
    if (!m_encounter.questId.empty()) {
        party.markQuestObjectiveComplete(m_encounter.questId);
    }

    if (m_encounter.campaignFinal) {
        party.setCampaignCompleted(true);
        party.saveToFile();
        m_game.triggerFadeIn(0.32f);
        m_game.getStates().replaceAll(std::make_unique<VictoryState>(m_game));
        return;
    }

    party.saveToFile();
    m_game.pushToast("+" + std::to_string(gold) + " G  ·  +" + std::to_string(totalXp) + " XP", UiTheme::textGold());
    m_game.getStates().popState();
}

void CombatState::handleDefeat() {
    // TPK 시 정상 세이브를 삭제하지 않고 마지막 전체 세션 체크포인트로 비파괴 복구한다.
    auto& party = m_game.getParty();
    const auto loadRes = party.loadFromFile();
    if (loadRes) {
        if (party.getLastSessionSeed() != 0U) {
            SessionRng::global() = SessionRng(party.getLastSessionSeed(), party.getSessionRngDrawCount());
        }
        m_game.getStates().replaceAll(std::make_unique<GameOverState>(
            m_game, true, LocalizationManager::getInstance().get("GAMEOVER_CHECKPOINT_RESTORED")));
    } else {
        party.markRecoveryPending();
        m_game.getStates().replaceAll(std::make_unique<GameOverState>(
            m_game, false, LocalizationManager::getInstance().get("GAMEOVER_CHECKPOINT_FAILED")));
    }
}

} // namespace crawl
