#include "view/UiSystem.hpp"
#include "core/Game.hpp"

#include <algorithm>
#include <cmath>

namespace crawl {

namespace {

constexpr float PI_F = 3.14159265f;

} // namespace

void CombatVfxSystem::spawnBurst(sf::Vector2f origin, sf::Color color, int count, float speed) {
    for (int i = 0; i < count; ++i) {
        const float angle = (2.0f * PI_F * static_cast<float>(i)) / static_cast<float>(std::max(1, count)) +
                            static_cast<float>((i * 37) % 100) * 0.008f;
        const float spd = speed * (0.45f + static_cast<float>((i * 53) % 100) * 0.0085f);
        VfxParticle p;
        p.pos = origin;
        p.vel = {std::cos(angle) * spd, std::sin(angle) * spd - 24.0f};
        p.color = color;
        p.size = 14.0f + static_cast<float>((i * 19) % 16);
        p.maxLife = 0.36f + static_cast<float>((i * 29) % 100) * 0.003f;
        p.life = p.maxLife;
        m_particles.push_back(p);
    }
}

void CombatVfxSystem::spawnSlash(sf::Vector2f origin, sf::Color color) {
    VfxParticle s;
    s.pos = origin;
    s.vel = {0.0f, 0.0f};
    s.color = color;
    s.size = 135.0f;
    s.maxLife = 0.24f;
    s.life = s.maxLife;
    s.isSlash = true;
    s.rotation = -28.0f + std::sin(m_time * 17.0f) * 24.0f;
    m_particles.push_back(s);
    spawnBurst(origin, color, 14, 170.0f);
}

void CombatVfxSystem::spawnFloatingText(sf::Vector2f origin, const std::string& text,
                                        sf::Color color, unsigned int size) {
    FloatingNumber f;
    f.pos = origin;
    f.text = text;
    f.color = color;
    f.fontSize = size;
    f.maxLife = 0.92f;
    f.life = f.maxLife;
    m_floats.push_back(std::move(f));
}

void CombatVfxSystem::triggerShake(float amplitude, float duration) {
    m_shakeAmp = std::max(m_shakeAmp, amplitude);
    m_shakeDuration = std::max(0.05f, duration);
    m_shakeTime = std::max(m_shakeTime, duration);
}

void CombatVfxSystem::triggerDamageFlash(float intensity) {
    m_damageFlash = std::max(m_damageFlash, std::clamp(intensity, 0.0f, 1.0f));
}

void CombatVfxSystem::update(float dt) {
    m_time += dt;
    if (m_shakeTime > 0.0f) {
        m_shakeTime = std::max(0.0f, m_shakeTime - dt);
        if (m_shakeTime <= 0.0f) m_shakeAmp = 0.0f;
    }
    if (m_damageFlash > 0.0f) {
        m_damageFlash = std::max(0.0f, m_damageFlash - dt * 2.4f);
    }

    for (auto& p : m_particles) {
        p.life -= dt;
        p.pos += p.vel * dt;
        if (!p.isSlash) p.vel.y += 180.0f * dt;
    }
    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(), [](const VfxParticle& p) { return p.life <= 0.0f; }),
        m_particles.end());

    for (auto& f : m_floats) {
        f.life -= dt;
        f.pos.y -= 44.0f * dt;
    }
    m_floats.erase(
        std::remove_if(m_floats.begin(), m_floats.end(), [](const FloatingNumber& f) { return f.life <= 0.0f; }),
        m_floats.end());
}

