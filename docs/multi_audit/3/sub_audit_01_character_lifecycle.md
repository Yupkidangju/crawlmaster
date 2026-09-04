# Sub Audit Report

## 1. Audit Metadata

- Audit Turn: 3
- Perspective: Character lifecycle — domain, controller, UI, combat status, death/TPK, checkpoint and save serialization
- User Goal: 캐릭터 생성부터 모집·성장·장비·상태효과·전투·사망·TPK·GameOver·checkpoint 복구·직렬화까지의 전체 인과를 점검하고, 퀘스트 및 영속 맵 경계와 문서 정합성을 확인한다.
- Audit Basis: Standard-backed
- Standard Path: `/mnt/Projects_SSD/cpp/crawlmaster/AI_AUDIT_DOC_STANDARD.md`
- Report Contract: `/home/eunho1/.codex/skills/multi-audit/references/report-contract.md`
- Scope Baseline: HEAD `927753278f46b92a015197ee229edce4f52e0657` plus the current uncommitted working tree, including new character creation, quest journal, and `DungeonWorld` files.
- Write Boundary: Only this report was written. Source, tests, configuration, and product documents were not modified.

## 2. Assigned Scope

검사 범위는 다음 인과 경로다.

- `RecruitmentDraft` 및 `CharacterCreationState`의 identity 입력, Unicode/나이/성별/직업 검증, 4d6 reroll, 가중 10-point buy, confirm/cancel
- `Party`의 최대 4인/중복·null 불변식, 캐릭터 생성 장비·스킬·레벨업·HP/주문 슬롯
- 장비/아이템/스킬/버프/독·마비, 명시적 대상 선택, 사망자 행동 차단과 HUD/CharacterInfo 표시
- 일반/고정/최종 전투의 승리·개별 사망·전원 사망·TPK·GameOver 및 stale state 제거
- `Character`, `Quest`, `DungeonMap`, `DungeonWorld`, `Party`의 JSON v4 직렬화, v1~v3 migration, world/FOW/object state
- Town/Dungeon/Combat/Title의 checkpoint 저장·복원과 `SessionRng` seed/draw-count 연계
- 관련 명세/설계/구현요약/로드맵 및 CMake/현재 계약 테스트의 양방향 정합성

## 3. Excluded and Uninspected Scope

- `docs/audit/**`와 `docs/multi_audit/1`, `docs/multi_audit/2`, `docs/multi_audit/3`의 다른 감사보고서는 배정 지시에 따라 읽지 않았다. 다른 에이전트의 결론도 가정하지 않았다.
- 생성/vendor/reference 트리, `.git`, 기존 package/build 산출물의 소스 감사는 제외했다. 기존 build artifact는 현재 working tree의 증거로 승격하지 않았다.
- 현재 working tree를 다시 configure/build하지 않았다. 지시된 대로 새 build output 또는 tracked/untracked 제품 파일을 만들거나 바꾸지 않아 현재 13개 CTest의 실행 결과는 미확인이다.
- 실제 GUI 장시간 플레이, 실제 IME 조합 입력, clean Windows/macOS/high-DPI 실행은 이 하위 관점에서 실행하지 못했다.

## 4. Evidence Examined

### Documents

- `spec.md` — identity/point-buy/traits, combat/status, objective quest, 3-floor world, checkpoint/TPK/RNG, save schema v4, migration and gates
- `designs.md` — character creation, CharacterInfo status/buff display, destructive-action confirmation, HUD and FOW marker rules
- `IMPLEMENTATION_SUMMARY.md`, `DESIGN_DECISIONS.md`, `BUILD_GUIDE.md`, `README.md`, `CHANGELOG.md`, `LESSONS_LEARNED.md`, `audit_roadmap.md`
- `tasks/plan.md`, `tasks/todo.md`, `tests/fixtures/README.md`, `tests/fixtures/save_v1.json`

### Source and tests

- `src/model/Character.cpp`, `RecruitmentDraft.cpp`, `Party.cpp`, `Quest.cpp`, `DungeonMap.cpp`, `DungeonWorld.cpp`, `CombatRules.cpp`, `CombatActionRules.cpp`, `Skill.cpp`, `ItemFactory.cpp`, `MonsterFactory.cpp`
- `src/controller/CharacterCreationState.cpp`, `CharacterInfoState.cpp`, `CombatState.cpp`, `CombatStateActions.cpp`, `DungeonState.cpp`, `TownState.cpp`, `TitleState.cpp`, `GameOverState.cpp`, `VictoryState.cpp`
- `src/core/SessionRng.cpp`, `Persistence.cpp`, `Game.cpp`, `GameStateManager.cpp`
- `src/view/DungeonRenderer.cpp`, `PartyHudSnapshot.cpp`
- `tests/test_agency_contracts.cpp`, `test_combat_contracts.cpp`, `test_content_contracts.cpp`, `test_controller_contracts.cpp`, `test_hud_contracts.cpp`, `test_rng_process_replay.cpp`, `test_ui_state_raster.cpp`, `src/test_harness.cpp`
- `CMakeLists.txt` source list and CTest registration

### Commands and results

- `git rev-parse HEAD` → `927753278f46b92a015197ee229edce4f52e0657`.
- `git status --short` → the requested current working tree contains the new feature files and modified product/tests; no report existed before this report was created.
- `git diff --check` → passed with no output.
- `g++ -std=c++20 -fsyntax-only -Wall -Wextra -Werror -pedantic -Iinclude -Ibuild/_deps/sfml-src/include -Ibuild/_deps/json-src/include` on the scoped model sources → exit 0.
- The same syntax-only check on scoped controller sources → exit 0.
- `ctest --test-dir build --output-on-failure --no-tests=error` → `No tests were found!!!`; the existing build tree is stale/not configured for the current working tree, so this is not a current-source CTest verdict.
- `./build/TestHarness --run-all` → exit 134 in an older binary at a removed `DroidSansFallbackFull.ttf` assertion. It was treated as stale artifact evidence only, not as a current implementation result.

## 5. Findings

### [A01-F001] Character identity validation is incomplete and bypassable at the save boundary

