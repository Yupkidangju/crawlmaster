// [v0.2.0] DungeonRenderer.cpp 신규 작성
// 1인칭 사영 테이블을 활용한 와이어프레임 렌더링, FOW 동기화 미니맵, TUI 로그 인터페이스 그리기 구현.

#include "view/DungeonRenderer.hpp"
#include "model/DungeonMap.hpp"
#include "model/Party.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "view/PartyHudSnapshot.hpp"
#include <cmath>
#include <iostream>

namespace crawl {

DungeonRenderer::DungeonRenderer(Game& game) : m_game(game) {}

void DungeonRenderer::render(sf::RenderWindow& window, const DungeonMap& map, const Party& party,
                             const std::vector<std::string>& logQueue, int floorNumber,
                             const std::vector<WorldObject>& objects) {
    // 1. 1인칭 와이어프레임 3D 뷰포트 그리기
    draw3DViewport(window, map);

    // 2. HUD 격자 프레임선 그리기 (네온 스타일 경계선)
    drawHUDFrame(window, buildPartyHudSnapshot(party));

    // 3. 미니맵 그리기 (방문 지역 안개 해제 표시)
    drawMiniMap(window, map, party, floorNumber, objects);

    // 4. 로그 메시지 그리기
    drawLogWindow(window, logQueue);
}

void DungeonRenderer::draw3DViewport(sf::RenderWindow& window, const DungeonMap& map) {
    // 플레이어 정보 획득
    int px = map.getPlayerX();
    int py = map.getPlayerY();
    Direction dir = map.getPlayerDir();

    // 시선 방향 벡터(F) 및 우측 방향 수직 벡터(R) 산출
    int fx = 0, fy = 0;
    int rx = 0, ry = 0;

    switch (dir) {
        case Direction::NORTH: fx = 0;  fy = -1; rx = 1;  ry = 0;  break;
        case Direction::EAST:  fx = 1;  fy = 0;  rx = 0;  ry = 1;  break;
        case Direction::SOUTH: fx = 0;  fy = 1;  rx = -1; ry = 0;  break;
        case Direction::WEST:  fx = -1; fy = 0;  rx = 0;  ry = -1; break;
    }

    sf::Color lineColor = sf::Color(51, 255, 51); // 네온 그린

    // 깊이 d = 0부터 3까지 루프를 돌며 와이어프레임 선 그리기
    for (int d = 0; d < 4; ++d) {
        // 현재 및 다음 깊이에서의 코너 꼭짓점 좌표 미리 계산
        float w_curr = W_TABLE[d];
        float h_curr = H_TABLE[d];
        float w_next = W_TABLE[d+1];
        float h_next = H_TABLE[d+1];

        // 깊이 d의 좌상(TL), 우상(TR), 우하(BR), 좌하(BL)
        float tl_x = CX - w_curr / 2.0f;  float tl_y = CY - h_curr / 2.0f;
        float tr_x = CX + w_curr / 2.0f;  float tr_y = CY - h_curr / 2.0f;
        float br_x = CX + w_curr / 2.0f;  float br_y = CY + h_curr / 2.0f;
        float bl_x = CX - w_curr / 2.0f;  float bl_y = CY + h_curr / 2.0f;

        // 깊이 d+1의 좌상(nTL), 우상(nTR), 우하(nBR), 좌하(nBL)
        float ntl_x = CX - w_next / 2.0f; float ntl_y = CY - h_next / 2.0f;
        float ntr_x = CX + w_next / 2.0f; float ntr_y = CY - h_next / 2.0f;
        float nbr_x = CX + w_next / 2.0f; float nbr_y = CY + h_next / 2.0f;
        float nbl_x = CX - w_next / 2.0f; float nbl_y = CY + h_next / 2.0f;

        // 탐색 대상 격자 셀 좌표
        int cx = px + d * fx;
        int cy = py + d * fy;

        // 1. 현재 칸이 벽(WALL)인 경우 (예외적 상황 처리 및 1인칭 정면 차단)
        if (map.getTile(cx, cy) == TileType::WALL) {
            // 정면 전체 벽 사각형을 드로우하고 그 뒤는 시야 단절 처리
            drawLine(window, tl_x, tl_y, tr_x, tr_y, lineColor);
            drawLine(window, tr_x, tr_y, br_x, br_y, lineColor);
            drawLine(window, br_x, br_y, bl_x, bl_y, lineColor);
            drawLine(window, bl_x, bl_y, tl_x, tl_y, lineColor);
            break;
        }

        // 2. 현재 칸이 비어있다면, 좌/우측 셀이 벽인지 판단하여 원근 사선 드로우
        int lx = cx - rx;
        int ly = cy - ry;
        if (map.getTile(lx, ly) == TileType::WALL) {
            // 좌측 벽면 원근 라인 그리기
            drawLine(window, tl_x, tl_y, ntl_x, ntl_y, lineColor); // 천장선
            drawLine(window, bl_x, bl_y, nbl_x, nbl_y, lineColor); // 바닥선
            drawLine(window, tl_x, tl_y, bl_x, bl_y, lineColor);   // 수직 앞선
            drawLine(window, ntl_x, ntl_y, nbl_x, nbl_y, lineColor); // 수직 뒷선
        } else {
            // 좌측이 뚫린 복도라면, 천장과 바닥에 가로 라인을 그어 복도의 분기를 표현
            // 단, 복고풍 룩을 위해 불필요한 중복 선은 최소화
        }

        int rx_c = cx + rx;
        int ry_c = cy + ry;
        if (map.getTile(rx_c, ry_c) == TileType::WALL) {
            // 우측 벽면 원근 라인 그리기
            drawLine(window, tr_x, tr_y, ntr_x, ntr_y, lineColor);
            drawLine(window, br_x, br_y, nbr_x, nbr_y, lineColor);
            drawLine(window, tr_x, tr_y, br_x, br_y, lineColor);
            drawLine(window, ntr_x, ntr_y, nbr_x, nbr_y, lineColor);
        }

        // 3. 바로 한 칸 앞(d+1)의 격자가 벽인 경우
        int nfx = cx + fx;
        int nfy = cy + fy;
        if (map.getTile(nfx, nfy) == TileType::WALL) {
            // 깊이 d+1에 정면 벽면 사각형을 드로우하여 전방 막힘 연출
            drawLine(window, ntl_x, ntl_y, ntr_x, ntr_y, lineColor);
            drawLine(window, ntr_x, ntr_y, nbr_x, nbr_y, lineColor);
            drawLine(window, nbr_x, nbr_y, nbl_x, nbl_y, lineColor);
            drawLine(window, nbl_x, nbl_y, ntl_x, ntl_y, lineColor);
            // 시야가 전방 벽에 막혔으므로 더 멀리 있는 깊이 d+2 등은 그리지 않음
            break;
        } else {
            // 앞칸이 통로이고 좌우가 벽이라면 복도가 계속 이어짐을 표현하기 위해 가로선(천장/바닥)은 긋지 않고
            // 천장과 바닥의 세로 복도선(원근선)만 뻗어가도록 비워둠
        }
    }
}

void DungeonRenderer::drawMiniMap(sf::RenderWindow& window, const DungeonMap& map, const Party& party,
                                  int floorNumber, const std::vector<WorldObject>& objects) {
    float startX = 750.0f;
    float startY = 40.0f;
    float cellSize = 12.0f; // 12x12 크기 격자

    // 미니맵 배경 드로우
    sf::RectangleShape bg(sf::Vector2f(240.0f, 240.0f));
    bg.setPosition(startX, startY);
    bg.setFillColor(sf::Color(2, 6, 2));
    bg.setOutlineThickness(1.0f);
    bg.setOutlineColor(sf::Color(17, 68, 17));
    window.draw(bg);

    // 20x20 격자 순회
    for (int y = 0; y < DungeonMap::MAP_HEIGHT; ++y) {
        for (int x = 0; x < DungeonMap::MAP_WIDTH; ++x) {
            if (!map.isVisited(x, y)) {
                // 탐험되지 않은 지역은 완전 암전
                continue;
            }

            float cx = startX + x * cellSize;
            float cy = startY + y * cellSize;

            TileType tile = map.getTile(x, y);

            if (tile == TileType::WALL) {
                // [v0.6.0] 밝혀진 벽: 회색 사각형
                sf::RectangleShape wallRect(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                wallRect.setPosition(cx + 1.0f, cy + 1.0f);
                wallRect.setFillColor(sf::Color(100, 100, 100)); // 회색 벽면 채우기
                window.draw(wallRect);
            } else if (tile == TileType::UPSTAIRS || tile == TileType::DOWNSTAIRS) {
                // 입구 계단: 노란색/주황색 계열 사각형
                sf::RectangleShape stairRect(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                stairRect.setPosition(cx + 1.0f, cy + 1.0f);
                stairRect.setFillColor(tile == TileType::UPSTAIRS
                    ? sf::Color(255, 176, 0) : sf::Color(255, 255, 102));
                window.draw(stairRect);
            } else {
                // [v0.6.0] 바닥 (EMPTY, DOOR 등)
                sf::RectangleShape floorRect(sf::Vector2f(cellSize - 2.0f, cellSize - 2.0f));
                floorRect.setPosition(cx + 1.0f, cy + 1.0f);
                
                if (map.isStepped(x, y)) {
                    // 직접 지나간 곳: 밝은 네온 그린
                    floorRect.setFillColor(sf::Color(51, 255, 51));
                } else {
                    // 안개만 걷히고 가보지는 않은 곳: 어두운 녹색
                    floorRect.setFillColor(sf::Color(10, 50, 10));
                }
                window.draw(floorRect);
            }
        }
    }

    for (const auto& object : objects) {
        if (object.floor != floorNumber || object.state != WorldObjectState::DISCOVERED ||
            !party.hasQuest(object.questId) || !map.isVisited(object.x, object.y)) continue;
        const float cx = startX + object.x * cellSize + cellSize / 2.0f;
        const float cy = startY + object.y * cellSize + cellSize / 2.0f;
        if (object.kind == WorldObjectKind::QUEST_ITEM) {
            sf::ConvexShape marker(4);
            marker.setPoint(0, {0.0f, -4.0f}); marker.setPoint(1, {4.0f, 0.0f});
            marker.setPoint(2, {0.0f, 4.0f}); marker.setPoint(3, {-4.0f, 0.0f});
            marker.setFillColor(sf::Color(255, 176, 0));
            marker.setPosition(cx, cy);
            window.draw(marker);
        } else if (object.kind == WorldObjectKind::NPC) {
            sf::RectangleShape marker({7.0f, 7.0f});
            marker.setOrigin(3.5f, 3.5f);
            marker.setPosition(cx, cy);
            marker.setFillColor(sf::Color::Transparent);
            marker.setOutlineColor(sf::Color(102, 255, 255));
            marker.setOutlineThickness(2.0f);
            window.draw(marker);
        } else {
            sf::RectangleShape horizontal({9.0f, 3.0f});
            sf::RectangleShape vertical({3.0f, 9.0f});
            horizontal.setOrigin(4.5f, 1.5f); vertical.setOrigin(1.5f, 4.5f);
            horizontal.setPosition(cx, cy); vertical.setPosition(cx, cy);
            horizontal.setFillColor(sf::Color(255, 51, 51));
            vertical.setFillColor(sf::Color(255, 51, 51));
            window.draw(horizontal); window.draw(vertical);
        }
    }

    sf::Text floorLabel;
    floorLabel.setFont(m_game.getFont());
    floorLabel.setCharacterSize(LocalizationManager::getInstance().getScaledTextSize(14));
    floorLabel.setFillColor(sf::Color(255, 176, 0));
    std::string label = LocalizationManager::getInstance().format(
        "DUNGEON_FLOOR_LABEL", {{"floor", std::to_string(floorNumber)}});
    if (LocalizationManager::getInstance().getTextScale() <= 125) {
        const TileType currentTile = map.getTile(map.getPlayerX(), map.getPlayerY());
        bool canInteract = currentTile == TileType::UPSTAIRS || currentTile == TileType::DOWNSTAIRS;
        for (const auto& object : objects) {
            if (object.floor == floorNumber && object.x == map.getPlayerX() &&
                object.y == map.getPlayerY() && object.state != WorldObjectState::RESOLVED &&
                object.kind != WorldObjectKind::QUEST_BOSS && party.hasQuest(object.questId)) {
                canInteract = true;
            }
        }
        if (canInteract) label += " | " + LocalizationManager::getInstance().get("DUNGEON_INTERACT_PROMPT");
    }
    floorLabel.setString(sf::String::fromUtf8(label.begin(), label.end()));
    floorLabel.setPosition(startX, 285.0f);
    window.draw(floorLabel);

    // 플레이어 마커 그리기
    int px = map.getPlayerX();
    int py = map.getPlayerY();
    Direction pdir = map.getPlayerDir();

    float pMux = startX + px * cellSize + cellSize / 2.0f;
    float pMuy = startY + py * cellSize + cellSize / 2.0f;

    sf::ConvexShape playerMarker(3);
    playerMarker.setPoint(0, sf::Vector2f(0.0f, -5.0f));
    playerMarker.setPoint(1, sf::Vector2f(4.5f, 4.5f));
    playerMarker.setPoint(2, sf::Vector2f(-4.5f, 4.5f));
    playerMarker.setFillColor(sf::Color(102, 255, 255));
    playerMarker.setOutlineColor(sf::Color(2, 6, 2));
    playerMarker.setOutlineThickness(1.0f);
    playerMarker.setPosition(pMux, pMuy);

    switch (pdir) {
        case Direction::NORTH: playerMarker.setRotation(0.0f); break;
        case Direction::EAST: playerMarker.setRotation(90.0f); break;
        case Direction::SOUTH: playerMarker.setRotation(180.0f); break;
        case Direction::WEST: playerMarker.setRotation(270.0f); break;
    }

    window.draw(playerMarker);
}

void DungeonRenderer::drawHUDFrame(sf::RenderWindow& window, const PartyHudSnapshot& partySnapshot) {
    sf::Color neonColor = sf::Color(51, 255, 51);
    sf::Color mutedColor = sf::Color(17, 68, 17);

    // 1. 3D 뷰포트 테두리선 (700x500, 마진 20)
    sf::RectangleShape viewportFrame(sf::Vector2f(VIEWPORT_W, VIEWPORT_H));
    viewportFrame.setPosition(VIEWPORT_X, VIEWPORT_Y);
    viewportFrame.setFillColor(sf::Color::Transparent);
    viewportFrame.setOutlineThickness(2.0f);
    viewportFrame.setOutlineColor(neonColor);
    window.draw(viewportFrame);

    // 2. 하단 로그창 테두리선 (700x208, 마진 X:20, Y:540)
    sf::RectangleShape logFrame(sf::Vector2f(700.0f, 208.0f));
    logFrame.setPosition(20.0f, 540.0f);
    logFrame.setFillColor(sf::Color::Transparent);
    logFrame.setOutlineThickness(1.5f);
    logFrame.setOutlineColor(neonColor);
    window.draw(logFrame);

    // 3. 우측 미니맵 영역 프레임 헤더 테두리
    sf::RectangleShape minimapFrame(sf::Vector2f(264.0f, 280.0f));
    minimapFrame.setPosition(740.0f, 20.0f);
    minimapFrame.setFillColor(sf::Color::Transparent);
    minimapFrame.setOutlineThickness(1.5f);
    minimapFrame.setOutlineColor(mutedColor);
    window.draw(minimapFrame);

    // 4. 우측 하단 파티 상태 프레임 테두리
    sf::RectangleShape partyFrame(sf::Vector2f(264.0f, 408.0f));
    partyFrame.setPosition(740.0f, 340.0f);
    partyFrame.setFillColor(sf::Color::Transparent);
    partyFrame.setOutlineThickness(1.5f);
    partyFrame.setOutlineColor(neonColor);
    window.draw(partyFrame);

    // 파티 상태 헤더 텍스트 드로우
    sf::Text partyHeader;
    partyHeader.setFont(m_game.getFont());
    partyHeader.setString(LocalizationManager::getInstance().getSf("HUD_PARTY_STATUS"));
    partyHeader.setCharacterSize(LocalizationManager::getInstance().getScaledTextSize(14));
    partyHeader.setFillColor(neonColor);
    partyHeader.setPosition(760.0f, 355.0f);
    window.draw(partyHeader);

    std::string partyLines;
    const bool largeText = LocalizationManager::getInstance().getTextScale() > 125;
    for (const auto& slot : partySnapshot.slots()) {
        std::string className;
        switch (slot.characterClass) {
            case CharacterClass::WARRIOR: className = "CLASS_WARRIOR"; break;
            case CharacterClass::MAGE: className = "CLASS_MAGE"; break;
            case CharacterClass::ROGUE: className = "CLASS_ROGUE"; break;
            case CharacterClass::CLERIC: className = "CLASS_CLERIC"; break;
        }
        partyLines += std::to_string(slot.slotNumber) + ". " + slot.name;
        if (!largeText) {
            partyLines += " [" + LocalizationManager::getInstance().get(className) + "]";
        }
        partyLines += "\n";
        auto& localization = LocalizationManager::getInstance();
        partyLines += "   " + localization.get("COMBAT_HP_SHORT") + " " +
                      std::to_string(slot.hp) + "/" + std::to_string(slot.maxHp);
        if (slot.isDead) partyLines += " " + localization.get(largeText ? "STATUS_DEAD_SHORT" : "STATUS_DEAD");
        if (slot.poisonTurns > 0) partyLines += " " + localization.get(largeText ? "STATUS_POISON_SHORT" : "STATUS_POISON") + ":" + std::to_string(slot.poisonTurns);
        if (slot.paralysisTurns > 0) partyLines += " " + localization.get(largeText ? "STATUS_PARALYSIS_SHORT" : "STATUS_PARALYSIS") + ":" + std::to_string(slot.paralysisTurns);
        if (slot.blessTurns > 0) partyLines += " " + localization.get(largeText ? "STATUS_BLESS_SHORT" : "STATUS_BLESS") + ":" + std::to_string(slot.blessTurns);
        partyLines += "\n";
    }
    if (partyLines.empty()) {
        partyLines = LocalizationManager::getInstance().get("HUD_PARTY_EMPTY");
    }

    sf::Text membersText;
    membersText.setFont(m_game.getFont());
    membersText.setString(sf::String::fromUtf8(partyLines.begin(), partyLines.end()));
    membersText.setCharacterSize(LocalizationManager::getInstance().getScaledTextSize(14));
    membersText.setFillColor(sf::Color(180, 255, 180));
    membersText.setPosition(760.0f, 390.0f);
    window.draw(membersText);
}

void DungeonRenderer::drawLogWindow(sf::RenderWindow& window, const std::vector<std::string>& logQueue) {
    const sf::Font& font = m_game.getFont();
    float startX = 40.0f;
    float startY = 555.0f;
    float lineSpacing = 24.0f;

    // 최대 7개까지의 최근 로그 출력
    size_t logCount = logQueue.size();
    size_t startIdx = (logCount > 7) ? (logCount - 7) : 0;

    for (size_t i = startIdx; i < logCount; ++i) {
        sf::Text logText;
        logText.setFont(font);
        // [v0.5.0] std::string(UTF-8)을 직접 setString에 넘길 시 깨지는 SFML 한글 글리프 인코딩 오류 수정
        std::string logStr = logQueue[i];
        logText.setString(sf::String::fromUtf8(logStr.begin(), logStr.end()));
        logText.setCharacterSize(LocalizationManager::getInstance().getScaledTextSize(14));

        // 특별 로그 접두사 색상 변화 처리
        const auto& localization = LocalizationManager::getInstance();
        const std::string wallPrefix = "> " + localization.get("DUNGEON_WALL_BLOCKED");
        const std::string stairsPrefix = "> " + localization.get("MSG_DUNGEON_ESC");
        if (logQueue[i].rfind(wallPrefix, 0) == 0) {
            logText.setFillColor(sf::Color(255, 51, 51)); // 적색 (충돌 경고)
        } else if (logQueue[i].rfind(stairsPrefix, 0) == 0) {
            logText.setFillColor(sf::Color(255, 176, 0)); // 주황색 (특이 타일 안내)
        } else {
            logText.setFillColor(sf::Color(51, 255, 51)); // 일반 녹색
        }

        logText.setPosition(startX, startY + (i - startIdx) * lineSpacing);
        window.draw(logText);
    }
}

void DungeonRenderer::drawLine(sf::RenderWindow& window, float x1, float y1, float x2, float y2, sf::Color color) {
    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(x1, y1), color),
        sf::Vertex(sf::Vector2f(x2, y2), color)
    };
    window.draw(line, 2, sf::Lines);
}

} // namespace crawl