void CombatVfxSystem::draw(sf::RenderTarget& target, AssetManager& assets, const sf::Font& font) const {
    const sf::Texture* glowTex = assets.getUiTexture("particle_glow");
    const sf::Texture* slashTex = assets.getUiTexture("particle_slash");

    for (const auto& p : m_particles) {
        const float t = std::clamp(p.life / std::max(0.01f, p.maxLife), 0.0f, 1.0f);
        sf::Color col = p.color;
        col.a = static_cast<sf::Uint8>(static_cast<float>(col.a) * t);

        if (p.isSlash && slashTex) {
            sf::Sprite sp(*slashTex);
            const auto sz = slashTex->getSize();
            sp.setOrigin(static_cast<float>(sz.x) * 0.5f, static_cast<float>(sz.y) * 0.5f);
            const float sc = (p.size / static_cast<float>(sz.x)) * (1.25f - t * 0.35f);
            sp.setScale(sc, sc);
            sp.setRotation(p.rotation);
            sp.setPosition(p.pos);
            sp.setColor(col);
            target.draw(sp, sf::BlendAdd);
        } else if (glowTex) {
            sf::Sprite sp(*glowTex);
            const auto sz = glowTex->getSize();
            sp.setOrigin(static_cast<float>(sz.x) * 0.5f, static_cast<float>(sz.y) * 0.5f);
            const float sc = (p.size / static_cast<float>(sz.x)) * (0.4f + 0.6f * t);
            sp.setScale(sc, sc);
            sp.setPosition(p.pos);
            sp.setColor(col);
            target.draw(sp, sf::BlendAdd);
        } else {
            sf::CircleShape dot(p.size * 0.25f * t);
            dot.setOrigin(dot.getRadius(), dot.getRadius());
            dot.setPosition(p.pos);
            dot.setFillColor(col);
            target.draw(dot);
        }
    }

    for (const auto& f : m_floats) {
        const float t = std::clamp(f.life / std::max(0.01f, f.maxLife), 0.0f, 1.0f);
        sf::Color c = f.color;
        c.a = static_cast<sf::Uint8>(255.0f * std::min(1.0f, t * 1.6f));
        sf::Text txt;
        txt.setFont(font);
        txt.setCharacterSize(f.fontSize);
        txt.setString(UiSystem::toSf(f.text));
        const auto b = txt.getLocalBounds();
        txt.setOrigin(b.left + b.width * 0.5f, b.top + b.height * 0.5f);

        sf::Text shadow = txt;
        shadow.setFillColor(sf::Color(0, 0, 0, c.a));
        shadow.setPosition(f.pos.x + 2.0f, f.pos.y + 2.0f);
        target.draw(shadow);

        txt.setFillColor(c);
        txt.setPosition(f.pos);
        target.draw(txt);
    }
}

sf::Vector2f CombatVfxSystem::shakeOffset() const {
    if (m_shakeTime <= 0.0f || m_shakeAmp <= 0.0f) return {0.0f, 0.0f};
    const float decay = m_shakeTime / std::max(0.01f, m_shakeDuration);
    return {
        std::sin(m_time * 68.0f) * m_shakeAmp * decay,
        std::cos(m_time * 54.0f) * m_shakeAmp * decay * 0.7f
    };
}

void CombatVfxSystem::clear() {
    m_particles.clear();
    m_floats.clear();
    m_shakeTime = 0.0f;
    m_shakeAmp = 0.0f;
    m_damageFlash = 0.0f;
}