- Pass: Implementation / Security
- Area: Character creation input, Unicode/control characters, JSON deserialization
- Severity: Major
- Status: Confirmed
- Summary: `RecruitmentDraft` validates UTF-8 structure and code-point count, but its control check only rejects C0 (`< U+0020`) and `U+007F`; Unicode C1 controls such as `U+0085` pass. More importantly, `Character::fromJson` does not call the validator and checks only non-empty byte length `<=64`, so a v3/v4 save can inject invalid UTF-8/control content or more than 16 code points.
- Evidence:
  - `spec.md:140-145` requires a trimmed Unicode 1–16-character name and forbids control characters.
  - `src/model/RecruitmentDraft.cpp:123-161` decodes UTF-8 and checks `codePoint < 0x20` or `codePoint == 0x7F`, but has no C1/Unicode control-category check. The two-byte sequence for `U+0085` therefore passes the shown condition.
  - `src/model/Character.cpp:478-497` checks `name.empty()` and `name.size() > 64` only; it never calls `RecruitmentDraft::isValidName` and does not count code points or reject controls.
  - `tests/test_agency_contracts.cpp:121-125` covers empty/ASCII leading/trailing/17-byte names, but no C1 control, malformed persisted name, or 17-code-point v4 member.
- Expected Basis: `spec.md:141`; the creation and persisted character contract must not disagree. Legacy v1/v2 unknown age/gender is separately allowed, but no legacy exception is documented for malformed names.
- Actual: UI-created names normally pass through the draft, while a hand-edited or corrupted v3/v4 save is accepted into `Character`, then rendered/logged by `CharacterInfoState`, Town, HUD, and Combat.
- Impact: Persisted identity can violate the creation contract, display as corrupted text, and make save validation/recovery inconsistent. The persisted object is accepted instead of being quarantined as corrupt.
- Suggested Action: Put one UTF-8 scalar/code-point validator at the identity boundary and use it in draft, public identity construction, and schema v3/v4 deserialization. Define the exact permitted Unicode control/format policy and add C1, malformed UTF-8, surrogate, 17-code-point, and round-trip tests while preserving only the documented v1/v2 identity exception.
- Re-audit Method: Feed `Party::loadFromFile` a v4 save containing `U+0085`, malformed byte sequences, and 17 ASCII code points; require `Corrupt` plus quarantine and unchanged in-memory party. Exercise the same cases through `CharacterCreationState` and verify accepted names save/load byte-identically.
- Confidence: High
- Notes: The current test contract rejects leading/trailing ASCII spaces rather than trimming them; that observed behavior should remain explicit when the shared validator is introduced.

### [A01-F002] Title New Game bypasses the required confirmation when no save exists

- Pass: Implementation
- Area: Destructive New Game control and cancel invariant
- Severity: Major
- Status: Confirmed
- Summary: New Game is confirmed only when `hasRecoverableSave()` finds a primary or `.bak` file. On a fresh/no-save path, Enter immediately starts a new session, resets the in-memory party/world, and writes a save.
- Evidence:
  - `spec.md:354` says New Game is the explicit reset boundary and must be confirmed.
  - `designs.md:331-345` defines `TitleState -> New Game -> Confirm(New/Cancel)` and says destructive actions must not mutate domain/save before Enter confirmation.
  - `src/controller/TitleState.cpp:52-66` calls `Party::hasRecoverableSave()` and directly executes `startNewGlobalSession()`/`startNewGame()` in the `else` branch when no file exists; only the file-present branch sets `m_confirmingNewGame`.
  - `src/model/Party.cpp:30-31` defines recoverable solely by filesystem existence, not by whether a currently held in-memory session will be reset.
- Expected Basis: `spec.md:354`, `designs.md:331-345`, and the cancel/confirmation contract in the user scope.
- Actual: With no primary/backup file, one Enter on the selected New Game menu is sufficient to replace the current party/world and attempt a save; there is no New/Cancel preview state.
- Impact: The UI contract is inconsistent between fresh and existing-save sessions, and a stale in-memory session can be reset without the promised confirmation. Cancel invariance cannot be tested for this path because no cancel step exists.
- Suggested Action: Always enter the confirmation state, including no-save sessions. Show a distinct “no existing save” preview if desired, but only call `startNewGlobalSession` and `startNewGame` after the confirm Enter.
- Re-audit Method: Use an empty isolated save directory, construct production `TitleState`, press New Game Enter once and assert it remains in confirmation with unchanged party/save bytes; press Escape and assert unchanged; then press Enter and assert Town plus a new v4 save.
- Confidence: High

### [A01-F003] TPK restores party data but not the global RNG checkpoint

- Pass: Debug / Implementation
- Area: TPK, GameOver, checkpoint recovery, session RNG
- Severity: Major
- Status: Confirmed
- Summary: TPK calls `Party::loadFromFile()` and replaces the state stack with `GameOverState`, but never reconstructs `SessionRng::global()` from the restored `lastSessionSeed/sessionRngDrawCount`. If the user closes the window on GameOver, `Game::processEvents()` saves again and `Party::saveToFile()` records the already-advanced combat RNG draw count, changing the last normal checkpoint.
- Evidence:
  - `spec.md:54-55` requires TPK to restore the last town checkpoint without deleting/overwriting the normal save; `spec.md:356` requires seed plus raw draw count to resume the same stream.
  - `src/controller/CombatState.cpp:367-373` loads the checkpoint and calls `replaceAll(GameOverState)`; there is no `SessionRng::global() = SessionRng(savedSeed, savedDrawCount)` or equivalent.
  - `src/controller/TitleState.cpp:68-85` performs RNG reconstruction only on a later Continue path, not during TPK restoration.
  - `src/core/Game.cpp:64-67` saves any active session on a window-close event, including while `GameOverState` is the current root.
  - `src/model/Party.cpp:102-105` overwrites `m_lastSessionSeed` and `m_sessionRngDrawCount` from the current global RNG on every save.
  - `tests/test_controller_contracts.cpp:143-165` checks root replacement and party HP restoration, while `test_controller_contracts.cpp:258-286` checks Continue replay separately; no TPK-then-close RNG test exists.
- Expected Basis: `spec.md:54`, `spec.md:356`, and the checkpoint/replay contract in `DESIGN_DECISIONS.md:148-153`.
- Actual: The party fields are restored, but the process-level stream remains at the post-combat position. A normal-close save from GameOver can therefore mutate the checkpoint metadata even though TPK is supposed to preserve it.
- Impact: Continue after TPK is not deterministic under a common close path, and the promised “TPK does not overwrite normal save” guarantee is false for RNG state.
- Suggested Action: Centralize checkpoint restoration (party data plus `SessionRng`) and call it before entering GameOver, or suppress automatic save while the GameOver root is showing. Preserve typed durability/error status rather than silently writing an advanced RNG count.
- Re-audit Method: Save a known seed/draw count, advance global RNG in a deterministic combat, trigger TPK, assert party and global seed/count equal the checkpoint, invoke the production close path, compare save bytes/draw count, and verify the next 16 draws in an independent process.
- Confidence: High

