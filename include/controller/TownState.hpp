// [v0.3.0] TownState.hpp 수정 (마을 상호작용 TUI 서브시스템 구현)
// 길드(캐릭터 생성), 상점(장비 매매), 교회(부활/치료) 기능을 관리하기 위한 서브 상태 FSM 및 TUI 텍스트 요소 정의.

#ifndef TOWN_STATE_HPP
#define TOWN_STATE_HPP

#include "core/GameState.hpp"
#include <vector>
#include <string>
#include <memory>
#include "model/RecruitmentDraft.hpp"

namespace crawl {

class Game; // 전방 선언

// 마을 서브 상태 정의
enum class TownSubState {
    HUB,        // 마을 허브 광장
    GUILD,      // 모험가 길드 (캐릭터 생성/삭제)
    SHOP,       // 상점 (구매/판매 선택)
    SHOP_BUY,   // 상점 구매 카탈로그
    SHOP_SELL,  // 상점 판매 목록
    TEMPLE,     // 교회 (치료)
    CASTLE      // 영주 성/캠프 (퀘스트 수주/보고)
};

class TownState : public GameState {
public:
    explicit TownState(Game& game);
    ~TownState() override = default;

    void handleInput(const sf::Event& event) override;
    void update(sf::Time deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    Game& m_game;                       // 전역 Game 참조
    TownSubState m_subState;            // 현재 마을 활성 서브 장소

    sf::Text m_titleText;               // 상단 건물 타이틀 텍스트
    sf::Text m_menuText;                // 중앙 상호작용 옵션 텍스트
    sf::Text m_partyText;               // 우측 파티 상태 요약 텍스트
    sf::Text m_statusText;              // 하단 거래 결과 및 알림 텍스트

    std::string m_notifyMessage;        // 상점 거래 성공/실패 알림 버퍼

    void setSubState(TownSubState state); // 서브 상태 변경 및 텍스트 갱신 함수
    void initTexts();                   // UI 텍스트 기본 셋업
    void updateTuiContent();            // 서브 상태에 따라 드로우할 텍스트 동적 재구성

    // 무작위 캐릭터명 생성을 위한 기본 데이터베이스
    const std::vector<std::string> RANDOM_NAMES = {
        "Ragnar", "Elminster", "Lirael", "Kaelen", "Valerius", "Morgana", "Garrick", "Sariel"
    };
    int m_nameIndex = 0;
    std::unique_ptr<RecruitmentDraft> m_recruitmentDraft;
    bool m_confirmingDismiss = false;
    int m_pendingSaleIndex = -1;
};

} // namespace crawl

#endif // TOWN_STATE_HPP
