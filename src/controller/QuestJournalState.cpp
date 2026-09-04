#include "controller/QuestJournalState.hpp"

#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "model/Party.hpp"
#include <algorithm>
#include <sstream>

namespace crawl {

QuestJournalState::QuestJournalState(Game& game) : m_game(game) {
    const auto& font = m_game.getFont();
    m_title.setFont(font);
    m_body.setFont(font);
    m_guide.setFont(font);
    m_title.setPosition(48.0f, 42.0f);
    m_body.setPosition(56.0f, 115.0f);
    m_guide.setPosition(56.0f, 710.0f);
    refresh();
}

void QuestJournalState::handleInput(const sf::Event& event) {
    if (event.type != sf::Event::KeyPressed) return;
    const int count = static_cast<int>(m_game.getParty().getActiveQuests().size());
    if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::Q) {
        m_game.getStates().popState();
    } else if (event.key.code == sf::Keyboard::Up && count > 0) {
        m_selected = (m_selected + count - 1) % count;
    } else if (event.key.code == sf::Keyboard::Down && count > 0) {
        m_selected = (m_selected + 1) % count;
    }
    refresh();
}

void QuestJournalState::update(sf::Time /*deltaTime*/) {}

void QuestJournalState::draw(sf::RenderWindow& window) {
    refresh();
    window.draw(m_title);
    window.draw(m_body);
    window.draw(m_guide);
}

void QuestJournalState::refresh() {
    auto& localization = LocalizationManager::getInstance();
    const auto& font = m_game.getFont();
    m_title.setFont(font);
    m_body.setFont(font);
    m_guide.setFont(font);
    m_title.setCharacterSize(localization.getScaledTextSize(24));
    m_body.setCharacterSize(localization.getScaledTextSize(16));
    m_guide.setCharacterSize(localization.getScaledTextSize(14));
    m_title.setFillColor(sf::Color(51, 255, 51));
    m_body.setFillColor(sf::Color(102, 255, 102));
    m_guide.setFillColor(sf::Color(255, 176, 0));
    m_title.setString(localization.getSf("QUEST_JOURNAL_TITLE"));
    m_guide.setString(localization.getSf("QUEST_JOURNAL_GUIDE"));

    const auto& quests = m_game.getParty().getActiveQuests();
    std::ostringstream body;
    body << localization.format("QUEST_JOURNAL_KEY_ITEMS",
        {{"count", std::to_string(m_game.getParty().getKeyItems().size())}}) << "\n\n";
    for (const auto& keyItem : m_game.getParty().getKeyItems()) {
        if (keyItem == "key_moon_seal") {
            body << "- " << localization.get("KEY_ITEM_KEY_MOON_SEAL_NAME") << "\n";
        }
    }
    if (!m_game.getParty().getKeyItems().empty()) body << "\n";
    if (quests.empty()) {
        body << localization.get("QUEST_JOURNAL_EMPTY");
    } else {
        m_selected = std::clamp(m_selected, 0, static_cast<int>(quests.size()) - 1);
        const bool compact = localization.getTextScale() > 125;
        const std::size_t begin = compact ? static_cast<std::size_t>(m_selected) : 0U;
        const std::size_t end = compact ? begin + 1U : quests.size();
        for (std::size_t index = begin; index < end; ++index) {
            const auto& quest = quests[index];
            if (!quest) continue;
            body << (static_cast<int>(index) == m_selected ? "> " : "  ")
                 << quest->getName() << " ["
                 << localization.get(quest->isReadyToReport()
                    ? "QUEST_STATUS_READY" : "QUEST_STATUS_ACTIVE") << "]\n"
                 << "  ";
            if (quest->getTargetFloor() > 0) {
                body << localization.format("DUNGEON_FLOOR_LABEL",
                    {{"floor", std::to_string(quest->getTargetFloor())}}) << " - ";
            }
            else body << localization.get("QUEST_LEGACY_OBJECTIVE") << " - ";
            body << quest->getDescription() << "\n\n";
        }
    }
    const std::string bodyText = body.str();
    m_body.setString(sf::String::fromUtf8(bodyText.begin(), bodyText.end()));
}

} // namespace crawl