### [A01-F004] Temporary STR/DEX buffs and Bless survive a successful escape

- Pass: Implementation
- Area: Combat termination, temporary status lifecycle
- Severity: Major
- Status: Confirmed
- Summary: Temporary buffs are cleared on victory and rest, but the successful non-boss escape path pops `CombatState` directly without calling `clearCombatBuffs()`.
- Evidence:
  - `spec.md:171-172` requires STR/DEX potion buffs to last only during combat and Bless to be a combat attack-roll buff; `spec.md:200`/`designs.md:345-348` treat escape as a completed combat flow.
  - `src/model/Character.cpp:379-385` provides `clearCombatBuffs()` for STR, DEX, and Bless.
  - `src/controller/CombatState.cpp:349-363` clears buffs only after `checkVictory()`/`distributeRewards()`.
  - `src/controller/CombatState.cpp:531-555` returns from a successful escape by `m_game.getStates().popState()` and has no clear call.
  - `src/controller/CharacterInfoState.cpp:396-400` blocks drinking STR/DEX potions outside combat, but it cannot remove buffs leaked by the prior combat.
  - No controller test exercises potion/Bless, successful escape, and post-escape values together.
- Expected Basis: `spec.md:171-172`, `designs.md:291-302`, and the combat lifecycle contract.
- Actual: After a successful escape, `getStrBuffAmount()`, `getDexBuffAmount()`, and `getBlessTurns()` remain positive until rest or a later combat-victory cleanup. The next dungeon actions/encounter can benefit from them.
- Impact: Buff duration and combat balance are wrong; a player can intentionally escape to carry combat-only advantages into exploration or a new fight. Save/reload also disagrees with the live process because these fields are not serialized.
- Suggested Action: Make every combat terminal path (victory, successful escape, TPK/load failure handling, and any future abort) pass through one cleanup/termination routine. Add a lifecycle test for both STR/DEX and Bless.
- Re-audit Method: Apply each buff, force a deterministic non-boss escape, assert all temporary amounts/turns are zero before the DungeonState resumes, then close/reload and verify no live-vs-save discrepancy.
- Confidence: High

### [A01-F005] CharacterInfoState omits the required status and buff display

- Pass: Implementation
- Area: Character lifecycle UI observability
- Severity: Minor
- Status: Confirmed
- Summary: The Dungeon HUD exposes poison/paralysis/Bless, but the CharacterInfo sheet does not render the selected character’s status effects or STR/DEX/Bless temporary values in either normal or large layout.
- Evidence:
  - `designs.md:291-302` requires CharacterInfo to display current status, `[POISONED]`, `[PARALYZED]`, `[OK]`, and buff amounts; `spec.md:580` requires actual status visibility in the HUD/lifecycle UI.
  - `src/controller/CharacterInfoState.cpp:188-225` draws HP, XP, AC, spells, abilities and equipment without calling `getPoisonTurns()`, `getParalysisTurns()`, `getBlessTurns()`, `getStrBuffAmount()`, or `getDexBuffAmount()`.
  - `src/controller/CharacterInfoState.cpp:329-347` large layout likewise omits all status/buff values.
  - `src/view/DungeonRenderer.cpp:310-344` confirms status fields are available in the separate HUD path, so this is a CharacterInfo-specific omission rather than a missing domain field.
  - `tests/test_hud_contracts.cpp:27-39` tests snapshot fields, but `test_ui_state_raster.cpp:202-205` only captures CharacterInfo images and does not assert status/buff text.
- Expected Basis: `designs.md:291-302` and the UI/status requirement in `spec.md:166-172`.
- Actual: A poisoned/paralyzed/blessed character can appear in the CharacterInfo panel with only HP/stat/equipment information; the user cannot inspect the full state required to decide on cure or combat actions.
- Impact: Reduced gameplay observability and a document-to-UI drift. This also hides the escaped-buff defect in F004 from the most direct character screen.
- Suggested Action: Add localized status/buff rows to both CharacterInfo layouts, including a clear dead indicator, and add transcript/raster assertions for healthy, poisoned, paralyzed, buffed, and dead characters at the supported scales.
- Re-audit Method: Render a fixture containing each status/buff at normal and large scale in all locales, inspect text bounds, and assert that the values match the model snapshot.
- Confidence: High

### [A01-F006] Character deserialization accepts class/level-invalid spell-slot states

- Pass: Implementation
- Area: Character semantic validation and save/load
- Severity: Major
- Status: Confirmed
- Summary: `Character::fromJson` validates only numeric slot ranges, not the class/level invariant established by the constructor and level-up logic. A v4 Mage/Cleric may load with zero max slots, or a non-caster with arbitrary positive slots.
- Evidence:
  - `spec.md:150-153` requires Mage/Cleric level-1 slots to start at 2 and increase per level; non-casters have no spell slots.
  - `src/model/Character.cpp:44-47` initializes caster max/current slots to 2 and `src/model/Character.cpp:190-194` increments caster max by one per level.
  - `src/model/Character.cpp:486-487` checks only `0 <= spellSlots <= maxSpellSlots <= 100`, then `src/model/Character.cpp:505-510` blindly assigns the values.
  - `tests/test_harness.cpp:329-354` and agency fixtures cover broad ranges but do not reject a level-1 Mage with `maxSpellSlots:0` or a Warrior with positive slots.
- Expected Basis: `spec.md:150-153`, `spec.md:164-165`, and the constructor/level-up invariant in current source.
- Actual: A validly shaped, hand-edited v4 save can load a level-1 Mage unable to cast any spell, or load phantom slots on a Warrior/Rogue. It is then reserialized as canonical v4 without rejection.
- Impact: Class identity and progression are silently corrupted at the persistence boundary; a player can lose or gain a core resource without a migration/error signal.
- Suggested Action: Validate/derive expected max slots from class and level (`0` for Warrior/Rogue, `level + 1` for Mage/Cleric under the current formula), validate current slots against it, and add tampered-save tests. If variable slot caps are intended later, add an explicit contract instead of accepting arbitrary values.
- Re-audit Method: Mutate v4 fixtures for every class/level combination, require invalid combinations to return `Corrupt` without party mutation, and round-trip valid consumed-slot states.
- Confidence: High

