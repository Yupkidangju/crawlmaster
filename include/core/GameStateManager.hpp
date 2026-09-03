// [v0.1.0] GameStateManager.hpp 신규 작성
// 유한 상태 기계(FSM)를 기반으로 게임 화면의 전환과 현재 활성화된 상태의 수명주기를 관리하는 헤더 정의.

#ifndef GAME_STATE_MANAGER_HPP
#define GAME_STATE_MANAGER_HPP

#include <memory>
#include <cstddef>
#include <stack>
#include "GameState.hpp"

namespace crawl {

// GameStateManager 클래스: 현재 활성화된 상태들을 스택 형태로 관리하여 화면 전환 처리
class GameStateManager {
public:
    GameStateManager() = default;
    ~GameStateManager() = default;

    // 새로운 상태로 즉시 교체 (기존 상태는 파괴됨)
    void changeState(std::unique_ptr<GameState> newState);

    // 모든 이전 상태를 제거하고 새 root 상태 하나로 교체
    void replaceAll(std::unique_ptr<GameState> newState);

    // 새로운 상태를 스택에 추가 (기존 상태는 보존됨)
    void pushState(std::unique_ptr<GameState> newState);

    // 현재 상태를 스택에서 제거하고 이전 상태로 복귀
    void popState();

    // 현재 활성화된 최상위 상태 반환
    GameState* getCurrentState() const;

    // 스택이 비어있는지 여부 확인
    bool isEmpty() const;

    std::size_t size() const;

private:
    std::stack<std::unique_ptr<GameState>> m_states;
};

} // namespace crawl

#endif // GAME_STATE_MANAGER_HPP
