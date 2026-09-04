#include "controller/CharacterCreationState.hpp"

#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/Party.hpp"

#include <algorithm>
#include <array>
#include <sstream>
#include <vector>

namespace crawl {
namespace {

std::string toUtf8(const sf::String& value) {
    const auto bytes = value.toUtf8();
    return {bytes.begin(), bytes.end()};
}

std::string className(CharacterClass value, const LocalizationManager& localization) {
    switch (value) {
        case CharacterClass::WARRIOR: return localization.get("CLASS_WARRIOR");
        case CharacterClass::MAGE: return localization.get("CLASS_MAGE");
        case CharacterClass::ROGUE: return localization.get("CLASS_ROGUE");
        case CharacterClass::CLERIC: return localization.get("CLASS_CLERIC");
    }
    return {};
}

std::string genderName(Gender value, const LocalizationManager& localization) {
    switch (value) {
        case Gender::MALE: return localization.get("GENDER_MALE");
        case Gender::FEMALE: return localization.get("GENDER_FEMALE");
        case Gender::NON_BINARY: return localization.get("GENDER_NON_BINARY");
        case Gender::UNSPECIFIED: return localization.get("GENDER_UNSPECIFIED");
    }
    return {};
}

const char* traitKey(CharacterClass value) {
    switch (value) {
        case CharacterClass::WARRIOR: return "TRAIT_WARRIOR";
        case CharacterClass::MAGE: return "TRAIT_MAGE";
        case CharacterClass::ROGUE: return "TRAIT_ROGUE";
        case CharacterClass::CLERIC: return "TRAIT_CLERIC";
    }
    return "";
}

const char* profileKey(CharacterClass value) {
    switch (value) {
        case CharacterClass::WARRIOR: return "CLASS_PROFILE_WARRIOR";
        case CharacterClass::MAGE: return "CLASS_PROFILE_MAGE";
        case CharacterClass::ROGUE: return "CLASS_PROFILE_ROGUE";
        case CharacterClass::CLERIC: return "CLASS_PROFILE_CLERIC";
    }
    return "";
}

const std::array<Ability, 6> abilities = {
    Ability::STRENGTH, Ability::DEXTERITY, Ability::CONSTITUTION,
    Ability::INTELLIGENCE, Ability::WISDOM, Ability::CHARISMA};

const std::array<const char*, 6> abilityKeys = {
    "CHAR_INFO_STR", "CHAR_INFO_DEX", "CHAR_INFO_CON",
    "CHAR_INFO_INT", "CHAR_INFO_WIS", "CHAR_INFO_CHA"};

int abilityValue(const AbilityScore& scores, Ability ability) {
    switch (ability) {
        case Ability::STRENGTH: return scores.strength;
        case Ability::DEXTERITY: return scores.dexterity;
        case Ability::CONSTITUTION: return scores.constitution;
        case Ability::INTELLIGENCE: return scores.intelligence;
        case Ability::WISDOM: return scores.wisdom;
        case Ability::CHARISMA: return scores.charisma;
    }
    return scores.strength;
}

} // namespace

CharacterCreationState::CharacterCreationState(Game& game)
    : m_game(game), m_draft(SessionRng::global()) {
    m_statusMessage = LocalizationManager::getInstance().get("CREATE_IDENTITY_GUIDE");
}

void CharacterCreationState::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::TextEntered && m_stage == CharacterCreationStage::IDENTITY &&
        m_identityRow == 0) {
        handleTextEntered(event.text.unicode);
        return;
    }
    if (event.type != sf::Event::KeyPressed) return;
    if (m_stage == CharacterCreationStage::IDENTITY) handleIdentityKey(event.key.code);
    else if (m_stage == CharacterCreationStage::ABILITIES) handleAbilityKey(event.key.code);
    else handleConfirmKey(event.key.code);
}

