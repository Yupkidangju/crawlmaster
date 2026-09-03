// [v0.4.0] Monster.hpp 신규 작성
// [v0.8.0] 몬스터용 상태이상(독, 마비) 관리 인터페이스 및 턴 차감/피해 처리 구체 구현 추가.
// 던전 전투 시 출현할 몬스터의 기본 정보 및 공격 주사위 롤러를 다루는 추상 인터페이스 및 구체 클래스 정의.

#ifndef MONSTER_HPP
#define MONSTER_HPP

#include <string>
#include <memory>
#include <vector>
#include "core/SessionRng.hpp"
#include "core/LocalizationManager.hpp"
#include "model/Character.hpp" // AbilityScore 구조체 재사용용

namespace crawl {

// Monster 추상 클래스: 전투 참여 가능한 몬스터 대상 인터페이스 정의
class Monster {
public:
    virtual ~Monster() = default;

    // 대미지 가해 및 HP 감쇄 처리
    virtual void takeDamage(int damage) = 0;

    // 공격 명중 주사위 굴림값 획득 (d20 + 보정치)
    virtual int getAttackRoll() = 0;

    // 공격 피해 주사위 굴림값 획득
    virtual int getDamageRoll() = 0;

    // 사망 여부 확인
    virtual bool isDead() const = 0;

    // Getter 함수들
    virtual std::string getId() const = 0;
    virtual std::string getName() const = 0;
    virtual int getAc() const = 0;
    virtual int getXpReward() const = 0;
    virtual int getHp() const = 0;
    virtual int getMaxHp() const = 0;
    virtual int getTier() const = 0;

    // --- [v0.8.0] 상태이상 관리 전용 함수 정의 ---
    virtual void setPoison(int turns) = 0;
    virtual void setParalysis(int turns) = 0;
    virtual int getPoisonTurns() const = 0;
    virtual int getParalysisTurns() const = 0;
    virtual void processTurnEffects(std::vector<std::string>& logOutput) = 0;
};

// ConcreteMonster 클래스: Monster 인터페이스를 구현하며 주사위 기반 공격력을 주입받아 구동
class ConcreteMonster : public Monster {
public:
    ConcreteMonster(std::string id, std::string name, int maxHp, int ac, int xpReward,
                    int dmgDiceCount, int dmgDiceSides, int dmgBonus, int tier = 1)
        : m_id(std::move(id)), m_name(std::move(name)), m_hp(maxHp), m_maxHp(maxHp), m_ac(ac),
          m_xpReward(xpReward), m_dmgDiceCount(dmgDiceCount), m_dmgDiceSides(dmgDiceSides), m_dmgBonus(dmgBonus),
          m_tier(tier),
          m_poisonTurns(0), m_paralysisTurns(0) {}

    ~ConcreteMonster() override = default;

    void takeDamage(int damage) override {
        m_hp = std::max(0, m_hp - damage);
    }

    int getAttackRoll() override {
        // 몬스터 기본 명중 주사위 굴림: 1d20 + 3 보너스 고정 적용
        return SessionRng::global().rollDie(20) + 3;
    }

    int getDamageRoll() override {
        int sum = 0;
        for (int i = 0; i < m_dmgDiceCount; ++i) {
            sum += SessionRng::global().rollDie(m_dmgDiceSides);
        }
        return sum + m_dmgBonus;
    }

    bool isDead() const override { return m_hp <= 0; }

    std::string getId() const override { return m_id; }
    std::string getName() const override { return LocalizationManager::getInstance().get(m_name); }
    int getAc() const override { return m_ac; }
    int getXpReward() const override { return m_xpReward; }
    int getHp() const override { return m_hp; }
    int getMaxHp() const override { return m_maxHp; }
    int getTier() const override { return m_tier; }

    // --- [v0.8.0] 상태이상 관리 전용 함수 구현 ---
    void setPoison(int turns) override { m_poisonTurns = turns; }
    void setParalysis(int turns) override { m_paralysisTurns = turns; }
    int getPoisonTurns() const override { return m_poisonTurns; }
    int getParalysisTurns() const override { return m_paralysisTurns; }

    void processTurnEffects(std::vector<std::string>& logOutput) override {
        if (m_hp <= 0) return;

        // 독 상태 정산
        if (m_poisonTurns > 0) {
            int poisonDmg = SessionRng::global().rollDie(3);
            takeDamage(poisonDmg);
            logOutput.push_back(LocalizationManager::getInstance().format("STATUS_LOG_POISON_DAMAGE", {
                {"target", getName()}, {"damage", std::to_string(poisonDmg)}}));
            m_poisonTurns = std::max(0, m_poisonTurns - 1);
            if (m_hp <= 0) {
                logOutput.push_back(LocalizationManager::getInstance().format("COMBAT_LOG_DEFEATED", {
                    {"target", getName()}}));
            }
        }

        // 마비 상태 감소
        if (m_paralysisTurns > 0) {
            m_paralysisTurns = std::max(0, m_paralysisTurns - 1);
        }
    }

private:
    std::string m_id;
    std::string m_name;
    int m_hp;
    int m_maxHp;
    int m_ac;
    int m_xpReward;

    // 공격 사양
    int m_diceCount = 1;
    int m_dmgDiceCount;
    int m_dmgDiceSides;
    int m_dmgBonus;
    int m_tier;

    // [v0.8.0] 상태이상 변수
    int m_poisonTurns;
    int m_paralysisTurns;
};

} // namespace crawl

#endif // MONSTER_HPP