### [A01-F007] World objective DISCOVERED state is not tied to an active quest or visited FOW

- Pass: Implementation
- Area: Objective lifecycle, world snapshot integrity, minimap information boundary
- Severity: Major
- Status: Confirmed
- Summary: The loader accepts a `DISCOVERED` world object even when its quest is inactive or its map cell is not visited. The renderer then draws a discovered marker based only on object state and quest acceptance, without checking current FOW.
- Evidence:
  - `spec.md:200-207` requires accepted objectives to remain hidden until discovered and persists `PRESENT|DISCOVERED|RESOLVED` with FOW.
  - `src/model/DungeonWorld.cpp:174-205` validates canonical ID/kind/floor/EMPTY position and uniqueness, but not state-to-quest or state-to-visited relationships.
  - `src/model/Party.cpp:304-325` rejects `RESOLVED` without active/completed quest and mismatched ready states, but has no equivalent rejection for `DISCOVERED` without an active quest or without `world.getFloor(floor).isVisited(x,y)`.
  - `src/controller/DungeonState.cpp:353-360` is the only normal path that changes PRESENT to DISCOVERED and requires `party.hasQuest()` plus `map().isVisited()`.
  - `src/view/DungeonRenderer.cpp:185-188` draws a marker for `DISCOVERED` plus `party.hasQuest()` and never checks `map.isVisited()`.
- Expected Basis: `spec.md:206-208`, `designs.md:58-60`, and `tasks/plan.md:21` (marker/FOW integrity).
- Actual: A tampered or stale v4 snapshot can contain `DISCOVERED` with no active quest, or with an unvisited cell. After the player later accepts the quest, the marker appears immediately in fog and reveals the objective location; a discovered inactive object is never downgraded by normal discovery logic.
- Impact: Hidden-objective/FOW contract is bypassed and stale state can become an orphaned causal path. The issue is user-visible even though the world’s canonical IDs remain valid.
- Suggested Action: Enforce `PRESENT` for inactive/uncompleted objectives, `DISCOVERED` only for an active quest and visited cell, and `RESOLVED` only for a ready/completed-consistent quest. Keep the renderer defensive by requiring current FOW before drawing a marker.
- Re-audit Method: Mutate a generated v4 world to each invalid state combination and require load quarantine; separately render a discovered/unvisited fixture and assert no marker is drawn until FOW is visited.
- Confidence: High

### [A01-F008] Dungeon snapshot validation omits landmark shape and BossGate-farthest invariants

- Pass: Implementation / Debug
- Area: Map generation, snapshot load, progression gate
- Severity: Major
- Status: Confirmed
- Summary: `DungeonMap::fromJson` checks dimensions, tile codes, wall boundary, connectivity, and U/V/B counts, but does not require one `DOOR` or require the `BOSS_GATE` to remain at the farthest reachable tile.
- Evidence:
  - `spec.md:174-180` defines the map tile model and projection; `spec.md:206` requires all reserved tiles to be reachable; `spec.md:573-574` ties the vertical slice to a landmark and farthest boss gate.
  - `src/model/DungeonMap.cpp:335-363` generated maps place exactly one Door and put the DOWNSTAIRS/BOSS_GATE at the farthest tile.
  - `src/model/DungeonMap.cpp:409-452` counts only `UPSTAIRS`, `DOWNSTAIRS`, and `BOSS_GATE`; no Door count is maintained, and no comparison of the BOSS_GATE coordinate with `m_bossDistance` is made.
  - `src/model/DungeonMap.cpp:461-467` checks only that every walkable tile is connected to `(1,1)`.
  - `tests/test_content_contracts.cpp:223-243` checks generated-map Door/BossGate placement, while `test_content_contracts.cpp:374-385` only mutates a tile row and does not test landmark tampering.
- Expected Basis: generated-map invariants in current source plus `spec.md:178`, `spec.md:206`, and the vertical-slice gate in `spec.md:573`.
- Actual: A v4 snapshot with the Door removed/duplicated or the B tile moved to an earlier reachable coordinate can load as valid. A moved gate can trigger the final boss before the intended farthest progression point; a missing Door changes the expected landmark topology.
- Impact: Corrupt or edited saves can bypass the exploration/progression gate while still passing load, and save/reload does not repair the invalid topology.
- Suggested Action: Validate exact per-floor landmark counts and compare the final gate’s distance to the recomputed farthest distance; reject noncanonical placement before mutating `Party`. Add generated-snapshot mutation tests for Door, stairs, and gate coordinates.
- Re-audit Method: Serialize a generated world, move/remove Door and move BOSS_GATE to a nearer connected EMPTY tile, call `DungeonWorld::fromJson`, and require `Corrupt`/quarantine with unchanged prior party.
- Confidence: High

### [A01-F009] Seedless v1/v2 migration is nondeterministic and can bind world seed to a different session seed

- Pass: Implementation / Debug
- Area: Legacy migration, world generation, RNG checkpoint metadata
- Severity: Major
- Status: Needs Clarification
- Summary: The current specification claims v1–v3 migration from a saved session seed, but the supplied v1 fixture has no seed. The implementation falls back to the process-global entropy seed, then `TitleState` starts another entropy session before saving the migration, so the generated world seed and persisted `lastSessionSeed` can differ.
- Evidence:
  - `spec.md:345-347` says v1–v3 migration preserves progress and deterministically generates the 3-floor world from the saved session seed.
  - `tests/fixtures/save_v1.json:1-9` contains no `schemaVersion`, `lastSessionSeed`, or `sessionRngDrawCount`.
  - `src/model/Party.cpp:286-301` defaults missing legacy seed to `0`, then uses `SessionRng::global().seed()` for world generation.
  - `src/core/SessionRng.cpp:26-29` initializes the global stream from `random_device` entropy, so independent processes use different fallback seeds.
  - `src/controller/TitleState.cpp:68-88` sees `savedSeed == 0`, starts a new global session, and saves; the already generated world retains the old fallback seed while `Party::saveToFile()` records the new global seed.
  - `tests/test_content_contracts.cpp:314-349` checks deterministic v3 migration only when a nonzero `lastSessionSeed` is present; `test_rng_process_replay.cpp:23-67` covers a v4 checkpoint, not seedless v1/v2.
