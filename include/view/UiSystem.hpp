#ifndef UI_SYSTEM_HPP
#define UI_SYSTEM_HPP

#include <SFML/Graphics.hpp>
#include "core/AssetManager.hpp"
#include "view/PartyHudSnapshot.hpp"
#include "view/UiTheme.hpp"

#include <string>
#include <vector>

namespace crawl {

struct UiButtonSpec {
    sf::FloatRect rect;
    std::string label;
    std::string hotkey;
    bool selected = false;
    bool hovered = false;
    bool enabled = true;
    bool goldAccent = false;
};

struct VfxParticle {
    sf::Vector2f pos;
    sf::Vector2f vel;
    sf::Color color;
    float size = 10.0f;
    float life = 0.5f;
    float maxLife = 0.5f;
    bool isSlash = false;
    float rotation = 0.0f;
};

struct FloatingNumber {
    sf::Vector2f pos;
    std::string text;
    sf::Color color;
    unsigned int fontSize = 20;
    float life = 0.9f;
    float maxLife = 0.9f;
};

class CombatVfxSystem {
public:
    void spawnBurst(sf::Vector2f origin, sf::Color color, int count = 18, float speed = 150.0f);
    void spawnSlash(sf::Vector2f origin, sf::Color color);
    void spawnFloatingText(sf::Vector2f origin, const std::string& text, sf::Color color, unsigned int size = 22);
    void triggerShake(float amplitude, float duration);
    void triggerDamageFlash(float intensity = 0.75f);

    void update(float dt);
    void draw(sf::RenderTarget& target, AssetManager& assets, const sf::Font& font) const;

    [[nodiscard]] sf::Vector2f shakeOffset() const;
    [[nodiscard]] float damageFlash() const { return m_damageFlash; }
    void clear();

private:
    std::vector<VfxParticle> m_particles;
    std::vector<FloatingNumber> m_floats;
    float m_shakeTime = 0.0f;
    float m_shakeDuration = 0.01f;
    float m_shakeAmp = 0.0f;
    float m_damageFlash = 0.0f;
    float m_time = 0.0f;
};

namespace UiSystem {

sf::String toSf(const std::string& utf8);
std::string truncateUtf8ToWidth(const std::string& utf8, const sf::Font& font,
                                unsigned int charSize, float maxWidth);
std::vector<std::string> wrapUtf8Lines(const std::string& utf8, const sf::Font& font,
                                       unsigned int charSize, float maxWidth);

void drawPanel(sf::RenderTarget& target, AssetManager& assets, sf::FloatRect rect,
               bool goldAccent = false, sf::Uint8 alphaOverride = 236);

void drawText(sf::RenderTarget& target, const sf::Font& font, const std::string& utf8,
              sf::Vector2f pos, unsigned int charSize, sf::Color color,
              bool dropShadow = true, float maxWidth = 0.0f);

void drawCenteredText(sf::RenderTarget& target, const sf::Font& font, const std::string& utf8,
                      sf::FloatRect bounds, unsigned int charSize, sf::Color color,
                      bool dropShadow = true);

void drawButton(sf::RenderTarget& target, AssetManager& assets, const sf::Font& font,
                const UiButtonSpec& spec);

void drawGaugeBar(sf::RenderTarget& target, const sf::Font& font, sf::FloatRect rect,
                  float ratio, float trailRatio, sf::Color fillColor, sf::Color trailColor,
                  const std::string& overlayText = "");

void drawBadge(sf::RenderTarget& target, const sf::Font& font, sf::Vector2f pos,
               const std::string& label, sf::Color bgColor, sf::Color textColor,
               unsigned int charSize = 11);

void drawPortraitCard(sf::RenderTarget& target, AssetManager& assets, const sf::Font& font,
                      const PartyHudSlot& slot, sf::FloatRect rect,
                      bool activeTurn = false, bool hovered = false, bool selected = false,
                      float hpTrailRatio = -1.0f);

void drawTooltip(sf::RenderTarget& target, AssetManager& assets, const sf::Font& font,
                 sf::Vector2f anchor, const std::string& title,
                 const std::vector<std::string>& lines,
                 sf::Color titleColor = UiTheme::textGold());

void drawBackdrop(sf::RenderTarget& target, AssetManager& assets, const std::string& bgName,
                  sf::Uint8 dimAlpha = 110);

} // namespace UiSystem

} // namespace crawl

#endif // UI_SYSTEM_HPP
