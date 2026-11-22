#ifndef MONSTER_HPP
#define MONSTER_HPP

#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/Character.hpp"
#include "model/Equipment.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace crawl {

enum class EnemyIntentKind {
    ATTACK = 0,
    HEAVY_STRIKE = 1,
    AOE_BREATH = 2,
    SPELL = 3,
    DEBUFF = 4,
    BUFF = 5,
    DEFEND = 6,
    CHARGING = 7
};

struct EnemyIntent {
    EnemyIntentKind kind = EnemyIntentKind::ATTACK;
    DamageType damageType = DamageType::SLASHING;
    int estimatedDamage = 4;
    std::string labelKey = "INTENT_ATTACK";
};

class Monster {
public:
    virtual ~Monster() = default;

    virtual void takeDamage(int damage) = 0;
    virtual int getAttackRoll() = 0;
    virtual int getDamageRoll() = 0;
    [[nodiscard]] virtual bool isDead() const = 0;

    [[nodiscard]] virtual std::string getId() const = 0;
    [[nodiscard]] virtual std::string getName() const = 0;
    [[nodiscard]] virtual int getAc() const = 0;
    [[nodiscard]] virtual int getXpReward() const = 0;
    [[nodiscard]] virtual int getHp() const = 0;
    [[nodiscard]] virtual int getMaxHp() const = 0;
    [[nodiscard]] virtual int getTier() const = 0;
    [[nodiscard]] virtual bool isBoss() const = 0;
    [[nodiscard]] virtual int getPhase() const = 0;
    virtual bool checkPhaseTransition(std::vector<std::string>& logOutput) = 0;

    // 스태거/브레이크 & 약점/저항
    [[nodiscard]] virtual int getMaxStagger() const = 0;
    [[nodiscard]] virtual int getStagger() const = 0;
    [[nodiscard]] virtual bool isBroken() const = 0;
    [[nodiscard]] virtual int getBrokenTurns() const = 0;
    virtual bool applyStagger(int amount) = 0;
    virtual void recoverFromBreak() = 0;

    [[nodiscard]] virtual const std::vector<DamageType>& getWeaknesses() const = 0;
    [[nodiscard]] virtual const std::vector<DamageType>& getResistances() const = 0;
    [[nodiscard]] virtual bool isWeakTo(DamageType type) const = 0;
    [[nodiscard]] virtual bool isResistantTo(DamageType type) const = 0;

    // 행동 예고(텔레그래프)
    [[nodiscard]] virtual const EnemyIntent& getNextIntent() const = 0;
    virtual void rollNextIntent(SessionRng& rng) = 0;
    virtual void interruptIntent() = 0;

    // 상태이상
    virtual void setPoison(int turns) = 0;
    virtual void setParalysis(int turns) = 0;
    virtual void setBurn(int turns) = 0;
    virtual void setBleed(int turns) = 0;
    virtual void setFrost(int turns) = 0;
    virtual void setDefending(bool defending) = 0;
    [[nodiscard]] virtual bool isDefending() const = 0;
    [[nodiscard]] virtual int getPoisonTurns() const = 0;
    [[nodiscard]] virtual int getParalysisTurns() const = 0;
    [[nodiscard]] virtual int getBurnTurns() const = 0;
    [[nodiscard]] virtual int getBleedTurns() const = 0;
    [[nodiscard]] virtual int getFrostTurns() const = 0;
    virtual void processTurnEffects(std::vector<std::string>& logOutput) = 0;
};

