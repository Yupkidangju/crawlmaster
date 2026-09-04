// [v0.1.0] Game.cpp 신규 작성
// SFML 메인 루프 구조를 상세히 구현하고, 폰트 에셋 로드 및 최초 TitleState 상태 주입을 처리한다.

#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/ResourceLocator.hpp"
#include "core/SessionRng.hpp"
#include "controller/TitleState.hpp"
#include "controller/ShutdownState.hpp"
#include <iostream>

namespace crawl {

Game::Game(bool headless) {
    std::cout << "[Session] RNG seed: " << SessionRng::global().seed() << std::endl;
    if (headless) return;
    // 1024x768 고정 크기로 윈도우 생성 (크기 조정 금지 옵션 적용)
    m_window.create(sf::VideoMode(1024, 768), "Crawlmaster", sf::Style::Titlebar | sf::Style::Close);
    // 프레임 레이트 상한을 60fps로 설정하여 CPU 과열 방지
    m_window.setFramerateLimit(60);

    // 폰트 자산 로딩 진행
    loadResources();

    // [v0.9.0] 초기 로컬라이제이션 설정 자동 로딩
    LocalizationManager::getInstance().loadConfig();

    // 초기 상태인 TitleState를 할당하여 게임 엔진 시작점 지정
    m_stateManager.changeState(std::make_unique<TitleState>(*this));
}

void Game::run() {
    sf::Clock clock;
    // 윈도우가 열려 있고 상태 매니저에 활성화된 상태가 남아 있는 동안 루프 유지
    while (m_window.isOpen() && !m_stateManager.isEmpty()) {
        sf::Time deltaTime = clock.restart();

        processEvents();
        update(deltaTime);
        render();
    }
}

const sf::Font& Game::getFont() const {
    // [v0.9.2] 현재 설정 언어가 일어/중국어인 경우 다국어 글리프를 완벽 지원하는 CJK 폰트 반환
    auto lang = LocalizationManager::getInstance().getLanguage();
    if (lang == Language::JA || lang == Language::ZH_TW || lang == Language::ZH_CN) {
        return m_cjkFont;
    }
    return m_font;
}

GameStateManager& Game::getStates() {
    return m_stateManager;
}

Party& Game::getParty() {
    return m_party;
}

void Game::requestShutdown() {
    if (dynamic_cast<ShutdownState*>(m_stateManager.getCurrentState()) != nullptr) return;
    if (m_party.isRecoveryPending()) {
        m_stateManager.pushState(
            std::make_unique<ShutdownState>(*this, PersistenceStatus::RecoveryPending));
        return;
    }
    if (!m_party.hasActiveSaveSession()) {
        completeShutdown();
        return;
    }
    const auto result = m_party.saveToFile();
    if (result.status == PersistenceStatus::Saved) {
        completeShutdown();
        return;
    }
    m_stateManager.pushState(std::make_unique<ShutdownState>(*this, result.status));
}

void Game::completeShutdown() {
    m_shutdownApproved = true;
    m_window.close();
}

bool Game::isShutdownApproved() const { return m_shutdownApproved; }

void Game::processEvents() {
    sf::Event event;
    // SFML 이벤트 큐에 쌓인 메시지 순차 수집
    while (m_window.pollEvent(event)) {
        // 윈도우 닫기 버튼 클릭 시 창 종료
        if (event.type == sf::Event::Closed) {
            requestShutdown();
            return;
        }

        // 현재 활성화된 게임 상태 객체로 이벤트 넘기기
        GameState* currentState = m_stateManager.getCurrentState();
        if (currentState) {
            currentState->handleInput(event);
        }
    }
}

void Game::update(sf::Time deltaTime) {
    GameState* currentState = m_stateManager.getCurrentState();
    if (currentState) {
        currentState->update(deltaTime);
    }
}

void Game::render() {
    // 흑색에 가까운 다크 그린 레트로 배경색으로 프레임 버퍼 클리어
    m_window.clear(sf::Color(5, 11, 5));

    GameState* currentState = m_stateManager.getCurrentState();
    if (currentState) {
        currentState->draw(m_window);
    }

    // 변경된 프레임 버퍼 화면 출력
    m_window.display();
}

void Game::loadResources() {
    // 혼합 CJK/ASCII raster 검증을 통과한 Noto Sans CJK Regular를 사용한다.
    if (!m_cjkFont.loadFromFile(ResourceLocator::assetPath("fonts/NotoSansCJK-Regular.ttc").string())) {
        std::cerr << "[Critical] CJK 다국어 폴백 폰트를 로드하지 못했습니다." << std::endl;
    }

    // 2. 한글/영어 공용 네오둥근모 레트로 폰트 우선 로드 시도
    if (!m_font.loadFromFile(ResourceLocator::assetPath("fonts/neodgm.ttf").string())) {
        std::cerr << "[Warning] neodgm.ttf 로드 실패. UbuntuMono[wght].ttf 로드를 시도합니다." << std::endl;
        if (!m_font.loadFromFile(ResourceLocator::assetPath("fonts/UbuntuMono[wght].ttf").string())) {
            std::cerr << "[Warning] Ubuntu Mono 폰트 로드 실패. CJK 폰트로 대체 사용합니다." << std::endl;
            m_font = m_cjkFont; // CJK 폰트로 덮어쓰기 폴백
        }
    }
}

} // namespace crawl
