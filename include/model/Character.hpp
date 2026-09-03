// [v0.3.0] Character.hpp 신규 작성
// [v0.8.0] 상태이상(독, 마비), 전투 임시 버프(STR/DEX, 축복), 스킬 벡터 멤버 변수 및 다형성 제어 수단 탑재.
// D&D 5e 스타일 캐릭터 정보, 6대 능력치 및 수정된 보정 공식 적용, 장비 슬롯 장착 및 레벨업 공식을 구현한 헤더 정의.

#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include <string>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>
#include "model/Equipment.hpp"
#include "model/DungeonMap.hpp" // Direction 등 공유용

namespace crawl {

// 스킬 전방 선언
class Skill;

// 캐릭터 클래스 종류
enum class CharacterClass {
    WARRIOR,
    MAGE,
    ROGUE,
    CLERIC
};

// 6대 D&D 능력치 구조체
struct AbilityScore {
    int strength = 10;
    int dexterity = 10;
    int constitution = 10;
    int intelligence = 10;
    int wisdom = 10;
    int charisma = 10;
    
    // C++ 정수 나눗셈의 0 방향 내림 오류를 보정한 D&D 전용 보정치 획득 함수
    int getModifier(int score) const {
        int diff = score - 10;
        return (diff < 0) ? (diff - 1) / 2 : diff / 2;
    }
};

// Character 클래스: 개별 플레이어 캐릭터의 능력, 스탯, 성장을 관리
class Character {
public:
    Character(std::string name, CharacterClass charClass);
    ~Character() = default;

    // 주사위 4d6 Drop-Lowest 방식으로 능력치 6종을 무작위 롤링 셋업
    void rollAbilities();

    // 현재 장착 정보 및 스탯(버프 반영)을 토대로 AC(Armor Class) 계산
    int getAc() const;

    // 장비 장착 / 해제 처리
    bool canEquip(const Equipment& equipItem) const;
    bool equip(std::shared_ptr<Equipment> equipItem);
    void unequip(EquipSlot slot);

    // 경험치 추가 및 레벨업 유무 체크 (레벨업 시 해당 레벨 스킬 자동 획득)
    bool addXp(int amount);

    // 마을 여관 휴식 시 HP 및 주문 슬롯 완전 회복 및 상태이상 해제
    void rest();

    // 상처 치료 (일정 수치 HP 회복)
    void heal(int amount);

    // 피해 입기
    void takeDamage(int damage);

    // 캐릭터 생사 확인
    bool isDead() const;

    // Getter 함수들
    std::string getName() const;
    CharacterClass getClass() const;
    std::string getClassString() const;
    int getLevel() const;
    int getXp() const;
    int getHp() const;
    int getMaxHp() const;
    int getSpellSlots() const;
    int getMaxSpellSlots() const;
    void consumeSpellSlot();
    void recoverSpellSlot(int amount = 1); // [v0.8.0] 주문 슬롯 회복

    AbilityScore getAbilities() const; // 버프가 적용된 실시간 능력치 반환
    AbilityScore getRawAbilities() const; // 버프가 적용되지 않은 순수 능력치 반환
    std::shared_ptr<Equipment> getEquippedItem(EquipSlot slot) const;

    // --- [v0.8.0] 상태이상 및 버프 제어 함수 정의 ---
    void setPoison(int turns);
    void setParalysis(int turns);
    int getPoisonTurns() const;
    int getParalysisTurns() const;
    bool isParalyzed() const;

    void applyStrBuff(int amount, int turns);
    void applyDexBuff(int amount, int turns);
    void applyBless(int turns);
    int getStrBuffAmount() const;
    int getDexBuffAmount() const;
    int getBlessTurns() const;

    // 전투 중 턴 시작/종료 시 상태이상 데미지 및 턴 차감 정산
    void processTurnEffects(std::vector<std::string>& logOutput);
    void advanceCombatBuffDurations(std::vector<std::string>& logOutput);
    // 전투 종료 시 임시 버프 클리어
    void clearCombatBuffs();

    // --- [v0.8.0] 보유 스킬 제어 함수 정의 ---
    void learnSkill(std::shared_ptr<Skill> skill);
    const std::vector<std::shared_ptr<Skill>>& getSkills() const;
    void initSkillsForLevel(); // 레벨에 맞는 스킬 자동 초기화

    // --- nlohmann/json 파일 세이브 연동을 위한 직렬화/역직렬화 인터페이스 ---
    nlohmann::json toJson() const;
    static std::unique_ptr<Character> fromJson(const nlohmann::json& j, int schemaVersion = 2);

private:
    Character(std::string name, CharacterClass charClass, bool initializeRandomState);

    std::string m_name;                             // 캐릭터 이름
    CharacterClass m_class;                         // 캐릭터 클래스
    int m_level;                                    // 레벨 (1~3)
    int m_xp;                                       // 누적 경험치
    int m_hp;                                       // 현재 체력
    int m_maxHp;                                    // 최대 체력
    int m_spellSlots;                               // 현재 마법/성직자 주문 슬롯
    int m_maxSpellSlots;                            // 최대 주문 슬롯

    AbilityScore m_abilities;                       // 6대 능력치 스코어

    // [v0.8.0] 상태이상 및 버프 지속 턴
    int m_poisonTurns = 0;
    int m_paralysisTurns = 0;
    int m_strBuffTurns = 0;
    int m_strBuffAmount = 0;
    int m_dexBuffTurns = 0;
    int m_dexBuffAmount = 0;
    int m_blessTurns = 0;

    // [v0.8.0] 영웅 습득 스킬 리스트
    std::vector<std::shared_ptr<Skill>> m_skills;

    // 장비 장착 포인터
    std::shared_ptr<Equipment> m_equippedWeapon;
    std::shared_ptr<Equipment> m_equippedArmor;
    std::shared_ptr<Equipment> m_equippedShield;

    // 클래스별 주사위 크기 획득
    int getHitDieSides() const;
};

} // namespace crawl

#endif // CHARACTER_HPP
