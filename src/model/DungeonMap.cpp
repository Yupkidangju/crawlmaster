// [v0.2.0] DungeonMap.cpp 신규 작성
// DFS 알고리즘을 사용한 랜덤 미로 생성 및 8개 무작위 순환 경로 뚫기, 플레이어의 이동과 충돌 검사를 처리한다.

#include "model/DungeonMap.hpp"
#include "core/SessionRng.hpp"
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iterator>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <string>

namespace crawl {

DungeonMap::DungeonMap()
    : m_playerX(1), m_playerY(1), m_playerDir(Direction::NORTH) {
    // [v0.6.0] 맵 전체를 벽으로 채우고 탐험 안개 및 밟은 타일 기록 초기화
    for (int x = 0; x < MAP_WIDTH; ++x) {
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            m_tiles[x][y] = TileType::WALL;
            m_visited[x][y] = false;
            m_stepped[x][y] = false;
        }
    }
}

void DungeonMap::generate(std::uint32_t seed) {
    generate(seed, 3);
}

void DungeonMap::generate(std::uint32_t seed, int floorNumber) {
    if (floorNumber < 1 || floorNumber > 3) {
        throw std::invalid_argument("던전 층은 1~3이어야 합니다.");
    }
    SessionRng generationRandom(seed);
    // 1. 모든 셀을 벽으로 완전 리셋
    for (int x = 0; x < MAP_WIDTH; ++x) {
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            m_tiles[x][y] = TileType::WALL;
            m_visited[x][y] = false;
            m_stepped[x][y] = false;
        }
    }

    // 2. DFS 미로 생성 시작 (홀수 좌표에서 개시하여 외벽 보호)
    generateDFS(1, 1, generationRandom);

    // 3. 미로의 순환 흐름을 위해 일부 벽 헐기
    createLoops(generationRandom);

    // 4. 입구(마을 복귀용 계단) 지정
    m_tiles[1][1] = TileType::UPSTAIRS;
    m_visited[1][1] = true;
    m_stepped[1][1] = true; // [v0.6.0] 스폰 위치 자동 마킹

    // 5. 플레이어 스폰 위치 동결
    m_playerX = 1;
    m_playerY = 1;
    m_playerDir = Direction::NORTH;

    placeLandmarks(floorNumber);

    std::cout << "[Map] 20x20 DFS 던전 맵 랜덤 생성 완료." << std::endl;
}

TileType DungeonMap::getTile(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return TileType::WALL;
    }
    return m_tiles[x][y];
}

bool DungeonMap::isWalkable(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return false;
    }
    return m_tiles[x][y] != TileType::WALL;
}

bool DungeonMap::isVisited(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return false;
    }
    return m_visited[x][y];
}

void DungeonMap::setVisited(int x, int y, bool visited) {
    if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
        m_visited[x][y] = visited;
    }
}

bool DungeonMap::isStepped(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return false;
    }
    return m_stepped[x][y];
}

void DungeonMap::setStepped(int x, int y, bool stepped) {
    if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
        m_stepped[x][y] = stepped;
    }
}

