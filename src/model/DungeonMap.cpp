#include "model/DungeonMap.hpp"
#include "core/SessionRng.hpp"

#include <algorithm>
#include <array>
#include <queue>
#include <stdexcept>

namespace crawl {

DungeonMap::DungeonMap() {
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            m_tiles[x][y] = TileType::WALL;
            m_visited[x][y] = false;
            m_stepped[x][y] = false;
            m_distanceFromStart[x][y] = -1;
        }
    }
}

BiomeType DungeonMap::biomeForFloor(int floorNumber) {
    switch (std::clamp(floorNumber, 1, 5)) {
        case 1: return BiomeType::CATACOMBS;
        case 2: return BiomeType::CISTERN;
        case 3: return BiomeType::OSSUARY;
        case 4: return BiomeType::FOUNDRY;
        default: return BiomeType::SANCTUM;
    }
}

std::string DungeonMap::biomeSlug(BiomeType biome) {
    switch (biome) {
        case BiomeType::CATACOMBS: return "catacombs";
        case BiomeType::CISTERN:   return "cistern";
        case BiomeType::OSSUARY:   return "ossuary";
        case BiomeType::FOUNDRY:   return "foundry";
        case BiomeType::SANCTUM:   return "sanctum";
    }
    return "catacombs";
}

std::string DungeonMap::getBiomeSlug() const {
    return biomeSlug(m_biome);
}

void DungeonMap::generate(std::uint32_t seed) {
    generate(seed, 1);
}

void DungeonMap::generate(std::uint32_t seed, int floorNumber) {
    m_floorNumber = std::clamp(floorNumber, 1, 5);
    m_biome = biomeForFloor(m_floorNumber);

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            m_tiles[x][y] = TileType::WALL;
            m_visited[x][y] = false;
            m_stepped[x][y] = false;
            m_distanceFromStart[x][y] = -1;
        }
    }

    SessionRng random(seed);
    generateDFS(1, 1, random);
    carveChambers(random);
    createLoops(random);

    m_playerX = 1;
    m_playerY = 1;
    m_playerDir = Direction::EAST;
    if (!isWalkable(2, 1) && isWalkable(1, 2)) {
        m_playerDir = Direction::SOUTH;
    }

    placeLandmarks(m_floorNumber, random);
    recomputeDistances();

    m_visited[1][1] = true;
    m_stepped[1][1] = true;
}

void DungeonMap::generateDFS(int cx, int cy, SessionRng& random) {
    m_tiles[cx][cy] = TileType::EMPTY;
    std::array<int, 4> dirs = {0, 1, 2, 3};
    for (int i = 3; i > 0; --i) {
        const int j = random.rollRange(0, i);
        std::swap(dirs[static_cast<std::size_t>(i)], dirs[static_cast<std::size_t>(j)]);
    }

    static constexpr int dx[4] = {0, 2, 0, -2};
    static constexpr int dy[4] = {-2, 0, 2, 0};

    for (int d : dirs) {
        const int nx = cx + dx[d];
        const int ny = cy + dy[d];
        if (nx > 0 && nx < MAP_WIDTH - 1 && ny > 0 && ny < MAP_HEIGHT - 1 &&
            m_tiles[nx][ny] == TileType::WALL) {
            m_tiles[cx + dx[d] / 2][cy + dy[d] / 2] = TileType::EMPTY;
            generateDFS(nx, ny, random);
        }
    }
}

void DungeonMap::carveChambers(SessionRng& random) {
    // 각 층마다 3개의 작은 던전 방(3x3)을 파내어 3D 공간감과 시야 변화를 극대화한다.
    for (int r = 0; r < 3; ++r) {
        const int rx = random.rollRange(3, MAP_WIDTH - 6);
        const int ry = random.rollRange(3, MAP_HEIGHT - 6);
        for (int y = ry; y < ry + 3; ++y) {
            for (int x = rx; x < rx + 3; ++x) {
                if (x > 1 && x < MAP_WIDTH - 2 && y > 1 && y < MAP_HEIGHT - 2) {
                    m_tiles[x][y] = TileType::EMPTY;
                }
            }
        }
    }
}

