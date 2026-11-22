#ifndef DUNGEON_STATE_HPP
#define DUNGEON_STATE_HPP

#include "core/GameState.hpp"
#include "model/DungeonMap.hpp"
#include "model/DungeonWorld.hpp"
#include "view/DungeonRenderer.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace crawl {

class Game;

class DungeonState : public GameState {
public:
    explicit DungeonState(Game& game, int startFloor = 1);
    ~DungeonState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

    void addLog(const std::string& message);
    [[nodiscard]] int getCurrentFloor() const { return m_currentFloor; }

private:
    DungeonMap& currentFloorMap();
    const DungeonMap& currentFloorMap() const;
    void updateFogOfWar();
    void attemptMoveTo(int nx, int ny);
    void handleStepTriggers(bool fromAutoWalk);
    void handleInteractAction();
    void updateInteractPrompt();
    void handleMapClick(sf::Vector2f mousePos, sf::FloatRect gridRect);
    void triggerTrap(WorldObject& trap);
    void attemptDisarmTrap(WorldObject& trap);

    Game& m_game;
    int m_currentFloor = 1;
    std::unique_ptr<DungeonRenderer> m_renderer;
    DungeonHudInteraction m_ui;
    std::vector<std::string> m_logQueue;
    std::vector<std::pair<int, int>> m_autoWalkPath;
    float m_autoWalkTimer = 0.0f;
    int m_stepsSinceEncounter = 0;
};

} // namespace crawl

#endif // DUNGEON_STATE_HPP
