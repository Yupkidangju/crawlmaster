#include "controller/DungeonState.hpp"
#include "controller/BestiaryState.hpp"
#include "controller/CharacterInfoState.hpp"
#include "controller/CombatState.hpp"
#include "controller/QuestJournalState.hpp"
#include "controller/SettingsState.hpp"
#include "controller/TownState.hpp"
#include "core/AudioManager.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/ItemFactory.hpp"
#include "model/MonsterFactory.hpp"

#include <algorithm>
#include <cmath>

namespace crawl {

DungeonState::DungeonState(Game& game, int startFloor)
    : m_game(game),
      m_currentFloor(std::clamp(startFloor, 1, DungeonWorld::FLOOR_COUNT)),
      m_renderer(std::make_unique<DungeonRenderer>(game)) {
    auto& party = m_game.getParty();
    if (!party.getWorld().isGenerated()) {
        party.getWorld().generate(SessionRng::global().seed());
    }
    party.markFloorReached(m_currentFloor);
    auto& map = currentFloorMap();
    map.setPlayerPos(1, 1);
    map.setPlayerDir(map.isWalkable(2, 1) ? Direction::EAST : Direction::SOUTH);
    m_renderer->syncCameraImmediate(map);
    updateFogOfWar();
    updateInteractPrompt();

    AudioManager::getInstance().playBgm("bgm_dungeon");
    AudioManager::getInstance().playAmbience(m_currentFloor);

    const auto& lm = LocalizationManager::getInstance();
    addLog(lm.format("DUNGEON_ENTER_FLOOR", {
        {"floor", std::to_string(m_currentFloor)},
        {"biome", lm.get("BIOME_" + std::to_string(m_currentFloor) + "_NAME")}
    }));
}

DungeonMap& DungeonState::currentFloorMap() {
    return m_game.getParty().getWorld().getFloor(m_currentFloor);
}

const DungeonMap& DungeonState::currentFloorMap() const {
    return m_game.getParty().getWorld().getFloor(m_currentFloor);
}

void DungeonState::addLog(const std::string& message) {
    m_logQueue.push_back(message);
    if (m_logQueue.size() > 24) {
        m_logQueue.erase(m_logQueue.begin());
    }
}

void DungeonState::updateFogOfWar() {
    auto& map = currentFloorMap();
    const int px = map.getPlayerX();
    const int py = map.getPlayerY();
    map.setStepped(px, py, true);
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            if (std::abs(dx) + std::abs(dy) <= 3) {
                map.setVisited(px + dx, py + dy, true);
            }
        }
    }

    auto& party = m_game.getParty();
    const bool hasRogue = party.hasClassInParty(CharacterClass::ROGUE);
    const auto& lm = LocalizationManager::getInstance();
    for (auto& obj : party.getWorld().getObjects()) {
        if (obj.floor != m_currentFloor || obj.state != WorldObjectState::PRESENT) continue;
        const int dist = std::abs(obj.x - px) + std::abs(obj.y - py);
        if (obj.kind == WorldObjectKind::TRAP) {
            if (hasRogue && dist <= 2) {
                obj.state = WorldObjectState::DISCOVERED;
                addLog(lm.get("DUNGEON_TRAP_SPOTTED"));
            }
        } else if (dist <= 2) {
            obj.state = WorldObjectState::DISCOVERED;
        }
    }
}

void DungeonState::triggerTrap(WorldObject& trap) {
    auto& party = m_game.getParty();
    const auto& lm = LocalizationManager::getInstance();
    trap.state = WorldObjectState::RESOLVED;
    AudioManager::getInstance().playSfx("sfx_trap");
    m_renderer->triggerDamageFlash(0.85f);

    const int dmg = 4 + m_currentFloor * 2 + SessionRng::global().rollDie(4);
    for (const auto& m : party.getMembers()) {
        if (!m || m->isDead()) continue;
        m->takeDamage(dmg);
        if (trap.targetId == "trap_poison") m->setPoison(3);
    }
    addLog(lm.format("DUNGEON_TRAP_TRIGGERED", {{"damage", std::to_string(dmg)}}));
    m_game.pushToast(lm.format("DUNGEON_TRAP_TRIGGERED", {{"damage", std::to_string(dmg)}}), UiTheme::hpCrimson());
}

