#include "controller/DungeonState.hpp"
#include "controller/TownState.hpp"
#include "controller/CombatState.hpp"
#include "controller/CharacterInfoState.hpp"
#include "controller/SettingsState.hpp"
#include "controller/QuestJournalState.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include <iostream>
#include <random>

namespace crawl {

DungeonState::DungeonState(Game& game)
    : m_game(game), m_renderer(game) {
    auto& world = m_game.getParty().getWorld();
    if (!world.isGenerated()) {
        const std::uint32_t seed = SessionRng::global().seed();
        world.generate(seed == 0U ? 0x9E3779B9U : seed);
    }
    map().setPlayerPos(1, 1);
    map().setPlayerDir(Direction::NORTH);

    // 2. 초기 스폰 위치 및 주변 미니맵 안개(시야) 개방
    revealFogOfWar();

    // 3. 탐험 개시 웰컴 로그 등록 (다국어 키 적용)
    auto& lm = LocalizationManager::getInstance();
    addLog("> " + lm.get("MSG_DUNGEON_ENTER"));
    if (lm.getTextScale() > 125) {
        addLog("> " + lm.get("DUNGEON_GUIDE_MOVE_SHORT"));
        addLog("> " + lm.get("DUNGEON_GUIDE_ACTIONS_SHORT"));
    } else {
        addLog("> " + lm.get("SETTINGS_KEY_GUIDE"));
        addLog("> " + lm.get("SETTINGS_GUIDE_MOVE"));
        addLog("> " + lm.get("SETTINGS_GUIDE_ESC"));
    }
}

void DungeonState::revealFogOfWar() {
    int px = map().getPlayerX();
    int py = map().getPlayerY();
    Direction dir = map().getPlayerDir();

    // 현재 발을 디딘 곳을 밟은 타일로 영구 저장
    map().setStepped(px, py, true);

    // 플레이어 주변 3x3 범위 안개 제거
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            map().setVisited(px + dx, py + dy, true);
        }
    }

    // 플레이어 시야 방향 전방 3칸 및 좌우 밝히기 (1인칭 와이어프레임과 정합성 유지)
    int fx = 0, fy = 0;
    int rx = 0, ry = 0;
    switch (dir) {
        case Direction::NORTH: fx = 0;  fy = -1; rx = 1;  ry = 0;  break;
        case Direction::EAST:  fx = 1;  fy = 0;  rx = 0;  ry = 1;  break;
        case Direction::SOUTH: fx = 0;  fy = 1;  rx = -1; ry = 0;  break;
        case Direction::WEST:  fx = -1; fy = 0;  rx = 0;  ry = -1; break;
    }

    for (int d = 1; d <= 3; ++d) {
        int cx = px + d * fx;
        int cy = py + d * fy;

        map().setVisited(cx, cy, true);
        map().setVisited(cx - rx, cy - ry, true);
        map().setVisited(cx + rx, cy + ry, true);

        // 시선상 벽을 만나면 더 이상 투영 및 시야 개방 불가
        if (map().getTile(cx, cy) == TileType::WALL) {
            break;
        }
    }
    m_worldDirty = true;
    discoverQuestObjects();
}

