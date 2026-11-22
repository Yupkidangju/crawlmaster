#include "view/DungeonRenderer.hpp"
#include "core/AssetManager.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "model/DungeonMap.hpp"
#include "model/Party.hpp"
#include "view/UiSystem.hpp"
#include "view/UiTheme.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace crawl {
namespace {

constexpr float PI_F = 3.14159265359f;
constexpr float VIEWPORT_W = 1280.0f;
constexpr float VIEWPORT_H = 588.0f;

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct ClipVertex {
    Vec3 viewPos;
    float u = 0.0f;
    float v = 0.0f;
};

struct RenderQuad {
    const sf::Texture* albedo = nullptr;
    const sf::Texture* normal = nullptr;
    std::array<ClipVertex, 4> corners;
    float avgDepth = 0.0f;
    float dirShade = 1.0f;
    int sortLayer = 0; // 0: floor/ceiling, 1: wall, 2: wall-mounted prop, 3: billboard
    bool isBillboard = false;
    bool additiveGlow = false;
};

// 근평면(z >= nearZ) Sutherland-Hodgman 폴리곤 클리핑
std::vector<ClipVertex> clipToNearPlane(const std::array<ClipVertex, 4>& quad, float nearZ) {
    std::vector<ClipVertex> poly(quad.begin(), quad.end());
    std::vector<ClipVertex> out;
    out.reserve(6);

    for (std::size_t i = 0; i < poly.size(); ++i) {
        const ClipVertex& a = poly[i];
        const ClipVertex& b = poly[(i + 1) % poly.size()];
        const bool aIn = a.viewPos.z >= nearZ;
        const bool bIn = b.viewPos.z >= nearZ;

        if (aIn && bIn) {
            out.push_back(b);
        } else if (aIn && !bIn) {
            const float t = (nearZ - a.viewPos.z) / (b.viewPos.z - a.viewPos.z);
            out.push_back({
                {a.viewPos.x + (b.viewPos.x - a.viewPos.x) * t,
                 a.viewPos.y + (b.viewPos.y - a.viewPos.y) * t, nearZ},
                a.u + (b.u - a.u) * t, a.v + (b.v - a.v) * t
            });
        } else if (!aIn && bIn) {
            const float t = (nearZ - a.viewPos.z) / (b.viewPos.z - a.viewPos.z);
            out.push_back({
                {a.viewPos.x + (b.viewPos.x - a.viewPos.x) * t,
                 a.viewPos.y + (b.viewPos.y - a.viewPos.y) * t, nearZ},
                a.u + (b.u - a.u) * t, a.v + (b.v - a.v) * t
            });
            out.push_back(b);
        }
    }
    return out;
}

float shortestAngleDelta(float from, float to) {
    float diff = std::fmod(to - from + PI_F, 2.0f * PI_F);
    if (diff < 0.0f) diff += 2.0f * PI_F;
    return diff - PI_F;
}

sf::Color blendColor(sf::Color a, sf::Color b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return sf::Color(
        static_cast<sf::Uint8>(static_cast<float>(a.r) * (1.0f - t) + static_cast<float>(b.r) * t),
        static_cast<sf::Uint8>(static_cast<float>(a.g) * (1.0f - t) + static_cast<float>(b.g) * t),
        static_cast<sf::Uint8>(static_cast<float>(a.b) * (1.0f - t) + static_cast<float>(b.b) * t),
        255
    );
}

} // namespace

DungeonRenderer::DungeonRenderer(Game& game) : m_game(game) {
    m_dust.resize(36);
    for (std::size_t i = 0; i < m_dust.size(); ++i) {
        m_dust[i] = DustMote{
            static_cast<float>((i * 173) % 1280),
            static_cast<float>((i * 97) % 560),
            -12.0f + static_cast<float>((i * 29) % 24),
            -18.0f - static_cast<float>((i * 17) % 18),
            3.0f + static_cast<float>((i * 13) % 5),
            static_cast<float>(i) * 0.45f
        };
    }
}

float DungeonRenderer::directionToYaw(Direction dir) {
    switch (dir) {
        case Direction::NORTH: return -PI_F * 0.5f;
        case Direction::EAST:  return 0.0f;
        case Direction::SOUTH: return PI_F * 0.5f;
        case Direction::WEST:  return PI_F;
    }
    return 0.0f;
}

void DungeonRenderer::syncCameraImmediate(const DungeonMap& map) {
    m_camX = static_cast<float>(map.getPlayerX());
    m_camY = static_cast<float>(map.getPlayerY());
    m_camYaw = directionToYaw(map.getPlayerDir());
    m_cameraInitialized = true;
}

void DungeonRenderer::notifyStep(bool bumped) {
    if (bumped) {
        m_bumpTimer = 0.14f;
    } else {
        m_headBobAmp = 1.0f;
    }
}

void DungeonRenderer::triggerDamageFlash(float intensity) {
    m_damageFlash = std::max(m_damageFlash, std::clamp(intensity, 0.0f, 1.0f));
}

void DungeonRenderer::update(float dt, const DungeonMap& map) {
    m_time += dt;
    if (!m_cameraInitialized) {
        syncCameraImmediate(map);
    }

    const float targetX = static_cast<float>(map.getPlayerX());
    const float targetY = static_cast<float>(map.getPlayerY());
    const float targetYaw = directionToYaw(map.getPlayerDir());

    // 층 전환 등으로 거리가 멀어졌을 때는 즉시 스냅
    if (std::abs(targetX - m_camX) > 2.2f || std::abs(targetY - m_camY) > 2.2f) {
        m_camX = targetX;
        m_camY = targetY;
        m_camYaw = targetYaw;
    } else {
        const float moveAlpha = 1.0f - std::exp(-dt * 15.0f);
        const float turnAlpha = 1.0f - std::exp(-dt * 17.0f);
        m_camX += (targetX - m_camX) * moveAlpha;
        m_camY += (targetY - m_camY) * moveAlpha;
        m_camYaw += shortestAngleDelta(m_camYaw, targetYaw) * turnAlpha;
    }

    if (m_headBobAmp > 0.01f) {
        m_headBobPhase += dt * 15.0f;
        m_headBobAmp = std::max(0.0f, m_headBobAmp - dt * 4.5f);
    }
    if (m_bumpTimer > 0.0f) {
        m_bumpTimer = std::max(0.0f, m_bumpTimer - dt);
    }
    if (m_damageFlash > 0.0f) {
        m_damageFlash = std::max(0.0f, m_damageFlash - dt * 2.2f);
    }

    for (auto& d : m_dust) {
        d.x += (d.vx + std::sin(m_time + d.phase) * 9.0f) * dt;
        d.y += d.vy * dt;
        if (d.y < -12.0f) {
            d.y = VIEWPORT_H + 8.0f;
            d.x = std::fmod(d.x + 1400.0f, VIEWPORT_W);
        }
        if (d.x < -12.0f) d.x += VIEWPORT_W + 24.0f;
        if (d.x > VIEWPORT_W + 12.0f) d.x -= VIEWPORT_W + 24.0f;
    }
}