class ConcreteMonster : public Monster {
public:
    ConcreteMonster(std::string id, std::string nameKey, int maxHp, int ac, int xpReward,
                    int dmgDiceCount, int dmgDiceSides, int dmgBonus, int tier,
                    int maxStagger, std::vector<DamageType> weaknesses,
                    std::vector<DamageType> resistances, bool boss = false,
                    DamageType primaryElement = DamageType::SLASHING)
        : m_id(std::move(id)),
          m_nameKey(std::move(nameKey)),
          m_hp(maxHp),
          m_maxHp(maxHp),
          m_ac(ac),
          m_xpReward(xpReward),
          m_dmgDiceCount(dmgDiceCount),
          m_dmgDiceSides(dmgDiceSides),
          m_dmgBonus(dmgBonus),
          m_tier(tier),
          m_maxStagger(std::max(2, maxStagger)),
          m_stagger(std::max(2, maxStagger)),
          m_weaknesses(std::move(weaknesses)),
          m_resistances(std::move(resistances)),
          m_boss(boss),
          m_primaryElement(primaryElement) {
        m_nextIntent.kind = EnemyIntentKind::ATTACK;
        m_nextIntent.damageType = m_primaryElement;
        m_nextIntent.estimatedDamage = std::max(1, m_dmgDiceCount * (m_dmgDiceSides / 2 + 1) + m_dmgBonus);
        m_nextIntent.labelKey = "INTENT_ATTACK";
    }

    void takeDamage(int damage) override {
        if (damage <= 0) return;
        m_hp = std::max(0, m_hp - damage);
    }

    int getAttackRoll() override {
        return SessionRng::global().rollDie(20) + 2 + m_tier + (m_phase > 1 ? 2 : 0);
    }

    int getDamageRoll() override {
        int sum = 0;
        for (int i = 0; i < m_dmgDiceCount; ++i) {
            sum += SessionRng::global().rollDie(m_dmgDiceSides);
        }
        return std::max(1, sum + m_dmgBonus + (m_phase > 1 ? 4 : 0));
    }

    [[nodiscard]] bool isDead() const override { return m_hp <= 0; }
    [[nodiscard]] std::string getId() const override { return m_id; }
    [[nodiscard]] std::string getName() const override {
        return LocalizationManager::getInstance().get(m_nameKey);
    }
    [[nodiscard]] int getAc() const override {
        int ac = m_ac + (m_defending ? 3 : 0) - (m_brokenTurns > 0 ? 3 : 0) - (m_frostTurns > 0 ? 1 : 0);
        return std::max(6, ac);
    }
    [[nodiscard]] int getXpReward() const override { return m_xpReward; }
    [[nodiscard]] int getHp() const override { return m_hp; }
    [[nodiscard]] int getMaxHp() const override { return m_maxHp; }
    [[nodiscard]] int getTier() const override { return m_tier; }
    [[nodiscard]] bool isBoss() const override { return m_boss; }
    [[nodiscard]] int getPhase() const override { return m_phase; }

    bool checkPhaseTransition(std::vector<std::string>& logOutput) override {
        if (m_id == "mon_elder_wyrm" && m_phase == 1 && !isDead() && m_hp <= m_maxHp / 2) {
            m_phase = 2;
            m_stagger = m_maxStagger;
            m_brokenTurns = 0;
            m_nextIntent.kind = EnemyIntentKind::CHARGING;
            m_nextIntent.damageType = DamageType::FIRE;
            m_nextIntent.estimatedDamage = 24;
            m_nextIntent.labelKey = "INTENT_CHARGING";
            logOutput.push_back(LocalizationManager::getInstance().get("BOSS_ELDER_WYRM_PHASE2"));
            return true;
        }
        return false;
    }

    [[nodiscard]] int getMaxStagger() const override { return m_maxStagger; }
    [[nodiscard]] int getStagger() const override { return m_stagger; }
    [[nodiscard]] bool isBroken() const override { return m_brokenTurns > 0; }
    [[nodiscard]] int getBrokenTurns() const override { return m_brokenTurns; }

    bool applyStagger(int amount) override {
        if (amount <= 0 || isDead() || m_brokenTurns > 0) return false;
        m_stagger = std::max(0, m_stagger - amount);
        if (m_stagger == 0) {
            m_brokenTurns = 1;
            m_defending = false;
            interruptIntent();
            return true;
        }
        return false;
    }

