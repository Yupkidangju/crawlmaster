#include "controller/ShutdownState.hpp"

#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "model/Party.hpp"

namespace crawl {

ShutdownState::ShutdownState(Game& game, PersistenceStatus status)
    : m_game(game), m_status(status) {
    const auto& font = m_game.getFont();
    m_title.setFont(font);
    m_body.setFont(font);
    m_guide.setFont(font);
    m_title.setPosition(512.0f, 230.0f);
    m_body.setPosition(512.0f, 340.0f);
    m_guide.setPosition(512.0f, 470.0f);
    refresh();
}

void ShutdownState::handleInput(const sf::Event& event) {
    if (event.type != sf::Event::KeyPressed) return;
    if (event.key.code == sf::Keyboard::Escape) {
        m_game.completeShutdown();
        return;
    }
    if (event.key.code == sf::Keyboard::Enter) {
        const bool retryingRecovery = m_status == PersistenceStatus::RecoveryPending;
        const auto result = retryingRecovery
            ? m_game.getParty().loadFromFile()
            : m_game.getParty().saveToFile();
        m_status = result.status;
        if (retryingRecovery && !result.succeeded()) {
            m_game.getParty().markRecoveryPending();
            m_status = PersistenceStatus::RecoveryPending;
        }
        if ((!retryingRecovery && result.status == PersistenceStatus::Saved) ||
            (retryingRecovery && result.succeeded())) {
            m_game.completeShutdown();
            return;
        }
        refresh();
    }
}

void ShutdownState::update(sf::Time /*deltaTime*/) {}

void ShutdownState::draw(sf::RenderWindow& window) {
    refresh();
    window.draw(m_title);
    window.draw(m_body);
    window.draw(m_guide);
}

void ShutdownState::refresh() {
    auto& localization = LocalizationManager::getInstance();
    const auto& font = m_game.getFont();
    for (sf::Text* text : {&m_title, &m_body, &m_guide}) text->setFont(font);
    m_title.setCharacterSize(localization.getScaledTextSize(24));
    m_body.setCharacterSize(localization.getScaledTextSize(16));
    m_guide.setCharacterSize(localization.getScaledTextSize(14));
    m_title.setFillColor(sf::Color(255, 176, 0));
    m_body.setFillColor(sf::Color(255, 102, 102));
    m_guide.setFillColor(sf::Color(102, 255, 102));
    m_title.setString(localization.getSf("SHUTDOWN_TITLE"));
    const bool recoveryPending = m_status == PersistenceStatus::RecoveryPending;
    const char* bodyKey = "SHUTDOWN_SAVE_FAILED";
    if (recoveryPending) {
        bodyKey = "SHUTDOWN_RECOVERY_PENDING";
    } else if (m_status == PersistenceStatus::CommittedDurabilityUnknown) {
        bodyKey = "SHUTDOWN_DURABILITY_UNKNOWN";
    }
    m_body.setString(localization.getSf(bodyKey));
    m_guide.setString(localization.getSf(recoveryPending
        ? "SHUTDOWN_RECOVERY_GUIDE" : "SHUTDOWN_GUIDE"));
    for (sf::Text* text : {&m_title, &m_body, &m_guide}) {
        const auto bounds = text->getLocalBounds();
        text->setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    }
}

} // namespace crawl