void DungeonRenderer::ensureOffscreenTarget(unsigned int width, unsigned int height) {
    if (m_offscreenReady && m_sceneBuffer.getSize().x == width && m_sceneBuffer.getSize().y == height) {
        return;
    }
    m_offscreenReady = m_sceneBuffer.create(width, height);
    if (m_offscreenReady) {
        m_sceneBuffer.setSmooth(true);
    }
}

void DungeonRenderer::render3DScene(sf::RenderTarget& target, sf::FloatRect vp, const DungeonMap& map,
                                    int floorNumber, const std::vector<WorldObject>& objects,
                                    const Party* party, bool combatBackdropMode, float extraDamageFlash) {
    auto& assets = m_game.getAssets();
    const BiomeType biome = map.getBiome();
    const std::string slug = map.getBiomeSlug();
    const auto profile = UiTheme::biomeProfile(biome);

    const bool useShaders = assets.shadersAvailable() &&
                            LocalizationManager::getInstance().getGraphicsQuality() > 0;
    if (useShaders) {
        ensureOffscreenTarget(static_cast<unsigned int>(vp.width), static_cast<unsigned int>(vp.height));
    }

    sf::RenderTarget* drawDst = (useShaders && m_offscreenReady)
                                    ? static_cast<sf::RenderTarget*>(&m_sceneBuffer)
                                    : &target;
    const float offX = (drawDst == &m_sceneBuffer) ? 0.0f : vp.left;
    const float offY = (drawDst == &m_sceneBuffer) ? 0.0f : vp.top;

    // 1. 천장/바닥 원거리 안개 그라데이션 배경
    if (drawDst == &m_sceneBuffer) {
        m_sceneBuffer.clear(profile.fogColor);
    } else {
        sf::RectangleShape sky({vp.width, vp.height});
        sky.setPosition(offX, offY);
        sky.setFillColor(profile.fogColor);
        drawDst->draw(sky);
    }

    // 텍스처 확보
    const sf::Texture* wallTex = assets.getBiomeTexture(slug, "wall");
    const sf::Texture* wallNorm = assets.getTexture("textures/biomes/wall_" + slug + "_n.png", true, true);
    const sf::Texture* floorTex = assets.getBiomeTexture(slug, "floor");
    const sf::Texture* floorNorm = assets.getTexture("textures/biomes/floor_" + slug + "_n.png", true, true);
    const sf::Texture* ceilTex = assets.getBiomeTexture(slug, "ceiling");
    const sf::Texture* doorWood = assets.getTexture("textures/biomes/door_wood.png", false, true);
    const sf::Texture* doorWoodN = assets.getTexture("textures/biomes/door_wood_n.png", false, true);
    const sf::Texture* doorLocked = assets.getTexture("textures/biomes/door_locked.png", false, true);
    const sf::Texture* doorLockedN = assets.getTexture("textures/biomes/door_locked_n.png", false, true);
    const sf::Texture* bossGate = assets.getTexture("textures/biomes/boss_gate.png", false, true);
    const sf::Texture* bossGateN = assets.getTexture("textures/biomes/boss_gate_n.png", false, true);
    const sf::Texture* secretTex = assets.getTexture("textures/biomes/secret_wall.png", false, true);
    const sf::Texture* secretNorm = assets.getTexture("textures/biomes/secret_wall_n.png", false, true);
    const sf::Texture* stairsUp = assets.getTexture("textures/biomes/stairs_up.png", false, true);
    const sf::Texture* stairsDown = assets.getTexture("textures/biomes/stairs_down.png", false, true);
    const sf::Texture* torchProp = assets.getPropTexture("prop_torch");

    // 2. 카메라 기준 벡터 계산
    const float fx = std::cos(m_camYaw);
    const float fz = std::sin(m_camYaw);
    const float rx = -fz;
    const float rz = fx;
    const float bumpOffset = (m_bumpTimer > 0.0f) ? std::sin(m_bumpTimer * 45.0f) * 0.08f : 0.0f;
    const float eyeX = m_camX + 0.5f + fx * bumpOffset;
    const float eyeZ = m_camY + 0.5f + fz * bumpOffset;
    const float eyeY = 0.52f + std::sin(m_headBobPhase) * 0.028f * m_headBobAmp;

    auto toView = [&](float wx, float wy, float wz) -> Vec3 {
        const float dx = wx - eyeX;
        const float dy = wy - eyeY;
        const float dz = wz - eyeZ;
        return {dx * rx + dz * rz, dy, dx * fx + dz * fz};
    };

    std::vector<RenderQuad> quads;
    quads.reserve(420);

    auto pushSubdividedQuad = [&](Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3,
                                  const sf::Texture* alb, const sf::Texture* nrm,
                                  float dirShade, int subdiv, int sortLayer = 1,
                                  bool billboard = false, bool addGlow = false) {
        if (!alb) return;
        const auto tsz = alb->getSize();
        const float tw = static_cast<float>(tsz.x);
        const float th = static_cast<float>(tsz.y);

        auto lerp3 = [](Vec3 a, Vec3 b, float t) -> Vec3 {
            return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
        };

        for (int sy = 0; sy < subdiv; ++sy) {
            const float v0 = static_cast<float>(sy) / static_cast<float>(subdiv);
            const float v1 = static_cast<float>(sy + 1) / static_cast<float>(subdiv);
            const Vec3 left0 = lerp3(p0, p3, v0);
            const Vec3 right0 = lerp3(p1, p2, v0);
            const Vec3 left1 = lerp3(p0, p3, v1);
            const Vec3 right1 = lerp3(p1, p2, v1);

            for (int sx = 0; sx < subdiv; ++sx) {
                const float u0 = static_cast<float>(sx) / static_cast<float>(subdiv);
                const float u1 = static_cast<float>(sx + 1) / static_cast<float>(subdiv);
                const Vec3 c0 = toView(lerp3(left0, right0, u0).x, lerp3(left0, right0, u0).y, lerp3(left0, right0, u0).z);
                const Vec3 c1 = toView(lerp3(left0, right0, u1).x, lerp3(left0, right0, u1).y, lerp3(left0, right0, u1).z);
                const Vec3 c2 = toView(lerp3(left1, right1, u1).x, lerp3(left1, right1, u1).y, lerp3(left1, right1, u1).z);
                const Vec3 c3 = toView(lerp3(left1, right1, u0).x, lerp3(left1, right1, u0).y, lerp3(left1, right1, u0).z);
                if (c0.z < 0.14f && c1.z < 0.14f && c2.z < 0.14f && c3.z < 0.14f) continue;

                RenderQuad q;
                q.albedo = alb;
                q.normal = nrm ? nrm : alb;
                q.corners = {{
                    {c0, u0 * tw, v0 * th},
                    {c1, u1 * tw, v0 * th},
                    {c2, u1 * tw, v1 * th},
                    {c3, u0 * tw, v1 * th}
                }};
                q.avgDepth = (std::max(0.14f, c0.z) + std::max(0.14f, c1.z) +
                              std::max(0.14f, c2.z) + std::max(0.14f, c3.z)) * 0.25f;
                q.dirShade = dirShade;
                q.sortLayer = sortLayer;
                q.isBillboard = billboard;
                q.additiveGlow = addGlow;
                quads.push_back(q);
            }
        }
    };

    // 3. 가시 영역(반경 7칸) 내 3D 던전 셀 메시 수집
    const int cx0 = static_cast<int>(std::floor(eyeX));
    const int cz0 = static_cast<int>(std::floor(eyeZ));
    constexpr int RADIUS = 7;

    for (int gz = cz0 - RADIUS; gz <= cz0 + RADIUS; ++gz) {
        for (int gx = cx0 - RADIUS; gx <= cx0 + RADIUS; ++gx) {
            if (gx < 0 || gx >= DungeonMap::MAP_WIDTH || gz < 0 || gz >= DungeonMap::MAP_HEIGHT) continue;
            const float cellDist = std::hypot(static_cast<float>(gx) + 0.5f - eyeX,
                                              static_cast<float>(gz) + 0.5f - eyeZ);
            if (cellDist > 7.4f) continue;

            // 카메라 뒤쪽 셀은 빠르게 컬링
            const float fwdDot = (static_cast<float>(gx) + 0.5f - eyeX) * fx +
                                 (static_cast<float>(gz) + 0.5f - eyeZ) * fz;
            if (fwdDot < -1.1f) continue;

            const TileType t = map.getTile(gx, gz);
            if (!map.isWalkable(gx, gz)) continue;

            const float x0 = static_cast<float>(gx);
            const float x1 = x0 + 1.0f;
            const float z0 = static_cast<float>(gz);
            const float z1 = z0 + 1.0f;
            // 근접 셀 정밀 세분화로 2D 아핀 UV 왜곡을 완전 제거
            const int sub = (cellDist < 1.35f) ? 8 : ((cellDist < 2.6f) ? 4 : ((cellDist < 4.2f) ? 2 : 1));

            // 바닥 & 천장
            const sf::Texture* fTex = floorTex;
            if (t == TileType::UPSTAIRS && stairsUp) fTex = stairsUp;
            else if (t == TileType::DOWNSTAIRS && stairsDown) fTex = stairsDown;

            pushSubdividedQuad({x0, 0.0f, z0}, {x1, 0.0f, z0}, {x1, 0.0f, z1}, {x0, 0.0f, z1},
                               fTex, floorNorm, 0.92f, sub, 0);
            pushSubdividedQuad({x0, 1.0f, z1}, {x1, 1.0f, z1}, {x1, 1.0f, z0}, {x0, 1.0f, z0},
                               ceilTex, wallNorm, 0.68f, sub, 0);

            // 인접 비통행/특수 타일 벽면 생성 + 벽면 횃불 브래킷 배치
            struct WallEdge {
                int nx, nz;
                Vec3 a, b, c, d;
                Vec3 torchPos;
                float shade;
            };
            const std::array<WallEdge, 4> edges = {{
                {gx, gz - 1, {x0, 1.0f, z0}, {x1, 1.0f, z0}, {x1, 0.0f, z0}, {x0, 0.0f, z0}, {x0 + 0.5f, 0.62f, z0 + 0.06f}, 1.00f},
                {gx, gz + 1, {x1, 1.0f, z1}, {x0, 1.0f, z1}, {x0, 0.0f, z1}, {x1, 0.0f, z1}, {x0 + 0.5f, 0.62f, z1 - 0.06f}, 0.96f},
                {gx - 1, gz, {x0, 1.0f, z1}, {x0, 1.0f, z0}, {x0, 0.0f, z0}, {x0, 0.0f, z1}, {x0 + 0.06f, 0.62f, z0 + 0.5f}, 0.84f},
                {gx + 1, gz, {x1, 1.0f, z0}, {x1, 1.0f, z1}, {x1, 0.0f, z1}, {x1, 0.0f, z0}, {x1 - 0.06f, 0.62f, z0 + 0.5f}, 0.84f}
            }};

            for (const auto& e : edges) {
                const TileType nt = map.getTile(e.nx, e.nz);
                const bool renderFace = !map.isWalkable(e.nx, e.nz) ||
                                        nt == TileType::BOSS_GATE || nt == TileType::DOOR;
                if (!renderFace) continue;

                const sf::Texture* wAlb = wallTex;
                const sf::Texture* wNrm = wallNorm;
                if (nt == TileType::SECRET_WALL && secretTex) {
                    wAlb = secretTex;
                    wNrm = secretNorm;
                } else if (nt == TileType::LOCKED_GATE && doorLocked) {
                    wAlb = doorLocked;
                    wNrm = doorLockedN;
                } else if (nt == TileType::BOSS_GATE && bossGate) {
                    wAlb = bossGate;
                    wNrm = bossGateN;
                } else if (nt == TileType::DOOR && doorWood) {
                    // 문은 인접한 문 타일 쪽으로 바라볼 때만 나무 문 아치 표현
                    if (t == TileType::DOOR) continue;
                    wAlb = doorWood;
                    wNrm = doorWoodN;
                }

                pushSubdividedQuad(e.a, e.b, e.c, e.d, wAlb, wNrm, e.shade, sub, 1);

                // 규칙적 간격의 석벽에 흔들리는 벽면 횃불 소품 배치
                if (nt == TileType::WALL && torchProp && ((gx * 7 + gz * 13 + e.nx * 3) & 7) == 0 && cellDist > 0.6f) {
                    const float hs = 0.16f;
                    const float ht = 0.28f;
                    const Vec3 tp = e.torchPos;
                    pushSubdividedQuad(
                        {tp.x - rx * hs, tp.y + ht, tp.z - rz * hs},
                        {tp.x + rx * hs, tp.y + ht, tp.z + rz * hs},
                        {tp.x + rx * hs, tp.y - ht * 0.6f, tp.z + rz * hs},
                        {tp.x - rx * hs, tp.y - ht * 0.6f, tp.z - rz * hs},
                        torchProp, nullptr, 1.0f, 1, 2, true, false);
                }
            }
        }
    }

    // 4. 3D 월드 오브젝트 빌보드 (보물상자·성소·레버·탐지함정·NPC·성유물·로어·보스)
    if (!combatBackdropMode) {
        const bool hasRogue = party && party->hasClassInParty(CharacterClass::ROGUE);
        for (const auto& obj : objects) {
            if (obj.floor != floorNumber) continue;
            if (obj.state == WorldObjectState::RESOLVED && obj.kind != WorldObjectKind::CHEST) continue;
            if (obj.kind == WorldObjectKind::TRAP && !hasRogue && obj.state == WorldObjectState::PRESENT) continue;
            if ((obj.kind == WorldObjectKind::QUEST_ITEM || obj.kind == WorldObjectKind::NPC) &&
                party && !party->hasQuest(obj.questId)) continue;

            const float ox = static_cast<float>(obj.x) + 0.5f;
            const float oz = static_cast<float>(obj.y) + 0.5f;
            const float dist = std::hypot(ox - eyeX, oz - eyeZ);
            if (dist < 0.35f || dist > 6.8f) continue;

            const sf::Texture* propTex = nullptr;
            float width = 0.52f;
            float height = 0.56f;
            float baseY = 0.02f;
            switch (obj.kind) {
                case WorldObjectKind::CHEST:
                    propTex = assets.getPropTexture(
                        obj.state == WorldObjectState::RESOLVED ? "prop_chest_open" : "prop_chest_closed");
                    width = 0.46f;
                    height = 0.42f;
                    break;
                case WorldObjectKind::SANCTUARY:
                    propTex = assets.getPropTexture("prop_sanctuary");
                    width = 0.62f;
                    height = 0.66f;
                    break;
                case WorldObjectKind::LEVER:
                    propTex = assets.getPropTexture("prop_lever");
                    width = 0.38f;
                    height = 0.48f;
                    break;
                case WorldObjectKind::TRAP:
                    propTex = assets.getPropTexture("prop_trap");
                    width = 0.54f;
                    height = 0.32f;
                    break;
                case WorldObjectKind::NPC:
                    propTex = assets.getPropTexture("prop_npc_scout");
                    width = 0.56f;
                    height = 0.72f;
                    break;
                case WorldObjectKind::QUEST_ITEM:
                    propTex = assets.getPropTexture("prop_relic");
                    width = 0.42f;
                    height = 0.48f;
                    baseY = 0.14f + std::sin(m_time * 3.0f) * 0.04f;
                    break;
                case WorldObjectKind::LORE_PLAQUE:
                    propTex = assets.getPropTexture("prop_lore");
                    width = 0.46f;
                    height = 0.62f;
                    break;
                case WorldObjectKind::QUEST_BOSS:
                    propTex = assets.getMonsterTexture(obj.targetId);
                    width = 0.78f;
                    height = 0.86f;
                    break;
            }
            if (!propTex) continue;
            const float hw = width * 0.5f;
            pushSubdividedQuad(
                {ox - rx * hw, baseY + height, oz - rz * hw},
                {ox + rx * hw, baseY + height, oz + rz * hw},
                {ox + rx * hw, baseY, oz + rz * hw},
                {ox - rx * hw, baseY, oz - rz * hw},
                propTex, nullptr, 1.0f, 1, 3, true, false);
        }
    }

    // 원근 깊이 정렬 (엄격한 약순서 보장: 먼 것부터, 동일 깊이 띠에서는 바닥/천장 → 벽 → 오브젝트 순)
    std::sort(quads.begin(), quads.end(), [](const RenderQuad& a, const RenderQuad& b) {
        const float keyA = a.avgDepth - static_cast<float>(a.sortLayer) * 0.015f;
        const float keyB = b.avgDepth - static_cast<float>(b.sortLayer) * 0.015f;
        return keyA > keyB;
    });

    // 5. 원근 투영 및 GLSL / Software-Textured 3D 드로우
    const float focal = vp.height * 0.90f;
    const float halfW = vp.width * 0.5f;
    const float halfH = vp.height * 0.5f;
    const float torchFlicker = 1.05f + std::sin(m_time * 7.3f) * 0.08f + std::sin(m_time * 13.7f) * 0.05f;
    sf::Shader* litShader = (useShaders && m_offscreenReady) ? assets.getDungeonShader() : nullptr;

    if (litShader) {
        litShader->setUniform("u_fogColor", sf::Glsl::Vec3(
            profile.fogColor.r / 255.0f, profile.fogColor.g / 255.0f, profile.fogColor.b / 255.0f));
        litShader->setUniform("u_torchColor", sf::Glsl::Vec3(
            profile.torchColor.r / 255.0f, profile.torchColor.g / 255.0f, profile.torchColor.b / 255.0f));
        litShader->setUniform("u_ambientColor", sf::Glsl::Vec3(
            profile.ambientColor.r / 255.0f, profile.ambientColor.g / 255.0f, profile.ambientColor.b / 255.0f));
        litShader->setUniform("u_torchIntensity", combatBackdropMode ? torchFlicker * 0.72f : torchFlicker * 1.18f);
        litShader->setUniform("u_time", m_time);
        litShader->setUniform("u_wetness", profile.wetness);
        litShader->setUniform("u_emissiveBoost", profile.emissiveBoost);
    }

    for (const auto& q : quads) {
        const auto clipped = clipToNearPlane(q.corners, 0.14f);
        if (clipped.size() < 3) continue;

        sf::VertexArray fan(sf::TriangleFan, clipped.size());
        for (std::size_t i = 0; i < clipped.size(); ++i) {
            const auto& cv = clipped[i];
            const float invZ = 1.0f / std::max(0.14f, cv.viewPos.z);
            const float sx = offX + halfW + cv.viewPos.x * focal * invZ;
            const float sy = offY + halfH - cv.viewPos.y * focal * invZ;

            const float dist = std::sqrt(cv.viewPos.x * cv.viewPos.x + cv.viewPos.z * cv.viewPos.z);
            const float lightAtten = std::clamp(1.0f / (1.0f + 0.19f * dist + 0.08f * dist * dist), 0.05f, 1.0f);
            const float fogFactor = std::clamp(std::exp(-dist * profile.fogDensity), 0.04f, 1.0f);

            sf::Color vcol;
            if (litShader && !q.isBillboard) {
                vcol = sf::Color(
                    static_cast<sf::Uint8>(lightAtten * 255.0f),
                    static_cast<sf::Uint8>(std::clamp(q.dirShade, 0.0f, 1.0f) * 255.0f),
                    static_cast<sf::Uint8>(fogFactor * 255.0f),
                    255
                );
            } else {
                // GL 1.1 소프트웨어 텍스처 3D 폴백 및 빌보드용 정밀 CPU 동적 횃불 + 안개 색조
                const float litScale = (combatBackdropMode ? 0.62f : 1.0f) *
                                       q.dirShade * (0.34f + lightAtten * torchFlicker * 0.92f);
                const sf::Color litTint(
                    static_cast<sf::Uint8>(std::clamp(profile.ambientColor.r + profile.torchColor.r * litScale * 0.85f, 0.0f, 255.0f)),
                    static_cast<sf::Uint8>(std::clamp(profile.ambientColor.g + profile.torchColor.g * litScale * 0.85f, 0.0f, 255.0f)),
                    static_cast<sf::Uint8>(std::clamp(profile.ambientColor.b + profile.torchColor.b * litScale * 0.85f, 0.0f, 255.0f)),
                    255
                );
                vcol = blendColor(profile.fogColor, litTint, fogFactor);
            }

            fan[i] = sf::Vertex({sx, sy}, vcol, {cv.u, cv.v});
        }

        sf::RenderStates states;
        states.texture = q.albedo;
        if (q.additiveGlow) states.blendMode = sf::BlendAdd;
        if (litShader && !q.isBillboard) {
            litShader->setUniform("u_albedo", sf::Shader::CurrentTexture);
            litShader->setUniform("u_normal", *q.normal);
            states.shader = litShader;
        }
        drawDst->draw(fan, states);
    }

    // 6. 바이옴 3D 부유 입자 (먼지·포자·영혼불·화염 불씨·금빛 재)
    const sf::Texture* glowTex = assets.getUiTexture("particle_glow");
    for (const auto& d : m_dust) {
        const float pulse = 0.45f + 0.55f * std::sin(m_time * 2.2f + d.phase);
        sf::Color pc = profile.particleColor;
        pc.a = static_cast<sf::Uint8>(static_cast<float>(pc.a) * pulse * (combatBackdropMode ? 0.7f : 1.0f));
        if (glowTex) {
            sf::Sprite sp(*glowTex);
            const auto sz = glowTex->getSize();
            sp.setOrigin(static_cast<float>(sz.x) * 0.5f, static_cast<float>(sz.y) * 0.5f);
            const float sc = (d.size * 3.2f) / static_cast<float>(sz.x);
            sp.setScale(sc, sc);
            sp.setPosition(offX + d.x * (vp.width / VIEWPORT_W), offY + d.y * (vp.height / VIEWPORT_H));
            sp.setColor(pc);
            drawDst->draw(sp, sf::BlendAdd);
        }
    }

    // 7. 포스트프로세싱 (블룸 + 다크 판타지 필름 톤매핑 + 비네트 + 피격 플래시)
    const float flash = std::max(m_damageFlash, extraDamageFlash);
    if (drawDst == &m_sceneBuffer) {
        m_sceneBuffer.display();
        sf::Sprite sceneSprite(m_sceneBuffer.getTexture());
        sceneSprite.setPosition(vp.left, vp.top);
        if (sf::Shader* post = assets.getPostShader()) {
            post->setUniform("u_scene", sf::Shader::CurrentTexture);
            post->setUniform("u_texelSize", sf::Vector2f(1.0f / vp.width, 1.0f / vp.height));
            post->setUniform("u_tint", sf::Glsl::Vec3(profile.postTint.x, profile.postTint.y, profile.postTint.z));
            post->setUniform("u_vignetteStrength", combatBackdropMode ? 0.62f : 0.44f);
            post->setUniform("u_bloomStrength", combatBackdropMode ? 0.35f : 0.52f);
            post->setUniform("u_damageFlash", flash);
            post->setUniform("u_time", m_time);
            target.draw(sceneSprite, post);
        } else {
            target.draw(sceneSprite);
        }
    } else {
        // 소프트웨어 텍스처 폴백 경로용 비네트 및 피격 플래시
        if (const sf::Texture* vig = assets.getUiTexture("vignette")) {
            sf::Sprite vsp(*vig);
            const auto vsz = vig->getSize();
            vsp.setScale(vp.width / static_cast<float>(vsz.x), vp.height / static_cast<float>(vsz.y));
            vsp.setPosition(vp.left, vp.top);
            target.draw(vsp);
        }
        if (flash > 0.01f) {
            sf::RectangleShape hit({vp.width, vp.height});
            hit.setPosition(vp.left, vp.top);
            hit.setFillColor(sf::Color(185, 22, 22, static_cast<sf::Uint8>(flash * 125.0f)));
            target.draw(hit);
        }
    }
}