void DungeonState::attemptDisarmTrap(WorldObject& trap) {
    auto& party = m_game.getParty();
    const auto& lm = LocalizationManager::getInstance();
    int bestBonus = 0;
    for (const auto& m : party.getMembers()) {
        if (!m || m->isDead()) continue;
        int b = m->getAbilities().getModifier(m->getAbilities().dexterity);
        if (m->getClass() == CharacterClass::ROGUE) b += 4;
        if (m->getSpecialization() == Specialization::TRAPSMITH) b += 6;
        bestBonus = std::max(bestBonus, b);
    }
    const int roll = SessionRng::global().rollDie(20) + bestBonus;
    const int dc = 11 + m_currentFloor;
    if (roll >= dc) {
        trap.state = WorldObjectState::RESOLVED;
        party.addGold(15 * m_currentFloor);
        AudioManager::getInstance().playSfx("sfx_lever");
        addLog(lm.get("DUNGEON_TRAP_DISARMED"));
        m_game.pushToast(lm.get("DUNGEON_TRAP_DISARMED"), UiTheme::statPos());
    } else {
        triggerTrap(trap);
    }
    party.saveToFile();
}

void DungeonState::updateInteractPrompt() {
    const auto& map = currentFloorMap();
    const auto& party = m_game.getParty();
    const auto& lm = LocalizationManager::getInstance();
    const int px = map.getPlayerX();
    const int py = map.getPlayerY();

    m_ui.canInteract = false;
    m_ui.interactPrompt.clear();

    // 1. 앞칸 비밀 벽 검사
    int fx = px, fy = py;
    map.getNextCoords(fx, fy, true);
    if (map.getTile(fx, fy) == TileType::SECRET_WALL) {
        m_ui.canInteract = true;
        m_ui.interactPrompt = lm.get("PROMPT_SECRET_WALL");
        return;
    }

    // 2. 앞칸 탐지 함정 해제 검사
    if (const auto* frontObj = party.getWorld().findObjectAt(m_currentFloor, fx, fy)) {
        if (frontObj->kind == WorldObjectKind::TRAP && frontObj->state == WorldObjectState::DISCOVERED) {
            m_ui.canInteract = true;
            m_ui.interactPrompt = lm.get("PROMPT_DISARM_TRAP");
            return;
        }
    }

    // 3. 현재 칸 오브젝트 상호작용
    if (const auto* obj = party.getWorld().findObjectAt(m_currentFloor, px, py)) {
        if (obj->state != WorldObjectState::RESOLVED) {
            if (obj->kind == WorldObjectKind::CHEST) {
                m_ui.canInteract = true; m_ui.interactPrompt = lm.get("PROMPT_OPEN_CHEST"); return;
            }
            if (obj->kind == WorldObjectKind::LEVER) {
                m_ui.canInteract = true; m_ui.interactPrompt = lm.get("PROMPT_PULL_LEVER"); return;
            }
            if (obj->kind == WorldObjectKind::SANCTUARY) {
                m_ui.canInteract = true; m_ui.interactPrompt = lm.get("PROMPT_SANCTUARY"); return;
            }
            if (obj->kind == WorldObjectKind::LORE_PLAQUE) {
                m_ui.canInteract = true; m_ui.interactPrompt = lm.get("PROMPT_LORE"); return;
            }
            if (obj->kind == WorldObjectKind::QUEST_ITEM && party.hasQuest(obj->questId)) {
                m_ui.canInteract = true; m_ui.interactPrompt = lm.get("PROMPT_RELIC"); return;
            }
            if (obj->kind == WorldObjectKind::NPC && party.hasQuest(obj->questId)) {
                m_ui.canInteract = true; m_ui.interactPrompt = lm.get("PROMPT_NPC"); return;
            }
        }
    }

    // 4. 계단 / 보스 게이트
    const TileType cur = map.getTile(px, py);
    if (cur == TileType::UPSTAIRS) {
        m_ui.canInteract = true;
        m_ui.interactPrompt = (m_currentFloor == 1) ? lm.get("PROMPT_RETURN_TOWN") : lm.get("PROMPT_ASCEND");
    } else if (cur == TileType::DOWNSTAIRS) {
        m_ui.canInteract = true;
        m_ui.interactPrompt = lm.get("PROMPT_DESCEND");
    } else if (cur == TileType::BOSS_GATE) {
        m_ui.canInteract = true;
        m_ui.interactPrompt = lm.get("PROMPT_BOSS_GATE");
    }
}