namespace UiSystem {

sf::String toSf(const std::string& utf8) {
    return sf::String::fromUtf8(utf8.begin(), utf8.end());
}

std::string truncateUtf8ToWidth(const std::string& utf8, const sf::Font& font,
                                unsigned int charSize, float maxWidth) {
    if (maxWidth <= 8.0f || utf8.empty()) return utf8;
    sf::Text measure;
    measure.setFont(font);
    measure.setCharacterSize(charSize);
    const sf::String full = toSf(utf8);
    measure.setString(full);
    if (measure.getLocalBounds().width <= maxWidth) return utf8;

    sf::String clipped;
    const sf::String ellipsis = "...";
    for (std::size_t i = 0; i < full.getSize(); ++i) {
        sf::String candidate = clipped + full[i] + ellipsis;
        measure.setString(candidate);
        if (measure.getLocalBounds().width > maxWidth) break;
        clipped += full[i];
    }
    clipped += ellipsis;
    const auto bytes = clipped.toUtf8();
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

std::vector<std::string> wrapUtf8Lines(const std::string& utf8, const sf::Font& font,
                                       unsigned int charSize, float maxWidth) {
    std::vector<std::string> out;
    if (utf8.empty()) return out;
    if (maxWidth <= 20.0f) {
        out.push_back(utf8);
        return out;
    }

    sf::Text measure;
    measure.setFont(font);
    measure.setCharacterSize(charSize);
    const sf::String src = toSf(utf8);
    sf::String line;
    for (std::size_t i = 0; i < src.getSize(); ++i) {
        const sf::Uint32 ch = src[i];
        if (ch == '\n') {
            const auto b = line.toUtf8();
            out.emplace_back(reinterpret_cast<const char*>(b.data()), b.size());
            line.clear();
            continue;
        }
        sf::String cand = line + ch;
        measure.setString(cand);
        if (measure.getLocalBounds().width > maxWidth && !line.isEmpty()) {
            const auto b = line.toUtf8();
            out.emplace_back(reinterpret_cast<const char*>(b.data()), b.size());
            line.clear();
            if (ch != ' ') line += ch;
        } else {
            line = cand;
        }
    }
    if (!line.isEmpty()) {
        const auto b = line.toUtf8();
        out.emplace_back(reinterpret_cast<const char*>(b.data()), b.size());
    }
    return out;
}

void drawPanel(sf::RenderTarget& target, AssetManager& assets, sf::FloatRect rect,
               bool goldAccent, sf::Uint8 alphaOverride) {
    const sf::Texture* tex = assets.getUiTexture(goldAccent ? "panel_gold_9slice" : "panel_9slice");
    if (tex && !UiTheme::highContrast()) {
        // 9-슬라이스 정밀 분할 (모서리 리벳과 테두리 왜곡 방지)
        const float b = 14.0f;
        const auto sz = tex->getSize();
        const float tw = static_cast<float>(sz.x);
        const float th = static_cast<float>(sz.y);
        const float x[4] = {rect.left, rect.left + b, rect.left + rect.width - b, rect.left + rect.width};
        const float y[4] = {rect.top, rect.top + b, rect.top + rect.height - b, rect.top + rect.height};
        const float u[4] = {0.0f, b, tw - b, tw};
        const float v[4] = {0.0f, b, th - b, th};

        sf::VertexArray va(sf::Quads, 9 * 4);
        const sf::Color tint(255, 255, 255, alphaOverride);
        std::size_t idx = 0;
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                va[idx++] = sf::Vertex({x[col], y[row]}, tint, {u[col], v[row]});
                va[idx++] = sf::Vertex({x[col + 1], y[row]}, tint, {u[col + 1], v[row]});
                va[idx++] = sf::Vertex({x[col + 1], y[row + 1]}, tint, {u[col + 1], v[row + 1]});
                va[idx++] = sf::Vertex({x[col], y[row + 1]}, tint, {u[col], v[row + 1]});
            }
        }
        target.draw(va, sf::RenderStates(tex));
        return;
    }

    sf::RectangleShape shape({rect.width, rect.height});
    shape.setPosition(rect.left, rect.top);
    sf::Color bg = UiTheme::panelBg();
    bg.a = alphaOverride;
    shape.setFillColor(bg);
    shape.setOutlineThickness(goldAccent ? 2.5f : 1.5f);
    shape.setOutlineColor(goldAccent ? UiTheme::borderGold() : UiTheme::borderBronze());
    target.draw(shape);
}

void drawText(sf::RenderTarget& target, const sf::Font& font, const std::string& utf8,
              sf::Vector2f pos, unsigned int charSize, sf::Color color,
              bool dropShadow, float maxWidth) {
    const std::string finalStr = maxWidth > 0.0f
        ? truncateUtf8ToWidth(utf8, font, charSize, maxWidth)
        : utf8;
    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(charSize);
    text.setString(toSf(finalStr));
    if (dropShadow) {
        sf::Text sh = text;
        sh.setFillColor(sf::Color(0, 0, 0, std::min<sf::Uint8>(210, color.a)));
        sh.setPosition(std::floor(pos.x) + 1.5f, std::floor(pos.y) + 1.5f);
        target.draw(sh);
    }
    text.setFillColor(color);
    text.setPosition(std::floor(pos.x), std::floor(pos.y));
    target.draw(text);
}