- Expected Basis: `spec.md:345-356` and the migration/replay contract. The legacy fixture and the claimed requirement conflict, so the exact seedless policy must be decided by the authority owner.
- Actual: Two independent processes loading the same seedless v1 fixture can generate different worlds. On Continue migration, the v4 metadata can record a session seed unrelated to the generated world snapshot.
- Impact: Legacy players cannot rely on deterministic migration or a truthful world/session seed relationship; replay and future migration evidence are not reproducible. This is a gate-blocking ambiguity, not merely a missing test.
- Suggested Action: Decide and document one policy: store a deterministic derivation/fixed migration seed for seedless v1/v2 and use it consistently as `lastSessionSeed`, or explicitly downgrade seedless migration from deterministic and prevent a false v4 claim. Add two-process v1/v2 migration tests and validate v4 `world.seed == lastSessionSeed` when that invariant is intended.
- Re-audit Method: Run the exact seedless fixture in two clean processes, compare world JSON and migrated metadata, then run Continue and verify the next RNG draws. Require either byte-identical deterministic output or an explicit documented non-deterministic status.
- Confidence: High for the observed behavior; expected resolution requires specification clarification.

### [A01-F010] Quest progress mutators accept negative counts and can manufacture a save that reloads as corrupt

- Pass: Implementation
- Area: Quest domain invariants and persistence
- Severity: Minor
- Status: Confirmed
- Summary: `Quest::updateProgress` and `setCurrentCount` accept negative values. A caller or future event path can lower progress below zero, and `toJson()` then writes it even though `Quest::fromJson()` rejects negative `currentCount`.
- Evidence:
  - `src/model/Quest.cpp:18-27` computes `m_currentCount = min(target, current + count)` and `m_currentCount = min(target, count)` without rejecting/clamping negative input.
  - `src/model/Party.cpp:503-524` forwards kill/collect updates without a nonnegative count guard.
  - `src/model/Quest.cpp:158-160` rejects a loaded `currentCount < 0`, creating an in-memory-to-save mismatch.
  - Existing tests use positive counts only (`tests/test_content_contracts.cpp:117-119`, `tests/test_harness.cpp:690-723`).
- Expected Basis: quest progress is a bounded `0..targetCount` state under `spec.md:198-200` and the v4 save contract.
- Actual: `updateQuestKillProgress(id, -1)` or `setCurrentCount(-1)` produces negative progress; a subsequent save writes it and the next load reports `Corrupt`.
- Impact: A bad event or direct model caller can convert valid progression into a self-invalidating checkpoint and lose the ability to Continue without recovery.
- Suggested Action: Reject nonpositive incremental counts or explicitly define no-op semantics, clamp direct setters to `[0,target]`, and validate constructor target/reward bounds. Add negative/overflow progression tests and verify save/load remains valid.
- Re-audit Method: Apply negative, zero, and very large updates to KILL/COLLECT quests, assert bounded state, save, reload, and compare progress.
- Confidence: High

### [A01-F011] Quest completion ledger and Quest::isCompleted flag diverge

- Pass: Implementation
- Area: Quest completion state API
- Severity: Minor
- Status: Confirmed
- Summary: `Party::completeQuest` inserts the ID into `m_completedQuestIds` and erases the active quest, but never calls `Quest::setCompleted(true)`. Any retained shared pointer reports `isCompleted()==false` after the same quest has been completed in the Party.
- Evidence:
  - `src/model/Quest.cpp:53-56` exposes `isCompleted()`/`setCompleted()` and serializes `m_isCompleted` at line 113.
  - `src/model/Party.cpp:424-485` grants rewards, inserts `m_completedQuestIds`, and erases the quest at lines 480-481 without setting the Quest object’s completion flag.
  - `tests/test_content_contracts.cpp:106-129` checks the Party ledger and reward amount, but never checks the retained Quest pointer’s `isCompleted()`.
- Expected Basis: `Quest`’s documented completion flag and the typed quest state in `spec.md:198-200`; one completion should have one coherent representation.
- Actual: The Party-level `isQuestCompleted(id)` is true while a caller holding the accepted quest pointer observes false. If the pointer is inspected by a controller or tool, it reports stale state.
- Impact: Domain/UI/serialization consumers can disagree about whether a quest completed, increasing the risk of duplicate or stale UI logic in future code. The current board mostly uses the Party ledger, masking the divergence.
- Suggested Action: Either set the Quest object completed before erasing it and make the field authoritative, or remove the unused per-object flag and document the Party completion ledger as the sole authority. Add a pointer/ledger coherence regression test.
- Re-audit Method: Retain a quest shared pointer, complete it through Party, assert both APIs agree, serialize/reload, and ensure no duplicate reward path appears.
- Confidence: High

### [A01-F012] Party::addMember accepts null members and creates a crashing active-slot state

- Pass: Implementation / Debug
- Area: Party member invariant, HUD/controller lifetime
- Severity: Minor
- Status: Confirmed
- Summary: `Party::addMember` enforces only the size limit and pushes a null `shared_ptr`. Save serialization silently drops null members, while Town and CharacterInfo dereference every counted member.
- Evidence:
  - `src/model/Party.cpp:34-41` checks `m_members.size() >= 4` but has no `if (!member)` guard before `push_back`.
  - `src/model/Party.cpp:127-133` serializes only non-null members, so `getMemberCount()` and persisted member count can diverge.
  - `src/controller/TownState.cpp:474-486` dereferences `member->getClass()`/`getName()` for every counted member; `src/controller/CharacterInfoState.cpp:148-171` similarly dereferences summary/detail members.
  - Production creation checks candidate non-null, but the public Party model API and factory failure paths are not closed by the invariant; no null-member test exists.
- Expected Basis: `Party`’s maximum-4 member list is a list of actual characters (`spec.md:51`, `spec.md:88-89`), not nullable holes.
- Actual: A null add returns true until the fourth slot, can crash a Town/CharacterInfo draw, and is silently removed from the saved JSON.
- Impact: A malformed/orphan in-memory party can crash the UI and then change shape during save/load, breaking slot numbering and lifecycle assumptions.
- Suggested Action: Reject null in `addMember`, reject null at any deserialization boundary, and add a four-slot/null invariant test. If nullable slots are intentional, make them explicit and make all render/controller paths handle them consistently.
- Re-audit Method: Call `addMember(nullptr)` through the model and production HUD/Town paths; require false, unchanged count, and no null dereference. Test save/load with a deliberately malformed member entry.
- Confidence: High