void DungeonMap::createLoops(SessionRng& random) {
    for (int y = 2; y < MAP_HEIGHT - 2; ++y) {
        for (int x = 2; x < MAP_WIDTH - 2; ++x) {
            if (m_tiles[x][y] != TileType::WALL) continue;
            const bool horiz = (m_tiles[x - 1][y] == TileType::EMPTY && m_tiles[x + 1][y] == TileType::EMPTY &&
                                m_tiles[x][y - 1] == TileType::WALL && m_tiles[x][y + 1] == TileType::WALL);
            const bool vert = (m_tiles[x][y - 1] == TileType::EMPTY && m_tiles[x][y + 1] == TileType::EMPTY &&
                               m_tiles[x - 1][y] == TileType::WALL && m_tiles[x + 1][y] == TileType::WALL);
            if ((horiz || vert) && random.rollRange(1, 100) <= 18) {
                m_tiles[x][y] = (random.rollRange(1, 100) <= 28) ? TileType::DOOR : TileType::EMPTY;
            }
        }
    }
}

void DungeonMap::placeLandmarks(int floorNumber, SessionRng& random) {
    m_tiles[1][1] = TileType::UPSTAIRS;
    recomputeDistances();

    // 시작점에서 가장 먼 통로에 다음 층 계단 또는 5층 최종 보스 게이트 배치
    int farX = 1;
    int farY = 1;
    int maxDist = -1;
    for (int y = 1; y < MAP_HEIGHT - 1; ++y) {
        for (int x = 1; x < MAP_WIDTH - 1; ++x) {
            if (m_distanceFromStart[x][y] > maxDist && m_tiles[x][y] == TileType::EMPTY) {
                maxDist = m_distanceFromStart[x][y];
                farX = x;
                farY = y;
            }
        }
    }
    m_tiles[farX][farY] = (floorNumber >= 5) ? TileType::BOSS_GATE : TileType::DOWNSTAIRS;

    // 숏컷용 잠긴 철창 문 1개 배치 (단, 기존 주 경로의 도달성을 깨뜨리지 않는 루프 지점에만 배치!)
    bool placedLockedGate = false;
    for (int y = 2; y < MAP_HEIGHT - 2 && !placedLockedGate; ++y) {
        for (int x = 2; x < MAP_WIDTH - 2 && !placedLockedGate; ++x) {
            if (m_tiles[x][y] != TileType::DOOR && m_tiles[x][y] != TileType::EMPTY) continue;
            if ((x == 1 && y == 1) || (x == farX && y == farY)) continue;
            if (m_distanceFromStart[x][y] < 4 || m_distanceFromStart[x][y] > 14) continue;
            const TileType prev = m_tiles[x][y];
            m_tiles[x][y] = TileType::LOCKED_GATE;
            if (!findPath(1, 1, farX, farY).empty()) {
                placedLockedGate = true;
            } else {
                m_tiles[x][y] = prev;
            }
        }
    }

    // 비밀 벽(SECRET_WALL) 1곳 배치: 외곽 경계 안쪽에서 통로 옆 벽 뒤에 1칸 비밀 방 생성
    std::vector<std::tuple<int, int, int, int>> secretCandidates;
    static constexpr int dx[4] = {0, 1, 0, -1};
    static constexpr int dy[4] = {-1, 0, 1, 0};
    for (int y = 2; y < MAP_HEIGHT - 2; ++y) {
        for (int x = 2; x < MAP_WIDTH - 2; ++x) {
            if (m_tiles[x][y] != TileType::WALL) continue;
            for (int d = 0; d < 4; ++d) {
                const int px = x - dx[d];
                const int py = y - dy[d];
                const int bx = x + dx[d];
                const int by = y + dy[d];
                if (bx <= 1 || bx >= MAP_WIDTH - 2 || by <= 1 || by >= MAP_HEIGHT - 2) continue;
                if (m_tiles[px][py] == TileType::EMPTY && m_tiles[bx][by] == TileType::WALL) {
                    secretCandidates.emplace_back(x, y, bx, by);
                }
            }
        }
    }
    if (!secretCandidates.empty()) {
        const auto& chosen = secretCandidates[static_cast<std::size_t>(
            random.rollRange(0, static_cast<int>(secretCandidates.size()) - 1))];
        const auto [wx, wy, rx, ry] = chosen;
        m_tiles[wx][wy] = TileType::SECRET_WALL;
        m_tiles[rx][ry] = TileType::EMPTY;
    }
}

void DungeonMap::recomputeDistances() {
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            m_distanceFromStart[x][y] = -1;
        }
    }
    std::queue<std::pair<int, int>> q;
    m_distanceFromStart[1][1] = 0;
    q.push({1, 1});
    int maxDist = 1;

    static constexpr int dx[4] = {0, 1, 0, -1};
    static constexpr int dy[4] = {-1, 0, 1, 0};

    while (!q.empty()) {
        const auto [cx, cy] = q.front();
        q.pop();
        for (int d = 0; d < 4; ++d) {
            const int nx = cx + dx[d];
            const int ny = cy + dy[d];
            if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) continue;
            if (!isWalkable(nx, ny) || m_distanceFromStart[nx][ny] != -1) continue;
            m_distanceFromStart[nx][ny] = m_distanceFromStart[cx][cy] + 1;
            maxDist = std::max(maxDist, m_distanceFromStart[nx][ny]);
            q.push({nx, ny});
        }
    }
    m_bossDistance = std::max(1, maxDist);
}

