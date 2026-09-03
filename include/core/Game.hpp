// [v0.1.0] Game.hpp 신규 작성
// SFML 메인 윈도우 생성, 폰트 자산 로드 및 핵심 게임 루프(이벤트-업데이트-렌더링)를 관리하는 컨트롤러 클래스 정의.

#ifndef GAME_HPP
#define GAME_HPP

#include <SFML/Graphics.hpp>
#include <string>
#include "core/GameStateManager.hpp"
#include "model/Party.hpp"

namespace crawl {

// Game 클래스: 게임 기동 및 자산 관리, 메인 게임 루프 총괄
class Game {
public:
    explicit Game(bool headless = false);
    ~Game() = default;

    // 게임 루프 시작 및 윈도우 실행 함수
    void run();

    // 전역 폰트 로드 함수
    const sf::Font& getFont() const;

    // 게임 상태 매니저 인스턴스 반환 함수
    GameStateManager& getStates();

    // 전역 파티 매니저 인스턴스 반환 함수
    Party& getParty();

private:
    // SFML 윈도우 및 이벤트 처리
    void processEvents();
    // 로직 프레임 업데이트
    void update(sf::Time deltaTime);
    // 화면 드로우 렌더링
    void render();

    // 기본 리소스 로드
    void loadResources();

    sf::RenderWindow m_window;          // SFML 그래픽스 렌더 윈도우
    sf::Font m_font;                    // 게임 내 공용 복고풍 폰트 자산
    sf::Font m_cjkFont;                 // [v0.9.2] 일어/중국어 등 다국어 렌더링용 CJK 폰트 자산 추가
    GameStateManager m_stateManager;    // 전역 게임 상태 매니저
    Party m_party;                      // 전역 공용 파티 데이터 매니저
};

} // namespace crawl

#endif // GAME_HPP