std::vector<std::pair<int, int>> DungeonMap::findPath(int sx, int sy, int tx, int ty) const {
    std::vector<std::pair<int, int>> path;
    // 시작점과 목표점이 같으면 빈 경로 반환
    if (sx == tx && sy == ty) {
        return path;
    }

    // 목적지가 방문하지 않은 구역이거나 벽인 경우 탐색하지 않음
    if (!isVisited(tx, ty) || !isWalkable(tx, ty)) {
        return path;
    }

    // BFS 탐색을 위한 큐와 방문 정보 기록
    std::queue<std::pair<int, int>> q;
    std::vector<std::vector<bool>> visited(MAP_WIDTH, std::vector<bool>(MAP_HEIGHT, false));
    std::vector<std::vector<std::pair<int, int>>> parent(MAP_WIDTH, std::vector<std::pair<int, int>>(MAP_HEIGHT, {-1, -1}));

    q.push({sx, sy});
    visited[sx][sy] = true;

    bool found = false;
    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();

        if (cx == tx && cy == ty) {
            found = true;
            break;
        }

        // 인접 4방향 탐색 (북, 남, 서, 동 순서 무관)
        int dx[] = {0, 0, -1, 1};
        int dy[] = {-1, 1, 0, 0};

        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
                // 탐험한 빈 바닥 타일만 경로로 허용
                if (!visited[nx][ny] && isWalkable(nx, ny) && isVisited(nx, ny)) {
                    visited[nx][ny] = true;
                    parent[nx][ny] = {cx, cy};
                    q.push({nx, ny});
                }
            }
        }
    }

    // 경로가 도출되면 역추적하여 빌드
    if (found) {
        std::pair<int, int> curr = {tx, ty};
        while (curr.first != -1 && curr.second != -1) {
            path.push_back(curr);
            curr = parent[curr.first][curr.second];
        }
        std::reverse(path.begin(), path.end());
        // 시작 지점(sx, sy)은 결과 경로에서 제외
        if (!path.empty()) {
            path.erase(path.begin());
        }
    }

    return path;
}

int DungeonMap::getPlayerX() const { return m_playerX; }
int DungeonMap::getPlayerY() const { return m_playerY; }
Direction DungeonMap::getPlayerDir() const { return m_playerDir; }
int DungeonMap::getProgressPercent() const {
    const int distance = m_distanceFromStart[m_playerX][m_playerY];
    if (distance < 0 || m_bossDistance <= 0) return 0;
    return std::clamp(distance * 100 / m_bossDistance, 0, 100);
}

int DungeonMap::getDistanceFromStart(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return -1;
    return m_distanceFromStart[x][y];
}

void DungeonMap::setPlayerPos(int x, int y) {
    m_playerX = x;
    m_playerY = y;
}

void DungeonMap::setPlayerDir(Direction dir) {
    m_playerDir = dir;
}

void DungeonMap::getNextCoords(int& outX, int& outY, bool forward) const {
    int dx = 0;
    int dy = 0;

    switch (m_playerDir) {
        case Direction::NORTH: dy = -1; break;
        case Direction::EAST:  dx = 1;  break;
        case Direction::SOUTH: dy = 1;  break;
        case Direction::WEST:  dx = -1; break;
    }

    if (forward) {
        outX = m_playerX + dx;
        outY = m_playerY + dy;
    } else {
        outX = m_playerX - dx;
        outY = m_playerY - dy;
    }
}

void DungeonMap::turn(bool right) {
    int dirVal = static_cast<int>(m_playerDir);
    if (right) {
        // 시계 방향 회전 (NORTH -> EAST -> SOUTH -> WEST)
        dirVal = (dirVal + 1) % 4;
    } else {
        // 반시계 방향 회전
        dirVal = (dirVal + 3) % 4;
    }
    m_playerDir = static_cast<Direction>(dirVal);
}

void DungeonMap::generateDFS(int cx, int cy, SessionRng& random) {
    m_tiles[cx][cy] = TileType::EMPTY;

    // 네 방향 이동 정의 (2칸 거리)
    struct DirOffset { int dx; int dy; };
    std::vector<DirOffset> directions = {
        {0, -2}, // 북
        {2, 0},  // 동
        {0, 2},  // 남
        {-2, 0}  // 서
    };

    for (std::size_t index = directions.size(); index > 1; --index) {
        const std::size_t swapIndex = static_cast<std::size_t>(
            random.rollRange(0, static_cast<int>(index) - 1));
        std::swap(directions[index - 1], directions[swapIndex]);
    }

    for (const auto& dir : directions) {
        int nx = cx + dir.dx;
        int ny = cy + dir.dy;

        // 맵 범위 내에 있고, 대상 셀이 벽(방문하지 않음)인지 체크
        if (nx > 0 && nx < MAP_WIDTH - 1 && ny > 0 && ny < MAP_HEIGHT - 1) {
            if (m_tiles[nx][ny] == TileType::WALL) {
                // 현재와 대상 셀의 중간에 놓인 벽을 통로로 개방
                m_tiles[cx + dir.dx / 2][cy + dir.dy / 2] = TileType::EMPTY;
                // 대상 셀을 개방하고 재귀 탐색
                generateDFS(nx, ny, random);
            }
        }
    }
}