void DungeonState::handleInput(const sf::Event& event) {
    // [v0.6.0] 마우스 미니맵 클릭 시 자동 이동 좌표 역산 및 BFS 탐색 수행
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        float mx = static_cast<float>(event.mouseButton.x);
        float my = static_cast<float>(event.mouseButton.y);

        // 미니맵 좌표 영역(750x40 ~ 990x280) 안에 들어왔는지 검출
        if (mx >= 750.0f && mx < 990.0f && my >= 40.0f && my < 280.0f) {
            int tx = static_cast<int>((mx - 750.0f) / 12.0f);
            int ty = static_cast<int>((my - 40.0f) / 12.0f);

            // 목적지가 안개가 걷힌 지역이자 이동 가능한 타일인지 체크
            if (map().isVisited(tx, ty) && map().isWalkable(tx, ty)) {
                auto path = map().findPath(map().getPlayerX(), map().getPlayerY(), tx, ty);
                if (!path.empty()) {
                    m_autoPath = path;
                    m_autoPathIndex = 0;
                    m_autoMoveElapsed = sf::Time::Zero;
                    addLog("> " + LocalizationManager::getInstance().get("DUNGEON_AUTO_STARTED"));
                } else {
                    addLog("> " + LocalizationManager::getInstance().get("DUNGEON_AUTO_AT_DESTINATION"));
                }
            } else {
                addLog("> " + LocalizationManager::getInstance().get("DUNGEON_FOG_BLOCKED"));
            }
        }
    }

    if (event.type == sf::Event::KeyPressed) {
        // [v0.6.0] 키보드 조작 시 자동 이동을 즉시 중단(캔슬) 처리
        if (!m_autoPath.empty()) {
            m_autoPath.clear();
            addLog("> " + LocalizationManager::getInstance().get("DUNGEON_AUTO_CANCELLED"));
        }

        int nextX = 0, nextY = 0;

        switch (event.key.code) {
            case sf::Keyboard::W:
            case sf::Keyboard::Up:
                // 전방 좌표 검출
                map().getNextCoords(nextX, nextY, true);
                if (map().isWalkable(nextX, nextY)) {
                    map().setPlayerPos(nextX, nextY);
                    revealFogOfWar();
                    addLog("> " + LocalizationManager::getInstance().get("DUNGEON_MOVED_FORWARD"));
                    if (checkCurrentTileLog()) return;
 
                    // 10% 확률로 랜덤 인카운터 전투 진입 (계단 타일 1,1 제외)
                    if (!(nextX == 1 && nextY == 1)) {
                        if (SessionRng::global().rollRange(1, 100) <= 10) {
                            std::cout << "[FSM] 몬스터를 조우하여 전투 상태로 진입합니다." << std::endl;
                            // [v0.5.0] changeState 사용 시 기존 탐험 상태 메모리가 소멸되어 좌표 롤백이 일어나는 현상 해결을 위해 pushState 적용
                            m_game.getStates().pushState(std::make_unique<CombatState>(m_game, currentEncounterTier()));
                            return; // 키 입력 루프 즉시 종료
                        }
                    }
                } else {
                    addLog("> " + LocalizationManager::getInstance().get("DUNGEON_WALL_BLOCKED"));
                }
                break;
 
            case sf::Keyboard::S:
            case sf::Keyboard::Down:
                // 후방 좌표 검출
                map().getNextCoords(nextX, nextY, false);
                if (map().isWalkable(nextX, nextY)) {
                    map().setPlayerPos(nextX, nextY);
                    revealFogOfWar();
                    addLog("> " + LocalizationManager::getInstance().get("DUNGEON_MOVED_BACKWARD"));
                    if (checkCurrentTileLog()) return;
 
                    // 10% 확률로 랜덤 인카운터 전투 진입 (계단 타일 1,1 제외)
                    if (!(nextX == 1 && nextY == 1)) {
                        if (SessionRng::global().rollRange(1, 100) <= 10) {
                            std::cout << "[FSM] 몬스터를 조우하여 전투 상태로 진입합니다." << std::endl;
                            // [v0.5.0] changeState 사용 시 기존 탐험 상태 메모리가 소멸되어 좌표 롤백이 일어나는 현상 해결을 위해 pushState 적용
                            m_game.getStates().pushState(std::make_unique<CombatState>(m_game, currentEncounterTier()));
                            return;
                        }
                    }
                } else {
                    addLog("> " + LocalizationManager::getInstance().get("DUNGEON_WALL_BLOCKED"));
                }
                break;
 
            case sf::Keyboard::A:
            case sf::Keyboard::Left:
                // 좌회전 90도
                map().turn(false);
                revealFogOfWar();
                addLog("> " + LocalizationManager::getInstance().get("DUNGEON_TURNED_LEFT"));
                break;
 
            case sf::Keyboard::D:
            case sf::Keyboard::Right:
                // 우회전 90도
                map().turn(true);
                revealFogOfWar();
                addLog("> " + LocalizationManager::getInstance().get("DUNGEON_TURNED_RIGHT"));
                break;
 
            case sf::Keyboard::Escape:
                // 현재 서 있는 위치가 마을 계단(UPSTAIRS)인지 체크
                if (m_floorNumber == 1 && map().getTile(map().getPlayerX(), map().getPlayerY()) == TileType::UPSTAIRS) {
                    addLog("> " + LocalizationManager::getInstance().get("DUNGEON_RETURNING_TOWN"));
                    const auto saveResult = m_game.getParty().saveToFile();
                    if (!saveResult.durabilityConfirmed()) {
                        addLog("> " + LocalizationManager::getInstance().get("MSG_DURABILITY_UNKNOWN_STAY"));
                        return;
                    }
                    if (!saveResult) {
                        addLog("> " + LocalizationManager::getInstance().get("MSG_SAVE_FAILED_STAY"));
                        return;
                    }
                    std::cout << "[FSM] DungeonState에서 TownState로 상태 귀환을 요청합니다." << std::endl;
                    m_game.getStates().changeState(std::make_unique<TownState>(m_game));
                } else {
                    auto& lmInstance = LocalizationManager::getInstance();
                    addLog("> " + lmInstance.get("MSG_DUNGEON_ESC"));
                }
                break;

            case sf::Keyboard::I:
            case sf::Keyboard::C:
                // [v0.6.0] 캐릭터 장비 및 인벤토리 관리 화면 기동
                m_game.getStates().pushState(std::make_unique<CharacterInfoState>(m_game, false));
                break;

            case sf::Keyboard::E:
                interactCurrentTile();
                break;
 
            case sf::Keyboard::O: // [v0.9.0] 설정 화면 기동 추가
                m_game.getStates().pushState(std::make_unique<SettingsState>(m_game));
                break;

            case sf::Keyboard::Q:
                m_game.getStates().pushState(std::make_unique<QuestJournalState>(m_game));
                break;
 
            default:
                break;
        }
    }
}