TileType DungeonMap::getTile(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return TileType::WALL;
    return m_tiles[x][y];
}

void DungeonMap::setTile(int x, int y, TileType type) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return;
    m_tiles[x][y] = type;
    recomputeDistances();
}

bool DungeonMap::isWalkable(int x, int y) const {
    const TileType t = getTile(x, y);
    return t == TileType::EMPTY || t == TileType::DOOR ||
           t == TileType::UPSTAIRS || t == TileType::DOWNSTAIRS ||
           t == TileType::BOSS_GATE;
}

bool DungeonMap::isOpaque(int x, int y) const {
    const TileType t = getTile(x, y);
    return t == TileType::WALL || t == TileType::SECRET_WALL;
}

bool DungeonMap::isVisited(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return false;
    return m_visited[x][y];
}

void DungeonMap::setVisited(int x, int y, bool visited) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return;
    m_visited[x][y] = visited;
}

bool DungeonMap::isStepped(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return false;
    return m_stepped[x][y];
}

void DungeonMap::setStepped(int x, int y, bool stepped) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return;
    m_stepped[x][y] = stepped;
}

bool DungeonMap::unlockAllGates() {
    bool changed = false;
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            if (m_tiles[x][y] == TileType::LOCKED_GATE) {
                m_tiles[x][y] = TileType::DOOR;
                changed = true;
            }
        }
    }
    if (changed) recomputeDistances();
    return changed;
}

bool DungeonMap::revealSecretWall(int x, int y) {
    if (getTile(x, y) != TileType::SECRET_WALL) return false;
    m_tiles[x][y] = TileType::EMPTY;
    m_visited[x][y] = true;
    recomputeDistances();
    return true;
}

std::vector<std::pair<int, int>> DungeonMap::findPath(int sx, int sy, int tx, int ty) const {
    if (!isWalkable(tx, ty)) return {};
    if (sx == tx && sy == ty) return {};

    bool visited[MAP_WIDTH][MAP_HEIGHT]{};
    std::pair<int, int> parent[MAP_WIDTH][MAP_HEIGHT]{};
    std::queue<std::pair<int, int>> q;
    visited[sx][sy] = true;
    parent[sx][sy] = {-1, -1};
    q.push({sx, sy});

    static constexpr int dx[4] = {0, 1, 0, -1};
    static constexpr int dy[4] = {-1, 0, 1, 0};

    bool found = false;
    while (!q.empty()) {
        const auto [cx, cy] = q.front();
        q.pop();
        if (cx == tx && cy == ty) {
            found = true;
            break;
        }
        for (int d = 0; d < 4; ++d) {
            const int nx = cx + dx[d];
            const int ny = cy + dy[d];
            if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) continue;
            if (visited[nx][ny] || !isWalkable(nx, ny)) continue;
            visited[nx][ny] = true;
            parent[nx][ny] = {cx, cy};
            q.push({nx, ny});
        }
    }
    if (!found) return {};

    std::vector<std::pair<int, int>> path;
    int cx = tx;
    int cy = ty;
    while (!(cx == sx && cy == sy)) {
        path.push_back({cx, cy});
        const auto p = parent[cx][cy];
        cx = p.first;
        cy = p.second;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

int DungeonMap::getPlayerX() const { return m_playerX; }
int DungeonMap::getPlayerY() const { return m_playerY; }
Direction DungeonMap::getPlayerDir() const { return m_playerDir; }

int DungeonMap::getProgressPercent() const {
    const int d = getDistanceFromStart(m_playerX, m_playerY);
    if (d < 0) return 0;
    return std::clamp(d * 100 / std::max(1, m_bossDistance), 0, 100);
}

int DungeonMap::getDistanceFromStart(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return -1;
    return m_distanceFromStart[x][y];
}

void DungeonMap::setPlayerPos(int x, int y) {
    if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
        m_playerX = x;
        m_playerY = y;
    }
}

void DungeonMap::setPlayerDir(Direction dir) { m_playerDir = dir; }

