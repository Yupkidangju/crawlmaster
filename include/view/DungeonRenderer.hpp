// [v0.2.0] DungeonRenderer.hpp 신규 작성
// 1인칭 와이어프레임 3D 뷰포트, 미니맵, HUD 윈도우 프레임 및 로그를 그리는 SFML 렌더러 헤더 선언.

#ifndef DUNGEON_RENDERER_HPP
#define DUNGEON_RENDERER_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

namespace crawl {

class DungeonMap; // 전방 선언
class Game;       // 전방 선언
class Party;
class PartyHudSnapshot;

// DungeonRenderer 클래스: 던전 상태에서의 모든 2D/3D 드로잉 연산 수행
class DungeonRenderer {
public:
    explicit DungeonRenderer(Game& game);
    ~DungeonRenderer() = default;

    // 던전 뷰포트 전체 그리기 (와이어프레임, 미니맵, 인터페이스 로그)
    void render(sf::RenderWindow& window, const DungeonMap& map, const Party& party,
                const std::vector<std::string>& logQueue);

private:
    Game& m_game;                       // 전역 Game 참조 (폰트 정보 획득용)
    
    // 뷰포트 상수 정의 (700x500 뷰포트, 마진 (20, 20))
    const float VIEWPORT_X = 20.0f;
    const float VIEWPORT_Y = 20.0f;
    const float VIEWPORT_W = 700.0f;
    const float VIEWPORT_H = 500.0f;
    const float CX = 370.0f;            // 뷰포트 가로 중심 (20 + 350)
    const float CY = 270.0f;            // 뷰포트 세로 중심 (20 + 250)

    // 원근 투영 높이 및 폭 테이블 (d = 0, 1, 2, 3, 4)
    // d=4는 d=3의 벽 끝 투영 처리를 위해 선언
    const float H_TABLE[5] = { 480.0f, 320.0f, 200.0f, 100.0f, 50.0f };
    const float W_TABLE[5] = { 600.0f, 400.0f, 240.0f, 120.0f, 60.0f };

    // 1인칭 3D 와이어프레임 벽 드로우 헬퍼
    void draw3DViewport(sf::RenderWindow& window, const DungeonMap& map);
    // 우측 미니맵(FOW 연동) 드로우 헬퍼
    void drawMiniMap(sf::RenderWindow& window, const DungeonMap& map);
    // 하단 및 우측 HUD 사각형 프레임 드로우 헬퍼
    void drawHUDFrame(sf::RenderWindow& window, const PartyHudSnapshot& partySnapshot);
    // 최근 스크롤 로그 내용 드로우 헬퍼
    void drawLogWindow(sf::RenderWindow& window, const std::vector<std::string>& logQueue);

    // 단순 선 그리기 유틸리티
    void drawLine(sf::RenderWindow& window, float x1, float y1, float x2, float y2, sf::Color color);
};

} // namespace crawl

#endif // DUNGEON_RENDERER_HPP