void DungeonState::update(sf::Time deltaTime) {
    // [v0.6.0] 자동 이동 루프 수행 (0.1초 주기적 업데이트)
    if (!m_autoPath.empty() && m_autoPathIndex < m_autoPath.size()) {
        m_autoMoveElapsed += deltaTime;
        while (m_autoMoveElapsed.asSeconds() >= 0.1f && !m_autoPath.empty() && m_autoPathIndex < m_autoPath.size()) {
            m_autoMoveElapsed -= sf::seconds(0.1f);
            stepAutoMove();
        }
    }
    if (m_worldDirty) {
        m_persistenceElapsed += deltaTime;
        if (m_persistenceElapsed >= sf::seconds(2.0f)) persistWorldCheckpoint();
    }
}

void DungeonState::stepAutoMove() {
    if (m_autoPathIndex >= m_autoPath.size()) {
        m_autoPath.clear();
        return;
    }

    int px = map().getPlayerX();
    int py = map().getPlayerY();

    auto [nx, ny] = m_autoPath[m_autoPathIndex];

    // 다음 좌표 이동에 맞춰 플레이어 바라보는 방향 동적 변환
    int dx = nx - px;
    int dy = ny - py;
    if (dx == 1) {
        map().setPlayerDir(Direction::EAST);
    } else if (dx == -1) {
        map().setPlayerDir(Direction::WEST);
    } else if (dy == 1) {
        map().setPlayerDir(Direction::SOUTH);
    } else if (dy == -1) {
        map().setPlayerDir(Direction::NORTH);
    }

    // 위치 이동 및 시야 갱신
    map().setPlayerPos(nx, ny);
    revealFogOfWar();

    addLog("> " + LocalizationManager::getInstance().get("DUNGEON_AUTO_MOVING"));
    if (checkCurrentTileLog()) {
        m_autoPath.clear();
        return;
    }

    m_autoPathIndex++;

    // 10%의 확률로 인카운터 격발 검사 (계단 1,1 구역 제외)
    if (!(nx == 1 && ny == 1)) {
        if (SessionRng::global().rollRange(1, 100) <= 10) {
            std::cout << "[FSM] 자동 이동 도중 몬스터를 조우하여 전투로 이행합니다." << std::endl;
            m_autoPath.clear(); // 몬스터 조우 시 자동 이동을 영구 정지(큐 비우기)
            m_game.getStates().pushState(std::make_unique<CombatState>(m_game, currentEncounterTier()));
            return;
        }
    }

    // 목표지 도착 완료 검사
    if (m_autoPathIndex >= m_autoPath.size()) {
        m_autoPath.clear();
        addLog("> " + LocalizationManager::getInstance().get("DUNGEON_ARRIVED"));
    }
}

