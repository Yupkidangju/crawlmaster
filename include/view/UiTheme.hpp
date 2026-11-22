#ifndef UI_THEME_HPP
#define UI_THEME_HPP

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector3.hpp>
#include "core/LocalizationManager.hpp"
#include "model/DungeonMap.hpp"
#include "model/Equipment.hpp"
#include "model/Monster.hpp"

#include <string>

namespace crawl {

struct BiomeVisualProfile {
    sf::Color fogColor;
    sf::Color torchColor;
    sf::Color ambientColor;
    sf::Color ceilingTint;
    sf::Color particleColor;
    sf::Vector3f postTint;
    float fogDensity = 0.24f;
    float wetness = 0.2f;
    float emissiveBoost = 0.0f;
};

namespace UiTheme {

inline bool highContrast() {
    return LocalizationManager::getInstance().getHighContrast();
}

inline sf::Color bgVoid() { return highContrast() ? sf::Color(0, 0, 0) : sf::Color(9, 9, 13); }
inline sf::Color panelBg() { return highContrast() ? sf::Color(10, 11, 16, 245) : sf::Color(19, 20, 28, 232); }
inline sf::Color panelAlt() { return highContrast() ? sf::Color(20, 22, 32, 245) : sf::Color(29, 31, 44, 236); }
inline sf::Color panelHover() { return highContrast() ? sf::Color(44, 48, 66, 250) : sf::Color(44, 42, 54, 242); }
inline sf::Color borderBronze() { return highContrast() ? sf::Color(216, 184, 106) : sf::Color(122, 99, 67); }
inline sf::Color borderGold() { return highContrast() ? sf::Color(255, 224, 102) : sf::Color(212, 175, 95); }
inline sf::Color textPrimary() { return highContrast() ? sf::Color(255, 255, 255) : sf::Color(237, 230, 214); }
inline sf::Color textMuted() { return highContrast() ? sf::Color(212, 207, 196) : sf::Color(158, 149, 132); }
inline sf::Color textGold() { return highContrast() ? sf::Color(255, 230, 112) : sf::Color(232, 194, 108); }
inline sf::Color hpCrimson() { return highContrast() ? sf::Color(255, 68, 68) : sf::Color(200, 50, 50); }
inline sf::Color hpTrail() { return highContrast() ? sf::Color(255, 204, 51) : sf::Color(232, 157, 60); }
inline sf::Color spAzure() { return highContrast() ? sf::Color(85, 170, 255) : sf::Color(59, 125, 216); }
inline sf::Color staggerAmber() { return highContrast() ? sf::Color(255, 210, 77) : sf::Color(224, 160, 48); }
inline sf::Color breakMagenta() { return highContrast() ? sf::Color(255, 102, 178) : sf::Color(232, 67, 147); }
inline sf::Color statPos() { return highContrast() ? sf::Color(112, 255, 144) : sf::Color(88, 201, 114); }
inline sf::Color statNeg() { return highContrast() ? sf::Color(255, 102, 102) : sf::Color(224, 82, 82); }
inline sf::Color cyanAccent() { return highContrast() ? sf::Color(110, 245, 255) : sf::Color(84, 212, 230); }

inline sf::Color rarityColor(ItemRarity rarity) {
    switch (rarity) {
        case ItemRarity::COMMON:    return sf::Color(214, 207, 194);
        case ItemRarity::UNCOMMON:  return sf::Color(88, 201, 114);
        case ItemRarity::RARE:      return sf::Color(77, 166, 255);
        case ItemRarity::EPIC:      return sf::Color(188, 114, 255);
        case ItemRarity::LEGENDARY: return sf::Color(255, 184, 51);
    }
    return textPrimary();
}

inline sf::Color elementColor(DamageType type) {
    switch (type) {
        case DamageType::SLASHING:
        case DamageType::PIERCING:
        case DamageType::BLUDGEONING: return sf::Color(200, 196, 188);
        case DamageType::FIRE:        return sf::Color(255, 107, 53);
        case DamageType::ICE:         return sf::Color(94, 200, 255);
        case DamageType::LIGHTNING:   return sf::Color(255, 224, 85);
        case DamageType::HOLY:        return sf::Color(255, 240, 168);
        case DamageType::POISON:      return sf::Color(108, 212, 72);
        case DamageType::ARCANE:      return sf::Color(192, 108, 255);
    }
    return textPrimary();
}

inline std::string elementShortCode(DamageType type) {
    switch (type) {
        case DamageType::SLASHING:    return "SLH";
        case DamageType::PIERCING:    return "PRC";
        case DamageType::BLUDGEONING: return "BLT";
        case DamageType::FIRE:        return "FIR";
        case DamageType::ICE:         return "ICE";
        case DamageType::LIGHTNING:   return "LTN";
        case DamageType::HOLY:        return "HLY";
        case DamageType::POISON:      return "PSN";
        case DamageType::ARCANE:      return "ARC";
    }
    return "PHY";
}

inline sf::Color intentColor(EnemyIntentKind kind) {
    switch (kind) {
        case EnemyIntentKind::ATTACK:       return sf::Color(225, 92, 78);
        case EnemyIntentKind::HEAVY_STRIKE: return sf::Color(255, 68, 52);
        case EnemyIntentKind::AOE_BREATH:   return sf::Color(255, 128, 36);
        case EnemyIntentKind::CHARGING:     return sf::Color(255, 192, 56);
        case EnemyIntentKind::SPELL:        return sf::Color(186, 108, 255);
        case EnemyIntentKind::DEBUFF:       return sf::Color(112, 214, 78);
        case EnemyIntentKind::BUFF:         return sf::Color(92, 188, 255);
        case EnemyIntentKind::DEFEND:       return sf::Color(148, 178, 214);
    }
    return textPrimary();
}

inline BiomeVisualProfile biomeProfile(BiomeType biome) {
    switch (biome) {
        case BiomeType::CATACOMBS:
            return {sf::Color(14, 18, 16), sf::Color(255, 184, 98), sf::Color(52, 58, 52),
                    sf::Color(18, 22, 18), sf::Color(225, 206, 152, 140),
                    {1.02f, 1.00f, 0.94f}, 0.24f, 0.22f, 0.08f};
        case BiomeType::CISTERN:
            return {sf::Color(10, 22, 28), sf::Color(146, 224, 218), sf::Color(44, 68, 78),
                    sf::Color(12, 24, 30), sf::Color(112, 232, 225, 150),
                    {0.92f, 1.03f, 1.08f}, 0.25f, 0.68f, 0.16f};
        case BiomeType::OSSUARY:
            return {sf::Color(18, 14, 24), sf::Color(214, 168, 245), sf::Color(62, 54, 72),
                    sf::Color(22, 18, 30), sf::Color(196, 148, 255, 150),
                    {1.00f, 0.94f, 1.08f}, 0.25f, 0.18f, 0.18f};
        case BiomeType::FOUNDRY:
            return {sf::Color(24, 12, 10), sf::Color(255, 144, 64), sf::Color(68, 42, 34),
                    sf::Color(26, 14, 12), sf::Color(255, 142, 48, 185),
                    {1.10f, 0.96f, 0.88f}, 0.23f, 0.42f, 0.55f};
        case BiomeType::SANCTUM:
            return {sf::Color(22, 10, 14), sf::Color(255, 198, 92), sf::Color(66, 44, 48),
                    sf::Color(20, 12, 16), sf::Color(255, 198, 86, 185),
                    {1.08f, 0.98f, 0.92f}, 0.22f, 0.48f, 0.45f};
    }
    return biomeProfile(BiomeType::CATACOMBS);
}

} // namespace UiTheme

} // namespace crawl

#endif // UI_THEME_HPP