void DungeonState::handleInteractAction() {
    auto& map = currentFloorMap();
    auto& party = m_game.getParty();
    const auto& lm = LocalizationManager::getInstance();
    const int px = map.getPlayerX();
    const int py = map.getPlayerY();
    int fx = px, fy = py;
    map.getNextCoords(fx, fy, true);

    // 1. 비밀 벽 개방
    if (map.getTile(fx, fy) == TileType::SECRET_WALL) {
        map.revealSecretWall(fx, fy);
        AudioManager::getInstance().playSfx("sfx_door");
        addLog(lm.get("DUNGEON_SECRET_FOUND"));
        m_game.pushToast(lm.get("DUNGEON_SECRET_FOUND"), UiTheme::textGold());
        party.saveToFile();
        updateFogOfWar();
        updateInteractPrompt();
        return;
    }

    // 2. 앞칸 탐지 함정 해제
    if (auto* frontObj = party.getWorld().findObjectAt(m_currentFloor, fx, fy)) {
        if (frontObj->kind == WorldObjectKind::TRAP && frontObj->state == WorldObjectState::DISCOVERED) {
            attemptDisarmTrap(*frontObj);
            updateInteractPrompt();
            return;
        }
    }

    // 3. 현재 칸 오브젝트 상호작용
    if (auto* obj = party.getWorld().findObjectAt(m_currentFloor, px, py)) {
        if (obj->state != WorldObjectState::RESOLVED) {
            if (obj->kind == WorldObjectKind::CHEST) {
                obj->state = WorldObjectState::RESOLVED;
                const int gold = 30 * m_currentFloor + SessionRng::global().rollRange(10, 35 * m_currentFloor);
                auto loot = ItemFactory::rollChestLoot(m_currentFloor, SessionRng::global());
                party.addGold(gold);
                if (loot) party.addItem(loot);
                party.saveToFile();
                AudioManager::getInstance().playSfx("sfx_chest");
                std::string msg = lm.format("DUNGEON_CHEST_LOOT", {
                    {"gold", std::to_string(gold)},
                    {"item", loot ? loot->getName() : "-"}
                });
                addLog(msg);
                m_game.pushToast(msg, UiTheme::textGold());
                updateInteractPrompt();
                return;
            }
            if (obj->kind == WorldObjectKind::LEVER) {
                obj->state = WorldObjectState::RESOLVED;
                map.unlockAllGates();
                party.saveToFile();
                AudioManager::getInstance().playSfx("sfx_lever");
                addLog(lm.get("DUNGEON_LEVER_PULLED"));
                m_game.pushToast(lm.get("DUNGEON_LEVER_PULLED"), UiTheme::cyanAccent());
                updateInteractPrompt();
                return;
            }
            if (obj->kind == WorldObjectKind::SANCTUARY) {
                obj->state = WorldObjectState::RESOLVED;
                for (const auto& m : party.getMembers()) {
                    if (m) m->sanctuaryRest();
                }
                party.saveToFile();
                AudioManager::getInstance().playSfx("sfx_sanctuary");
                addLog(lm.get("DUNGEON_SANCTUARY_USED"));
                m_game.pushToast(lm.get("DUNGEON_SANCTUARY_USED"), UiTheme::statPos());
                updateInteractPrompt();
                return;
            }
            if (obj->kind == WorldObjectKind::LORE_PLAQUE) {
                obj->state = WorldObjectState::RESOLVED;
                for (const auto& m : party.getMembers()) {
                    if (m && !m->isDead()) m->addXp(25 * m_currentFloor);
                }
                party.saveToFile();
                AudioManager::getInstance().playSfx("sfx_ui_click");
                const std::string lore = lm.get(obj->targetId);
                addLog(lore);
                m_game.pushToast(lore, UiTheme::cyanAccent());
                updateInteractPrompt();
                return;
            }
            if (obj->kind == WorldObjectKind::QUEST_ITEM && party.hasQuest(obj->questId)) {
                obj->state = WorldObjectState::RESOLVED;
                party.addKeyItem(obj->targetId);
                party.markQuestObjectiveComplete(obj->questId);
                party.saveToFile();
                AudioManager::getInstance().playSfx("sfx_chest");
                addLog(lm.get("DUNGEON_RELIC_OBTAINED"));
                m_game.pushToast(lm.get("DUNGEON_RELIC_OBTAINED"), UiTheme::textGold());
                updateInteractPrompt();
                return;
            }
            if (obj->kind == WorldObjectKind::NPC && party.hasQuest(obj->questId)) {
                obj->state = WorldObjectState::RESOLVED;
                party.markQuestObjectiveComplete(obj->questId);
                party.saveToFile();
                AudioManager::getInstance().playSfx("sfx_sanctuary");
                addLog(lm.get("DUNGEON_NPC_RESCUED"));
                m_game.pushToast(lm.get("DUNGEON_NPC_RESCUED"), UiTheme::cyanAccent());
                updateInteractPrompt();
                return;
            }
        }
    }

    // 4. 계단 & 최종 보스 게이트
    const TileType cur = map.getTile(px, py);
    if (cur == TileType::UPSTAIRS) {
        AudioManager::getInstance().playSfx("sfx_door");
        if (m_currentFloor == 1) {
            party.saveToFile();
            m_game.triggerFadeIn(0.26f);
            m_game.getStates().changeState(std::make_unique<TownState>(m_game));
            return;
        }
        --m_currentFloor;
        auto& prevMap = currentFloorMap();
        prevMap.setPlayerPos(1, 1);
        m_renderer->syncCameraImmediate(prevMap);
        AudioManager::getInstance().playAmbience(m_currentFloor);
        updateFogOfWar();
        updateInteractPrompt();
        addLog(lm.format("DUNGEON_ENTER_FLOOR", {
            {"floor", std::to_string(m_currentFloor)},
            {"biome", lm.get("BIOME_" + std::to_string(m_currentFloor) + "_NAME")}
        }));
        party.saveToFile();
        return;
    }

    if (cur == TileType::DOWNSTAIRS && m_currentFloor < DungeonWorld::FLOOR_COUNT) {
        AudioManager::getInstance().playSfx("sfx_door");
        ++m_currentFloor;
        party.markFloorReached(m_currentFloor);
        auto& nextMap = currentFloorMap();
        nextMap.setPlayerPos(1, 1);
        nextMap.setPlayerDir(nextMap.isWalkable(2, 1) ? Direction::EAST : Direction::SOUTH);
        m_renderer->syncCameraImmediate(nextMap);
        AudioManager::getInstance().playAmbience(m_currentFloor);
        m_game.triggerFadeIn(0.28f);
        updateFogOfWar();
        updateInteractPrompt();
        const std::string msg = lm.format("DUNGEON_ENTER_FLOOR", {
            {"floor", std::to_string(m_currentFloor)},
            {"biome", lm.get("BIOME_" + std::to_string(m_currentFloor) + "_NAME")}
        });
        addLog(msg);
        m_game.pushToast(msg, UiTheme::textGold());
        party.saveToFile();
        return;
    }

    if (cur == TileType::BOSS_GATE) {
        EncounterSpec spec;
        spec.tier = EncounterTier::SANCTUM;
        spec.fixedMonsterId = "mon_elder_wyrm";
        spec.questId = "qst_08_elder_wyrm";
        spec.bossBattle = true;
        spec.campaignFinal = true;
        spec.floorNumber = m_currentFloor;
        party.saveToFile();
        m_game.triggerFadeIn(0.28f);
        m_game.getStates().pushState(std::make_unique<CombatState>(m_game, spec));
    }
}