### [A01-F013] Party save ordering is not canonical and has no byte-idempotence regression

- Pass: Debug / Engineering Quality
- Area: Save/load idempotence and deterministic serialization
- Severity: Minor
- Status: Confirmed
- Summary: `m_keyItems` and `m_completedQuestIds` are `std::unordered_set`s serialized by iteration order. The current tests compare `DungeonWorld::toJson()` round-trip but do not assert `Party` save bytes after save→load→save or across processes.
- Evidence:
  - `include/model/Party.hpp:91-92` stores completion/key IDs in unordered sets.
  - `src/model/Party.cpp:123-149` appends those IDs directly to JSON arrays without sorting/canonical ordering.
  - `spec.md:345-356` and the user scope require stable save/checkpoint behavior; `DESIGN_DECISIONS.md:148-153` emphasizes deterministic replay.
  - `tests/test_content_contracts.cpp:303-312` compares only a standalone world JSON; `test_content_contracts.cpp:334-349` checks migration fields but not identical Party bytes.
- Expected Basis: Save/load idempotence and deterministic checkpoint evidence in the assigned user goal, `spec.md:52-55`, and `spec.md:356`.
- Actual: Same semantic state relies on implementation-specific unordered iteration order. It may happen to remain stable on one libstdc++ build, but no canonical guarantee or regression protects Linux/Windows/process changes.
- Impact: Byte hashes, backup comparison, audit manifests, and replay fixtures can drift despite unchanged game state; this weakens corruption/recovery evidence.
- Suggested Action: Sort set-derived IDs before serialization (and define canonical object ordering if required) and add a production-linked save→load→save byte equality test plus an independent-process comparison.
- Re-audit Method: Populate all three completed quest IDs and the key-item set, save, load in a second process, save again, and require identical bytes and semantic state.
- Confidence: Medium-high
- Notes: This is a determinism/verification gap even if the current standard library happens to produce the same order locally.

### [A01-F014] CharacterInfo consumes full-heal potions on a full target

- Pass: Implementation
- Area: Item effect/no-op policy, out-of-combat character management
- Severity: Minor
- Status: Needs Clarification
- Summary: Combat uses `CombatActionRules` to reject no-effect healing, but CharacterInfo’s out-of-combat path does not check HP before applying/removing `pot_heal` or `pot_greater_heal`. The docs explicitly forbid effect-free combat actions, but do not clearly say whether the same no-op rule applies in CharacterInfo, so the policy needs to be made explicit.
- Evidence:
  - `src/model/CombatActionRules.cpp:8-19` rejects healing consumables when `target.getHp() >= target.getMaxHp()`; `CombatState` uses that gate before consuming an item.
  - `src/controller/CharacterInfoState.cpp:394-424` checks only STR/DEX combat-only, mana class/full slots, and poison for cure scroll; it has no full-HP check for healing potions.
  - `src/controller/CharacterInfoState.cpp:432-435` always applies the consumable and removes it from inventory.
  - `designs.md:298-302` describes CharacterInfo consumable effects but only gives the explicit non-combat ambiguity for buffs; `designs.md:345-346` gives the no-effect rule for combat item/ally actions.
- Expected Basis: If the no-effect policy is global, `spec.md:577`/`DESIGN_DECISIONS.md:136-140`; if it is combat-only, the authority must state that exception.
- Actual: In CharacterInfo, a full character can consume a healing potion and receive no HP change, while the same target is rejected in CombatState.
- Impact: Resource loss and inconsistent item semantics; whether this is a defect depends on the unresolved out-of-combat contract.
- Suggested Action: Clarify the scope of the no-op rule. If global, reuse `CombatActionRules` or add a shared effect-commit result and do not remove no-op items. If out-of-combat consumption is intentional, document it and test the intended behavior.
- Re-audit Method: Exercise full/injured/dead targets for all six consumables in Town and Dungeon CharacterInfo and compare resource, HP/status, and save bytes to the declared policy.
- Confidence: High for actual behavior; expected policy is unclear.

### [A01-F015] Quest v4 deserialization silently accepts missing canonical fields

- Pass: Implementation
- Area: Quest save schema validation
- Severity: Minor
- Status: Confirmed
- Summary: `Quest::fromJson` validates canonical values only when fields are present. Missing `targetId`, `targetCount`, reward, type, `isCompleted`, `readyToReport`, or `targetFloor` fields are often filled from the canonical factory/defaults instead of being rejected as a malformed v4 object.
- Evidence:
  - `spec.md:395-404` shows the v4 active-quest contract with ID, type, target, counts, readiness, and floor fields.
  - `src/model/Quest.cpp:120-156` uses `contains()` guards for type/target/count/reward/floor consistency.
  - `src/model/Quest.cpp:158-173` uses `value(..., 0/false)` for current count, completion, readiness and fills the canonical object when fields are absent.
  - `tests/test_harness.cpp:320-327` checks a subset of canonical fields after normal save, but no missing-field rejection test exists.
- Expected Basis: The declared v4 save shape in `spec.md:359-422` and schema-versioned corruption handling in `spec.md:344-350`.
- Actual: An active quest object with only `{ "id": "qst_recover_moon_seal" }` can be reconstructed with canonical hidden values and later reserialized as a fuller object; some malformed states are repaired silently rather than quarantined.
- Impact: Corruption and migration errors are hidden, and evidence cannot distinguish a valid v4 checkpoint from an implicitly repaired input.
- Suggested Action: Require all fields for schema v4, validate types/ranges and canonical values, and reserve defaulting/alias behavior for the explicitly documented v1–v3 shapes. Add per-field omission/tamper tests.
- Re-audit Method: Remove each required field from a valid v4 active quest and require `Corrupt` without mutating the party; separately confirm documented legacy aliases still migrate.
- Confidence: High

### [A01-F016] Failed Party load clears the active-session flag while retaining old in-memory state