void drawCenteredText(sf::RenderTarget& target, const sf::Font& font, const std::string& utf8,
                      sf::FloatRect bounds, unsigned int charSize, sf::Color color,
                      bool dropShadow) {
    const std::string finalStr = truncateUtf8ToWidth(utf8, font, charSize, std::max(24.0f, bounds.width - 6.0f));
    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(charSize);
    text.setString(toSf(finalStr));
    const auto b = text.getLocalBounds();
    const float px = std::floor(bounds.left + (bounds.width - b.width) * 0.5f - b.left);
    const float py = std::floor(bounds.top + (bounds.height - b.height) * 0.5f - b.top - 1.0f);
    if (dropShadow) {
        sf::Text sh = text;
        sh.setFillColor(sf::Color(0, 0, 0, 210));
        sh.setPosition(px + 1.5f, py + 1.5f);
        target.draw(sh);
    }
    text.setFillColor(color);
    text.setPosition(px, py);
    target.draw(text);
}

void drawButton(sf::RenderTarget& target, AssetManager& /*assets*/, const sf::Font& font,
                const UiButtonSpec& spec) {
    const bool lit = spec.enabled && (spec.hovered || spec.selected);
    sf::RectangleShape bg({spec.rect.width, spec.rect.height});
    bg.setPosition(spec.rect.left, spec.rect.top);
    if (!spec.enabled) {
        bg.setFillColor(sf::Color(18, 19, 24, 175));
        bg.setOutlineColor(sf::Color(58, 54, 48, 160));
        bg.setOutlineThickness(1.0f);
    } else if (lit) {
        bg.setFillColor(UiTheme::panelHover());
        bg.setOutlineColor(UiTheme::borderGold());
        bg.setOutlineThickness(2.0f);
    } else {
        bg.setFillColor(UiTheme::panelAlt());
        bg.setOutlineColor(spec.goldAccent ? UiTheme::borderGold() : UiTheme::borderBronze());
        bg.setOutlineThickness(1.5f);
    }
    target.draw(bg);

    // 좌측 강조 바 (선택/호버 시)
    if (lit) {
        sf::RectangleShape accent({4.0f, spec.rect.height - 6.0f});
        accent.setPosition(spec.rect.left + 3.0f, spec.rect.top + 3.0f);
        accent.setFillColor(UiTheme::borderGold());
        target.draw(accent);
    }

    const unsigned int fontSz = std::min(18U, LocalizationManager::getInstance().getScaledTextSize(14));
    const sf::Color labelCol = !spec.enabled ? sf::Color(102, 98, 92)
                                             : (lit ? UiTheme::textGold() : UiTheme::textPrimary());

    if (!spec.hotkey.empty()) {
        sf::Text hkMeasure;
        hkMeasure.setFont(font);
        hkMeasure.setCharacterSize(12);
        hkMeasure.setString(toSf(spec.hotkey));
        const float badgeW = std::max(30.0f, std::ceil(hkMeasure.getLocalBounds().width) + 14.0f);
        sf::RectangleShape kb({badgeW, spec.rect.height - 10.0f});
        kb.setPosition(spec.rect.left + 9.0f, spec.rect.top + 5.0f);
        kb.setFillColor(sf::Color(12, 13, 18, 220));
        kb.setOutlineThickness(1.0f);
        kb.setOutlineColor(lit ? UiTheme::borderGold() : UiTheme::borderBronze());
        target.draw(kb);
        drawCenteredText(target, font, spec.hotkey,
                         {spec.rect.left + 9.0f, spec.rect.top + 5.0f, badgeW, spec.rect.height - 10.0f},
                         12, lit ? UiTheme::textGold() : UiTheme::textMuted(), false);

        const float textLeft = spec.rect.left + 12.0f + badgeW;
        drawCenteredText(target, font, spec.label,
                         {textLeft, spec.rect.top, std::max(24.0f, spec.rect.left + spec.rect.width - textLeft - 6.0f), spec.rect.height},
                         fontSz, labelCol, true);
    } else {
        drawCenteredText(target, font, spec.label, spec.rect, fontSz, labelCol, true);
    }
}