void DungeonState::handleStepTriggers(bool fromAutoWalk) {
    auto& party = m_game.getParty();
    const auto& map = currentFloorMap();
    const int px = map.getPlayerX();
    const int py = map.getPlayerY();

    party.incrementTurns(1);
    updateFogOfWar();
    updateInteractPrompt();

    // 1. 미해제 함정 밟음 판정
    if (auto* obj = party.getWorld().findObjectAt(m_currentFloor, px, py)) {
        if (obj->kind == WorldObjectKind::TRAP && obj->state != WorldObjectState::RESOLVED) {
            m_autoWalkPath.clear();
            triggerTrap(*obj);
            updateInteractPrompt();
            return;
        }
        // 2. 고정 층 보스 조우
        if (obj->kind == WorldObjectKind::QUEST_BOSS && obj->state != WorldObjectState::RESOLVED) {
            m_autoWalkPath.clear();
            EncounterSpec spec;
            spec.tier = MonsterFactory::tierForFloor(m_currentFloor);
            spec.fixedMonsterId = obj->targetId;
            spec.questId = obj->questId;
            spec.worldObjectId = obj->id;
            spec.bossBattle = true;
            spec.campaignFinal = false;
            spec.floorNumber = m_currentFloor;
            party.saveToFile();
            m_game.triggerFadeIn(0.24f);
            m_game.getStates().pushState(std::make_unique<CombatState>(m_game, spec));
            return;
        }
    }

    // 3. 자동 이동 중 최종 보스 게이트 진입
    if (fromAutoWalk && map.getTile(px, py) == TileType::BOSS_GATE) {
        m_autoWalkPath.clear();
        handleInteractAction();
        return;
    }

    // 4. 랜덤 조우 (입구 2칸 안전지대 보장, 3보 이상 이동 후 확률)
    ++m_stepsSinceEncounter;
    if (map.getDistanceFromStart(px, py) >= 2 && m_stepsSinceEncounter >= 4) {
        const int chance = 10 + m_stepsSinceEncounter * 2;
        if (SessionRng::global().rollRange(1, 100) <= chance) {
            m_stepsSinceEncounter = 0;
            m_autoWalkPath.clear();
            EncounterSpec spec;
            spec.tier = MonsterFactory::tierForFloor(m_currentFloor);
            spec.floorNumber = m_currentFloor;
            party.saveToFile();
            m_game.triggerFadeIn(0.22f);
            m_game.getStates().pushState(std::make_unique<CombatState>(m_game, spec));
        }
    }
}

