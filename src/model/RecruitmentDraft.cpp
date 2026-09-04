#include "model/RecruitmentDraft.hpp"

#include "core/SessionRng.hpp"
#include "model/Party.hpp"
#include "model/CharacterIdentityRules.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>

namespace crawl {
namespace {

bool isKnownAbility(Ability ability) {
    const int value = static_cast<int>(ability);
    return value >= static_cast<int>(Ability::STRENGTH) &&
           value <= static_cast<int>(Ability::CHARISMA);
}

} // namespace

RecruitmentDraft::RecruitmentDraft(SessionRng& random) : m_random(random) {
    rollAbilities();
}

bool RecruitmentDraft::setName(const std::string& name) {
    const auto normalized = CharacterIdentityRules::normalizeName(name);
    if (!normalized) return false;
    m_identity.name = *normalized;
    return true;
}

bool RecruitmentDraft::setAge(int age) {
    if (age < 18 || age > 80) return false;
    m_identity.age = age;
    return true;
}

bool RecruitmentDraft::setGender(Gender gender) {
    const int value = static_cast<int>(gender);
    if (value < static_cast<int>(Gender::MALE) ||
        value > static_cast<int>(Gender::NON_BINARY)) return false;
    m_identity.gender = gender;
    return true;
}

bool RecruitmentDraft::setClass(CharacterClass characterClass) {
    const int value = static_cast<int>(characterClass);
    if (value < static_cast<int>(CharacterClass::WARRIOR) ||
        value > static_cast<int>(CharacterClass::CLERIC)) return false;
    m_identity.characterClass = characterClass;
    return true;
}

const CharacterIdentity& RecruitmentDraft::identity() const {
    return m_identity;
}

const AbilityScore& RecruitmentDraft::baseAbilities() const {
    return m_baseAbilities;
}

const AbilityScore& RecruitmentDraft::abilities() const {
    return m_abilities;
}

int RecruitmentDraft::remainingPoints() const {
    return m_remainingPoints;
}

int RecruitmentDraft::increaseCost(Ability ability) const {
    if (!isKnownAbility(ability)) return 0;
    const int current = select(m_abilities, ability);
    if (current >= 18) return 0;
    const int next = current + 1;
    if (next <= 12) return 1;
    if (next <= 15) return 2;
    return 3;
}

bool RecruitmentDraft::increase(Ability ability) {
    if (!isKnownAbility(ability)) return false;
    const int cost = increaseCost(ability);
    if (cost == 0 || cost > m_remainingPoints) return false;
    ++select(m_abilities, ability);
    m_remainingPoints -= cost;
    return true;
}

bool RecruitmentDraft::decrease(Ability ability) {
    if (!isKnownAbility(ability)) return false;
    int& current = select(m_abilities, ability);
    if (current <= select(m_baseAbilities, ability)) return false;
    const int refund = current <= 12 ? 1 : (current <= 15 ? 2 : 3);
    --current;
    m_remainingPoints += refund;
    return true;
}

void RecruitmentDraft::reroll() {
    if (m_confirmed) return;
    rollAbilities();
}

bool RecruitmentDraft::isReady() const {
    return !m_confirmed && isValidName(m_identity.name) &&
           m_identity.age >= 18 && m_identity.age <= 80 &&
           m_identity.gender != Gender::UNSPECIFIED && m_remainingPoints == 0;
}

std::shared_ptr<Character> RecruitmentDraft::createCandidate() const {
    if (!isReady()) return nullptr;
    return std::make_shared<Character>(m_identity, m_abilities);
}

bool RecruitmentDraft::confirm(Party& party) {
    if (!isReady()) return false;
    auto candidate = createCandidate();
    if (!party.addMember(std::move(candidate))) return false;
    m_confirmed = true;
    return true;
}

bool RecruitmentDraft::isValidName(const std::string& name) {
    return CharacterIdentityRules::isValidName(name);
}

int& RecruitmentDraft::select(AbilityScore& scores, Ability ability) {
    switch (ability) {
        case Ability::STRENGTH: return scores.strength;
        case Ability::DEXTERITY: return scores.dexterity;
        case Ability::CONSTITUTION: return scores.constitution;
        case Ability::INTELLIGENCE: return scores.intelligence;
        case Ability::WISDOM: return scores.wisdom;
        case Ability::CHARISMA: return scores.charisma;
    }
    throw std::invalid_argument("알 수 없는 ability 값입니다.");
}

int RecruitmentDraft::select(const AbilityScore& scores, Ability ability) {
    switch (ability) {
        case Ability::STRENGTH: return scores.strength;
        case Ability::DEXTERITY: return scores.dexterity;
        case Ability::CONSTITUTION: return scores.constitution;
        case Ability::INTELLIGENCE: return scores.intelligence;
        case Ability::WISDOM: return scores.wisdom;
        case Ability::CHARISMA: return scores.charisma;
    }
    throw std::invalid_argument("알 수 없는 ability 값입니다.");
}

void RecruitmentDraft::rollAbilities() {
    auto rollOne = [this]() {
        std::array<int, 4> rolls{};
        for (int& roll : rolls) roll = m_random.rollDie(6);
        std::sort(rolls.begin(), rolls.end());
        return rolls[1] + rolls[2] + rolls[3];
    };
    m_baseAbilities = {rollOne(), rollOne(), rollOne(), rollOne(), rollOne(), rollOne()};
    m_abilities = m_baseAbilities;
    m_remainingPoints = 10;
}

} // namespace crawl
