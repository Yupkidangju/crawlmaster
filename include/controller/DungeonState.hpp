// [v0.2.0] DungeonState.hpp 신규 작성
// 던전 탐험 상태 컨트롤러 선언. 20x20 미로 모델과 1인칭 렌더러를 탑재하여 이동 제어 및 충돌 판정을 처리한다.

#ifndef DUNGEON_STATE_HPP
#define DUNGEON_STATE_HPP

#include "core/GameState.hpp"
#include "model/DungeonMap.hpp"
#include "view/DungeonRenderer.hpp"
#include <vector>
#include <string>

namespace crawl {

class Game; // 전방 선언
enum class EncounterTier;

class DungeonState : public GameState {
public:
    explicit DungeonState(Game& game);
    ~DungeonState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    friend class ControllerTestAccess;
    Game& m_game;                       // 전역 Game 참조
    DungeonMap m_map;                   // 20x20 격자 던전 맵 모델
    DungeonRenderer m_renderer;         // 1인칭 3D 및 HUD 렌더러

    std::vector<std::string> m_logQueue; // 하단 TUI에 표시될 텍스트 로그 목록

    // [v0.6.0] 자동 이동 경로, 현재 인덱스 및 시간 누적 타이머
    std::vector<std::pair<int, int>> m_autoPath;
    size_t m_autoPathIndex = 0;
    sf::Time m_autoMoveElapsed;

    void addLog(const std::string& message); // 로그 메시지 추가 헬퍼
    bool checkCurrentTileLog();              // 플레이어 현재 타일 상태 검출 및 전이
    EncounterTier currentEncounterTier() const;
    void revealFogOfWar();                   // [v0.6.0] 플레이어 위치 기준 미니맵 안개(시야) 해제 함수
    void stepAutoMove();                     // [v0.6.0] 자동 이동의 한 단계를 수행하고 전투 조우 검사 수행
};

} // namespace crawl

#endif // DUNGEON_STATE_HPP
