#include "controller/GameOverState.hpp"

#include "controller/TitleState.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"

namespace crawl {

GameOverState::GameOverState(Game& game, bool checkpointRestored)
    : m_game(game), m_checkpointRestored(checkpointRestored) {}

void GameOverState::handleInput(const sf::Event& event) {
    if (event.type != sf::Event::KeyPressed) return;
    if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Escape) {
        m_game.getStates().replaceAll(std::make_unique<TitleState>(m_game));
    }
}

void GameOverState::update(sf::Time /*deltaTime*/) {}

void GameOverState::draw(sf::RenderWindow& window) {
    auto& localization = LocalizationManager::getInstance();
    const bool largeText = localization.getTextScale() > 125;
    sf::Text title(localization.getSf("GAME_OVER_TITLE"), m_game.getFont(),
                   localization.getScaledTextSize(largeText ? 24 : 32));
    title.setFillColor(sf::Color(255, 90, 90));
    auto titleBounds = title.getLocalBounds();
    title.setOrigin(titleBounds.left + titleBounds.width / 2.0f,
                    titleBounds.top + titleBounds.height / 2.0f);
    title.setPosition(512.0f, 280.0f);

    const std::string bodyKey = m_checkpointRestored
        ? (largeText ? "GAME_OVER_BODY_LARGE" : "GAME_OVER_BODY")
        : (largeText ? "GAME_OVER_LOAD_ERROR_LARGE" : "GAME_OVER_LOAD_ERROR");
    sf::Text body(localization.getSf(bodyKey), m_game.getFont(),
                  localization.getScaledTextSize(largeText ? 14 : 18));
    body.setFillColor(sf::Color(220, 255, 220));
    auto bodyBounds = body.getLocalBounds();
    body.setOrigin(bodyBounds.left + bodyBounds.width / 2.0f,
                   bodyBounds.top + bodyBounds.height / 2.0f);
    body.setPosition(512.0f, 360.0f);

    sf::Text guide(localization.getSf("GAME_OVER_GUIDE"), m_game.getFont(),
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