    void recoverFromBreak() override {
        if (m_brokenTurns > 0) {
            --m_brokenTurns;
            if (m_brokenTurns == 0) {
                m_stagger = m_maxStagger;
            }
        }
    }

    [[nodiscard]] const std::vector<DamageType>& getWeaknesses() const override { return m_weaknesses; }
    [[nodiscard]] const std::vector<DamageType>& getResistances() const override { return m_resistances; }

    [[nodiscard]] bool isWeakTo(DamageType type) const override {
        return std::find(m_weaknesses.begin(), m_weaknesses.end(), type) != m_weaknesses.end();
    }

    [[nodiscard]] bool isResistantTo(DamageType type) const override {
        return std::find(m_resistances.begin(), m_resistances.end(), type) != m_resistances.end();
    }

    [[nodiscard]] const EnemyIntent& getNextIntent() const override { return m_nextIntent; }

    void interruptIntent() override {
        m_wasCharging = false;
        m_nextIntent.kind = EnemyIntentKind::ATTACK;
        m_nextIntent.damageType = m_primaryElement;
        m_nextIntent.estimatedDamage = std::max(1, m_dmgDiceCount * (m_dmgDiceSides / 2 + 1) + m_dmgBonus);
        m_nextIntent.labelKey = "INTENT_ATTACK";
    }

    void rollNextIntent(SessionRng& rng) override {
        const int baseEst = std::max(2, m_dmgDiceCount * (m_dmgDiceSides / 2 + 1) + m_dmgBonus +
                                     (m_phase > 1 ? 4 : 0));
        // 직전 턴이 CHARGING이었으면 이번 턴은 광역 브레스/강타 확정 발동!
        if (m_wasCharging) {
            m_wasCharging = false;
            m_nextIntent.kind = EnemyIntentKind::AOE_BREATH;
            m_nextIntent.damageType = (m_id == "mon_bog_hydra") ? DamageType::POISON
                                    : (m_id == "mon_ossuary_lich") ? DamageType::ARCANE
                                    : DamageType::FIRE;
            m_nextIntent.estimatedDamage = static_cast<int>(baseEst * 1.45f);
            m_nextIntent.labelKey = "INTENT_AOE_BREATH";
            return;
        }

        const int r = rng.rollRange(1, 100);
        if (m_boss && r <= (m_phase > 1 ? 42 : 30)) {
            m_wasCharging = true;
            m_nextIntent.kind = EnemyIntentKind::CHARGING;
            m_nextIntent.damageType = m_primaryElement;
            m_nextIntent.estimatedDamage = static_cast<int>(baseEst * 1.6f);
            m_nextIntent.labelKey = "INTENT_CHARGING";
            return;
        }
        if ((m_id == "mon_goblin_shaman" || m_id == "mon_ossuary_lich" ||
             m_id == "mon_ash_cultist" || m_id == "mon_wraith") && r <= 48) {
            m_nextIntent.kind = EnemyIntentKind::SPELL;
            m_nextIntent.damageType = m_primaryElement;
            m_nextIntent.estimatedDamage = baseEst + 3;
            m_nextIntent.labelKey = "INTENT_SPELL";
            return;
        }
        if ((m_id == "mon_giant_spider" || m_id == "mon_ghoul" ||
             m_id == "mon_mire_slime" || m_id == "mon_bog_hydra") && r <= 42) {
            m_nextIntent.kind = EnemyIntentKind::DEBUFF;
            m_nextIntent.damageType = DamageType::POISON;
            m_nextIntent.estimatedDamage = baseEst;
            m_nextIntent.labelKey = "INTENT_DEBUFF";
            return;
        }
        if (r <= 24) {
            m_nextIntent.kind = EnemyIntentKind::HEAVY_STRIKE;
            m_nextIntent.damageType = m_primaryElement;
            m_nextIntent.estimatedDamage = static_cast<int>(baseEst * 1.4f);
            m_nextIntent.labelKey = "INTENT_HEAVY_STRIKE";
        } else if (r >= 88 && !m_boss) {
            m_nextIntent.kind = EnemyIntentKind::DEFEND;
            m_nextIntent.damageType = m_primaryElement;
            m_nextIntent.estimatedDamage = 0;
            m_nextIntent.labelKey = "INTENT_DEFEND";
        } else {
            m_nextIntent.kind = EnemyIntentKind::ATTACK;
            m_nextIntent.damageType = m_primaryElement;
            m_nextIntent.estimatedDamage = baseEst;
            m_nextIntent.labelKey = "INTENT_ATTACK";
        }
    }

