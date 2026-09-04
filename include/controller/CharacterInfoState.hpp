// [v0.6.0] CharacterInfoState.hpp 신규 작성
// 캐릭터 스탯 정보, 능력치 보정치 및 장착 장비 현황을 확인하고 공용 인벤토리의 아이템을 소모 및 장착하는 UI 및 이벤트 제어 헤더 정의.

#ifndef CHARACTER_INFO_STATE_HPP
#define CHARACTER_INFO_STATE_HPP

#include "core/GameState.hpp"
#include <vector>
#include <string>

namespace crawl {

class Game; // 전방 선언
class Character;

// CharacterInfoState 클래스: 캐릭터 상세 조회, 장비 장착/해제, 인벤토리 아이템 소모를 제어하는 상태 클래스
class CharacterInfoState : public GameState {
public:
    CharacterInfoState(Game& game, bool persistChanges);
    ~CharacterInfoState() override = default;

    // 키보드 입력 시스템 이벤트 처리
    void handleInput(const sf::Event& event) override;
    // 게임 틱 업데이트 (시간 경과 기반 갱신)
    void update(sf::Time deltaTime) override;
    // 화면 드로잉 연산 수행
    void draw(sf::RenderWindow& window) override;

private:
    friend class ControllerTestAccess;
    Game& m_game;
    bool m_persistChanges;

    // 현재 포커스 대상 영역 (0: 좌측 캐릭터 정보 및 장비 슬롯 영역, 1: 우측 인벤토리 영역)
    int m_focusArea; 

    // 현재 선택/조회 중인 캐릭터 멤버의 인덱스 (0~3)
    int m_selectedCharIndex;

    // 좌측 장비 영역 내 포커스된 슬롯 인덱스 (0: Weapon, 1: Armor, 2: Shield)
    int m_equipmentSlotIndex;

    // 우측 인벤토리 영역 내 포커스된 아이템 리스트 인덱스
    int m_inventoryIndex;

    // 하단 상태 경고 로그 메시지
    std::string m_statusMsg;

    // 소모성 아이템(예: 치유 물약)을 포커스된 캐릭터에게 사용 및 가방에서 제거 처리
    void useSelectedConsumable();
    // 장비류 아이템을 포커스된 캐릭터에게 장착 시도 (기존 장비는 가방 반환)
    void equipSelectedItem();
    // 캐릭터가 착용 중인 장비를 해제하여 공용 인벤토리 가방으로 복귀
    void unequipSelectedSlot();
    void drawLargeTextLayout(sf::RenderWindow& window);
    std::string statusSummary(const Character& character) const;
    
    // 단순 선/사각형 상자 그리기 유틸리티
    void drawBox(sf::RenderWindow& window, float x, float y, float w, float h, sf::Color color, float thickness = 1.0f);
    // 레트로 둥근모 폰트 출력 헬퍼
    void drawText(sf::RenderWindow& window, const std::string& str, float x, float y, int size, sf::Color color, bool center = false);
};

} // namespace crawl

#endif // CHARACTER_INFO_STATE_HPP