void DungeonState::attemptMoveTo(int nx, int ny) {
    auto& map = currentFloorMap();
    if (!map.isWalkable(nx, ny)) {
        m_renderer->notifyStep(true);
        AudioManager::getInstance().playSfx("sfx_bump");
        if (map.getTile(nx, ny) == TileType::LOCKED_GATE) {
            addLog(LocalizationManager::getInstance().get("DUNGEON_GATE_LOCKED"));
        }
        return;
    }
    const bool isDoor = (map.getTile(nx, ny) == TileType::DOOR);
    map.setPlayerPos(nx, ny);
    m_renderer->notifyStep(false);
    AudioManager::getInstance().playSfx(isDoor ? "sfx_door" : "sfx_step");
    handleStepTriggers(false);
}

void DungeonState::handleMapClick(sf::Vector2f mousePos, sf::FloatRect gridRect) {
    if (!gridRect.contains(mousePos)) return;
    const auto& map = currentFloorMap();
    const float cellW = gridRect.width / static_cast<float>(DungeonMap::MAP_WIDTH);
    const float cellH = gridRect.height / static_cast<float>(DungeonMap::MAP_HEIGHT);
    const int tx = static_cast<int>((mousePos.x - gridRect.left) / cellW);
    const int ty = static_cast<int>((mousePos.y - gridRect.top) / cellH);
    if (!map.isVisited(tx, ty) || !map.isWalkable(tx, ty)) return;
    m_autoWalkPath = map.findPath(map.getPlayerX(), map.getPlayerY(), tx, ty);
    m_autoWalkTimer = 0.0f;
    m_ui.fullMapOpen = false;
    AudioManager::getInstance().playSfx("sfx_ui_click");
}

