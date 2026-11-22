#ifndef DUNGEON_MAP_HPP
#define DUNGEON_MAP_HPP

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace crawl {

enum class BiomeType {
    CATACOMBS = 0, // 1F: 수몰된 납골당
    CISTERN = 1,   // 2F: 익사자의 수로
    OSSUARY = 2,   // 3F: 저주받은 납골회랑
    FOUNDRY = 3,   // 4F: 흑요석 대장간
    SANCTUM = 4    // 5F: 용의 성소
};

enum class TileType {
    WALL = 0,
    EMPTY = 1,
    DOOR = 2,
    UPSTAIRS = 3,
    DOWNSTAIRS = 4,
    BOSS_GATE = 5,
    LOCKED_GATE = 6,
    SECRET_WALL = 7
};

enum class Direction {
    NORTH = 0,
    EAST = 1,
    SOUTH = 2,
    WEST = 3
};

class DungeonMap {
public:
    static constexpr int MAP_WIDTH = 20;
    static constexpr int MAP_HEIGHT = 20;

    DungeonMap();
    ~DungeonMap() = default;

    void generate(std::uint32_t seed);
    void generate(std::uint32_t seed, int floorNumber);

    [[nodiscard]] BiomeType getBiome() const { return m_biome; }
    [[nodiscard]] std::string getBiomeSlug() const;
    [[nodiscard]] static BiomeType biomeForFloor(int floorNumber);
    [[nodiscard]] static std::string biomeSlug(BiomeType biome);

    [[nodiscard]] TileType getTile(int x, int y) const;
    void setTile(int x, int y, TileType type);
    [[nodiscard]] bool isWalkable(int x, int y) const;
    [[nodiscard]] bool isOpaque(int x, int y) const;

    [[nodiscard]] bool isVisited(int x, int y) const;
    void setVisited(int x, int y, bool visited);
    [[nodiscard]] bool isStepped(int x, int y) const;
    void setStepped(int x, int y, bool stepped);

    bool unlockAllGates();
    bool revealSecretWall(int x, int y);

    [[nodiscard]] std::vector<std::pair<int, int>> findPath(int sx, int sy, int tx, int ty) const;

    [[nodiscard]] int getPlayerX() const;
    [[nodiscard]] int getPlayerY() const;
    [[nodiscard]] Direction getPlayerDir() const;
    [[nodiscard]] int getProgressPercent() const;
    [[nodiscard]] int getDistanceFromStart(int x, int y) const;
    [[nodiscard]] int getFloorNumber() const { return m_floorNumber; }

    [[nodiscard]] nlohmann::json toJson() const;
    static DungeonMap fromJson(const nlohmann::json& json, int floorNumber);
    void validateForFloor(int floorNumber) const;

    void setPlayerPos(int x, int y);
    void setPlayerDir(Direction dir);
    void getNextCoords(int& outX, int& outY, bool forward) const;
    void getStrafeCoords(int& outX, int& outY, bool right) const;
    void turn(bool right);

private:
    TileType m_tiles[MAP_WIDTH][MAP_HEIGHT]{};
    bool m_visited[MAP_WIDTH][MAP_HEIGHT]{};
    bool m_stepped[MAP_WIDTH][MAP_HEIGHT]{};
    int m_distanceFromStart[MAP_WIDTH][MAP_HEIGHT]{};

    int m_floorNumber = 1;
    BiomeType m_biome = BiomeType::CATACOMBS;
    int m_playerX = 1;
    int m_playerY = 1;
    Direction m_playerDir = Direction::EAST;
    int m_bossDistance = 1;

    void generateDFS(int cx, int cy, class SessionRng& random);
    void carveChambers(class SessionRng& random);
    void createLoops(class SessionRng& random);
    void placeLandmarks(int floorNumber, class SessionRng& random);
    void recomputeDistances();
};

} // namespace crawl

#endif // DUNGEON_MAP_HPP
