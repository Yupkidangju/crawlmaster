#include "controller/CombatState.hpp"
#include "controller/GameOverState.hpp"
#include "controller/SettingsState.hpp"
#include "controller/VictoryState.hpp"
#include "core/AudioManager.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/CombatActionRules.hpp"
#include "model/CombatRules.hpp"
#include "model/ConcreteItems.hpp"
#include "model/ItemFactory.hpp"
#include "model/Skill.hpp"
#include "view/UiSystem.hpp"
#include "view/UiTheme.hpp"

#include <algorithm>
#include <cmath>

namespace crawl {

CombatState::CombatState(Game& game, EncounterTier tier, bool bossBattle)
    : CombatState(game, EncounterSpec{
          tier, bossBattle ? "mon_elder_wyrm" : "", "", "", bossBattle, bossBattle,
          static_cast<int>(tier) + 1}) {}

CombatState::CombatState(Game& game, EncounterSpec encounter)
    : m_game(game),
      m_encounter(std::move(encounter)),
      m_backdropRenderer(std::make_unique<DungeonRenderer>(game)) {
    spawnMonsters();
    rollInitiatives();

    // 도감 조우 기록 및 초기 인텐트 결정
    auto& bestiary = m_game.getParty().getBestiary();
    for (auto& foe : m_foes) {
        if (!foe) continue;
        bestiary.recordEncounter(foe->getId());
        foe->rollNextIntent(SessionRng::global());
    }

    m_foeHitFlash.assign(m_foes.size(), 0.0f);
    m_foeLunge.assign(m_foes.size(), 0.0f);
    m_foeHpTrail.assign(m_foes.size(), 1.0f);
    for (int i = 0; i < m_game.getParty().getMemberCount() && i < 4; ++i) {
        const auto m = m_game.getParty().getMember(i);
        m_partyHpTrail[static_cast<std::size_t>(i)] =
            m ? static_cast<float>(m->getHp()) / static_cast<float>(std::max(1, m->getMaxHp())) : 0.0f;
    }

    AudioManager::getInstance().playBgm(m_encounter.bossBattle ? "bgm_boss" : "bgm_combat");
    addLog(LocalizationManager::getInstance().get("COMBAT_LOG_START"));
    selectFirstLivingFoe();
    startCurrentTurn();
}

void CombatState::spawnMonsters() {
    m_foes.clear();
    if (!m_encounter.fixedMonsterId.empty()) {
        if (auto boss = MonsterFactory::createMonster(m_encounter.fixedMonsterId)) {
            m_foes.push_back(std::move(boss));
        }
        return;
    }
    const int count = SessionRng::global().rollRange(1, 3);
    for (int i = 0; i < count; ++i) {
        if (auto mon = MonsterFactory::createRandomMonster(m_encounter.tier, SessionRng::global())) {
            m_foes.push_back(std::move(mon));
        }
    }
}

void CombatState::rollInitiatives() {
    m_turnOrder.clear();
    const auto& party = m_game.getParty();
    for (int i = 0; i < party.getMemberCount(); ++i) {
        const auto m = party.getMember(i);
        if (!m || m->isDead()) continue;
        const int init = SessionRng::global().rollDie(20) + CombatRules::initiativeBonus(*m);
        m_turnOrder.push_back({false, i, init});
    }
    for (int i = 0; i < static_cast<int>(m_foes.size()); ++i) {
        if (!m_foes[static_cast<std::size_t>(i)] || m_foes[static_cast<std::size_t>(i)]->isDead()) continue;
        const int init = SessionRng::global().rollDie(20) + m_foes[static_cast<std::size_t>(i)]->getTier();
        m_turnOrder.push_back({true, i, init});
    }
    std::sort(m_turnOrder.begin(), m_turnOrder.end(), [](const TurnEntity& a, const TurnEntity& b) {
        if (a.initiative != b.initiative) return a.initiative > b.initiative;
        return !a.isMonster && b.isMonster;
    });
    m_currentTurnIdx = 0;
}

void CombatState::addLog(const std::string& message) {
    m_combatLog.push_back(message);
    if (m_combatLog.size() > 16) m_combatLog.erase(m_combatLog.begin());
}

void CombatState::selectFirstLivingFoe() {
    for (int i = 0; i < static_cast<int>(m_foes.size()); ++i) {
        if (m_foes[static_cast<std::size_t>(i)] && !m_foes[static_cast<std::size_t>(i)]->isDead()) {
            m_selectedTargetIdx = i;
            return;
        }
    }
    m_selectedTargetIdx = 0;
}

sf::Vector2f CombatState::monsterScreenCenter(std::size_t idx) const {
    const std::size_t n = std::max<std::size_t>(1, m_foes.size());
    const float span = n == 1 ? 0.0f : (n == 2 ? 360.0f : 310.0f);
    const float cx = Game::LOGICAL_WIDTH * 0.5f +
                     (static_cast<float>(idx) - static_cast<float>(n - 1) * 0.5f) * span;
    return {cx, 305.0f};
}

sf::Vector2f CombatState::partyCardCenter(std::size_t idx) const {
    return {14.0f + static_cast<float>(idx) * 204.0f + 98.0f, 646.0f};
}

void CombatState::startCurrentTurn() {
    if (m_combatEnded || m_turnOrder.empty()) return;
    m_subMenu = SubMenu::NONE;
    m_rowSwappedThisTurn = false;

    for (int tries = 0; tries < static_cast<int>(m_turnOrder.size()); ++tries) {
        const auto& cur = m_turnOrder[static_cast<std::size_t>(m_currentTurnIdx)];
        if (cur.isMonster) {
            auto mon = m_foes[static_cast<std::size_t>(cur.index)];
            if (mon && !mon->isDead()) {
                m_monsterActionDelay = 0.52f;
                return;
            }
        } else {
            auto hero = m_game.getParty().getMember(cur.index);
            if (hero && !hero->isDead()) {
                std::vector<std::string> logs;
                hero->processTurnEffects(logs);
                for (const auto& l : logs) addLog(l);
                if (hero->isDead()) {
                    checkCombatResolution();
                    if (m_combatEnded) return;
                } else if (hero->isParalyzed()) {
                    addLog(LocalizationManager::getInstance().format("STATUS_LOG_PARALYZED", {{"target", hero->getName()}}));
                    hero->advanceCombatBuffDurations(logs);
                } else {
                    m_selectedAllyIdx = cur.index;
                    selectFirstLivingFoe();
                    return;
                }
            }
        }
        m_currentTurnIdx = (m_currentTurnIdx + 1) % static_cast<int>(m_turnOrder.size());
    }
    checkCombatResolution();
}

void CombatState::nextTurn() {
    checkCombatResolution();
    if (m_combatEnded || m_turnOrder.empty()) return;

    const auto& cur = m_turnOrder[static_cast<std::size_t>(m_currentTurnIdx)];
    if (!cur.isMonster) {
        if (auto hero = m_game.getParty().getMember(cur.index)) {
            std::vector<std::string> logs;
            hero->advanceCombatBuffDurations(logs);
        }
    }
    m_currentTurnIdx = (m_currentTurnIdx + 1) % static_cast<int>(m_turnOrder.size());
    startCurrentTurn();
}

void CombatState::handleInput(const sf::Event& event) {
    if (m_combatEnded) {
        if ((event.type == sf::Event::KeyPressed &&
             (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space)) ||
            (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)) {
            if (m_victoryResult) handleVictory();
            else handleDefeat();
        }
        return;
    }

    if (m_turnOrder.empty()) return;
    const auto& cur = m_turnOrder[static_cast<std::size_t>(m_currentTurnIdx)];
    if (cur.isMonster) return;
    auto actor = m_game.getParty().getMember(cur.index);
    if (!actor || actor->isDead()) return;

    // 마우스 클릭 처리
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const auto pos = m_game.mapPixelToLogical({event.mouseButton.x, event.mouseButton.y});

        // 적 스프라이트 클릭 시 타겟 전환
        for (std::size_t i = 0; i < m_foeRects.size() && i < m_foes.size(); ++i) {
            if (!m_foes[i]->isDead() && m_foeRects[i].contains(pos)) {
                m_selectedTargetIdx = static_cast<int>(i);
                AudioManager::getInstance().playSfx("sfx_ui_hover");
                return;
            }
        }

        // 아군 초상화 클릭 시 (아군 대상 지정 모드면 즉시 실행!)
        if (m_subMenu == SubMenu::SKILL_ALLY_TARGET || m_subMenu == SubMenu::ITEM_TARGET) {
            for (int i = 0; i < m_game.getParty().getMemberCount() && i < 4; ++i) {
                if (m_partyRects[static_cast<std::size_t>(i)].contains(pos)) {
                    m_selectedAllyIdx = i;
                    if (m_subMenu == SubMenu::SKILL_ALLY_TARGET) executePendingAllySkill();
                    else executeSelectedItem(m_pendingItemInvIndex);
                    return;
                }
            }
        }

        // 서브 메뉴 항목 클릭
        if (m_subMenu == SubMenu::SKILL_LIST) {
            for (std::size_t i = 0; i < m_subItemRects.size(); ++i) {
                if (m_subItemRects[i].contains(pos)) {
                    executeSelectedSkill(static_cast<int>(i));
                    return;
                }
            }
        } else if (m_subMenu == SubMenu::ITEM_LIST) {
            const auto indices = getUsableConsumableIndices();
            for (std::size_t i = 0; i < m_subItemRects.size() && i < indices.size(); ++i) {
                if (m_subItemRects[i].contains(pos)) {
                    m_pendingItemInvIndex = indices[i];
                    auto cons = std::dynamic_pointer_cast<ConsumableItem>(
                        m_game.getParty().getInventory()[static_cast<std::size_t>(m_pendingItemInvIndex)]);
                    if (cons && cons->isOffensiveBomb()) {
                        executeSelectedItem(m_pendingItemInvIndex);
                    } else {
                        m_subMenu = SubMenu::ITEM_TARGET;
                        AudioManager::getInstance().playSfx("sfx_ui_click");
                    }
                    return;
                }
            }
        }

        // 6개 메인 커맨드 버튼 클릭
        if (m_cmdRects[0].contains(pos)) { m_subMenu = SubMenu::NONE; executeBasicAttack(*actor); return; }
        if (m_cmdRects[1].contains(pos)) { m_subMenu = SubMenu::SKILL_LIST; m_subSelection = 0; AudioManager::getInstance().playSfx("sfx_ui_click"); return; }
        if (m_cmdRects[2].contains(pos)) { m_subMenu = SubMenu::NONE; executeGuard(*actor); return; }
        if (m_cmdRects[3].contains(pos)) { m_subMenu = SubMenu::NONE; executeRowSwap(*actor); return; }
        if (m_cmdRects[4].contains(pos)) { m_subMenu = SubMenu::ITEM_LIST; m_subSelection = 0; AudioManager::getInstance().playSfx("sfx_ui_click"); return; }
        if (m_cmdRects[5].contains(pos)) { m_subMenu = SubMenu::NONE; attemptFlee(); return; }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
        if (m_subMenu != SubMenu::NONE) {
            m_subMenu = SubMenu::NONE;
            AudioManager::getInstance().playSfx("sfx_ui_click");
        }
        return;
    }