void DungeonMap::getNextCoords(int& outX, int& outY, bool forward) const {
    int dx = 0;
    int dy = 0;
    switch (m_playerDir) {
        case Direction::NORTH: dy = -1; break;
        case Direction::EAST:  dx = 1;  break;
        case Direction::SOUTH: dy = 1;  break;
        case Direction::WEST:  dx = -1; break;
    }
    if (!forward) {
        dx = -dx;
        dy = -dy;
    }
    outX = m_playerX + dx;
    outY = m_playerY + dy;
}

void DungeonMap::getStrafeCoords(int& outX, int& outY, bool right) const {
    int dx = 0;
    int dy = 0;
    switch (m_playerDir) {
        case Direction::NORTH: dx = 1;  break;
        case Direction::EAST:  dy = 1;  break;
        case Direction::SOUTH: dx = -1; break;
        case Direction::WEST:  dy = -1; break;
    }
    if (!right) {
        dx = -dx;
        dy = -dy;
    }
    outX = m_playerX + dx;
    outY = m_playerY + dy;
}

void DungeonMap::turn(bool right) {
    const int cur = static_cast<int>(m_playerDir);
    m_playerDir = static_cast<Direction>((cur + (right ? 1 : 3)) % 4);
}

nlohmann::json DungeonMap::toJson() const {
    nlohmann::json tiles = nlohmann::json::array();
    nlohmann::json visited = nlohmann::json::array();
    nlohmann::json stepped = nlohmann::json::array();
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        nlohmann::json tRow = nlohmann::json::array();
        nlohmann::json vRow = nlohmann::json::array();
        nlohmann::json sRow = nlohmann::json::array();
        for (int x = 0; x < MAP_WIDTH; ++x) {
            tRow.push_back(static_cast<int>(m_tiles[x][y]));
            vRow.push_back(m_visited[x][y]);
            sRow.push_back(m_stepped[x][y]);
        }
        tiles.push_back(std::move(tRow));
        visited.push_back(std::move(vRow));
        stepped.push_back(std::move(sRow));
    }
    return nlohmann::json{
        {"floor", m_floorNumber},
        {"biome", static_cast<int>(m_biome)},
        {"tiles", std::move(tiles)},
        {"visited", std::move(visited)},
        {"stepped", std::move(stepped)}
    };
}

DungeonMap DungeonMap::fromJson(const nlohmann::json& json, int floorNumber) {
    if (!json.is_object()) throw std::runtime_error("floor map must be object");
    DungeonMap map;
    map.m_floorNumber = std::clamp(floorNumber, 1, 5);
    map.m_biome = biomeForFloor(map.m_floorNumber);
    const auto& tiles = json.at("tiles");
    const auto& visited = json.at("visited");
    const auto& stepped = json.at("stepped");
    if (!tiles.is_array() || tiles.size() != MAP_HEIGHT ||
        !visited.is_array() || visited.size() != MAP_HEIGHT ||
        !stepped.is_array() || stepped.size() != MAP_HEIGHT) {
        throw std::runtime_error("invalid floor grid height");
    }
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        if (tiles[y].size() != MAP_WIDTH || visited[y].size() != MAP_WIDTH || stepped[y].size() != MAP_WIDTH) {
            throw std::runtime_error("invalid floor grid width");
        }
        for (int x = 0; x < MAP_WIDTH; ++x) {
            const int tv = tiles[y][x].get<int>();
            if (tv < 0 || tv > static_cast<int>(TileType::SECRET_WALL)) {
                throw std::runtime_error("invalid tile value");
            }
            map.m_tiles[x][y] = static_cast<TileType>(tv);
            map.m_visited[x][y] = visited[y][x].get<bool>();
            map.m_stepped[x][y] = stepped[y][x].get<bool>();
        }
    }
    map.m_playerX = 1;
    map.m_playerY = 1;
    map.m_playerDir = map.isWalkable(2, 1) ? Direction::EAST : Direction::SOUTH;
    map.recomputeDistances();
    map.validateForFloor(floorNumber);
    return map;
}

void DungeonMap::validateForFloor(int floorNumber) const {
    if (m_tiles[1][1] != TileType::UPSTAIRS) {
        throw std::runtime_error("floor entrance must be UPSTAIRS");
    }
    const TileType expectedExit = (floorNumber >= 5) ? TileType::BOSS_GATE : TileType::DOWNSTAIRS;
    bool exitFound = false;
    for (int y = 1; y < MAP_HEIGHT - 1; ++y) {
        for (int x = 1; x < MAP_WIDTH - 1; ++x) {
            if (m_tiles[x][y] == expectedExit && m_distanceFromStart[x][y] > 0) {
                exitFound = true;
            }
        }
    }
    if (!exitFound) {
        throw std::runtime_error("floor exit is missing or unreachable");
    }
}

} // namespace crawl