void DungeonState::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const auto pos = m_game.mapPixelToLogical({event.mouseButton.x, event.mouseButton.y});
        if (m_ui.fullMapOpen) {
            if (m_ui.fullMapGridRect.contains(pos)) {
                handleMapClick(pos, m_ui.fullMapGridRect);
            } else {
                m_ui.fullMapOpen = false;
                AudioManager::getInstance().playSfx("sfx_ui_click");
            }
            return;
        }
        if (m_ui.minimapRect.contains(pos)) {
            m_ui.fullMapOpen = true;
            AudioManager::getInstance().playSfx("sfx_ui_click");
            return;
        }
        if (m_ui.canInteract && m_ui.interactBannerRect.contains(pos)) {
            handleInteractAction();
            return;
        }
        for (int i = 0; i < 4; ++i) {
            if (m_ui.portraitRects[static_cast<std::size_t>(i)].contains(pos) &&
                i < m_game.getParty().getMemberCount()) {
                AudioManager::getInstance().playSfx("sfx_ui_click");
                m_game.getStates().pushState(std::make_unique<CharacterInfoState>(m_game, i));
                return;
            }
        }
        if (m_ui.dockRects[0].contains(pos)) { m_game.getStates().pushState(std::make_unique<CharacterInfoState>(m_game, 0)); return; }
        if (m_ui.dockRects[1].contains(pos)) { m_game.getStates().pushState(std::make_unique<QuestJournalState>(m_game)); return; }
        if (m_ui.dockRects[2].contains(pos)) { m_game.getStates().pushState(std::make_unique<BestiaryState>(m_game)); return; }
        if (m_ui.dockRects[3].contains(pos)) { m_ui.fullMapOpen = !m_ui.fullMapOpen; AudioManager::getInstance().playSfx("sfx_ui_click"); return; }
        if (m_ui.dockRects[4].contains(pos)) { m_game.getStates().pushState(std::make_unique<SettingsState>(m_game)); return; }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
        if (m_ui.fullMapOpen) {
            m_ui.fullMapOpen = false;
            AudioManager::getInstance().playSfx("sfx_ui_click");
        }
        return;
    }

    if (event.type != sf::Event::KeyPressed) return;
    const auto key = event.key.code;

    if (m_ui.fullMapOpen) {
        if (key == sf::Keyboard::Escape || key == sf::Keyboard::M) {
            m_ui.fullMapOpen = false;
            AudioManager::getInstance().playSfx("sfx_ui_click");
        }
        return;
    }

    m_autoWalkPath.clear();
    auto& map = currentFloorMap();

    if (key == sf::Keyboard::W || key == sf::Keyboard::Up) {
        int nx = 0, ny = 0;
        map.getNextCoords(nx, ny, true);
        attemptMoveTo(nx, ny);
    } else if (key == sf::Keyboard::S || key == sf::Keyboard::Down) {
        int nx = 0, ny = 0;
        map.getNextCoords(nx, ny, false);
        attemptMoveTo(nx, ny);
    } else if (key == sf::Keyboard::A || key == sf::Keyboard::Left) {
        map.turn(false);
        AudioManager::getInstance().playSfx("sfx_turn");
        updateInteractPrompt();
    } else if (key == sf::Keyboard::D || key == sf::Keyboard::Right) {
        map.turn(true);
        AudioManager::getInstance().playSfx("sfx_turn");
        updateInteractPrompt();
    } else if (key == sf::Keyboard::Z) {
        int nx = 0, ny = 0;
        map.getStrafeCoords(nx, ny, false);
        attemptMoveTo(nx, ny);
    } else if (key == sf::Keyboard::X) {
        int nx = 0, ny = 0;
        map.getStrafeCoords(nx, ny, true);
        attemptMoveTo(nx, ny);
    } else if (key == sf::Keyboard::E || key == sf::Keyboard::Enter || key == sf::Keyboard::Space) {
        handleInteractAction();
    } else if (key == sf::Keyboard::M) {
        m_ui.fullMapOpen = !m_ui.fullMapOpen;
        AudioManager::getInstance().playSfx("sfx_ui_click");
    } else if (key == sf::Keyboard::C) {
        m_game.getStates().pushState(std::make_unique<CharacterInfoState>(m_game, 0));
    } else if (key == sf::Keyboard::J || key == sf::Keyboard::Q) {
        m_game.getStates().pushState(std::make_unique<QuestJournalState>(m_game));
    } else if (key == sf::Keyboard::B) {
        m_game.getStates().pushState(std::make_unique<BestiaryState>(m_game));
    } else if (key == sf::Keyboard::O || key == sf::Keyboard::Escape) {
        m_game.getStates().pushState(std::make_unique<SettingsState>(m_game));
    }
}