void drawGaugeBar(sf::RenderTarget& target, const sf::Font& font, sf::FloatRect rect,
                  float ratio, float trailRatio, sf::Color fillColor, sf::Color trailColor,
                  const std::string& overlayText) {
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    trailRatio = std::clamp(std::max(ratio, trailRatio), 0.0f, 1.0f);

    sf::RectangleShape bg({rect.width, rect.height});
    bg.setPosition(rect.left, rect.top);
    bg.setFillColor(sf::Color(12, 12, 16, 230));
    bg.setOutlineThickness(1.0f);
    bg.setOutlineColor(sf::Color(76, 66, 52));
    target.draw(bg);

    if (trailRatio > ratio + 0.005f) {
        sf::RectangleShape trail({(rect.width - 2.0f) * trailRatio, rect.height - 2.0f});
        trail.setPosition(rect.left + 1.0f, rect.top + 1.0f);
        trail.setFillColor(trailColor);
        target.draw(trail);
    }

    if (ratio > 0.001f) {
        sf::RectangleShape fill({(rect.width - 2.0f) * ratio, rect.height - 2.0f});
        fill.setPosition(rect.left + 1.0f, rect.top + 1.0f);
        fill.setFillColor(fillColor);
        target.draw(fill);

        // 상단 광택 하이라이트 라인
        sf::RectangleShape sheen({(rect.width - 2.0f) * ratio, std::max(2.0f, rect.height * 0.28f)});
        sheen.setPosition(rect.left + 1.0f, rect.top + 1.0f);
        sheen.setFillColor(sf::Color(255, 255, 255, 42));
        target.draw(sheen);
    }

    if (!overlayText.empty()) {
        drawCenteredText(target, font, overlayText, rect,
                         static_cast<unsigned int>(std::clamp(rect.height - 4.0f, 10.0f, 14.0f)),
                         sf::Color(248, 245, 238), true);
    }
}

void drawBadge(sf::RenderTarget& target, const sf::Font& font, sf::Vector2f pos,
               const std::string& label, sf::Color bgColor, sf::Color textColor,
               unsigned int charSize) {
    sf::Text measure;
    measure.setFont(font);
    measure.setCharacterSize(charSize);
    measure.setString(toSf(label));
    const float w = std::max(26.0f, std::ceil(measure.getLocalBounds().width) + 16.0f);
    const float h = static_cast<float>(charSize) + 7.0f;

    sf::RectangleShape pill({w, h});
    pill.setPosition(pos);
    pill.setFillColor(bgColor);
    pill.setOutlineThickness(1.0f);
    pill.setOutlineColor(sf::Color(textColor.r, textColor.g, textColor.b, 190));
    target.draw(pill);
    drawCenteredText(target, font, label, {pos.x, pos.y, w, h}, charSize, textColor, false);
}