- Pass: Debug / Implementation
- Area: Failed load, stale state, normal-close persistence
- Severity: Minor
- Status: Confirmed
- Summary: `Party::loadFromFile` sets `m_hasActiveSaveSession=false` before parsing. On NotFound, UnsupportedVersion, quarantine failure, or corrupt-without-backup returns, it leaves the existing members/world in memory but no longer marks the session as active; `Game::processEvents` then skips its normal-close save.
- Evidence:
  - `src/model/Party.cpp:174-176` clears `m_hasActiveSaveSession` before any candidate is accepted.
  - `src/model/Party.cpp:347-385` returns failed/not-found/corrupt results without restoring the prior active-session flag or replacing the in-memory party with an explicit empty state.
  - `src/core/Game.cpp:64-67` saves only when `m_party.hasActiveSaveSession()` is true on window close.
  - `tests/test_harness.cpp:616-664` verifies invalid loads do not mutate party fields but does not verify active-session flag/close semantics.
- Expected Basis: Non-destructive load failure and checkpoint safety in `spec.md:348-354`; failed load should not silently make retained memory unsaveable without an explicit UI state.
- Actual: A party can still contain the previous session after a failed load, but close no longer saves it. The caller receives an error while state ownership/dirty policy is implicit.
- Impact: Depending on the caller, valid in-memory progress can be discarded on close, or stale data can remain visible under a “no save/corrupt save” banner. This is especially risky for recovery paths that call `loadFromFile` opportunistically.
- Suggested Action: Preserve the active-session/dirty flag on a failed load, or explicitly clear/reinitialize all session state and route to a dedicated non-session error state. Add tests for NotFound, corrupt-without-backup, unsupported, and close behavior.
- Re-audit Method: Load a valid session, attempt each failed load path, inspect party/world/flag, invoke the production close handler, and verify whether bytes follow the documented policy.
- Confidence: High

### [A01-F017] Objective quest reporting is not domain-guarded by resolved world state

- Pass: Implementation
- Area: Quest/world causal integrity and report transaction
- Severity: Major
- Status: Confirmed
- Summary: `Party::completeQuest` requires a key item only for `RETRIEVE_KEY_ITEM`; it does not require the corresponding `QUEST_BOSS`/`NPC` world object to be `RESOLVED`. A caller can mark an objective ready and report it while the world object remains PRESENT, producing a completed quest whose next save/load is rejected by Party’s world consistency checks.
- Evidence:
  - `spec.md:198-201` requires field objective resolution, report-only rewards, and one-time completion; `spec.md:206-208` binds objective state to the world.
  - `src/model/Party.cpp:424-435` checks `checkCompletion()` and the retrieve key item only; no world-object lookup/state check exists for DEFEAT_BOSS or FIND_NPC.
  - `src/model/Party.cpp:475-482` inserts the completed ID and erases the quest after rewarding, without resolving the world object.
  - `src/model/Party.cpp:304-325` later rejects a completed quest whose world object is not `RESOLVED`.
  - `src/controller/DungeonState.cpp:433-441` performs the correct object resolution in the normal item/NPC interaction path, while `src/controller/CombatState.cpp:764-768` does it for a quest boss; the public Party report method itself is not closed against an orphan caller.
  - `tests/test_content_contracts.cpp:82-104` reports a retrieve quest with no generated world and therefore does not exercise boss/NPC resolution; controller tests only use the normal resolved path.
- Expected Basis: The objective/world consistency contract in `spec.md:198-208` and the loader’s own invariant at `Party.cpp:309-324`.
- Actual: `markQuestObjectiveComplete("qst_defeat_crypt_warden")` followed by `completeQuest` can grant rewards and mark completion without resolving `obj_crypt_warden`; a subsequent save/load can become `Corrupt`.
- Impact: A legitimate domain API sequence can orphan the world objective, invalidate the checkpoint, and make the one-time reward ledger disagree with the world. The controller path currently avoids it, but the domain boundary does not enforce the invariant.
- Suggested Action: Make report/commit validate the canonical world object and quest type/state atomically, or move completion into one transaction that resolves the object and records reward together. Keep legacy quantity quests on their separate path and add boss/NPC negative tests.
- Re-audit Method: For each objective quest, try ready/report with PRESENT, DISCOVERED, RESOLVED, missing-object, and wrong-object states; require only the canonical RESOLVED path to reward and remain loadable.
- Confidence: High

### [A01-F018] Poison death at turn start does not skip a non-TPK entity’s turn

- Pass: Implementation / Debug
- Area: Turn-start status effects, individual death, non-TPK combat flow
- Severity: Major
- Status: Confirmed
- Summary: `nextTurn()` checks whether the next entity is already dead before processing turn effects, but does not check again after poison damage. If poison reduces a character to 0 HP while another party member is alive, the dead character remains the current player entity and can receive input/perform an action. The same omission lets a poisoned monster attack after dying when other foes remain.
- Evidence:
  - `spec.md:166-170` requires poison damage at the start of each combat turn and defines death at HP 0 (`spec.md:190`); a dead entity must not continue its turn.
  - `src/controller/CombatState.cpp:380-411` checks `monster->isDead()` before `processTurnEffects()`, then calls `handleMonsterTurn(nextEntity)` whenever the monster was not previously dead. There is no post-effect `isDead()` guard when poison kills it and `checkVictory()` is false because another foe survives.
  - `src/controller/CombatState.cpp:413-437` checks `member->isDead()` before `processTurnEffects()`, then only checks party-wide `checkDefeat()` and the pre-effect `wasParalyzed` flag. A poison-killed member therefore reaches the player-turn state if the situation is not TPK.
  - `src/controller/CombatState.cpp:443-458`/`505-517`/`519-528` use the current turn entity without an `actor->isDead()` guard, so the stale current player can attack, choose a skill, or use an item.
  - `src/test_harness.cpp:900-935` tests standalone poison/paralysis state changes, but no production `CombatState::nextTurn()` case kills exactly one entity with poison while another entity remains alive.
- Expected Basis: `spec.md:168-170`, the `HP 0 이하 시 사망` rule at `spec.md:190`, and the current-turn/dead-entity skip invariant used by the pre-effect guards.
- Actual: Poison damage is applied and its duration decreases, but a non-TPK poisoned victim is not removed/skipped after dying. A poisoned enemy can also execute its normal attack after death if another enemy remains.
- Impact: Individual-death semantics, action economy, and TPK boundary are wrong. A dead party member may consume actions/resources or alter combat state; a defeated monster may damage the party once more. This can also mask the intended `Dead` HUD state until a later turn.
- Suggested Action: Immediately after `processTurnEffects()` re-check `isDead()` for both entity kinds; recurse/advance without action when dead, and perform `checkDefeat()` before continuing. Add a non-TPK character poison-death and monster poison-death integration regression with exact turn order and no extra action/attack assertions.
- Re-audit Method: Construct production-linked combats with at least two living party members (and separately two foes), apply lethal poison to the next turn entity, call `nextTurn()`/advance through the actual controller path, and assert the dead entity never reaches a player action or monster attack while another entity remains alive; assert TPK still routes to GameOver/checkpoint.
- Confidence: High