void DungeonState::update(sf::Time deltaTime) {
    const float dt = deltaTime.asSeconds();
    auto& map = currentFloorMap();
    m_renderer->update(dt, map);

    // 전투 복귀 시 던전 BGM/바이옴 환경음 자동 복원
    if (AudioManager::getInstance().currentBgm() != "bgm_dungeon") {
        AudioManager::getInstance().playBgm("bgm_dungeon");
        AudioManager::getInstance().playAmbience(m_currentFloor);
    }

    if (!m_autoWalkPath.empty()) {
        m_autoWalkTimer += dt;
        if (m_autoWalkTimer >= 0.14f) {
            m_autoWalkTimer = 0.0f;
            const auto [nx, ny] = m_autoWalkPath.front();
            m_autoWalkPath.erase(m_autoWalkPath.begin());
            const int dx = nx - map.getPlayerX();
            const int dy = ny - map.getPlayerY();
            if (dx > 0) map.setPlayerDir(Direction::EAST);
            else if (dx < 0) map.setPlayerDir(Direction::WEST);
            else if (dy > 0) map.setPlayerDir(Direction::SOUTH);
            else if (dy < 0) map.setPlayerDir(Direction::NORTH);

            if (map.isWalkable(nx, ny)) {
                map.setPlayerPos(nx, ny);
                m_renderer->notifyStep(false);
                AudioManager::getInstance().playSfx("sfx_step");
                handleStepTriggers(true);
            } else {
                m_autoWalkPath.clear();
            }
        }
    }
}

void DungeonState::draw(sf::RenderWindow& window) {
    m_ui.mousePos = m_game.getMouseLogicalPosition();
    m_renderer->render(window, currentFloorMap(), m_game.getParty(),
                       m_logQueue, m_currentFloor,
                       m_game.getParty().getWorld().getObjects(), m_ui);
}

} // namespace crawl
