// [v0.1.0] TitleState.hpp 신규 작성
// 게임의 진입점인 메인 타이틀 화면의 상태 클래스를 선언하고, ASCII 로고 및 점멸 텍스트 요소를 정의한다.

#ifndef TITLE_STATE_HPP
#define TITLE_STATE_HPP

#include "core/GameState.hpp"
#include <vector>

namespace crawl {

class Game; // 전방 선언

class TitleState : public GameState {
public:
    explicit TitleState(Game& game);
    ~TitleState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    friend class ControllerTestAccess;
    Game& m_game;                       // 전역 Game 객체 참조
    sf::Text m_logoText;                // ASCII 아트 로고 텍스트
    sf::Text m_instructionText;         // 시작 안내 문구 텍스트
    sf::Text m_creditText;              // 개발자 정보 표시 텍스트
    sf::Text m_statusText;              // 저장/확인 상태를 유지하는 배너

    sf::Time m_blinkTimer;              // 안내 문구 점멸용 시간 누적기
    bool m_showInstruction;             // 점멸 텍스트 표출 플래그
    int m_selectedMenuIndex = 0;        // [v0.9.0] 타이틀 메뉴 선택 인덱스
    bool m_confirmingNewGame = false;
    std::string m_statusMessage;

    void initTexts();                   // UI 텍스트 설정 초기화 함수
};

} // namespace crawl

#endif // TITLE_STATE_HPP