void DungeonState::draw(sf::RenderWindow& window) {
    // 렌더러를 호출해 와이어프레임 3D 및 HUD 정보 출력
    m_renderer.render(window, map(), m_game.getParty(), m_logQueue,
                      m_floorNumber, m_game.getParty().getWorld().getObjects());
}

void DungeonState::addLog(const std::string& message) {
    m_logQueue.push_back(message);
    // 큐 크기를 최대 30개로 유지하여 메모리 누수 방지
    if (m_logQueue.size() > 30) {
        m_logQueue.erase(m_logQueue.begin());
    }
}

bool DungeonState::checkCurrentTileLog() {
    int px = map().getPlayerX();
    int py = map().getPlayerY();

    if (auto* object = m_game.getParty().getWorld().findObjectAt(m_floorNumber, px, py);
        object && object->kind == WorldObjectKind::QUEST_BOSS &&
        object->state != WorldObjectState::RESOLVED) {
        if (!m_game.getParty().hasQuest(object->questId)) {
            addLog("> " + LocalizationManager::getInstance().get("DUNGEON_QUEST_REQUIRED"));
            return false;
        }
        object->state = WorldObjectState::DISCOVERED;
        m_game.getStates().pushState(std::make_unique<CombatState>(m_game, EncounterSpec{
            EncounterTier::LATE, object->targetId, object->questId, object->id, true, false}));
        return true;
    }

    if (map().getTile(px, py) == TileType::UPSTAIRS) {
        auto& lmInstance = LocalizationManager::getInstance();
        addLog("> " + lmInstance.get("MSG_DUNGEON_ESC"));
    } else if (map().getTile(px, py) == TileType::DOOR) {
        addLog("> " + LocalizationManager::getInstance().get("MSG_DUNGEON_LANDMARK"));
    } else if (map().getTile(px, py) == TileType::BOSS_GATE) {
        if (m_game.getParty().isCampaignCompleted()) {
            addLog("> " + LocalizationManager::getInstance().get("MSG_DUNGEON_BOSS_COMPLETE"));
            return false;
        }
        addLog("> " + LocalizationManager::getInstance().get("MSG_DUNGEON_BOSS_GATE"));
        m_game.getStates().pushState(std::make_unique<CombatState>(m_game, EncounterTier::LATE, true));
        return true;
    }
    return false;
}

EncounterTier DungeonState::currentEncounterTier() const {
    const int progress = map().getProgressPercent();
    if (progress <= 33) return EncounterTier::EARLY;
    if (progress <= 66) return EncounterTier::MIDDLE;
    return EncounterTier::LATE;
}

void DungeonState::discoverQuestObjects() {
    for (auto& object : m_game.getParty().getWorld().getObjects()) {
        if (object.floor == m_floorNumber && object.state == WorldObjectState::PRESENT &&
            m_game.getParty().hasQuest(object.questId) && map().isVisited(object.x, object.y)) {
            object.state = WorldObjectState::DISCOVERED;
            m_worldDirty = true;
        }
    }
}