void DungeonMap::createLoops(SessionRng& random) {
    // 맵 전체를 돌며 가로지르는 무작위 벽 8개를 통로로 전환하여 외골격 순환 고리 형성
    std::vector<std::pair<int, int>> candidates;
    
    // 외벽을 제외한 내부 벽 타일 후보 수집
    for (int x = 2; x < MAP_WIDTH - 2; ++x) {
        for (int y = 2; y < MAP_HEIGHT - 2; ++y) {
            if (m_tiles[x][y] == TileType::WALL) {
                // 가로로 통과하는 통로 사이에 낀 벽
                bool horizontalPath = (m_tiles[x-1][y] == TileType::EMPTY && m_tiles[x+1][y] == TileType::EMPTY);
                // 세로로 통과하는 통로 사이에 낀 벽
                bool verticalPath = (m_tiles[x][y-1] == TileType::EMPTY && m_tiles[x][y+1] == TileType::EMPTY);
                
                if (horizontalPath || verticalPath) {
                    candidates.push_back({x, y});
                }
            }
        }
    }

    if (!candidates.empty()) {
        for (std::size_t index = candidates.size(); index > 1; --index) {
            const std::size_t swapIndex = static_cast<std::size_t>(
                random.rollRange(0, static_cast<int>(index) - 1));
            std::swap(candidates[index - 1], candidates[swapIndex]);
        }
        int loopsToCreate = std::min(8, static_cast<int>(candidates.size()));
        for (int i = 0; i < loopsToCreate; ++i) {
            m_tiles[candidates[i].first][candidates[i].second] = TileType::EMPTY;
        }
    }
}

void DungeonMap::recomputeDistances() {
    int distance[MAP_WIDTH][MAP_HEIGHT];
    for (auto& column : distance) {
        std::fill(std::begin(column), std::end(column), -1);
    }

    std::queue<std::pair<int, int>> pending;
    pending.push({1, 1});
    distance[1][1] = 0;
    std::pair<int, int> farthest = {1, 1};

    const int deltaX[4] = {0, 1, 0, -1};
    const int deltaY[4] = {-1, 0, 1, 0};
    while (!pending.empty()) {
        const auto [x, y] = pending.front();
        pending.pop();
        if (distance[x][y] > distance[farthest.first][farthest.second]) farthest = {x, y};
        for (int direction = 0; direction < 4; ++direction) {
            const int nextX = x + deltaX[direction];
            const int nextY = y + deltaY[direction];
            if (nextX < 0 || nextX >= MAP_WIDTH || nextY < 0 || nextY >= MAP_HEIGHT ||
                distance[nextX][nextY] >= 0 || !isWalkable(nextX, nextY)) continue;
            distance[nextX][nextY] = distance[x][y] + 1;
            pending.push({nextX, nextY});
        }
    }

    m_bossDistance = std::max(1, distance[farthest.first][farthest.second]);
    for (int x = 0; x < MAP_WIDTH; ++x) {
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            m_distanceFromStart[x][y] = distance[x][y];
        }
    }
}

