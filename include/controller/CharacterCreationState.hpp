#ifndef CHARACTER_CREATION_STATE_HPP
#define CHARACTER_CREATION_STATE_HPP

#include "core/GameState.hpp"
#include "model/RecruitmentDraft.hpp"

#include <SFML/System/String.hpp>
#include <string>

namespace crawl {

class Game;
class ControllerTestAccess;

enum class CharacterCreationStage {
    IDENTITY,
    ABILITIES,
    CONFIRM
};

class CharacterCreationState final : public GameState {
public:
    explicit CharacterCreationState(Game& game);

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    friend class ControllerTestAccess;

    Game& m_game;
    RecruitmentDraft m_draft;
    CharacterCreationStage m_stage = CharacterCreationStage::IDENTITY;
    sf::String m_nameInput;
    int m_identityRow = 0;
    int m_abilityRow = 0;
    bool m_committedWithWarning = false;
    std::string m_statusMessage;

    void handleIdentityKey(sf::Keyboard::Key key);
    void handleAbilityKey(sf::Keyboard::Key key);
    void handleConfirmKey(sf::Keyboard::Key key);
    void handleTextEntered(sf::Uint32 unicode);
    void commitCharacter();
    void drawText(sf::RenderWindow& window, const std::string& text, float x, float y,
                  unsigned int baseSize, sf::Color color, bool centered = false) const;
    void drawFrame(sf::RenderWindow& window, float x, float y, float width, float height,
                   sf::Color color) const;
};

} // namespace crawl

#endif