bool DungeonState::persistWorldCheckpoint() {
    const auto result = m_game.getParty().saveToFile();
    m_persistenceElapsed = sf::Time::Zero;
    if (!result.durabilityConfirmed()) {
        m_worldDirty = false;
        addLog("> " + LocalizationManager::getInstance().get("MSG_DURABILITY_UNKNOWN_STAY"));
        return true;
    }
    if (!result) {
        addLog("> " + LocalizationManager::getInstance().get("MSG_SAVE_FAILED_STAY"));
        return false;
    }
    m_worldDirty = false;
    return true;
}

bool DungeonState::changeFloor(int floorNumber) {
    if (floorNumber < 1 || floorNumber > DungeonWorld::FLOOR_COUNT) return false;
    const int previousFloor = m_floorNumber;
    const PartyCheckpoint checkpoint = m_game.getParty().captureCheckpoint();
    if (!persistWorldCheckpoint()) return false;
    m_floorNumber = floorNumber;
    int entryX = 1;
    int entryY = 1;
    if (floorNumber < previousFloor) {
        for (int x = 0; x < DungeonMap::MAP_WIDTH; ++x) {
            for (int y = 0; y < DungeonMap::MAP_HEIGHT; ++y) {
                if (map().getTile(x, y) == TileType::DOWNSTAIRS) {
                    entryX = x;
                    entryY = y;
                }
            }
        }
    }
    map().setPlayerPos(entryX, entryY);
    map().setPlayerDir(Direction::NORTH);
    revealFogOfWar();
    addLog("> " + LocalizationManager::getInstance().format("DUNGEON_FLOOR_CHANGED",
        {{"floor", std::to_string(m_floorNumber)}}));
    if (persistWorldCheckpoint()) return true;
    m_game.getParty().restoreCheckpoint(checkpoint);
    m_floorNumber = previousFloor;
    return false;
}

bool DungeonState::interactCurrentTile() {
    const int x = map().getPlayerX();
    const int y = map().getPlayerY();
    const TileType tile = map().getTile(x, y);
    if (tile == TileType::DOWNSTAIRS) return changeFloor(m_floorNumber + 1);
    if (tile == TileType::UPSTAIRS) {
        if (m_floorNumber > 1) return changeFloor(m_floorNumber - 1);
        sf::Event event{};
        event.type = sf::Event::KeyPressed;
        event.key.code = sf::Keyboard::Escape;
        handleInput(event);
        return true;
    }

    auto* object = m_game.getParty().getWorld().findObjectAt(m_floorNumber, x, y);
    if (!object || object->kind == WorldObjectKind::QUEST_BOSS ||
        object->state == WorldObjectState::RESOLVED) {
        addLog("> " + LocalizationManager::getInstance().get("DUNGEON_NOTHING_TO_INTERACT"));
        return false;
    }
    if (!m_game.getParty().hasQuest(object->questId)) {
        addLog("> " + LocalizationManager::getInstance().get("DUNGEON_QUEST_REQUIRED"));
        return false;
    }
    const PartyCheckpoint checkpoint = m_game.getParty().captureCheckpoint();

    if (object->kind == WorldObjectKind::QUEST_ITEM &&
        !m_game.getParty().hasKeyItem(object->targetId)) {
        if (!m_game.getParty().addKeyItem(object->targetId)) return false;
    }
    if (!m_game.getParty().markQuestObjectiveComplete(object->questId)) return false;
    object->state = WorldObjectState::RESOLVED;
    m_worldDirty = true;
    if (!persistWorldCheckpoint()) {
        m_game.getParty().restoreCheckpoint(checkpoint);
        return false;
    }
    addLog("> " + LocalizationManager::getInstance().get(
        object->kind == WorldObjectKind::QUEST_ITEM ? "DUNGEON_ITEM_RECOVERED" : "DUNGEON_NPC_FOUND"));
    return true;
}

DungeonMap& DungeonState::map() {
    return m_game.getParty().getWorld().getFloor(m_floorNumber);
}

const DungeonMap& DungeonState::map() const {
    return m_game.getParty().getWorld().getFloor(m_floorNumber);
}

} // namespace crawl