void DungeonMap::placeLandmarks(int floorNumber) {
    recomputeDistances();
    std::pair<int, int> farthest = {1, 1};
    for (int x = 0; x < MAP_WIDTH; ++x) {
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            if (m_distanceFromStart[x][y] > m_distanceFromStart[farthest.first][farthest.second]) {
                farthest = {x, y};
            }
        }
    }

    const int targetDoorDistance = m_distanceFromStart[farthest.first][farthest.second] / 2;
    std::pair<int, int> door = {1, 1};
    int bestDifference = INT_MAX;
    for (int x = 0; x < MAP_WIDTH; ++x) {
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            if (m_distanceFromStart[x][y] <= 0 || std::pair{x, y} == farthest) continue;
            const int difference = std::abs(m_distanceFromStart[x][y] - targetDoorDistance);
            if (difference < bestDifference) {
                bestDifference = difference;
                door = {x, y};
            }
        }
    }

    m_tiles[door.first][door.second] = TileType::DOOR;
    m_tiles[farthest.first][farthest.second] = floorNumber < 3
        ? TileType::DOWNSTAIRS : TileType::BOSS_GATE;
}

nlohmann::json DungeonMap::toJson() const {
    auto tileCode = [](TileType tile) {
        switch (tile) {
            case TileType::WALL: return '#';
            case TileType::EMPTY: return '.';
            case TileType::DOOR: return 'D';
            case TileType::UPSTAIRS: return 'U';
            case TileType::DOWNSTAIRS: return 'V';
            case TileType::BOSS_GATE: return 'B';
        }
        return '#';
    };
    nlohmann::json tiles = nlohmann::json::array();
    nlohmann::json visited = nlohmann::json::array();
    nlohmann::json stepped = nlohmann::json::array();
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        std::string tileRow;
        std::string visitedRow;
        std::string steppedRow;
        for (int x = 0; x < MAP_WIDTH; ++x) {
            tileRow.push_back(tileCode(m_tiles[x][y]));
            visitedRow.push_back(m_visited[x][y] ? '1' : '0');
            steppedRow.push_back(m_stepped[x][y] ? '1' : '0');
        }
        tiles.push_back(tileRow);
        visited.push_back(visitedRow);
        stepped.push_back(steppedRow);
    }
    return {{"tiles", tiles}, {"visited", visited}, {"stepped", stepped}};
}

DungeonMap DungeonMap::fromJson(const nlohmann::json& json, int floorNumber) {
    if (!json.is_object() || floorNumber < 1 || floorNumber > 3) {
        throw std::runtime_error("던전 층 snapshot 형식이 잘못됐습니다.");
    }
    const auto& tiles = json.at("tiles");
    const auto& visited = json.at("visited");
    const auto& stepped = json.at("stepped");
    if (!tiles.is_array() || !visited.is_array() || !stepped.is_array() ||
        tiles.size() != MAP_HEIGHT || visited.size() != MAP_HEIGHT || stepped.size() != MAP_HEIGHT) {
        throw std::runtime_error("던전 층 snapshot 높이가 잘못됐습니다.");
    }

    DungeonMap map;
    int upCount = 0;
    int downCount = 0;
    int bossCount = 0;
    auto decode = [](char code) {
        switch (code) {
            case '#': return TileType::WALL;
            case '.': return TileType::EMPTY;
            case 'D': return TileType::DOOR;
            case 'U': return TileType::UPSTAIRS;
            case 'V': return TileType::DOWNSTAIRS;
            case 'B': return TileType::BOSS_GATE;
            default: throw std::runtime_error("알 수 없는 던전 타일 코드입니다.");
        }
    };
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        const std::string tileRow = tiles.at(y).get<std::string>();
        const std::string visitedRow = visited.at(y).get<std::string>();
        const std::string steppedRow = stepped.at(y).get<std::string>();
        if (tileRow.size() != MAP_WIDTH || visitedRow.size() != MAP_WIDTH ||
            steppedRow.size() != MAP_WIDTH) {
            throw std::runtime_error("던전 층 snapshot 너비가 잘못됐습니다.");
        }
        for (int x = 0; x < MAP_WIDTH; ++x) {
            if ((visitedRow[x] != '0' && visitedRow[x] != '1') ||
                (steppedRow[x] != '0' && steppedRow[x] != '1')) {
                throw std::runtime_error("던전 탐험 비트가 잘못됐습니다.");
            }
            map.m_tiles[x][y] = decode(tileRow[x]);
            map.m_visited[x][y] = visitedRow[x] == '1';
            map.m_stepped[x][y] = steppedRow[x] == '1';
            if (map.m_stepped[x][y] && (!map.m_visited[x][y] || map.m_tiles[x][y] == TileType::WALL)) {
                throw std::runtime_error("밟은 타일은 방문한 이동 가능 타일이어야 합니다.");
            }
            if (map.m_tiles[x][y] == TileType::UPSTAIRS) ++upCount;
            if (map.m_tiles[x][y] == TileType::DOWNSTAIRS) ++downCount;
            if (map.m_tiles[x][y] == TileType::BOSS_GATE) ++bossCount;
            if ((x == 0 || y == 0 || x == MAP_WIDTH - 1 || y == MAP_HEIGHT - 1) &&
                map.m_tiles[x][y] != TileType::WALL) {
                throw std::runtime_error("던전 외벽은 모두 벽이어야 합니다.");
            }
        }
    }
    if (upCount != 1 || downCount != (floorNumber < 3 ? 1 : 0) ||
        bossCount != (floorNumber == 3 ? 1 : 0)) {
        throw std::runtime_error("층별 계단 또는 최종 관문 수가 잘못됐습니다.");
    }
    if (map.m_tiles[1][1] != TileType::UPSTAIRS) {
        throw std::runtime_error("층 입구 계단은 (1,1)에 있어야 합니다.");
    }
    map.m_playerX = 1;
    map.m_playerY = 1;
    map.m_playerDir = Direction::NORTH;
    map.recomputeDistances();
    map.validateForFloor(floorNumber);
    return map;
}