### [A01-F019] TPK restores the latest save, but “town checkpoint only” semantics conflict with periodic Dungeon saves

- Pass: Implementation / Debug
- Area: TPK rollback boundary, Dungeon checkpoint cadence, world/FOW persistence
- Severity: Major
- Status: Needs Clarification
- Summary: The TPK root replacement and non-destructive load path are implemented, but there is no separate town-baseline snapshot. `DungeonState` periodically writes the same `save.json` while the party is inside the dungeon, so TPK restores the latest successful dungeon/world snapshot rather than an unambiguously separate last-town checkpoint. The documents contain both interpretations.
- Evidence:
  - `spec.md:53-54` calls the save point a Town/ending checkpoint and says TPK returns to the last normal town checkpoint without overwriting it.
  - `spec.md:352-353` simultaneously permits exploration/FOW changes to be coalesced and saved every two seconds, with immediate saves on floor movement, objective resolution, combat settlement, town return, and normal close.
  - `IMPLEMENTATION_SUMMARY.md:60-68` claims both “active dungeon coordinates are not saved” and that terrain/FOW/objective state plus a town checkpoint are saved.
  - `src/controller/DungeonState.cpp:230-243` calls `persistWorldCheckpoint()` every two seconds while `m_worldDirty`; `src/controller/DungeonState.cpp:363-377` writes the same Party save file.
  - `src/controller/CombatState.cpp:367-373` handles TPK with `m_game.getParty().loadFromFile()` and has no separate town-baseline load operation. `Party::loadFromFile` restores the complete latest `world` snapshot (`src/model/Party.cpp:295-337`).
  - `tests/test_controller_contracts.cpp:143-165` proves root replacement and member restoration but does not mutate FOW/world after a town save and assert which checkpoint TPK restores.
- Expected Basis: The literal town-checkpoint TPK rule in `spec.md:53-54` conflicts with the periodic exploration-save rule in `spec.md:352-353`; this requires authority clarification rather than an invented preference.
- Actual: A successful save made in Dungeon can contain mid-run `visited`, `stepped`, discovered/resolved object state, and floor/world progress. TPK loads that file, then GameOver/Continue starts at B1 entry but keeps those latest world changes. The normal “last town” baseline is not separately recoverable.
- Impact: If “town checkpoint” means the exact pre-run state, TPK rollback is too permissive and an in-run failed run partially persists. If periodic world checkpointing is intended, the current docs/summary overstate town-only rollback and the behavior needs a clearer name/contract. Either way the gate is not decidable from the current authority set.
- Suggested Action: Choose and document one model: (a) maintain a separate town baseline for TPK and a distinct incremental world snapshot, restoring the specified components explicitly; or (b) define the latest durable Party/world save as the checkpoint and update all “town-only” wording. Add an integration test that records town state, advances FOW/objective in Dungeon, triggers TPK, and asserts the chosen exact restoration matrix.
- Re-audit Method: Save a known town baseline, perform a dungeon move/object discovery and force a TPK, then inspect Party members, HP/status, world tiles/FOW/object states, RNG metadata, save bytes, GameOver root, and post-Continue entry position against the selected policy.
- Confidence: High for observed save/load behavior; expected result is specification-dependent.

## 6. Uncertainties and Clarifications Needed

- Poison turn-start behavior is now independently confirmed as a non-TPK skip defect (`A01-F018`); it is not an unresolved requirement. The expected post-effect dead-entity skip follows the existing dead checks and HP-0 death rule.
- TPK’s exact world rollback boundary is unresolved (`A01-F019`): the implementation restores the latest same-file checkpoint, while the documents also use “last town checkpoint” language. Root replacement, normal-save non-destructive loading, and B1-entry-on-Continue are independently observed; only the preservation/rollback matrix for in-run FOW/object changes needs authority choice.
- Seedless v1/v2 migration authority is unresolved (`A01-F009`). The current `spec.md` promises deterministic saved-seed migration, while the supplied v1 fixture has no seed. The integrator should not mark this area PASS without choosing and documenting a policy.
- Character name treatment for Unicode format controls (C1, bidi/zero-width, line separators) is not specified beyond “control characters”; `A01-F001` records the implementation failure but the exact allowlist should be decided before re-audit.
- Character duplicate identity policy is not documented. `Party::addMember` permits duplicate names and the same `shared_ptr`; only the maximum count is enforced. Quest IDs and key items do have duplicate guards (`Party.cpp:418-422`, `538-549`, `276-284`). Do not invent a unique-name requirement; either document duplicates as allowed or add an explicit identity/instance rule.
- Whether no-effect consumable use is forbidden outside combat is unclear (`A01-F014`). The combat contract is explicit; CharacterInfo’s out-of-combat policy needs a source-of-truth decision.
- The current source was syntax-checked, but the working-tree CTest/package/GUI runtime gate was not executed because the audit contract forbids producing/changing build artifacts. Existing `build` evidence is stale and cannot close these findings.
- Actual IME, high-DPI, long-play, and platform-specific UI behavior remain uninspected in this sub-audit.

## 7. Perspective Decision

- Decision: HOLD for the character-lifecycle perspective.
- Gate reasons: Confirmed Major findings `A01-F001`, `A01-F002`, `A01-F003`, `A01-F004`, `A01-F006`, `A01-F007`, `A01-F008`, `A01-F017`, and `A01-F018`; plus unresolved Major contracts `A01-F009` and `A01-F019`.
- Positive evidence: Draft point-buy arithmetic/reroll isolation, class trait rule helpers, max-four parser bound, duplicate quest/key-item/completion ledger guards, normal objective controller transactions, generated-world connectivity, world JSON round-trip, and TPK root replacement are present in current source/tests. These do not close the findings above.
- Required before a PASS judgment: resolve the seedless migration specification; close the confirmation, RNG restore, buff cleanup, identity/semantic validation, world snapshot invariants, and objective report transaction; add the targeted regressions and execute a fresh current-tree Debug/Release CTest plus production controller/raster checks.