void CharacterCreationState::handleIdentityKey(sf::Keyboard::Key key) {
    auto& localization = LocalizationManager::getInstance();
    if (key == sf::Keyboard::Escape) {
        m_game.getStates().popState();
    } else if (key == sf::Keyboard::Up) {
        m_identityRow = (m_identityRow + 3) % 4;
    } else if (key == sf::Keyboard::Down || key == sf::Keyboard::Tab) {
        m_identityRow = (m_identityRow + 1) % 4;
    } else if (key == sf::Keyboard::BackSpace && m_identityRow == 0 && !m_nameInput.isEmpty()) {
        m_nameInput.erase(m_nameInput.getSize() - 1);
    } else if (key == sf::Keyboard::Left || key == sf::Keyboard::Right) {
        const int direction = key == sf::Keyboard::Right ? 1 : -1;
        if (m_identityRow == 1) {
            const int age = std::clamp(m_draft.identity().age + direction, 18, 80);
            m_draft.setAge(age);
        } else if (m_identityRow == 2) {
            const int current = static_cast<int>(m_draft.identity().gender);
            m_draft.setGender(static_cast<Gender>((current + direction + 3) % 3));
        } else if (m_identityRow == 3) {
            const int current = static_cast<int>(m_draft.identity().characterClass);
            m_draft.setClass(static_cast<CharacterClass>((current + direction + 4) % 4));
        }
    } else if (key == sf::Keyboard::Enter) {
        if (!m_draft.setName(toUtf8(m_nameInput))) {
            m_statusMessage = localization.get("CREATE_NAME_INVALID");
            return;
        }
        m_stage = CharacterCreationStage::ABILITIES;
        m_statusMessage = localization.get("CREATE_ABILITY_GUIDE");
    }
}

void CharacterCreationState::handleAbilityKey(sf::Keyboard::Key key) {
    auto& localization = LocalizationManager::getInstance();
    const Ability selected = abilities[static_cast<std::size_t>(m_abilityRow)];
    if (key == sf::Keyboard::Escape) {
        m_stage = CharacterCreationStage::IDENTITY;
        m_statusMessage = localization.get("CREATE_IDENTITY_GUIDE");
    } else if (key == sf::Keyboard::Up) {
        m_abilityRow = (m_abilityRow + 5) % 6;
    } else if (key == sf::Keyboard::Down || key == sf::Keyboard::Tab) {
        m_abilityRow = (m_abilityRow + 1) % 6;
    } else if (key == sf::Keyboard::Right || key == sf::Keyboard::Add) {
        if (!m_draft.increase(selected)) m_statusMessage = localization.get("CREATE_CANNOT_INCREASE");
    } else if (key == sf::Keyboard::Left || key == sf::Keyboard::Subtract) {
        if (!m_draft.decrease(selected)) m_statusMessage = localization.get("CREATE_CANNOT_DECREASE");
    } else if (key == sf::Keyboard::R) {
        m_draft.reroll();
        m_statusMessage = localization.get("CREATE_REROLLED");
    } else if (key == sf::Keyboard::Enter) {
        if (!m_draft.isReady()) {
            m_statusMessage = localization.get("CREATE_SPEND_ALL");
            return;
        }
        m_stage = CharacterCreationStage::CONFIRM;
        m_statusMessage = localization.get("CREATE_CONFIRM_GUIDE");
    }
}

void CharacterCreationState::handleConfirmKey(sf::Keyboard::Key key) {
    if (m_committedWithWarning && (key == sf::Keyboard::Escape || key == sf::Keyboard::Enter)) {
        m_game.getStates().popState();
        return;
    }
    if (key == sf::Keyboard::Escape) {
        m_stage = CharacterCreationStage::ABILITIES;
        m_statusMessage = LocalizationManager::getInstance().get("CREATE_ABILITY_GUIDE");
    } else if (key == sf::Keyboard::Enter) {
        commitCharacter();
    }
}

void CharacterCreationState::handleTextEntered(sf::Uint32 unicode) {
    if (unicode < 0x20U || unicode == 0x7FU || m_nameInput.getSize() >= 16) return;
    sf::String candidate = m_nameInput;
    candidate += unicode;
    if (RecruitmentDraft::isValidName(toUtf8(candidate)) || unicode == ' ') {
        m_nameInput = std::move(candidate);
    }
}

void CharacterCreationState::commitCharacter() {
    const PartyCheckpoint checkpoint = m_game.getParty().captureCheckpoint();
    auto candidate = m_draft.createCandidate();
    if (!candidate || !m_game.getParty().addMember(candidate)) {
        m_statusMessage = LocalizationManager::getInstance().get("CREATE_PARTY_FULL");
        return;
    }
    const auto result = m_game.getParty().saveToFile();
    if (!result.durabilityConfirmed()) {
        m_statusMessage = LocalizationManager::getInstance().get("CREATE_DURABILITY_UNKNOWN");
        m_committedWithWarning = true;
    } else if (!result) {
        m_game.getParty().restoreCheckpoint(checkpoint);
        m_statusMessage = LocalizationManager::getInstance().get("CREATE_SAVE_FAILED");
    } else {
        m_game.getStates().popState();
    }
}