void drawPortraitCard(sf::RenderTarget& target, AssetManager& assets, const sf::Font& font,
                      const PartyHudSlot& slot, sf::FloatRect rect,
                      bool activeTurn, bool hovered, bool selected, float hpTrailRatio) {
    const bool highlight = activeTurn || selected;
    drawPanel(target, assets, rect, highlight, hovered ? 248 : 234);

    // 좌측 정사각형 유화 초상화
    const float pad = 9.0f;
    const float portraitSz = std::min(rect.height - pad * 2.0f, 88.0f);
    const sf::FloatRect pRect(rect.left + pad, rect.top + (rect.height - portraitSz) * 0.5f,
                              portraitSz, portraitSz);

    if (const sf::Texture* tex = assets.getPortraitTexture(slot.classSlug)) {
        sf::Sprite sp(*tex);
        const auto sz = tex->getSize();
        sp.setScale(portraitSz / static_cast<float>(sz.x), portraitSz / static_cast<float>(sz.y));
        sp.setPosition(pRect.left, pRect.top);
        if (slot.isDead) sp.setColor(sf::Color(110, 95, 95, 220));
        target.draw(sp);
    } else {
        sf::RectangleShape ph({portraitSz, portraitSz});
        ph.setPosition(pRect.left, pRect.top);
        ph.setFillColor(sf::Color(34, 32, 42));
        target.draw(ph);
    }

    sf::RectangleShape pFrame({portraitSz, portraitSz});
    pFrame.setPosition(pRect.left, pRect.top);
    pFrame.setFillColor(sf::Color::Transparent);
    pFrame.setOutlineThickness(highlight ? 2.0f : 1.5f);
    pFrame.setOutlineColor(slot.isDead ? UiTheme::hpCrimson()
                                       : (highlight ? UiTheme::borderGold() : UiTheme::borderBronze()));
    target.draw(pFrame);

    // 좌상단 전열/후열(F/B) 배지
    const std::string rowTag = (slot.row == CombatRow::FRONT) ? "F" : "B";
    drawBadge(target, font, {pRect.left + 2.0f, pRect.top + 2.0f}, rowTag,
              slot.row == CombatRow::FRONT ? sf::Color(115, 42, 34, 230) : sf::Color(36, 74, 118, 230),
              sf::Color(248, 240, 220), 10);

    // 우측 정보 영역 (이름 좌측 + 레벨 우측 분리 배치로 좁은 카드에서도 Lv 표시 보장)
    const float ix = pRect.left + portraitSz + 8.0f;
    const float iw = rect.left + rect.width - ix - pad;
    const std::string lvStr = "Lv." + std::to_string(slot.level);
    sf::Text lvMeasure;
    lvMeasure.setFont(font);
    lvMeasure.setCharacterSize(12);
    lvMeasure.setString(toSf(lvStr));
    const float lvW = std::ceil(lvMeasure.getLocalBounds().width) + 4.0f;
    const sf::Color hdrCol = slot.isDead ? UiTheme::hpCrimson()
                                         : (highlight ? UiTheme::textGold() : UiTheme::textPrimary());
    drawText(target, font, slot.name, {ix, rect.top + 8.0f}, 13, hdrCol, true, std::max(36.0f, iw - lvW - 4.0f));
    drawText(target, font, lvStr, {ix + iw - lvW, rect.top + 9.0f}, 12,
             highlight ? UiTheme::textGold() : UiTheme::textMuted(), true);

    const float hpRatio = slot.maxHp > 0 ? static_cast<float>(slot.hp) / static_cast<float>(slot.maxHp) : 0.0f;
    const float trail = hpTrailRatio >= 0.0f ? hpTrailRatio : hpRatio;
    drawGaugeBar(target, font, {ix, rect.top + 30.0f, iw, 17.0f},
                 hpRatio, trail, UiTheme::hpCrimson(), UiTheme::hpTrail(),
                 "HP " + std::to_string(slot.hp) + "/" + std::to_string(slot.maxHp));

    if (slot.maxSpellSlots > 0) {
        const float spRatio = static_cast<float>(slot.spellSlots) / static_cast<float>(slot.maxSpellSlots);
        drawGaugeBar(target, font, {ix, rect.top + 51.0f, iw, 16.0f},
                     spRatio, spRatio, UiTheme::spAzure(), UiTheme::spAzure(),
                     "SP " + std::to_string(slot.spellSlots) + "/" + std::to_string(slot.maxSpellSlots));
    } else {
        drawText(target, font, "AC " + std::to_string(slot.ac), {ix, rect.top + 51.0f},
                 12, UiTheme::textMuted(), false, iw);
    }

    // 상태이상 / 버프 / 방어 배지 줄
    float bx = ix;
    const float by = rect.top + rect.height - 24.0f;
    auto pushTag = [&](const std::string& tag, sf::Color bg, sf::Color fg) {
        if (bx + 36.0f > rect.left + rect.width - pad) return;
        drawBadge(target, font, {bx, by}, tag, bg, fg, 10);
        bx += 36.0f;
    };
    if (slot.isDead) pushTag("DEAD", sf::Color(92, 16, 16), sf::Color(255, 190, 190));
    if (slot.isGuarding) pushTag("GRD", sf::Color(36, 72, 118), sf::Color(200, 232, 255));
    if (slot.tauntTurns > 0) pushTag("TNT", sf::Color(126, 62, 24), sf::Color(255, 224, 150));
    if (slot.blessTurns > 0) pushTag("BLS", sf::Color(112, 96, 30), sf::Color(255, 244, 172));
    if (slot.wardTurns > 0) pushTag("WRD", sf::Color(54, 88, 128), sf::Color(214, 240, 255));
    if (slot.poisonTurns > 0) pushTag("PSN", sf::Color(32, 92, 34), sf::Color(172, 255, 168));
    if (slot.burnTurns > 0) pushTag("BRN", sf::Color(124, 44, 18), sf::Color(255, 196, 140));
    if (slot.bleedTurns > 0) pushTag("BLD", sf::Color(112, 22, 36), sf::Color(255, 168, 180));
    if (slot.paralysisTurns > 0) pushTag("PAR", sf::Color(102, 88, 20), sf::Color(255, 240, 130));
    if (slot.frostTurns > 0) pushTag("FRZ", sf::Color(26, 78, 108), sf::Color(180, 238, 255));
    if (slot.talentPoints > 0 && !slot.isDead) pushTag("+TP", sf::Color(126, 94, 22), sf::Color(255, 236, 128));
}

