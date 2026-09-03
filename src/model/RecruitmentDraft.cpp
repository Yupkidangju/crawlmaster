#include "model/RecruitmentDraft.hpp"

#include "core/SessionRng.hpp"
#include "model/Party.hpp"

#include <stdexcept>

namespace crawl {

RecruitmentDraft::RecruitmentDraft(SessionRng& random, std::vector<std::string> names)
    : m_random(random), m_names(std::move(names)) {
    if (m_names.empty()) throw std::invalid_argument("모집 후보 이름 목록은 비어 있을 수 없습니다.");
    createCandidate();
}

const Character& RecruitmentDraft::preview() const {
    return *m_candidate;
}

void RecruitmentDraft::reroll() {
    if (m_confirmed) return;
    m_nameIndex = (m_nameIndex + 1) % m_names.size();
    createCandidate();
}

bool RecruitmentDraft::confirm(Party& party) {
    if (m_confirmed || !m_candidate || !party.addMember(m_candidate)) return false;
    m_confirmed = true;
    return true;
}

void RecruitmentDraft::createCandidate() {
    const auto characterClass = static_cast<CharacterClass>(m_random.rollRange(0, 3));
    m_candidate = std::make_shared<Character>(m_names[m_nameIndex], characterClass);
}

} // namespace crawl