    if (event.type != sf::Event::KeyPressed) return;
    const auto key = event.key.code;

    if (key == sf::Keyboard::O) {
        m_game.getStates().pushState(std::make_unique<SettingsState>(m_game));
        return;
    }
    if (key == sf::Keyboard::Escape) {
        if (m_subMenu != SubMenu::NONE) {
            m_subMenu = SubMenu::NONE;
            AudioManager::getInstance().playSfx("sfx_ui_click");
        }
        return;
    }

    if (m_subMenu == SubMenu::SKILL_LIST) {
        const auto& skills = actor->getSkills();
        if (key == sf::Keyboard::Up && !skills.empty()) m_subSelection = (m_subSelection + static_cast<int>(skills.size()) - 1) % static_cast<int>(skills.size());
        else if (key == sf::Keyboard::Down && !skills.empty()) m_subSelection = (m_subSelection + 1) % static_cast<int>(skills.size());
        else if (key == sf::Keyboard::Enter && !skills.empty()) executeSelectedSkill(m_subSelection);
        else if (key >= sf::Keyboard::Num1 && key <= sf::Keyboard::Num9) {
            const int idx = key - sf::Keyboard::Num1;
            if (idx < static_cast<int>(skills.size())) executeSelectedSkill(idx);
        }
        return;
    }

