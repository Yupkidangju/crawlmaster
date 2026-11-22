#ifndef DUNGEON_RENDERER_HPP
#define DUNGEON_RENDERER_HPP

#include <SFML/Graphics.hpp>
#include "model/DungeonWorld.hpp"
#include "view/PartyHudSnapshot.hpp"

#include <array>
#include <string>
#include <vector>

namespace crawl {

class Game;
class DungeonMap;
class Party;

struct DungeonHudInteraction {
    sf::FloatRect minimapRect;
    sf::FloatRect fullMapGridRect;
    sf::FloatRect interactBannerRect;
    std::array<sf::FloatRect, 4> portraitRects{};
    std::array<sf::FloatRect, 5> dockRects{}; // Party, Quests, Bestiary, Map, Settings
    bool fullMapOpen = false;
    bool canInteract = false;
    std::string interactPrompt;
    sf::Vector2f mousePos{-1000.0f, -1000.0f};
};

class DungeonRenderer {
public:
    explicit DungeonRenderer(Game& game);
    ~DungeonRenderer() = default;

    // 60fps 카메라 보간·헤드밥·횃불 흔들림·파티클 업데이트
    void syncCameraImmediate(const DungeonMap& map);
    void notifyStep(bool bumped = false);
    void triggerDamageFlash(float intensity = 0.7f);
    void update(float dt, const DungeonMap& map);

    // 전체 1인칭 3D 던전 + AAA HUD + 선택적 전체 지도 오버레이 렌더링
    void render(sf::RenderWindow& window, const DungeonMap& map, const Party& party,
                const std::vector<std::string>& logQueue, int floorNumber,
                const std::vector<WorldObject>& objects,
                DungeonHudInteraction& outUi);

    // 전투 화면 배경용 3D 바이옴 복도 렌더링 (살짝 어둡게 처리)
    void renderCombatBackdrop(sf::RenderTarget& target, const DungeonMap& map,
                              float elapsedTime, float damageFlash = 0.0f);

private:
    struct DustMote {
        float x = 0.0f;
        float y = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float size = 4.0f;
        float phase = 0.0f;
    };

    void ensureOffscreenTarget(unsigned int width, unsigned int height);
    void render3DScene(sf::RenderTarget& target, sf::FloatRect vp, const DungeonMap& map,
                       int floorNumber, const std::vector<WorldObject>& objects,
                       const Party* party, bool combatBackdropMode, float extraDamageFlash);

    void drawMiniMap(sf::RenderTarget& window, sf::FloatRect frameRect, const DungeonMap& map,
                     const Party& party, int floorNumber, const std::vector<WorldObject>& objects);
    void drawFullMapOverlay(sf::RenderTarget& window, const DungeonMap& map, const Party& party,
                            int floorNumber, const std::vector<WorldObject>& objects,
                            sf::FloatRect& outGridRect);
    void drawPartyDock(sf::RenderTarget& window, const PartyHudSnapshot& snapshot,
                       DungeonHudInteraction& ui);
    void drawExplorationLog(sf::RenderTarget& window, sf::FloatRect rect,
                            const std::vector<std::string>& logQueue);

    static float directionToYaw(Direction dir);

    Game& m_game;
    sf::RenderTexture m_sceneBuffer;
    bool m_offscreenReady = false;

    // 부드러운 3D 카메라 상태
    float m_camX = 1.0f;
    float m_camY = 1.0f;
    float m_camYaw = 0.0f;
    bool m_cameraInitialized = false;
    float m_headBobPhase = 0.0f;
    float m_headBobAmp = 0.0f;
    float m_bumpTimer = 0.0f;
    float m_damageFlash = 0.0f;
    float m_time = 0.0f;

    std::vector<DustMote> m_dust;
};

} // namespace crawl

#endif // DUNGEON_RENDERER_HPP
