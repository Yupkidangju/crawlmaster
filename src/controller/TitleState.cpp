// [v0.1.0] TitleState.cpp 신규 작성
// [v0.9.0] 다국어 실시간 전환 지원, TUI 메뉴 인터페이스(시작/설정/종료) 및 SettingsState 연동 구현.

#include "controller/TitleState.hpp"
#include "controller/TownState.hpp"
#include "controller/SettingsState.hpp"
#include "controller/VictoryState.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include <filesystem>
#include <iostream>

namespace crawl {

TitleState::TitleState(Game& game)
    : m_game(game), m_showInstruction(true), m_selectedMenuIndex(0) {
    initTexts();
}

void TitleState::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Up:
                if (!m_confirmingNewGame) m_selectedMenuIndex = (m_selectedMenuIndex + 3) % 4;
                break;
            case sf::Keyboard::Down:
                if (!m_confirmingNewGame) m_selectedMenuIndex = (m_selectedMenuIndex + 1) % 4;
                break;
            case sf::Keyboard::Escape:
                if (m_confirmingNewGame) {
                    m_confirmingNewGame = false;
                    m_statusMessage.clear();
                }
                break;
            case sf::Keyboard::O:
                if (!m_confirmingNewGame) {
                    m_game.getStates().pushState(std::make_unique<SettingsState>(m_game));
                }
                break;
            case sf::Keyboard::Enter:
                if (m_confirmingNewGame) {
                    SessionRng::startNewGlobalSession();
                    auto saveResult = m_game.getParty().startNewGame();
                    if (!saveResult.durabilityConfirmed()) {
                        m_statusMessage = LocalizationManager::getInstance().get("TITLE_DURABILITY_UNKNOWN");
                    } else if (saveResult) {
                        m_game.getStates().replaceAll(std::make_unique<TownState>(m_game));
                    } else {
                        m_statusMessage = LocalizationManager::getInstance().get("TITLE_SAVE_ERROR");
                    }
                } else if (m_selectedMenuIndex == 0) {
                    if (Party::hasRecoverableSave()) {
                        m_confirmingNewGame = true;
                        m_statusMessage = LocalizationManager::getInstance().get("TITLE_NEW_CONFIRM");
                    } else {
                        SessionRng::startNewGlobalSession();
                        auto saveResult = m_game.getParty().startNewGame();
                        if (!saveResult.durabilityConfirmed()) {
                            m_statusMessage = LocalizationManager::getInstance().get("TITLE_DURABILITY_UNKNOWN");
                        } else if (saveResult) {
                            m_game.getStates().replaceAll(std::make_unique<TownState>(m_game));
                        } else {
                            m_statusMessage = LocalizationManager::getInstance().get("TITLE_SAVE_ERROR");
                        }
                    }
                } else if (m_selectedMenuIndex == 1) {
                    auto loadResult = m_game.getParty().loadFromFile();
                    if (loadResult) {
                        const auto savedSeed = m_game.getParty().getLastSessionSeed();
                        if (savedSeed == 0U) {
                            SessionRng::startNewGlobalSession();
                            const auto seedSaveResult = m_game.getParty().saveToFile();
                            if (!seedSaveResult.durabilityConfirmed()) {
                                m_statusMessage = LocalizationManager::getInstance().get("TITLE_DURABILITY_UNKNOWN");
                                return;
                            }
                            if (!seedSaveResult) {
                                m_statusMessage = LocalizationManager::getInstance().get("TITLE_SAVE_ERROR");
                                return;
                            }
                        } else {
                            SessionRng::global() = SessionRng(
                                savedSeed, m_game.getParty().getSessionRngDrawCount());
                        }
                        if (m_game.getParty().isCampaignCompleted()) {
                            m_game.getStates().replaceAll(std::make_unique<VictoryState>(m_game));
                        } else {
                            m_game.getStates().replaceAll(std::make_unique<TownState>(m_game));
                        }
                    } else if (loadResult.status == PersistenceStatus::NotFound) {
                        m_statusMessage = LocalizationManager::getInstance().get("TITLE_NO_SAVE");
                    } else {
                        m_statusMessage = LocalizationManager::getInstance().get("TITLE_CORRUPT_SAVE");
                    }
                } else if (m_selectedMenuIndex == 2) {
                    std::cout << "[FSM] TitleState에서 SettingsState로 상태 진입(Push)을 요청합니다." << std::endl;
                    m_game.getStates().pushState(std::make_unique<SettingsState>(m_game));
                } else if (m_selectedMenuIndex == 3) {
                    std::cout << "[FSM] 게임 종료를 위해 TitleState를 Pop 처리합니다." << std::endl;
                    m_game.getStates().popState();
                }
                break;
            default:
                break;
        }
    }
}

void TitleState::update(sf::Time deltaTime) {
    // 0.5초 주기로 안내 설명 문구가 깜빡이도록 타이머 로직 적용
    m_blinkTimer += deltaTime;
    if (m_blinkTimer >= sf::seconds(0.5f)) {
        m_showInstruction = !m_showInstruction;
        m_blinkTimer = sf::Time::Zero;
    }
}

