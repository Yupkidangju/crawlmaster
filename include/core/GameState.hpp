// [v0.1.0] GameState.hpp 신규 작성
// SFML 그래픽 창 및 키보드 이벤트를 수신하여 화면을 업데이트하고 그리기 위한 추상 인터페이스 정의.

#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace sf {
    class Time;
}

namespace crawl {

// GameState 클래스: 모든 게임 상태의 부모가 되는 추상 클래스
class GameState {
public:
    virtual ~GameState() = default;

    // 키보드 및 윈도 시스템 이벤트 처리 인터페이스
    virtual void handleInput(const sf::Event& event) = 0;

    // 프레임 델타 타임에 따른 비즈니스 로직 업데이트 인터페이스
    virtual void update(sf::Time deltaTime) = 0;

    // 화면 렌더링 그리기 인터페이스
    virtual void draw(sf::RenderWindow& window) = 0;
};

} // namespace crawl

#endif // GAME_STATE_HPP
