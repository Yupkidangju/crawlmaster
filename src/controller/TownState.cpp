// [v0.3.0] TownState.cpp 수정 (길드, 상점, 교회 TUI 및 파티 정보 동기화 연동)
// [v0.9.0] 다국어 i18n 실시간 렌더링 적용, S키 설정(SettingsState) 진입 지원 및 UI 전면 다국어화 구현.
// [v0.9.2] TownState.cpp 수정: UB iterator pair 해결, getSf() 사용, 5개 국어 i18n 메시지 대응 selectLang 헬퍼 도입.
// [v0.9.2] 재수정: 하드코딩 selectLang 제거 및 LocalizationManager JSON 리소스 매핑 일원화, 한국어 조사 혼입 오류 수정.

#include "controller/TownState.hpp"
#include "controller/TitleState.hpp"
#include "controller/DungeonState.hpp"
#include "controller/CharacterInfoState.hpp"
#include "controller/CharacterCreationState.hpp"
#include "controller/SettingsState.hpp"
#include "controller/QuestJournalState.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "model/ItemFactory.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace crawl {

// [v0.9.2] 문자열 내의 플레이스홀더를 동적으로 치환하는 다국어 헬퍼 함수
static std::string replacePlaceholder(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

static std::vector<std::string> questBoardIds(const Party& party) {
    std::vector<std::string> ids = Quest::getOfferableIds();
    for (const auto& active : party.getActiveQuests()) {
        if (active && std::find(ids.begin(), ids.end(), active->getId()) == ids.end()) {
            ids.push_back(active->getId());
        }
    }
    return ids;
}

TownState::TownState(Game& game)
    : m_game(game), m_subState(TownSubState::HUB) {
    initTexts();
    setSubState(TownSubState::HUB);
}

void TownState::handleInput(const sf::Event& event) {
    if (event.type != sf::Event::KeyPressed) return;

    Party& party = m_game.getParty();
    const PartyCheckpoint inputCheckpoint = party.captureCheckpoint();
    sf::Keyboard::Key key = event.key.code;
    auto& lm = LocalizationManager::getInstance();
    if (key == sf::Keyboard::Q) {
        m_game.getStates().pushState(std::make_unique<QuestJournalState>(m_game));
        return;
    }
    if (key == sf::Keyboard::O) {
        std::cout << "[FSM] TownState에서 SettingsState로 진입합니다." << std::endl;
        m_game.getStates().pushState(std::make_unique<SettingsState>(m_game));
        return;
    }
    auto persistTownChange = [&]() {
        const auto saveResult = party.saveToFile();
        if (!saveResult.durabilityConfirmed()) {
            m_notifyMessage = lm.get("TOWN_MSG_DURABILITY_UNKNOWN");
            return false;
        }
        if (saveResult) return true;
        party.restoreCheckpoint(inputCheckpoint);
        m_notifyMessage = lm.get("TOWN_MSG_SAVE_FAILED");
        return false;
    };

    // 1. 마을 허브 광장 분기 로직
    if (m_subState == TownSubState::HUB) {
        if (key == sf::Keyboard::Num1) {
            setSubState(TownSubState::GUILD);
        } else if (key == sf::Keyboard::Num2) {
            setSubState(TownSubState::SHOP);
        } else if (key == sf::Keyboard::Num3) {
            setSubState(TownSubState::TEMPLE);
        } else if (key == sf::Keyboard::Num4) {
            setSubState(TownSubState::CASTLE); // 성주실 분기 로직 진입
        } else if (key == sf::Keyboard::Num5 || key == sf::Keyboard::Enter || key == sf::Keyboard::D) {
            // 던전 입장: 파티에 1명 이상의 모험가가 필요함
            if (party.getMemberCount() == 0) {
                m_notifyMessage = lm.get("WARNING_GUILD_MEMBER");
                updateTuiContent();
            } else {
                std::cout << "[FSM] TownState에서 DungeonState로 진입합니다." << std::endl;
                m_game.getStates().changeState(std::make_unique<DungeonState>(m_game));
            }
        } else if (key == sf::Keyboard::Escape) {
            std::cout << "[FSM] TownState에서 TitleState로 복귀합니다." << std::endl;
            m_game.getStates().changeState(std::make_unique<TitleState>(m_game));
        } else if (key == sf::Keyboard::I || key == sf::Keyboard::C) {
            std::cout << "[FSM] TownState에서 CharacterInfoState로 진입합니다." << std::endl;
            m_game.getStates().pushState(std::make_unique<CharacterInfoState>(m_game, true));
        }
    }
    // 2. 모험가 길드 분기 로직
    else if (m_subState == TownSubState::GUILD) {
        if (m_confirmingDismiss) {
            if (key == sf::Keyboard::Enter && party.getMemberCount() > 0) {
                const int lastIndex = party.getMemberCount() - 1;
                auto dismissed = party.getMember(lastIndex);
                party.removeMember(lastIndex);
                const auto saveResult = party.saveToFile();
                if (!saveResult.durabilityConfirmed()) {
                    m_notifyMessage = lm.get("TOWN_MSG_DURABILITY_UNKNOWN");
                } else if (!saveResult) {
                    party.restoreCheckpoint(inputCheckpoint);
                    m_notifyMessage = lm.get("TOWN_MSG_SAVE_FAILED");
                } else {
                    m_notifyMessage = replacePlaceholder(lm.get("TOWN_MSG_DISBANDED"),
                                                         "{name}", dismissed->getName());
                }
                m_confirmingDismiss = false;
            } else if (key == sf::Keyboard::Escape) {
                m_confirmingDismiss = false;
                m_notifyMessage = lm.get("TOWN_MSG_CANCELLED");
            }
            updateTuiContent();
            return;
        }
        if (key == sf::Keyboard::Num1) {
            if (party.getMemberCount() >= 4) {
                m_notifyMessage = lm.get("TOWN_MSG_PARTY_FULL");
            } else {
                m_game.getStates().pushState(std::make_unique<CharacterCreationState>(m_game));
            }
            updateTuiContent();
        } else if (key == sf::Keyboard::Num2) {
            // 마지막 파티원 해제
            if (party.getMemberCount() == 0) {
                m_notifyMessage = lm.get("TOWN_MSG_NO_DISBAND");
            } else {
                m_confirmingDismiss = true;
                m_notifyMessage = replacePlaceholder(lm.get("GUILD_DISMISS_CONFIRM"),
                    "{name}", party.getMember(party.getMemberCount() - 1)->getName());
            }
            updateTuiContent();
        } else if (key == sf::Keyboard::Escape) {
            setSubState(TownSubState::HUB);
        }
    }
    // 3. 무기 상점 분기 로직 (구매/판매 메인 메뉴)
    else if (m_subState == TownSubState::SHOP) {
        if (key == sf::Keyboard::Num1) {
            setSubState(TownSubState::SHOP_BUY);
        } else if (key == sf::Keyboard::Num2) {
            setSubState(TownSubState::SHOP_SELL);
        } else if (key == sf::Keyboard::Num3 || key == sf::Keyboard::Escape) {
            setSubState(TownSubState::HUB);
        }
    }
    // 3-1. 무기 상점 구매 카탈로그 분기 로직
    else if (m_subState == TownSubState::SHOP_BUY) {
        const auto catalog = ItemFactory::getShopCatalog();
        if (key >= sf::Keyboard::Num1 && key <= sf::Keyboard::Num8) {
            const std::size_t index = static_cast<std::size_t>(key - sf::Keyboard::Num1);
            const auto purchasedItem = catalog[index];
            const int price = purchasedItem->getGoldValue();
            if (party.spendGold(price)) {
                party.addItem(purchasedItem);
                if (persistTownChange()) {
                    std::string purchasedMsg = lm.get("TOWN_MSG_PURCHASED");
                    purchasedMsg = replacePlaceholder(purchasedMsg, "{name}", purchasedItem->getName());
                    purchasedMsg = replacePlaceholder(purchasedMsg, "{price}", std::to_string(price));
                    m_notifyMessage = purchasedMsg;
                }
            } else {
                m_notifyMessage = lm.get("TOWN_MSG_NO_GOLD");
            }
            updateTuiContent();
        } else if (key == sf::Keyboard::Escape) {
            setSubState(TownSubState::SHOP);
        }
    }
    // 3-2. 무기 상점 판매 목록 분기 로직
    else if (m_subState == TownSubState::SHOP_SELL) {
        if (m_pendingSaleIndex >= 0) {
            if (key == sf::Keyboard::Enter) {
                const auto& inventory = party.getInventory();
                if (m_pendingSaleIndex < static_cast<int>(inventory.size())) {
                    auto item = inventory[static_cast<std::size_t>(m_pendingSaleIndex)];
                    const int sellPrice = item->getGoldValue() / 2;
                    party.addGold(sellPrice);
                    party.removeItem(m_pendingSaleIndex);
                    const auto saveResult = party.saveToFile();
                    if (!saveResult.durabilityConfirmed()) {
                        m_notifyMessage = lm.get("TOWN_MSG_DURABILITY_UNKNOWN");
                    } else if (!saveResult) {
                        party.restoreCheckpoint(inputCheckpoint);
                        m_notifyMessage = lm.get("TOWN_MSG_SAVE_FAILED");
                    } else {
                        auto soldMessage = replacePlaceholder(lm.get("TOWN_MSG_SOLD"), "{name}", item->getName());
                        m_notifyMessage = replacePlaceholder(soldMessage, "{price}", std::to_string(sellPrice));
                    }
                }
                m_pendingSaleIndex = -1;
            } else if (key == sf::Keyboard::Escape) {
                m_pendingSaleIndex = -1;
                m_notifyMessage = lm.get("TOWN_MSG_CANCELLED");
            }
            updateTuiContent();
            return;
        }
        if (key >= sf::Keyboard::Num1 && key <= sf::Keyboard::Num9) {
            int idx = key - sf::Keyboard::Num1;
            const auto& inv = party.getInventory();
            if (idx >= 0 && idx < static_cast<int>(inv.size())) {
                auto item = inv[idx];
                if (item) {
                    int sellPrice = item->getGoldValue() / 2;
                    m_pendingSaleIndex = idx;
                    auto confirm = replacePlaceholder(lm.get("SHOP_SELL_CONFIRM"), "{name}", item->getName());
                    m_notifyMessage = replacePlaceholder(confirm, "{price}", std::to_string(sellPrice));
                }
            } else {
                m_notifyMessage = lm.get("TOWN_MSG_INVALID_SLOT");
            }
            updateTuiContent();
        } else if (key == sf::Keyboard::Escape) {
            setSubState(TownSubState::SHOP);
        }
    }
    // 4. 교회 분기 로직
    else if (m_subState == TownSubState::TEMPLE) {
        if (key == sf::Keyboard::Num1) {
            if (party.getMemberCount() == 0) {
                m_notifyMessage = lm.get("TOWN_MSG_NO_HEAL");
            } else {
                // 치료 비용 10 Gold 차감 시도
                if (party.spendGold(10)) {
                    for (int i = 0; i < party.getMemberCount(); ++i) {
                        party.getMember(i)->rest(); // HP 및 스펠 슬롯 완전 복원
                    }
                    if (persistTownChange()) m_notifyMessage = lm.get("TOWN_MSG_HEALED");
                } else {
                    m_notifyMessage = lm.get("TOWN_MSG_TEMPLE_NO_GOLD");
                }
            }
            updateTuiContent();
        } else if (key == sf::Keyboard::Escape) {
            setSubState(TownSubState::HUB);
        }
    }
    // 5. 영주 성/퀘스트 보드 분기 로직
    else if (m_subState == TownSubState::CASTLE) {
        const std::vector<std::string> questIds = questBoardIds(party);
        if (key == sf::Keyboard::Up && !questIds.empty()) {
            m_questSelection = (m_questSelection + static_cast<int>(questIds.size()) - 1) %
                static_cast<int>(questIds.size());
        } else if (key == sf::Keyboard::Down && !questIds.empty()) {
            m_questSelection = (m_questSelection + 1) % static_cast<int>(questIds.size());
        } else if (key == sf::Keyboard::Enter && !questIds.empty()) {
            m_questSelection = std::clamp(m_questSelection, 0, static_cast<int>(questIds.size()) - 1);
            const std::string& questId = questIds[static_cast<std::size_t>(m_questSelection)];
            if (party.isQuestCompleted(questId)) {
                m_notifyMessage = lm.get("TOWN_MSG_QUEST_ALREADY_COMPLETED");
            } else if (!party.hasQuest(questId)) {
                party.acceptQuest(Quest::createCanonical(questId));
                if (persistTownChange()) m_notifyMessage = lm.get("TOWN_MSG_QUEST_ACCEPTED");
            } else {
                party.updateQuestCollectProgress();
                auto quest = party.getQuest(questId);
                if (quest && quest->isReadyToReport()) {
                    party.completeQuest(questId);
                    if (persistTownChange()) m_notifyMessage = lm.get("TOWN_MSG_QUEST_REPORTED");
                } else {
                    m_notifyMessage = lm.get("TOWN_MSG_QUEST_IN_PROGRESS");
                }
            }
        } else if (key == sf::Keyboard::Escape) {
            setSubState(TownSubState::HUB);
        }
        updateTuiContent();
    }
}

void TownState::update(sf::Time /*deltaTime*/) {
    // 마을 로직 동적 매칭 없음
}

void TownState::draw(sf::RenderWindow& window) {
    updateTuiContent(); // [v0.9.0] 실시간 다국어 리프레시 강제 보증
    // [v0.9.4] 언어 변경으로 선택 폰트가 바뀐 경우 지속 Text에도 즉시 재바인딩한다.
    const sf::Font& font = m_game.getFont();
    m_titleText.setFont(font);
    m_menuText.setFont(font);
    m_partyText.setFont(font);
    m_statusText.setFont(font);
    auto& localization = LocalizationManager::getInstance();
    m_titleText.setCharacterSize(localization.getScaledTextSize(24));
    m_menuText.setCharacterSize(localization.getScaledTextSize(14));
    m_partyText.setCharacterSize(localization.getScaledTextSize(14));
    m_statusText.setCharacterSize(localization.getScaledTextSize(14));
    window.draw(m_titleText);
    window.draw(m_menuText);
    if (localization.getTextScale() <= 125) window.draw(m_partyText);
    window.draw(m_statusText);
}

void TownState::setSubState(TownSubState state) {
    m_subState = state;
    m_notifyMessage = ""; // 화면 전환 시 알림 클리어
    updateTuiContent();
}

void TownState::initTexts() {
    const sf::Font& font = m_game.getFont();
    sf::Color neonGreen = sf::Color(51, 255, 51);

    // 1. 헤더 타이틀 설정
    m_titleText.setFont(font);
    m_titleText.setCharacterSize(24);
    m_titleText.setFillColor(neonGreen);
    m_titleText.setPosition(50.0f, 40.0f);

    // 2. 중앙 선택 메뉴 TUI 설정
    m_menuText.setFont(font);
    m_menuText.setCharacterSize(14);
    m_menuText.setFillColor(sf::Color(102, 255, 102));
    m_menuText.setPosition(50.0f, 120.0f);

    // 3. 우측 파티 상태 HUD 요약 설정
    m_partyText.setFont(font);
    m_partyText.setCharacterSize(14);
    m_partyText.setFillColor(sf::Color(102, 255, 102));
    m_partyText.setPosition(650.0f, 40.0f);

    // 4. 하단 알림 창 안내 텍스트 설정
    m_statusText.setFont(font);
    m_statusText.setCharacterSize(14);
    m_statusText.setFillColor(sf::Color(255, 176, 0)); // Amber 경고 색상
    m_statusText.setPosition(50.0f, 650.0f);
}

void TownState::updateTuiContent() {
    Party& party = m_game.getParty();
    auto& lm = LocalizationManager::getInstance();
    const bool largeText = lm.getTextScale() > 125;
    
    // 1. 서브 상태에 맞게 타이틀과 옵션 텍스트 생성
    std::ostringstream menuOss;
    
    switch (m_subState) {
        case TownSubState::HUB:
            m_titleText.setString(lm.getSf(largeText ? "TOWN_TITLE_SHORT" : "TOWN_TITLE"));
            menuOss << lm.get("TOWN_CAMP_WELCOME") << "\n\n"
                    << lm.get(largeText ? "TOWN_CAMP_OPTION_1_SHORT" : "TOWN_CAMP_OPTION_1") << "\n"
                    << lm.get(largeText ? "TOWN_CAMP_OPTION_2_SHORT" : "TOWN_CAMP_OPTION_2") << "\n"
                    << lm.get(largeText ? "TOWN_CAMP_OPTION_3_SHORT" : "TOWN_CAMP_OPTION_3") << "\n"
                    << lm.get(largeText ? "TOWN_CAMP_OPTION_4_SHORT" : "TOWN_CAMP_OPTION_4") << "\n"
                    << lm.get(largeText ? "TOWN_CAMP_OPTION_5_SHORT" : "TOWN_CAMP_OPTION_5") << "\n\n"
                    << lm.get(largeText ? "TOWN_CAMP_OPTION_C_SHORT" : "TOWN_CAMP_OPTION_C") << "\n"
                    << lm.get(largeText ? "TOWN_CAMP_OPTION_O_SHORT" : "TOWN_CAMP_OPTION_O") << "\n"
                    << lm.get(largeText ? "TOWN_CAMP_OPTION_ESC_SHORT" : "TOWN_CAMP_OPTION_ESC");
            break;

        case TownSubState::GUILD:
            m_titleText.setString(lm.getSf("GUILD_TITLE"));
            menuOss << lm.get("GUILD_DESK") << "\n\n"
                    << lm.get("GUILD_CREATE") << "\n"
                    << lm.get("GUILD_DISMISS") << "\n\n"
                    << "ESC. " << lm.get("GUILD_BACK");
            break;

        case TownSubState::SHOP:
            m_titleText.setString(lm.getSf("SHOP_TITLE"));
            menuOss << lm.get("SHOP_MENU") << "\n\n"
                    << "1. " << lm.get("SHOP_BUY") << "\n"
                    << "2. " << lm.get("SHOP_SELL") << "\n\n"
                    << "ESC. " << lm.get("SHOP_BACK");
            break;

        case TownSubState::SHOP_BUY:
            m_titleText.setString(lm.getSf("SHOP_TITLE"));
            menuOss << lm.get("SHOP_CATALOG") << "\n\n";
            {
                const auto catalog = ItemFactory::getShopCatalog();
                for (std::size_t index = 0; index < catalog.size(); ++index) {
                    const auto& item = catalog[index];
                    menuOss << (index + 1) << ". " << item->getName()
                            << " - " << item->getGoldValue() << " G\n";
                }
            }
            menuOss << "\nESC. " << lm.get("TOWN_TEXT_BACK");
            break;

        case TownSubState::SHOP_SELL:
            m_titleText.setString(lm.getSf("SHOP_TITLE"));
            menuOss << lm.get("TOWN_TEXT_SELECT_SELL");
            {
                const auto& inv = party.getInventory();
                if (inv.empty()) {
                    menuOss << lm.get("TOWN_TEXT_NO_SELL");
                } else {
                    for (size_t i = 0; i < inv.size() && i < 9; ++i) {
                        auto item = inv[i];
                        if (item) {
                            int sellPrice = item->getGoldValue() / 2;
                            menuOss << (i + 1) << ". " << item->getName() << " (+" << sellPrice << " G)\n";
                        }
                    }
                }
            }
            menuOss << "\nESC. " << lm.get("TOWN_TEXT_BACK");
            break;

        case TownSubState::TEMPLE:
            m_titleText.setString(lm.getSf("TEMPLE_TITLE"));
            menuOss << lm.get("TEMPLE_SANCTUARY") << "\n\n"
                    << lm.get("TEMPLE_REST") << "\n\n"
                    << "ESC. " << lm.get("TEMPLE_BACK");
            break;

        case TownSubState::CASTLE:
            m_titleText.setString(lm.getSf("CASTLE_TITLE"));
            {
                const std::vector<std::string> questIds = questBoardIds(party);
                if (!questIds.empty()) m_questSelection = std::clamp(
                    m_questSelection, 0, static_cast<int>(questIds.size()) - 1);
                menuOss << lm.get("CASTLE_QUEST_BOARD") << "\n\n";
                const bool compact = lm.getTextScale() > 125;
                const std::size_t begin = compact ? static_cast<std::size_t>(m_questSelection) : 0U;
                const std::size_t end = compact ? begin + 1U : questIds.size();
                if (compact) menuOss << (m_questSelection + 1) << "/" << questIds.size() << "\n";
                for (std::size_t index = begin; index < end; ++index) {
                    auto definition = Quest::createCanonical(questIds[index]);
                    if (!definition) continue;
                    std::string status = lm.get("QUEST_STATUS_AVAILABLE");
                    if (party.isQuestCompleted(definition->getId())) status = lm.get("QUEST_STATUS_COMPLETED");
                    else if (auto active = party.getQuest(definition->getId())) {
                        status = lm.get(active->isReadyToReport()
                            ? "QUEST_STATUS_READY" : "QUEST_STATUS_ACTIVE");
                    }
                    menuOss << (static_cast<int>(index) == m_questSelection ? "> " : "  ")
                            << definition->getName() << " [" << status << "]\n"
                            << "    " << definition->getDescription() << "\n";
                }
                menuOss << "\n" << lm.get("QUEST_BOARD_GUIDE") << " | ESC. " << lm.get("CASTLE_BACK");
            }
            break;
    }

    std::string menuStr = menuOss.str();
    m_menuText.setString(sf::String::fromUtf8(menuStr.begin(), menuStr.end()));

    // 2. 우측 파티 정보 동적 다국어화 갱신
    std::ostringstream partyOss;

    partyOss << lm.get("TOWN_PARTY_STATUS") << "\n"
             << lm.get("TOWN_GOLD") << ": " << party.getGold() << " G\n"
             << lm.get("TOWN_MEMBERS") << ": " << party.getMemberCount() << " / 4\n\n";

    for (int i = 0; i < party.getMemberCount(); ++i) {
        auto member = party.getMember(i);
        
        // 직업 다국어 가져오기
        std::string clsStr = "";
        switch (member->getClass()) {
            case CharacterClass::WARRIOR: clsStr = lm.get("CLASS_WARRIOR"); break;
            case CharacterClass::MAGE:    clsStr = lm.get("CLASS_MAGE");    break;
            case CharacterClass::ROGUE:   clsStr = lm.get("CLASS_ROGUE");   break;
            case CharacterClass::CLERIC:  clsStr = lm.get("CLASS_CLERIC");  break;
        }

        partyOss << i + 1 << ". " << member->getName() << " (" << clsStr << ")\n";
        if (largeText) continue;
        partyOss
                 << "   " << lm.get("TOWN_LEVEL") << ": " << member->getLevel() 
                 << " / " << lm.get("CHAR_INFO_HP") << ": " << member->getHp() << "/" << member->getMaxHp() << "\n"
                 << "   STR: " << member->getAbilities().strength
                 << " DEX: " << member->getAbilities().dexterity
                 << " CON: " << member->getAbilities().constitution << "\n"
                 << "   INT: " << member->getAbilities().intelligence
                 << " WIS: " << member->getAbilities().wisdom
                 << " CHA: " << member->getAbilities().charisma << "\n"
                 << "   " << lm.get("CHAR_INFO_AC") << ": " << member->getAc()
                 << " " << lm.get("TOWN_SPELL_SLOT") << ": " << member->getSpellSlots() << "\n\n";
    }

    // 인벤토리 소지품 목록 그리기
    partyOss << lm.get("TOWN_BAG") << "\n";
    const auto& inv = party.getInventory();
    if (inv.empty()) {
        partyOss << lm.get("TOWN_EMPTY_BAG") << "\n";
    } else if (largeText) {
        partyOss << lm.format("TOWN_BAG_COUNT", {{"count", std::to_string(inv.size())}}) << "\n";
    } else {
        std::vector<std::string> itemLines;
        for (const auto& item : inv) {
            if (item) {
                itemLines.push_back(item->getName());
            }
        }
        std::sort(itemLines.begin(), itemLines.end());
        
        std::string last = "";
        int count = 0;
        for (const auto& name : itemLines) {
            if (name == last) {
                count++;
            } else {
                if (count > 0) {
                    partyOss << " - " << last << " x" << count << "\n";
                }
                last = name;
                count = 1;
            }
        }
        if (count > 0) {
            partyOss << " - " << last << " x" << count << "\n";
        }
    }

    std::string partyStr = partyOss.str();
    m_partyText.setString(sf::String::fromUtf8(partyStr.begin(), partyStr.end()));

    // 3. 하단 상태 알림 텍스트 세팅
    std::string notifyStr = m_notifyMessage;
    m_statusText.setString(sf::String::fromUtf8(notifyStr.begin(), notifyStr.end()));
}

} // namespace crawl
