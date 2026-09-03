// [v0.1.0] GameStateManager.cpp 신규 작성
// GameStateManager 클래스의 멤버 함수들을 구현하여 스택 구조의 상태 전이 기능을 실행한다.

#include "core/GameStateManager.hpp"

namespace crawl {

// 새로운 상태로 현재 상태를 변경하는 함수
void GameStateManager::changeState(std::unique_ptr<GameState> newState) {
    // 스택이 비어있지 않다면 기존 상태를 완전히 팝하여 해제 (std::unique_ptr가 자동 메모리 해제 보장)
    if (!m_states.empty()) {
        m_states.pop();
    }
    // 새로운 상태를 스택에 추가
    m_states.push(std::move(newState));
}

void GameStateManager::replaceAll(std::unique_ptr<GameState> newState) {
    while (!m_states.empty()) {
        m_states.pop();
    }
    if (newState) {
        m_states.push(std::move(newState));
    }
}

// 새로운 상태를 스택의 맨 위에 올리는 함수 (이전 상태는 백그라운드에 유지됨)
void GameStateManager::pushState(std::unique_ptr<GameState> newState) {
    m_states.push(std::move(newState));
}

// 현재 활성화 상태를 스택에서 제거하는 함수
void GameStateManager::popState() {
    if (!m_states.empty()) {
        m_states.pop();
    }
}

// 최상위(현재 활성화된) 상태의 포인터를 반환하는 함수
GameState* GameStateManager::getCurrentState() const {
    if (m_states.empty()) {
        return nullptr;
    }
    return m_states.top().get();
}

// 상태 매니저 스택이 완전히 비어있는지 확인하는 함수
bool GameStateManager::isEmpty() const {
    return m_states.empty();
}

std::size_t GameStateManager::size() const {
    return m_states.size();
}

} // namespace crawl
