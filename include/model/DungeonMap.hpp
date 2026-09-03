// [v0.2.0] DungeonMap.hpp 신규 작성
// 20x20 크기의 격자형 던전 맵 데이터를 관리하며, DFS 무작위 미로 생성과 플레이어 이동/충돌 검사를 수행한다.

#ifndef DUNGEON_MAP_HPP
#define DUNGEON_MAP_HPP

#include <cstdint>
#include <vector>

namespace crawl {

// 타일 타입 정의
enum class TileType {
    WALL,       // 벽 (이동 불가능)
    EMPTY,      // 빈 바닥 (이동 가능)
    DOOR,       // 문 (이동 가능)
    UPSTAIRS,   // 마을 복귀용 계단 (이동 가능)
    BOSS_GATE   // 최종 전투 관문 (이동 가능)
};

// 플레이어 방향 정의
enum class Direction {
    NORTH,      // 북쪽 (-Y)
    EAST,       // 동쪽 (+X)
    SOUTH,      // 남쪽 (+Y)
    WEST        // 서쪽 (-X)
};

// DungeonMap 클래스: 던전 구조 및 안개(Fog of War), 플레이어 좌표 정보를 관리
class DungeonMap {
public:
    static const int MAP_WIDTH = 20;
    static const int MAP_HEIGHT = 20;

    DungeonMap();
    ~DungeonMap() = default;

    // 미로 생성 및 플레이어 기본 스폰 위치 설정
    void generate();
    void generate(std::uint32_t seed);

    // 특정 타일 좌표의 타입 반환
    TileType getTile(int x, int y) const;

    // 플레이어가 통과 가능한 타일인지 여부 판정
    bool isWalkable(int x, int y) const;

    // 방문 여부 확인 및 갱신 (Fog of War)
    bool isVisited(int x, int y) const;
    void setVisited(int x, int y, bool visited);

    // [v0.6.0] 플레이어가 직접 밟고 지나간 타일 확인 및 갱신
    bool isStepped(int x, int y) const;
    void setStepped(int x, int y, bool stepped);

    // [v0.6.0] BFS 기반 벽 회피 최단 경로 탐색 함수
    std::vector<std::pair<int, int>> findPath(int sx, int sy, int tx, int ty) const;

    // 플레이어의 현재 위치 및 방향 제어 (Getter / Setter)
    int getPlayerX() const;
    int getPlayerY() const;
    Direction getPlayerDir() const;
    int getProgressPercent() const;

    void setPlayerPos(int x, int y);
    void setPlayerDir(Direction dir);

    // 전진/후진 시 예상되는 다음 좌표 계산
    void getNextCoords(int& outX, int& outY, bool forward) const;

    // 좌/우 90도 회전 처리
    void turn(bool right);

private:
    TileType m_tiles[MAP_WIDTH][MAP_HEIGHT];  // 격자 벽/통로 타일 배열
    bool m_visited[MAP_WIDTH][MAP_HEIGHT];    // 미니맵 탐험 안개 플래그
    bool m_stepped[MAP_WIDTH][MAP_HEIGHT];    // [v0.6.0] 플레이어가 실제 밟은 타일 플래그
    
    int m_playerX;                            // 플레이어 격자 X 좌표
    int m_playerY;                            // 플레이어 격자 Y 좌표
    Direction m_playerDir;                    // 플레이어 시선 방향
    int m_distanceFromStart[MAP_WIDTH][MAP_HEIGHT]{};
    int m_bossDistance = 1;

    // DFS 재귀 미로 생성 헬퍼 함수
    void generateDFS(int cx, int cy, class SessionRng& random);
    // 미로의 막힌 벽을 일부 헐어 순환 루프를 만드는 함수
    void createLoops(class SessionRng& random);
    void placeLandmarks();
};

} // namespace crawl

#endif // DUNGEON_MAP_HPP