    if (m_subMenu == SubMenu::SKILL_ALLY_TARGET || m_subMenu == SubMenu::ITEM_TARGET) {
        const int mc = m_game.getParty().getMemberCount();
        if (mc <= 0) return;
        if (key == sf::Keyboard::Left || key == sf::Keyboard::Up) m_selectedAllyIdx = (m_selectedAllyIdx + mc - 1) % mc;
        else if (key == sf::Keyboard::Right || key == sf::Keyboard::Down) m_selectedAllyIdx = (m_selectedAllyIdx + 1) % mc;
        else if (key >= sf::Keyboard::Num1 && key <= sf::Keyboard::Num4) {
            const int r = key - sf::Keyboard::Num1;
            if (r < mc) m_selectedAllyIdx = r;
        } else if (key == sf::Keyboard::Enter) {
            if (m_subMenu == SubMenu::SKILL_ALLY_TARGET) executePendingAllySkill();
            else executeSelectedItem(m_pendingItemInvIndex);
        }
        return;
    }

    if (m_subMenu == SubMenu::ITEM_LIST) {
        const auto indices = getUsableConsumableIndices();
        if (indices.empty()) { m_subMenu = SubMenu::NONE; return; }
        if (key == sf::Keyboard::Up) m_subSelection = (m_subSelection + static_cast<int>(indices.size()) - 1) % static_cast<int>(indices.size());
        else if (key == sf::Keyboard::Down) m_subSelection = (m_subSelection + 1) % static_cast<int>(indices.size());
        else if (key == sf::Keyboard::Enter || (key >= sf::Keyboard::Num1 && key <= sf::Keyboard::Num9)) {
            int pick = m_subSelection;
            if (key >= sf::Keyboard::Num1 && key <= sf::Keyboard::Num9) pick = key - sf::Keyboard::Num1;
            if (pick >= 0 && pick < static_cast<int>(indices.size())) {
                m_pendingItemInvIndex = indices[static_cast<std::size_t>(pick)];
                auto cons = std::dynamic_pointer_cast<ConsumableItem>(
                    m_game.getParty().getInventory()[static_cast<std::size_t>(m_pendingItemInvIndex)]);
                if (cons && cons->isOffensiveBomb()) executeSelectedItem(m_pendingItemInvIndex);
                else m_subMenu = SubMenu::ITEM_TARGET;
            }
        }
        return;
    }

