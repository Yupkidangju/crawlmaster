#include "controller/VictoryState.hpp"

#include "controller/TitleState.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"

namespace crawl {

VictoryState::VictoryState(Game& game, bool durabilityUnknown)
    : m_game(game), m_durabilityUnknown(durabilityUnknown) {}

void VictoryState::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed &&
        (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Escape)) {
        m_game.getStates().replaceAll(std::make_unique<TitleState>(m_game));
    }
}

void VictoryState::update(sf::Time /*deltaTime*/) {}

void VictoryState::draw(sf::RenderWindow& window) {
    auto& localization = LocalizationManager::getInstance();
    const bool largeText = localization.getTextScale() > 125;
    sf::Text title(localization.getSf("VICTORY_TITLE"), m_game.getFont(),
                   localization.getScaledTextSize(largeText ? 24 : 32));
    title.setFillColor(sf::Color(120, 255, 160));
    auto titleBounds = title.getLocalBounds();
    title.setOrigin(titleBounds.left + titleBounds.width / 2.0f,
                    titleBounds.top + titleBounds.height / 2.0f);
    title.setPosition(512.0f, 280.0f);

    const std::string bodyKey = m_durabilityUnknown
        ? (largeText ? "VICTORY_DURABILITY_UNKNOWN_LARGE" : "VICTORY_DURABILITY_UNKNOWN")
        : (largeText ? "VICTORY_BODY_LARGE" : "VICTORY_BODY");
    sf::Text body(localization.getSf(bodyKey), m_game.getFont(),
                  localization.getScaledTextSize(largeText ? 14 : 18));
    body.setFillColor(sf::Color(220, 255, 220));
    auto bodyBounds = body.getLocalBounds();
    body.setOrigin(bodyBounds.left + bodyBounds.width / 2.0f,
                   bodyBounds.top + bodyBounds.height / 2.0f);
    body.setPosition(512.0f, 360.0f);

    sf::Text guide(localization.getSf("VICTORY_GUIDE"), m_game.getFont(),
                   localization.getScaledTextSize(14));
    guide.setFillColor(sf::Color(255, 196, 80));
    auto guideBounds = guide.getLocalBounds();
    guide.setOrigin(guideBounds.left + guideBounds.width / 2.0f,
                    guideBounds.top + guideBounds.height / 2.0f);
    guide.setPosition(512.0f, 440.0f);

    window.draw(title);
    window.draw(body);
    window.draw(guide);
}

} // namespace crawl