void DungeonRenderer::renderCombatBackdrop(sf::RenderTarget& target, const DungeonMap& map,
                                           float elapsedTime, float damageFlash) {
    m_time = elapsedTime;
    syncCameraImmediate(map);
    render3DScene(target, {0.0f, 0.0f, Game::LOGICAL_WIDTH, Game::LOGICAL_HEIGHT},
                  map, map.getFloorNumber(), {}, nullptr, true, damageFlash);
    sf::RectangleShape shade({Game::LOGICAL_WIDTH, Game::LOGICAL_HEIGHT});
    shade.setFillColor(sf::Color(6, 6, 10, 88));
    target.draw(shade);
}

void DungeonRenderer::render(sf::RenderWindow& window, const DungeonMap& map, const Party& party,
                             const std::vector<std::string>& logQueue, int floorNumber,
                             const std::vector<WorldObject>& objects,
                             DungeonHudInteraction& outUi) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    const auto& lm = LocalizationManager::getInstance();

    // 1. 풀폭 1인칭 3D 던전 뷰포트 (1280x588)
    render3DScene(window, {0.0f, 0.0f, VIEWPORT_W, VIEWPORT_H}, map, floorNumber, objects, &party, false, 0.0f);

    // 2. 좌상단 바이옴·층수·나침반 배지
    const sf::FloatRect badgeRect(14.0f, 12.0f, 332.0f, 58.0f);
    UiSystem::drawPanel(window, assets, badgeRect, true, 224);
    const std::string biomeName = lm.get("BIOME_" + std::to_string(floorNumber) + "_NAME");
    const std::string floorTitle = lm.format("DUNGEON_FLOOR_LABEL", {{"floor", std::to_string(floorNumber)}}) +
                                   " · " + biomeName;
    UiSystem::drawText(window, font, floorTitle, {28.0f, 20.0f}, 15, UiTheme::textGold(), true, 304.0f);

    const char* dirLabel = "N";
    switch (map.getPlayerDir()) {
        case Direction::NORTH: dirLabel = "N"; break;
        case Direction::EAST:  dirLabel = "E"; break;
        case Direction::SOUTH: dirLabel = "S"; break;
        case Direction::WEST:  dirLabel = "W"; break;
    }
    const std::string sub = std::string(dirLabel) + "  (" + std::to_string(map.getPlayerX()) + "," +
                            std::to_string(map.getPlayerY()) + ")  |  " +
                            std::to_string(map.getProgressPercent()) + "%  |  " +
                            std::to_string(party.getGold()) + " G";
    UiSystem::drawText(window, font, sub, {28.0f, 43.0f}, 12, UiTheme::textMuted(), false, 304.0f);

    // 3. 우상단 컬러 미니맵 & 하단 탐험 로그
    outUi.minimapRect = {1044.0f, 12.0f, 222.0f, 232.0f};
    drawMiniMap(window, outUi.minimapRect, map, party, floorNumber, objects);
    drawExplorationLog(window, {1044.0f, 252.0f, 222.0f, 168.0f}, logQueue);

    // 4. 중앙 하단 상호작용 프롬프트 배너 (상자/함정/레버/성소/계단/비밀벽/NPC/보스)
    if (outUi.canInteract && !outUi.interactPrompt.empty()) {
        outUi.interactBannerRect = {410.0f, 522.0f, 460.0f, 44.0f};
        const bool hov = outUi.interactBannerRect.contains(outUi.mousePos);
        UiSystem::drawButton(window, assets, font, {
            outUi.interactBannerRect, outUi.interactPrompt, "E", false, hov, true, true
        });
    } else {
        outUi.interactBannerRect = {};
    }

    // 5. 하단 4인 초상화 파티 독 + 우측 퀵 메뉴 독
    drawPartyDock(window, buildPartyHudSnapshot(party), outUi);

    // 6. 전체 지도 오버레이 (M키 또는 미니맵 클릭)
    if (outUi.fullMapOpen) {
        drawFullMapOverlay(window, map, party, floorNumber, objects, outUi.fullMapGridRect);
    } else {
        outUi.fullMapGridRect = {};
    }
}