    void setPoison(int turns) override { m_poisonTurns = std::max(0, turns); }
    void setParalysis(int turns) override { m_paralysisTurns = std::max(0, turns); }
    void setBurn(int turns) override { m_burnTurns = std::max(0, turns); }
    void setBleed(int turns) override { m_bleedTurns = std::max(0, turns); }
    void setFrost(int turns) override { m_frostTurns = std::max(0, turns); }
    void setDefending(bool defending) override { m_defending = defending; }
    [[nodiscard]] bool isDefending() const override { return m_defending; }
    [[nodiscard]] int getPoisonTurns() const override { return m_poisonTurns; }
    [[nodiscard]] int getParalysisTurns() const override { return m_paralysisTurns; }
    [[nodiscard]] int getBurnTurns() const override { return m_burnTurns; }
    [[nodiscard]] int getBleedTurns() const override { return m_bleedTurns; }
    [[nodiscard]] int getFrostTurns() const override { return m_frostTurns; }

    void processTurnEffects(std::vector<std::string>& logOutput) override {
        if (isDead()) return;
        m_defending = false;
        const auto& lm = LocalizationManager::getInstance();

        auto applyDot = [&](int& turns, int die, int flat, const char* key) {
            if (turns <= 0 || isDead()) return;
            const int dmg = SessionRng::global().rollDie(die) + flat;
            takeDamage(dmg);
            logOutput.push_back(lm.format(key, {
                {"target", getName()}, {"damage", std::to_string(dmg)}}));
            --turns;
            if (isDead()) {
                logOutput.push_back(lm.format("COMBAT_LOG_DEFEATED", {{"target", getName()}}));
            }
        };

        applyDot(m_poisonTurns, 4, 1, "STATUS_LOG_POISON_DAMAGE");
        applyDot(m_burnTurns, 5, 2, "STATUS_LOG_BURN_DAMAGE");
        applyDot(m_bleedTurns, 4, 1, "STATUS_LOG_BLEED_DAMAGE");

        if (m_paralysisTurns > 0) --m_paralysisTurns;
        if (m_frostTurns > 0) --m_frostTurns;
    }

private:
    std::string m_id;
    std::string m_nameKey;
    int m_hp;
    int m_maxHp;
    int m_ac;
    int m_xpReward;
    int m_dmgDiceCount;
    int m_dmgDiceSides;
    int m_dmgBonus;
    int m_tier;

    int m_maxStagger;
    int m_stagger;
    int m_brokenTurns = 0;
    std::vector<DamageType> m_weaknesses;
    std::vector<DamageType> m_resistances;
    bool m_boss = false;
    int m_phase = 1;
    DamageType m_primaryElement = DamageType::SLASHING;
    bool m_wasCharging = false;
    bool m_defending = false;
    EnemyIntent m_nextIntent;

    int m_poisonTurns = 0;
    int m_paralysisTurns = 0;
    int m_burnTurns = 0;
    int m_bleedTurns = 0;
    int m_frostTurns = 0;
};

} // namespace crawl

#endif // MONSTER_HPP