void drawTooltip(sf::RenderTarget& target, AssetManager& assets, const sf::Font& font,
                 sf::Vector2f anchor, const std::string& title,
                 const std::vector<std::string>& lines, sf::Color titleColor) {
    const float maxW = 340.0f;
    const float pad = 12.0f;
    std::vector<std::string> wrapped;
    for (const auto& l : lines) {
        auto w = wrapUtf8Lines(l, font, 13, maxW - pad * 2.0f);
        wrapped.insert(wrapped.end(), w.begin(), w.end());
    }

    const float boxH = pad * 2.0f + 24.0f + static_cast<float>(wrapped.size()) * 18.0f;
    float x = anchor.x + 16.0f;
    float y = anchor.y + 16.0f;
    if (x + maxW > Game::LOGICAL_WIDTH - 10.0f) x = anchor.x - maxW - 12.0f;
    if (y + boxH > Game::LOGICAL_HEIGHT - 10.0f) y = Game::LOGICAL_HEIGHT - boxH - 10.0f;
    x = std::max(10.0f, x);
    y = std::max(10.0f, y);

    drawPanel(target, assets, {x, y, maxW, boxH}, true, 248);
    drawText(target, font, title, {x + pad, y + pad}, 15, titleColor, true, maxW - pad * 2.0f);

    float cy = y + pad + 24.0f;
    for (const auto& wl : wrapped) {
        drawText(target, font, wl, {x + pad, cy}, 13, UiTheme::textPrimary(), false, maxW - pad * 2.0f);
        cy += 18.0f;
    }
}

void drawBackdrop(sf::RenderTarget& target, AssetManager& assets, const std::string& bgName,
                  sf::Uint8 dimAlpha) {
    if (const sf::Texture* tex = assets.getBackgroundTexture(bgName)) {
        sf::Sprite sp(*tex);
        const auto sz = tex->getSize();
        sp.setScale(Game::LOGICAL_WIDTH / static_cast<float>(sz.x),
                    Game::LOGICAL_HEIGHT / static_cast<float>(sz.y));
        target.draw(sp);
    } else {
        sf::RectangleShape bg({Game::LOGICAL_WIDTH, Game::LOGICAL_HEIGHT});
        bg.setFillColor(UiTheme::bgVoid());
        target.draw(bg);
    }

    if (dimAlpha > 0) {
        sf::RectangleShape dim({Game::LOGICAL_WIDTH, Game::LOGICAL_HEIGHT});
        dim.setFillColor(sf::Color(8, 8, 12, dimAlpha));
        target.draw(dim);
    }
    if (const sf::Texture* vig = assets.getUiTexture("vignette")) {
        sf::Sprite sp(*vig);
        const auto sz = vig->getSize();
        sp.setScale(Game::LOGICAL_WIDTH / static_cast<float>(sz.x),
                    Game::LOGICAL_HEIGHT / static_cast<float>(sz.y));
        target.draw(sp);
    }
}

} // namespace UiSystem

} // namespace crawl