void DungeonRenderer::drawMiniMap(sf::RenderTarget& window, sf::FloatRect frameRect,
                                  const DungeonMap& map, const Party& party, int floorNumber,
                                  const std::vector<WorldObject>& objects) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    UiSystem::drawPanel(window, assets, frameRect, false, 226);

    const float cell = 9.5f;
    const float gx = frameRect.left + (frameRect.width - cell * DungeonMap::MAP_WIDTH) * 0.5f;
    const float gy = frameRect.top + 14.0f;

    sf::RectangleShape mapBg({cell * DungeonMap::MAP_WIDTH, cell * DungeonMap::MAP_HEIGHT});
    mapBg.setPosition(gx, gy);
    mapBg.setFillColor(sf::Color(8, 9, 13, 240));
    window.draw(mapBg);

    for (int y = 0; y < DungeonMap::MAP_HEIGHT; ++y) {
        for (int x = 0; x < DungeonMap::MAP_WIDTH; ++x) {
            if (!map.isVisited(x, y)) continue;
            sf::RectangleShape tile({cell - 1.0f, cell - 1.0f});
            tile.setPosition(gx + x * cell + 0.5f, gy + y * cell + 0.5f);
            const TileType tt = map.getTile(x, y);
            if (tt == TileType::WALL || tt == TileType::SECRET_WALL) {
                tile.setFillColor(sf::Color(96, 90, 82));
            } else if (tt == TileType::LOCKED_GATE) {
                tile.setFillColor(sf::Color(210, 86, 52));
            } else if (tt == TileType::UPSTAIRS) {
                tile.setFillColor(sf::Color(240, 186, 72));
            } else if (tt == TileType::DOWNSTAIRS || tt == TileType::BOSS_GATE) {
                tile.setFillColor(tt == TileType::BOSS_GATE ? sf::Color(235, 58, 48) : sf::Color(112, 232, 138));
            } else if (map.isStepped(x, y)) {
                tile.setFillColor(sf::Color(68, 128, 112));
            } else {
                tile.setFillColor(sf::Color(32, 54, 52));
            }
            window.draw(tile);
        }
    }

    const bool hasRogue = party.hasClassInParty(CharacterClass::ROGUE);
    for (const auto& o : objects) {
        if (o.floor != floorNumber || o.state == WorldObjectState::RESOLVED) continue;
        if (!map.isVisited(o.x, o.y)) continue;
        if (o.kind == WorldObjectKind::TRAP && !hasRogue && o.state == WorldObjectState::PRESENT) continue;
        if ((o.kind == WorldObjectKind::QUEST_ITEM || o.kind == WorldObjectKind::NPC) &&
            !party.hasQuest(o.questId)) continue;

        sf::CircleShape dot(3.2f);
        dot.setOrigin(3.2f, 3.2f);
        dot.setPosition(gx + o.x * cell + cell * 0.5f, gy + o.y * cell + cell * 0.5f);
        switch (o.kind) {
            case WorldObjectKind::CHEST:       dot.setFillColor(sf::Color(246, 198, 72)); break;
            case WorldObjectKind::SANCTUARY:   dot.setFillColor(sf::Color(255, 134, 48)); break;
            case WorldObjectKind::LEVER:       dot.setFillColor(sf::Color(168, 212, 255)); break;
            case WorldObjectKind::TRAP:        dot.setFillColor(sf::Color(255, 76, 58)); break;
            case WorldObjectKind::QUEST_BOSS:  dot.setFillColor(sf::Color(255, 54, 88)); break;
            case WorldObjectKind::NPC:         dot.setFillColor(sf::Color(92, 240, 255)); break;
            case WorldObjectKind::QUEST_ITEM:  dot.setFillColor(sf::Color(255, 224, 96)); break;
            case WorldObjectKind::LORE_PLAQUE: dot.setFillColor(sf::Color(172, 146, 255)); break;
        }
        window.draw(dot);
    }

    // 플레이어 방향 삼각형
    sf::ConvexShape tri(3);
    tri.setPoint(0, {0.0f, -4.5f});
    tri.setPoint(1, {3.8f, 3.8f});
    tri.setPoint(2, {-3.8f, 3.8f});
    tri.setFillColor(UiTheme::cyanAccent());
    tri.setOutlineThickness(1.0f);
    tri.setOutlineColor(sf::Color(8, 12, 16));
    tri.setPosition(gx + map.getPlayerX() * cell + cell * 0.5f,
                    gy + map.getPlayerY() * cell + cell * 0.5f);
    switch (map.getPlayerDir()) {
        case Direction::NORTH: tri.setRotation(0.0f); break;
        case Direction::EAST:  tri.setRotation(90.0f); break;
        case Direction::SOUTH: tri.setRotation(180.0f); break;
        case Direction::WEST:  tri.setRotation(270.0f); break;
    }
    window.draw(tri);

    UiSystem::drawCenteredText(window, font,
                               LocalizationManager::getInstance().get("HUD_MINIMAP_HINT"),
                               {frameRect.left, frameRect.top + frameRect.height - 24.0f, frameRect.width, 20.0f},
                               11, UiTheme::textMuted(), false);
}