void CharacterCreationState::update(sf::Time) {}

void CharacterCreationState::draw(sf::RenderWindow& window) {
    auto& localization = LocalizationManager::getInstance();
    const sf::Color neon(51, 255, 51);
    const sf::Color bright(102, 255, 102);
    const sf::Color amber(255, 176, 0);
    const bool large = localization.getTextScale() > 125;
    drawText(window, localization.get("CREATE_TITLE"), 512.0f, large ? 24.0f : 30.0f,
             large ? 18U : 20U, bright, true);
    drawFrame(window, 40.0f, large ? 90.0f : 100.0f, 944.0f, large ? 545.0f : 520.0f, neon);

    std::vector<std::string> lines;
    if (m_stage == CharacterCreationStage::IDENTITY) {
        const auto& identity = m_draft.identity();
        const std::array<std::string, 4> values = {
            toUtf8(m_nameInput) + (m_identityRow == 0 ? "_" : ""),
            std::to_string(identity.age), genderName(identity.gender, localization),
            className(identity.characterClass, localization)};
        const std::array<const char*, 4> keys = {"CREATE_NAME", "CREATE_AGE", "CREATE_GENDER", "CREATE_CLASS"};
        for (int row = 0; row < 4; ++row) {
            lines.push_back(std::string(row == m_identityRow ? "> " : "  ") +
                            localization.get(keys[static_cast<std::size_t>(row)]) + ": " +
                            values[static_cast<std::size_t>(row)]);
        }
        lines.push_back(localization.get("CREATE_TRAIT") + ": " +
                        localization.get(traitKey(identity.characterClass)));
        lines.push_back(localization.get(profileKey(identity.characterClass)));
    } else {
        const auto& base = m_draft.baseAbilities();
        const auto& current = m_draft.abilities();
        for (int row = 0; row < 6; ++row) {
            const Ability ability = abilities[static_cast<std::size_t>(row)];
            std::ostringstream line;
            line << (m_stage == CharacterCreationStage::ABILITIES && row == m_abilityRow ? "> " : "  ")
                 << localization.get(abilityKeys[static_cast<std::size_t>(row)]) << ": "
                 << abilityValue(base, ability) << " -> " << abilityValue(current, ability);
            if (m_stage == CharacterCreationStage::ABILITIES && row == m_abilityRow) {
                line << "  (" << localization.get("CREATE_NEXT_COST") << ": "
                     << m_draft.increaseCost(ability) << ")";
            }
            lines.push_back(line.str());
        }
        lines.push_back(localization.get("CREATE_REMAINING") + ": " +
                        std::to_string(m_draft.remainingPoints()));
        if (m_stage == CharacterCreationStage::CONFIRM) {
            const auto& identity = m_draft.identity();
            lines.insert(lines.begin(), identity.name + " / " + std::to_string(identity.age) + " / " +
                         genderName(identity.gender, localization) + " / " +
                         className(identity.characterClass, localization));
        }
    }

    const float startY = large ? 125.0f : 135.0f;
    const float spacing = large ? 45.0f : 42.0f;
    const unsigned int bodySize = large ? 12U : 14U;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        const bool selected = lines[index].starts_with("> ");
        drawText(window, lines[index], 70.0f, startY + static_cast<float>(index) * spacing,
                 bodySize, selected ? amber : bright);
    }
    drawText(window, m_statusMessage, 512.0f, large ? 660.0f : 650.0f,
             large ? 10U : 12U, amber, true);
}

void CharacterCreationState::drawText(sf::RenderWindow& window, const std::string& text,
                                        float x, float y, unsigned int baseSize,
                                        sf::Color color, bool centered) const {
    sf::Text drawable;
    drawable.setFont(m_game.getFont());
    drawable.setString(sf::String::fromUtf8(text.begin(), text.end()));
    drawable.setCharacterSize(LocalizationManager::getInstance().getScaledTextSize(baseSize));
    drawable.setFillColor(color);
    if (centered) {
        const auto bounds = drawable.getLocalBounds();
        drawable.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    }
    drawable.setPosition(x, y);
    window.draw(drawable);
}

void CharacterCreationState::drawFrame(sf::RenderWindow& window, float x, float y,
                                         float width, float height, sf::Color color) const {
    sf::RectangleShape frame({width, height});
    frame.setPosition(x, y);
    frame.setFillColor(sf::Color::Transparent);
    frame.setOutlineColor(color);
    frame.setOutlineThickness(2.0f);
    window.draw(frame);
}

} // namespace crawl