    // 기본 전투 화면 키 조작
    if (key == sf::Keyboard::Left || key == sf::Keyboard::A) {
        for (int s = 1; s <= static_cast<int>(m_foes.size()); ++s) {
            const int cand = (m_selectedTargetIdx - s + static_cast<int>(m_foes.size())) % static_cast<int>(m_foes.size());
            if (!m_foes[static_cast<std::size_t>(cand)]->isDead()) { m_selectedTargetIdx = cand; break; }
        }
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (key == sf::Keyboard::Right || key == sf::Keyboard::D) {
        for (int s = 1; s <= static_cast<int>(m_foes.size()); ++s) {
            const int cand = (m_selectedTargetIdx + s) % static_cast<int>(m_foes.size());
            if (!m_foes[static_cast<std::size_t>(cand)]->isDead()) { m_selectedTargetIdx = cand; break; }
        }
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (key == sf::Keyboard::Num1 || key == sf::Keyboard::Enter) {
        executeBasicAttack(*actor);
    } else if (key == sf::Keyboard::Num2) {
        m_subMenu = SubMenu::SKILL_LIST; m_subSelection = 0;
        AudioManager::getInstance().playSfx("sfx_ui_click");
    } else if (key == sf::Keyboard::Num3 || key == sf::Keyboard::G) {
        executeGuard(*actor);
    } else if (key == sf::Keyboard::Num4 || key == sf::Keyboard::F) {
        executeRowSwap(*actor);
    } else if (key == sf::Keyboard::Num5) {
        m_subMenu = SubMenu::ITEM_LIST; m_subSelection = 0;
        AudioManager::getInstance().playSfx("sfx_ui_click");
    } else if (key == sf::Keyboard::Num6) {
        attemptFlee();
    }
}

void CombatState::update(sf::Time deltaTime) {
    const float dt = deltaTime.asSeconds();
    m_time += dt;

    if (m_hitStopTimer > 0.0f) {
        m_hitStopTimer = std::max(0.0f, m_hitStopTimer - dt);
        m_vfx.update(dt * 0.25f);
        return;
    }

    m_vfx.update(dt);

    for (std::size_t i = 0; i < m_foes.size(); ++i) {
        if (m_foeHitFlash[i] > 0.0f) m_foeHitFlash[i] = std::max(0.0f, m_foeHitFlash[i] - dt * 3.5f);
        if (m_foeLunge[i] > 0.0f) m_foeLunge[i] = std::max(0.0f, m_foeLunge[i] - dt * 3.2f);
        const float targetR = static_cast<float>(m_foes[i]->getHp()) / static_cast<float>(std::max(1, m_foes[i]->getMaxHp()));
        if (m_foeHpTrail[i] > targetR) m_foeHpTrail[i] = std::max(targetR, m_foeHpTrail[i] - dt * 0.55f);
        else m_foeHpTrail[i] = targetR;
    }

    for (int i = 0; i < m_game.getParty().getMemberCount() && i < 4; ++i) {
        const auto m = m_game.getParty().getMember(i);
        const float targetR = m ? static_cast<float>(m->getHp()) / static_cast<float>(std::max(1, m->getMaxHp())) : 0.0f;
        float& tr = m_partyHpTrail[static_cast<std::size_t>(i)];
        if (tr > targetR) tr = std::max(targetR, tr - dt * 0.55f);
        else tr = targetR;
    }

    if (!m_combatEnded && !m_turnOrder.empty()) {
        const auto& cur = m_turnOrder[static_cast<std::size_t>(m_currentTurnIdx)];
        if (cur.isMonster) {
            m_monsterActionDelay -= dt;
            if (m_monsterActionDelay <= 0.0f) {
                auto mon = m_foes[static_cast<std::size_t>(cur.index)];
                if (mon && !mon->isDead()) {
                    executeMonsterTurn(*mon, cur.index);
                }
                nextTurn();
            }
        }
    }
}

void CombatState::draw(sf::RenderWindow& window) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    const auto& lm = LocalizationManager::getInstance();
    const auto& party = m_game.getParty();
    const auto mouse = m_game.getMouseLogicalPosition();
    const auto shake = m_vfx.shakeOffset();

    // 1. 현재 층 3D 바이옴 던전 복도 배경 렌더링
    const int floorNum = std::clamp(m_encounter.floorNumber, 1, DungeonWorld::FLOOR_COUNT);
    if (party.getWorld().isGenerated()) {
        m_backdropRenderer->renderCombatBackdrop(
            window, party.getWorld().getFloor(floorNum), m_time, m_vfx.damageFlash());
    } else {
        UiSystem::drawBackdrop(window, assets, "title_bg", 130);
    }

    // 2. 최상단 턴 오더 타임라인 리본
    UiSystem::drawPanel(window, assets, {180.0f, 8.0f, 920.0f, 44.0f}, false, 224);
    UiSystem::drawText(window, font, "TURN:", {196.0f, 20.0f}, 13, UiTheme::textGold(), true);
    float tx = 254.0f;
    for (int step = 0; step < 7 && !m_turnOrder.empty(); ++step) {
        const int idx = (m_currentTurnIdx + step) % static_cast<int>(m_turnOrder.size());
        const auto& te = m_turnOrder[static_cast<std::size_t>(idx)];
        std::string nm;
        bool alive = false;
        if (te.isMonster && te.index < static_cast<int>(m_foes.size())) {
            alive = !m_foes[static_cast<std::size_t>(te.index)]->isDead();
            nm = m_foes[static_cast<std::size_t>(te.index)]->getName();
        } else if (!te.isMonster && te.index < party.getMemberCount()) {
            const auto m = party.getMember(te.index);
            alive = m && !m->isDead();
            if (m) nm = m->getName();
        }
        if (!alive) continue;
        const sf::FloatRect chip(tx, 14.0f, 112.0f, 30.0f);
        sf::RectangleShape cbg({chip.width, chip.height});
        cbg.setPosition(chip.left, chip.top);
        cbg.setFillColor(step == 0 ? sf::Color(86, 66, 26, 235)
                                   : (te.isMonster ? sf::Color(58, 22, 24, 215) : sf::Color(22, 42, 64, 215)));
        cbg.setOutlineThickness(step == 0 ? 2.0f : 1.0f);
        cbg.setOutlineColor(step == 0 ? UiTheme::borderGold()
                                      : (te.isMonster ? UiTheme::hpCrimson() : UiTheme::spAzure()));
        window.draw(cbg);
        UiSystem::drawCenteredText(window, font, nm, chip, 12,
                                   step == 0 ? UiTheme::textGold() : UiTheme::textPrimary(), false);
        tx += 118.0f;
        if (tx + 112.0f > 1088.0f) break;
    }

    // 3. 중앙 회화풍 몬스터 스테이지 (호흡·돌진·피격·인텐트·스태거·약점 표시)
    m_foeRects.resize(m_foes.size());
    const auto& bestiary = party.getBestiary();
    for (std::size_t i = 0; i < m_foes.size(); ++i) {
        const auto& mon = m_foes[i];
        const auto baseCenter = monsterScreenCenter(i) + shake;
        const float sprSize = mon->isBoss() ? 310.0f : 245.0f;
        const float breathY = std::sin(m_time * 2.4f + static_cast<float>(i) * 1.5f) * 5.0f;
        const float lungeY = m_foeLunge[i] * 48.0f;
        const sf::FloatRect rect(baseCenter.x - sprSize * 0.5f, baseCenter.y - sprSize * 0.5f + breathY + lungeY,
                                 sprSize, sprSize);
        m_foeRects[i] = rect;

        if (mon->isDead()) continue;

        const bool targeted = (static_cast<int>(i) == m_selectedTargetIdx);
        // 선택 타겟 황금 마법진 하이라이트
        if (targeted) {
            sf::CircleShape ring(sprSize * 0.36f);
            ring.setScale(1.25f, 0.32f);
            ring.setOrigin(ring.getRadius(), ring.getRadius());
            ring.setPosition(baseCenter.x, baseCenter.y + sprSize * 0.42f);
            ring.setFillColor(sf::Color(212, 175, 95, 45));
            ring.setOutlineThickness(2.5f);
            ring.setOutlineColor(UiTheme::borderGold());
            window.draw(ring);
        }

        // 2페이즈 보스 심홍 글로우
        if (mon->getPhase() > 1) {
            if (const sf::Texture* glow = assets.getUiTexture("particle_glow")) {
                sf::Sprite gsp(*glow);
                const auto gsz = glow->getSize();
                gsp.setOrigin(static_cast<float>(gsz.x) * 0.5f, static_cast<float>(gsz.y) * 0.5f);
                const float sc = (sprSize * 1.35f) / static_cast<float>(gsz.x);
                gsp.setScale(sc, sc);
                gsp.setPosition(baseCenter.x, baseCenter.y + breathY);
                gsp.setColor(sf::Color(255, 68, 28, 120));
                window.draw(gsp, sf::BlendAdd);
            }
        }

        if (const sf::Texture* tex = assets.getMonsterTexture(mon->getId())) {
            sf::Sprite sp(*tex);
            const auto sz = tex->getSize();
            sp.setScale(sprSize / static_cast<float>(sz.x), sprSize / static_cast<float>(sz.y));
            sp.setPosition(rect.left, rect.top);
            if (m_foeHitFlash[i] > 0.05f) {
                sp.setColor(sf::Color(255, 145, 145));
            } else if (mon->isBroken()) {
                sp.setColor(sf::Color(245, 180, 235));
            }
            window.draw(sp);
        }

        // 몬스터 머리 위 HUD: 인텐트 예고 배지 + 이름 + HP 바 + 스태거 핍 + 약점 배지
        const float hudW = 248.0f;
        const float hudX = baseCenter.x - hudW * 0.5f;
        const float hudY = 66.0f;
        UiSystem::drawPanel(window, assets, {hudX, hudY, hudW, 108.0f}, targeted || mon->isBoss(), 224);

        // 인텐트 텔레그래프 배지
        const auto& intent = mon->getNextIntent();
        std::string intentTxt = lm.get(intent.labelKey);
        if (intent.estimatedDamage > 0) intentTxt += " ~" + std::to_string(intent.estimatedDamage);
        if (mon->isBroken()) intentTxt = "BROKEN!";
        UiSystem::drawBadge(window, font, {hudX + 10.0f, hudY + 8.0f}, intentTxt,
                            sf::Color(24, 16, 18),
                            mon->isBroken() ? UiTheme::breakMagenta() : UiTheme::intentColor(intent.kind), 11);

        const std::string mTitle = mon->getName() + (mon->getPhase() > 1 ? " [II]" : "");
        UiSystem::drawText(window, font, mTitle, {hudX + 10.0f, hudY + 30.0f}, 14,
                           mon->isBoss() ? UiTheme::textGold() : UiTheme::textPrimary(), true, hudW - 20.0f);

        const float hpR = static_cast<float>(mon->getHp()) / static_cast<float>(std::max(1, mon->getMaxHp()));
        UiSystem::drawGaugeBar(window, font, {hudX + 10.0f, hudY + 51.0f, hudW - 20.0f, 16.0f},
                               hpR, m_foeHpTrail[i], UiTheme::hpCrimson(), UiTheme::hpTrail(),
                               std::to_string(mon->getHp()) + "/" + std::to_string(mon->getMaxHp()));

        // 스태거 핍 (노란 방패 블록들)
        const int maxStg = mon->getMaxStagger();
        const int curStg = mon->getStagger();
        const float pipW = std::min(22.0f, (hudW - 24.0f - static_cast<float>(maxStg - 1) * 4.0f) / static_cast<float>(maxStg));
        for (int p = 0; p < maxStg; ++p) {
            sf::RectangleShape pip({pipW, 8.0f});
            pip.setPosition(hudX + 10.0f + static_cast<float>(p) * (pipW + 4.0f), hudY + 72.0f);
            pip.setFillColor(mon->isBroken() ? UiTheme::breakMagenta()
                                             : (p < curStg ? UiTheme::staggerAmber() : sf::Color(44, 42, 48)));
            window.draw(pip);
        }

        // 약점 배지 (도감 해금 시 표시, 미해금 시 ???)
        float wx = hudX + 10.0f;
        const bool known = bestiary.getEntry(mon->getId()).weaknessRevealed;
        if (known) {
            for (const auto w : mon->getWeaknesses()) {
                if (wx + 38.0f > hudX + hudW - 6.0f) break;
                UiSystem::drawBadge(window, font, {wx, hudY + 85.0f}, UiTheme::elementShortCode(w),
                                    sf::Color(20, 18, 22), UiTheme::elementColor(w), 10);
                wx += 38.0f;
            }
        } else {
            UiSystem::drawText(window, font, "WEAK: ???", {wx, hudY + 86.0f}, 10, UiTheme::textMuted(), false);
        }
    }

    // 4. 전투 파티클 VFX 및 플로팅 데미지
    m_vfx.draw(window, assets, font);

    // 5. 하단 전투 독: 좌측 4인 초상화 + 중앙 전술 커맨드 6버튼 + 우측 전투 로그
    sf::RectangleShape dockBg({Game::LOGICAL_WIDTH, 152.0f});
    dockBg.setPosition(0.0f, 568.0f);
    dockBg.setFillColor(sf::Color(11, 12, 18, 244));
    window.draw(dockBg);
    sf::RectangleShape topEdge({Game::LOGICAL_WIDTH, 2.0f});
    topEdge.setPosition(0.0f, 568.0f);
    topEdge.setFillColor(UiTheme::borderBronze());
    window.draw(topEdge);

    const auto snap = buildPartyHudSnapshot(party);
    const auto& slots = snap.slots();
    const int activeActorIdx = (!m_turnOrder.empty() && !m_turnOrder[static_cast<std::size_t>(m_currentTurnIdx)].isMonster)
                                   ? m_turnOrder[static_cast<std::size_t>(m_currentTurnIdx)].index
                                   : -1;
    const bool pickingAlly = (m_subMenu == SubMenu::SKILL_ALLY_TARGET || m_subMenu == SubMenu::ITEM_TARGET);

    for (std::size_t i = 0; i < 4; ++i) {
        const sf::FloatRect r(12.0f + static_cast<float>(i) * 198.0f, 580.0f, 192.0f, 128.0f);
        m_partyRects[i] = r;
        if (i < slots.size()) {
            const bool active = (static_cast<int>(i) == activeActorIdx);
            const bool selAlly = pickingAlly && (static_cast<int>(i) == m_selectedAllyIdx);
            UiSystem::drawPortraitCard(window, assets, font, slots[i], r,
                                       active, r.contains(mouse), selAlly, m_partyHpTrail[i]);
        }
    }

    // 중앙 6개 커맨드 버튼 (2열 x 3행)
    const std::array<std::pair<std::string, std::string>, 6> cmds = {{
        {lm.get("COMBAT_CMD_ATTACK"), "1"},
        {lm.get("COMBAT_CMD_SKILL"),  "2"},
        {lm.get("COMBAT_CMD_GUARD"),  "3"},
        {lm.get("COMBAT_CMD_ROW"),    "4"},
        {lm.get("COMBAT_CMD_ITEM"),   "5"},
        {lm.get("COMBAT_CMD_FLEE"),   "6"}
    }};
    const bool playerTurn = (activeActorIdx >= 0 && !m_combatEnded);
    for (int i = 0; i < 6; ++i) {
        const float cx = 812.0f + static_cast<float>(i % 2) * 124.0f;
        const float cy = 580.0f + static_cast<float>(i / 2) * 44.0f;
        const sf::FloatRect r(cx, cy, 118.0f, 38.0f);
        m_cmdRects[static_cast<std::size_t>(i)] = r;
        const bool enabled = playerTurn && !(i == 5 && m_encounter.bossBattle) && !(i == 3 && m_rowSwappedThisTurn);
        UiSystem::drawButton(window, assets, font, {
            r, cmds[static_cast<std::size_t>(i)].first, cmds[static_cast<std::size_t>(i)].second,
            false, r.contains(mouse), enabled, i == 0 && enabled
        });
    }

    // 우측 실시간 전투 로그 (자동 줄바꿈 지원)
    const sf::FloatRect logR(1062.0f, 580.0f, 208.0f, 128.0f);
    UiSystem::drawPanel(window, assets, logR, false, 224);
    std::vector<std::pair<std::string, bool>> wrappedLogs;
    const int startLog = std::max(0, static_cast<int>(m_combatLog.size()) - 6);
    for (int i = startLog; i < static_cast<int>(m_combatLog.size()); ++i) {
        const bool isLatest = (i == static_cast<int>(m_combatLog.size()) - 1);
        const auto lines = UiSystem::wrapUtf8Lines(m_combatLog[static_cast<std::size_t>(i)], font, 11, logR.width - 16.0f);
        for (const auto& l : lines) wrappedLogs.emplace_back(l, isLatest);
    }
    const int maxVisibleLines = 7;
    const int firstLine = std::max(0, static_cast<int>(wrappedLogs.size()) - maxVisibleLines);
    float ly = logR.top + 8.0f;
    for (int i = firstLine; i < static_cast<int>(wrappedLogs.size()); ++i) {
        if (ly + 15.0f > logR.top + logR.height - 4.0f) break;
        UiSystem::drawText(window, font, wrappedLogs[static_cast<std::size_t>(i)].first,
                           {logR.left + 8.0f, ly}, 11,
                           wrappedLogs[static_cast<std::size_t>(i)].second ? UiTheme::textPrimary() : UiTheme::textMuted(),
                           false);
        ly += 16.0f;
    }

    // 6. 스킬/아이템 선택 오버레이
    m_subItemRects.clear();
    if (playerTurn && (m_subMenu == SubMenu::SKILL_LIST || m_subMenu == SubMenu::ITEM_LIST)) {
        const auto actor = party.getMember(activeActorIdx);
        const sf::FloatRect ov(330.0f, 200.0f, 620.0f, 340.0f);
        UiSystem::drawPanel(window, assets, ov, true, 246);
        UiSystem::drawText(window, font,
                           lm.get(m_subMenu == SubMenu::SKILL_LIST ? "COMBAT_SELECT_SKILL" : "COMBAT_SELECT_ITEM"),
                           {ov.left + 18.0f, ov.top + 14.0f}, 18, UiTheme::textGold(), true);

        if (m_subMenu == SubMenu::SKILL_LIST && actor) {
            const auto& skills = actor->getSkills();
            for (std::size_t i = 0; i < skills.size(); ++i) {
                const auto& sk = skills[i];
                const sf::FloatRect r(ov.left + 16.0f, ov.top + 52.0f + static_cast<float>(i) * 44.0f,
                                      ov.width - 32.0f, 38.0f);
                m_subItemRects.push_back(r);
                const bool canCast = !sk->isSpell() || actor->getSpellSlots() > 0;
                const std::string lbl = sk->getName() + (sk->isSpell() ? " [1 SP]" : "") +
                                        " — " + sk->getDescription();
                UiSystem::drawButton(window, assets, font, {
                    r, lbl, std::to_string(i + 1), m_subSelection == static_cast<int>(i),
                    r.contains(mouse), canCast, false
                });
            }
        } else if (m_subMenu == SubMenu::ITEM_LIST) {
            const auto indices = getUsableConsumableIndices();
            if (indices.empty()) {
                UiSystem::drawCenteredText(window, font, lm.get("COMBAT_NO_ITEMS"), ov, 15, UiTheme::textMuted());
            } else {
                const auto& inv = party.getInventory();
                for (std::size_t i = 0; i < indices.size() && i < 6; ++i) {
                    const auto& item = inv[static_cast<std::size_t>(indices[i])];
                    const sf::FloatRect r(ov.left + 16.0f, ov.top + 52.0f + static_cast<float>(i) * 44.0f,
                                          ov.width - 32.0f, 38.0f);
                    m_subItemRects.push_back(r);
                    UiSystem::drawButton(window, assets, font, {
                        r, item->getName() + " — " + item->getDescription(), std::to_string(i + 1),
                        m_subSelection == static_cast<int>(i), r.contains(mouse), true, false
                    });
                }
            }
        }
    } else if (pickingAlly) {
        UiSystem::drawPanel(window, assets, {390.0f, 504.0f, 500.0f, 44.0f}, true, 240);
        UiSystem::drawCenteredText(window, font, lm.get("COMBAT_SELECT_ALLY"),
                                   {390.0f, 504.0f, 500.0f, 44.0f}, 15, UiTheme::textGold(), true);
    }

    // 7. 전투 종결 배너
    if (m_combatEnded) {
        const sf::FloatRect banner(320.0f, 220.0f, 640.0f, 170.0f);
        UiSystem::drawPanel(window, assets, banner, true, 248);
        UiSystem::drawCenteredText(window, font,
                                   lm.get(m_victoryResult ? "COMBAT_VICTORY_BANNER" : "COMBAT_DEFEAT_BANNER"),
                                   {banner.left, banner.top + 26.0f, banner.width, 48.0f},
                                   30, m_victoryResult ? UiTheme::textGold() : UiTheme::hpCrimson(), true);
        UiSystem::drawCenteredText(window, font, lm.get("COMBAT_PRESS_CONTINUE"),
                                   {banner.left, banner.top + 104.0f, banner.width, 36.0f},
                                   15, UiTheme::textPrimary(), true);
    }
}

} // namespace crawl