void TitleState::draw(sf::RenderWindow& window) {
    // [v0.9.4] 설정 화면에서 복귀할 때 현재 언어의 폰트를 지속 Text에 재바인딩한다.
    const sf::Font& font = m_game.getFont();
    m_logoText.setFont(font);
    m_instructionText.setFont(font);
    m_creditText.setFont(font);
    m_statusText.setFont(font);
    auto& lm = LocalizationManager::getInstance();
    m_instructionText.setCharacterSize(lm.getScaledTextSize(14));
    m_creditText.setCharacterSize(lm.getScaledTextSize(14));
    m_statusText.setCharacterSize(lm.getScaledTextSize(16));

    // 1. 아스키 아트 로고 그리기
    window.draw(m_logoText);
    
    // 2. TUI 메뉴 그리기 (실시간 언어 변경 즉시 반영)
    float menuY = 440.0f;
    float menuSpacing = 35.0f;
    
    auto drawMenuItem = [&](const std::string& key, int idx) {
        sf::Text text;
        text.setFont(m_game.getFont());
        std::string label = lm.get(key);
        if (m_selectedMenuIndex == idx) {
            label = "> " + label;
            text.setFillColor(sf::Color(102, 255, 102)); // 강조 네온 그린
        } else {
            label = "  " + label;
            text.setFillColor(sf::Color(130, 170, 130));
        }
        text.setString(sf::String::fromUtf8(label.begin(), label.end()));
        text.setCharacterSize(lm.getScaledTextSize(16));
        
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
        text.setPosition(512.0f, menuY + idx * menuSpacing);
        window.draw(text);
    };

    drawMenuItem("TITLE_NEW_GAME", 0);
    drawMenuItem("TITLE_CONTINUE", 1);
    drawMenuItem("TITLE_SETTINGS", 2);
    drawMenuItem("TITLE_EXIT", 3);

    if (!m_statusMessage.empty()) {
        m_statusText.setString(sf::String::fromUtf8(m_statusMessage.begin(), m_statusMessage.end()));
        auto statusBounds = m_statusText.getLocalBounds();
        m_statusText.setOrigin(statusBounds.left + statusBounds.width / 2.0f,
                               statusBounds.top + statusBounds.height / 2.0f);
        window.draw(m_statusText);
    }

    // 3. 점멸 하단 추가 안내문구 렌더링
    if (m_showInstruction) {
        std::string rawInst = lm.get("TITLE_PRESS_KEY");
        m_instructionText.setString(sf::String::fromUtf8(rawInst.begin(), rawInst.end()));
        sf::FloatRect instBounds = m_instructionText.getLocalBounds();
        m_instructionText.setOrigin(instBounds.left + instBounds.width / 2.0f, instBounds.top + instBounds.height / 2.0f);
        window.draw(m_instructionText);
    }

    // 4. 저작권 크레딧 드로우
    window.draw(m_creditText);
}

void TitleState::initTexts() {
    const sf::Font& font = m_game.getFont();

    // 1. 아스키 아트 로고 설정
    std::string asciiLogo = 
        "  ____                             _                        _   \n"
        " / ___|_ __  __ ___      _| |_ __ ___   __ _ ___| |_ ___  _ __  \n"
        "| |   | '__|/ _` \\ \\ /\\ / / | '_ ` _ \\ / _` / __| __/ _ \\| '__| \n"
        "| |___| |  | (_| |\\ V  V /| | | | | | | (_| \\__ \\ ||  __/| |    \n"
        " \\____|_|   \\__,_| \\_/\\_/ |_|_| |_| |_|\\__,_|___/\\__\\___||_|    \n"
        "                                                                \n"
        "            --- 3D GRID DUNGEON CRAWLER RPG ---                 ";

    m_logoText.setFont(font);
    m_logoText.setString(asciiLogo);
    m_logoText.setCharacterSize(14);
    m_logoText.setFillColor(sf::Color(51, 255, 51));
    
    sf::FloatRect logoBounds = m_logoText.getLocalBounds();
    m_logoText.setOrigin(logoBounds.left + logoBounds.width / 2.0f, logoBounds.top + logoBounds.height / 2.0f);
    m_logoText.setPosition(512.0f, 220.0f);

    // 2. 안내 및 경고 점멸 문구 설정
    m_instructionText.setFont(font);
    m_instructionText.setCharacterSize(14);
    m_instructionText.setFillColor(sf::Color(255, 176, 0)); // 호박색 (Amber)
    m_instructionText.setPosition(512.0f, 580.0f);

    // 3. 저작권 크레딧 설정
    m_creditText.setFont(font);
    m_creditText.setString("Crawlmaster 0.9.4 - PRE-RELEASE DEMO CANDIDATE");
    m_creditText.setCharacterSize(14);
    m_creditText.setFillColor(sf::Color(130, 170, 130));
    
    sf::FloatRect creditBounds = m_creditText.getLocalBounds();
    m_creditText.setOrigin(creditBounds.left + creditBounds.width / 2.0f, creditBounds.top + creditBounds.height / 2.0f);
    m_creditText.setPosition(512.0f, 720.0f);

    m_statusText.setFont(font);
    m_statusText.setCharacterSize(16);
    m_statusText.setFillColor(sf::Color(255, 196, 80));
    m_statusText.setPosition(512.0f, 630.0f);
}

} // namespace crawl