void DungeonRenderer::drawFullMapOverlay(sf::RenderTarget& window, const DungeonMap& map,
                                         const Party& party, int floorNumber,
                                         const std::vector<WorldObject>& objects,
                                         sf::FloatRect& outGridRect) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    const auto& lm = LocalizationManager::getInstance();

    sf::RectangleShape dim({Game::LOGICAL_WIDTH, Game::LOGICAL_HEIGHT});
    dim.setFillColor(sf::Color(4, 4, 8, 185));
    window.draw(dim);

    const sf::FloatRect panel(290.0f, 34.0f, 700.0f, 650.0f);
    UiSystem::drawPanel(window, assets, panel, true, 248);
    UiSystem::drawCenteredText(window, font,
                               lm.format("DUNGEON_FLOOR_LABEL", {{"floor", std::to_string(floorNumber)}}) +
                                   " — " + lm.get("BIOME_" + std::to_string(floorNumber) + "_NAME"),
                               {panel.left, panel.top + 12.0f, panel.width, 32.0f}, 20, UiTheme::textGold(), true);

    const float cell = 26.0f;
    const float gx = panel.left + (panel.width - cell * DungeonMap::MAP_WIDTH) * 0.5f;
    const float gy = panel.top + 54.0f;
    outGridRect = {gx, gy, cell * DungeonMap::MAP_WIDTH, cell * DungeonMap::MAP_HEIGHT};

    sf::RectangleShape bg({outGridRect.width, outGridRect.height});
    bg.setPosition(gx, gy);
    bg.setFillColor(sf::Color(10, 11, 16));
    bg.setOutlineThickness(2.0f);
    bg.setOutlineColor(UiTheme::borderBronze());
    window.draw(bg);

    for (int y = 0; y < DungeonMap::MAP_HEIGHT; ++y) {
        for (int x = 0; x < DungeonMap::MAP_WIDTH; ++x) {
            if (!map.isVisited(x, y)) continue;
            sf::RectangleShape r({cell - 2.0f, cell - 2.0f});
            r.setPosition(gx + x * cell + 1.0f, gy + y * cell + 1.0f);
            const TileType tt = map.getTile(x, y);
            if (tt == TileType::WALL || tt == TileType::SECRET_WALL) r.setFillColor(sf::Color(86, 80, 74));
            else if (tt == TileType::LOCKED_GATE) r.setFillColor(sf::Color(210, 86, 52));
            else if (tt == TileType::UPSTAIRS) r.setFillColor(sf::Color(240, 186, 72));
            else if (tt == TileType::DOWNSTAIRS) r.setFillColor(sf::Color(108, 224, 134));
            else if (tt == TileType::BOSS_GATE) r.setFillColor(sf::Color(235, 58, 48));
            else if (map.isStepped(x, y)) r.setFillColor(sf::Color(62, 118, 102));
            else r.setFillColor(sf::Color(28, 48, 46));
            window.draw(r);
        }
    }

    const bool hasRogue = party.hasClassInParty(CharacterClass::ROGUE);
    for (const auto& o : objects) {
        if (o.floor != floorNumber || o.state == WorldObjectState::RESOLVED || !map.isVisited(o.x, o.y)) continue;
        if (o.kind == WorldObjectKind::TRAP && !hasRogue && o.state == WorldObjectState::PRESENT) continue;
        if ((o.kind == WorldObjectKind::QUEST_ITEM || o.kind == WorldObjectKind::NPC) &&
            !party.hasQuest(o.questId)) continue;
        const char* sym = "?";
        sf::Color col = UiTheme::textGold();
        switch (o.kind) {
            case WorldObjectKind::CHEST:       sym = "C"; col = sf::Color(255, 206, 78); break;
            case WorldObjectKind::SANCTUARY:   sym = "S"; col = sf::Color(255, 140, 52); break;
            case WorldObjectKind::LEVER:       sym = "L"; col = sf::Color(152, 212, 255); break;
            case WorldObjectKind::TRAP:        sym = "!"; col = sf::Color(255, 76, 58); break;
            case WorldObjectKind::QUEST_BOSS:  sym = "B"; col = sf::Color(255, 62, 92); break;
            case WorldObjectKind::NPC:         sym = "N"; col = sf::Color(92, 240, 255); break;
            case WorldObjectKind::QUEST_ITEM:  sym = "Q"; col = sf::Color(255, 228, 102); break;
            case WorldObjectKind::LORE_PLAQUE: sym = "I"; col = sf::Color(186, 152, 255); break;
        }
        UiSystem::drawCenteredText(window, font, sym,
                                   {gx + o.x * cell, gy + o.y * cell, cell, cell}, 13, col, true);
    }

    sf::ConvexShape tri(3);
    tri.setPoint(0, {0.0f, -8.0f});
    tri.setPoint(1, {7.0f, 7.0f});
    tri.setPoint(2, {-7.0f, 7.0f});
    tri.setFillColor(UiTheme::cyanAccent());
    tri.setPosition(gx + map.getPlayerX() * cell + cell * 0.5f,
                    gy + map.getPlayerY() * cell + cell * 0.5f);
    switch (map.getPlayerDir()) {
        case Direction::NORTH: tri.setRotation(0.0f); break;
        case Direction::EAST:  tri.setRotation(90.0f); break;
        case Direction::SOUTH: tri.setRotation(180.0f); break;
        case Direction::WEST:  tri.setRotation(270.0f); break;
    }
    window.draw(tri);

    UiSystem::drawCenteredText(window, font, lm.get("HUD_FULLMAP_LEGEND"),
                               {panel.left + 16.0f, panel.top + panel.height - 54.0f, panel.width - 32.0f, 42.0f},
                               13, UiTheme::textMuted(), false);
}