void DungeonMap::validateForFloor(int floorNumber) const {
    if (floorNumber < 1 || floorNumber > 3) throw std::runtime_error("던전 층 번호가 잘못됐습니다.");
    int upCount = 0;
    int downCount = 0;
    int bossCount = 0;
    int doorCount = 0;
    int farthestDistance = 0;
    int terminalDistance = -1;
    for (int x = 0; x < MAP_WIDTH; ++x) {
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            const TileType tile = m_tiles[x][y];
            if ((x == 0 || y == 0 || x == MAP_WIDTH - 1 || y == MAP_HEIGHT - 1) &&
                tile != TileType::WALL) throw std::runtime_error("던전 외벽이 열려 있습니다.");
            if (m_stepped[x][y] && (!m_visited[x][y] || tile == TileType::WALL)) {
                throw std::runtime_error("밟은 타일 상태가 유효하지 않습니다.");
            }
            if (tile == TileType::UPSTAIRS) ++upCount;
            if (tile == TileType::DOWNSTAIRS) { ++downCount; terminalDistance = m_distanceFromStart[x][y]; }
            if (tile == TileType::BOSS_GATE) { ++bossCount; terminalDistance = m_distanceFromStart[x][y]; }
            if (tile == TileType::DOOR) ++doorCount;
            if (isWalkable(x, y)) {
                if (m_distanceFromStart[x][y] < 0) throw std::runtime_error("도달할 수 없는 던전 바닥이 있습니다.");
                farthestDistance = std::max(farthestDistance, m_distanceFromStart[x][y]);
            }
        }
    }
    if (m_tiles[1][1] != TileType::UPSTAIRS || !m_visited[1][1] || !m_stepped[1][1]) {
        throw std::runtime_error("층 입구의 계단/FOW 상태가 잘못됐습니다.");
    }
    if (upCount != 1 || doorCount != 1 || downCount != (floorNumber < 3 ? 1 : 0) ||
        bossCount != (floorNumber == 3 ? 1 : 0)) {
        throw std::runtime_error("층 landmark 수가 canonical 계약과 다릅니다.");
    }
    if (terminalDistance != farthestDistance) {
        throw std::runtime_error("하행 계단 또는 최종 관문이 최장 거리 타일이 아닙니다.");
    }
}

} // namespace crawl