void DungeonRenderer::drawPartyDock(sf::RenderTarget& window, const PartyHudSnapshot& snapshot,
                                    DungeonHudInteraction& ui) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    const auto& lm = LocalizationManager::getInstance();

    // 하단 독 배경 바
    sf::RectangleShape bar({Game::LOGICAL_WIDTH, 134.0f});
    bar.setPosition(0.0f, 586.0f);
    bar.setFillColor(sf::Color(11, 12, 18, 245));
    window.draw(bar);

    sf::RectangleShape topLine({Game::LOGICAL_WIDTH, 2.0f});
    topLine.setPosition(0.0f, 586.0f);
    topLine.setFillColor(UiTheme::borderBronze());
    window.draw(topLine);

    const auto& slots = snapshot.slots();
    const float cardW = 248.0f;
    const float cardH = 112.0f;
    const float gap = 10.0f;
    const float startX = 14.0f;
    const float cardY = 597.0f;

    for (std::size_t i = 0; i < 4; ++i) {
        const sf::FloatRect r(startX + static_cast<float>(i) * (cardW + gap), cardY, cardW, cardH);
        ui.portraitRects[i] = r;
        if (i < slots.size()) {
            const bool hov = r.contains(ui.mousePos);
            UiSystem::drawPortraitCard(window, assets, font, slots[i], r, false, hov, false);
        } else {
            UiSystem::drawPanel(window, assets, r, false, 150);
            UiSystem::drawCenteredText(window, font, lm.get("HUD_EMPTY_SLOT"), r, 13, UiTheme::textMuted(), false);
        }
    }

    // 우측 퀵 액션 독 (5 버튼: 파티/퀘스트/도감/지도/설정)
    const float dockX = 1046.0f;
    const float dockW = 220.0f;
    const std::array<std::pair<std::string, std::string>, 5> dockItems = {{
        {lm.get("HUD_BTN_PARTY"), "C"},
        {lm.get("HUD_BTN_QUESTS"), "J"},
        {lm.get("HUD_BTN_BESTIARY"), "B"},
        {lm.get("HUD_BTN_MAP"), "M"},
        {lm.get("HUD_BTN_SETTINGS"), "O"}
    }};

    // 2열 3행 그리드 (마지막 버튼은 전체폭)
    for (std::size_t i = 0; i < 5; ++i) {
        sf::FloatRect r;
        if (i < 4) {
            const float col = static_cast<float>(i % 2);
            const float row = static_cast<float>(i / 2);
            r = {dockX + col * 112.0f, cardY + row * 38.0f, 108.0f, 34.0f};
        } else {
            r = {dockX, cardY + 76.0f, dockW, 34.0f};
        }
        ui.dockRects[i] = r;
        UiSystem::drawButton(window, assets, font, {
            r, dockItems[i].first, dockItems[i].second, false, r.contains(ui.mousePos), true, false
        });
    }
}

void DungeonRenderer::drawExplorationLog(sf::RenderTarget& window, sf::FloatRect rect,
                                         const std::vector<std::string>& logQueue) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    UiSystem::drawPanel(window, assets, rect, false, 212);

    const float pad = 10.0f;
    float y = rect.top + pad;
    const int maxShow = 6;
    const int startIdx = std::max(0, static_cast<int>(logQueue.size()) - maxShow);
    for (int i = startIdx; i < static_cast<int>(logQueue.size()); ++i) {
        const auto lines = UiSystem::wrapUtf8Lines(logQueue[static_cast<std::size_t>(i)], font, 12,
                                                   rect.width - pad * 2.0f);
        for (const auto& line : lines) {
            if (y + 15.0f > rect.top + rect.height - 6.0f) return;
            const bool latest = (i == static_cast<int>(logQueue.size()) - 1);
            UiSystem::drawText(window, font, line, {rect.left + pad, y}, 12,
                               latest ? UiTheme::textPrimary() : UiTheme::textMuted(), false);
            y += 15.5f;
        }
    }
}

} // namespace crawl
