# Sub Audit Report

## 1. Audit Metadata

- Audit Turn: 3
- Perspective: Contract, documentation, current-build and production-linked E2E coverage supplement
- User Goal: `$multi-audit 추가한 기능이 많음. 1. 캐릭터 시스템을 생성부터 플레이 사망까지 모두 전체 점검 2. 퀘스트 시스템을 전체 점검 3. 맵 생성 및 세이브/로드시 멱등성, 소멸, 재생성 기타 모든 부분 싹다 점검 4. 문서상 요청된 내용이 잘 구현되고 모순되거나 문제되는 점이 있으면 점검`
- Supplement Questions:
  1. current working tree와 기존 Debug/Release/CTest 증거가 실제 소스와 정렬되는가
  2. 4인 캐릭터 생성·저장부터 Dungeon/Combat·개별 사망·TPK·복구까지 production-linked E2E가 닫히는가
  3. 세 목적형 퀘스트 각각의 accept·field resolve·Castle report·save/load·반복 무보상·save failure rollback이 닫히는가
  4. 자동 이동의 quest boss/BossGate activation에서 `spec.md`와 `designs.md` 중 어느 계약이 충돌하며 코드가 어느 쪽인가
- Audit Basis: Standard-backed
- Standard Path: `/mnt/Projects_SSD/cpp/crawlmaster/AI_AUDIT_DOC_STANDARD.md`
- Report Contract: `/home/eunho1/.codex/skills/multi-audit/references/report-contract.md`
- Supplement Of: `sub_audit_04_contract_docs.md`

## 2. Assigned Scope

현재 working tree의 다음 범위를 독립 대조했다.

- `spec.md`, `designs.md`, `IMPLEMENTATION_SUMMARY.md`, `BUILD_GUIDE.md`, `README.md`, `CHANGELOG.md`, `DESIGN_DECISIONS.md`, `audit_roadmap.md`, `tasks/plan.md`, `tasks/todo.md`, `tests/fixtures/README.md`
- 캐릭터·파티·생성 draft: `Character`, `Party`, `RecruitmentDraft`, `CharacterCreationState`, `CharacterInfoState`
- 퀘스트·월드·맵: `Quest`, `DungeonWorld`, `DungeonMap`, `TownState`, `QuestJournalState`
- 플레이 흐름: `TitleState`, `DungeonState`, `CombatState`, `GameOverState`, `VictoryState`
- 현재 테스트: `tests/test_controller_contracts.cpp`, `tests/test_content_contracts.cpp`, `tests/test_agency_contracts.cpp`, `tests/test_hud_contracts.cpp`, `tests/test_ui_state_raster.cpp`, `tests/test_rng_process_replay.cpp`, `src/test_harness.cpp`
- CMake/빌드 증거: `CMakeLists.txt`, `build/debug/CMakeCache.txt`, `build/release/CMakeCache.txt`, `build/final-debug/CMakeCache.txt`, `build/final-release/CMakeCache.txt`, 각 빌드의 CTest 목록과 기존 raster 산출물 메타데이터

## 3. Excluded and Uninspected Scope

- 사용자의 지시에 따라 `docs/audit/**`와 `docs/multi_audit/1`, `docs/multi_audit/2`, `docs/multi_audit/3`의 기존 sub/final 감사보고서는 읽지 않았다. 원본 `sub_audit_04_contract_docs.md`도 읽지 않았다.
- 소스, 테스트, 설정, 제품 문서는 수정하지 않았다. 이 supplement 파일만 작성 대상이다.
- clean Windows 10/11 VC++ runtime, 실제 OS high-DPI, 장시간 3층 플레이, macOS, hosted 외부 환경은 현재 로컬 증거로 재현하지 않았다.
- UI raster 실행 파일은 이 보완에서 장시간 전체 재실행을 완료하지 않았다. 기존 `build/debug/ui-state-raster-evidence`와 `build/release/ui-state-raster-evidence`에는 각각 420개 PNG가 존재하지만, 해당 산출물만으로 이번 보완 시점의 전체 CTest 실행 transcript를 대체하지 않았다.

## 4. Evidence Examined

### 4.1 문서·계약

- `spec.md:139-145`: 생성 흐름, 10포인트 최종 확인, 취소 불변
- `spec.md:196-208`: 세 목적형 퀘스트, 현장 달성/성 보고/정확히 한 번 보상, 3층 월드, 활성 quest boss 진입 전투
- `spec.md:344-358`: schema v4, full world snapshot, checkpoint/TPK/RNG/PersistenceResult
- `spec.md:552-569`: Debug/Release/CTest/CPack gate
- `designs.md:32-53`: 생성 UI 세 단계와 저장 실패/취소 계약
- `designs.md:55-60`: Dungeon 목표 표식과 자동 이동 계약
- `designs.md:326-357`: Turn 1 UI/상태 전이 보완 계약
- `BUILD_GUIDE.md:29-55`: 13개 CTest 목록
- `BUILD_GUIDE.md:3`: 문서 작성 기준이 `0.9.4`로 남아 있음
- `IMPLEMENTATION_SUMMARY.md:60-81`: schema v4 및 `build/debug`, `build/release` 13/13 주장

### 4.2 현재 구현·테스트

- `CharacterCreationState::commitCharacter`: 후보를 파티에 넣고 `saveToFile()` 성공 시 종료하며, 일반 실패 시 마지막 멤버를 제거한다.
- `DungeonState::stepAutoMove` 및 `checkCurrentTileLog`: 자동 이동 한 칸마다 현재 타일 전이 검사를 호출한다. 검사 함수는 활성 quest boss와 `BOSS_GATE`에서 `CombatState`를 push한다.
- `DungeonState::interactCurrentTile`: 계단·quest item·NPC를 `E`로 처리하지만 quest boss는 직접 상호작용하지 않고 진입 검사에 맡긴다.
- `TownState`의 CASTLE 분기: available/active quest를 선택해 수주하고, ready quest를 `completeQuest`한 후 저장한다.
- `Party::completeQuest`: 골드/XP/아이템/중요품 소비와 completed ID 삽입을 수행한다. `Party::loadFromFile`은 active/completed/world 상태의 일부 일관성을 검증한다.
- `CombatState::nextTurn`, `distributeRewards`: TPK 때 저장을 로드하고 root를 `GameOverState`로 교체하며, 승리 보상·quest objective/world object 상태를 저장한다.
- `tests/test_controller_contracts.cpp:143-165`: 저장된 단일 fixture 캐릭터의 TPK root 교체 및 checkpoint 복구
- `tests/test_controller_contracts.cpp:288-330`: production `CharacterCreationState`로 한 명 생성
- `tests/test_controller_contracts.cpp:461-502`: 세 quest를 직접 accept하고 object helper로 현장 해결 후 world load 확인
- `tests/test_controller_contracts.cpp:504-532`: moon seal 한 건의 Castle report save failure rollback 및 재보고
- `tests/test_controller_contracts.cpp:556-587`: schema v3 월드 이관 후 Title Continue 경로
- `tests/test_controller_contracts.cpp:589-613`: crypt warden 전투 보상 저장 실패 후 재시도
- `tests/test_content_contracts.cpp:82-104`: 한 legacy KILL quest의 field completion/report/중복 보상 차단
- `tests/test_content_contracts.cpp:260-372`: 월드 snapshot round-trip, v3 deterministic migration, New Game reset
- `tests/test_ui_state_raster.cpp:124-135,164-230`: fixture party와 한 개 생성 draft를 이용한 화면 캡처; 생성 확인 저장과 전체 플레이 흐름은 아님

### 4.3 빌드·실행 증거

- `build/debug/CMakeCache.txt`, `build/release/CMakeCache.txt`: `CMAKE_BUILD_TYPE=Debug/Release`, project version `0.10.0`, source home path가 현재 `/mnt/Projects_SSD/cpp/crawlmaster`와 동일 inode의 `/home/eunho1/Projects/cpp/crawlmaster`를 가리킴.
- 관련 소스·테스트 파일 중 `build/debug/ControllerContractTests` 또는 `build/release/ControllerContractTests`보다 새로운 파일은 확인되지 않았다. 최신 binary 시각은 Debug `2026-09-04 15:58`, Release `2026-09-04 16:01`이다.
- `ctest --test-dir build/debug -N`, `ctest --test-dir build/release -N`은 각각 13개 테스트를 열거했다. 이는 등록 증거이지 실행 성공 증거가 아니다.
- 현재 `build/debug/Testing/Temporary/LastTest.log` 및 `build/release/Testing/Temporary/LastTest.log`는 시작/종료 3줄만 있어 개별 실행 결과를 증명하지 않는다.
- 현재 binary 직접 실행 결과: Debug의 resource verify, TestHarness, HUD/Combat/Content/Agency/Localization contract, FontRaster, 별도 process RNG write/continue, Xvfb ControllerContractTests는 exit 0이었다. UI state raster는 전체 재실행을 완료하지 않았다.
- Release의 resource verify, TestHarness, HUD/Combat/Content/Agency/Localization contract, Xvfb ControllerContractTests, 별도 process RNG write/continue는 exit 0이었다. Release 전체 CTest를 직접 실행한 증거는 없다.
- `build/final-debug/CMakeCache.txt`, `build/final-release/CMakeCache.txt`는 project version `0.9.4`이므로 현재 0.10.0 working tree의 최신 build provenance로 사용할 수 없다.

## 5. Findings

### [S04-F001] 현재 소스 정렬은 확인되지만 Debug/Release 전체 CTest 13/13 실행 증거가 닫히지 않음

- Pass: Debug / Engineering Quality
- Pattern: `BUILD-001`, `TEST-001`, `IMP-003`
- Area: current working tree, build provenance, CTest release gate
- Severity: Major
- Status: Confirmed
- Summary: 현재 `build/debug`와 `build/release` 실행 파일은 버전·경로·파일 시점상 working tree와 정렬되고 핵심 직접 실행도 통과했다. 그러나 문서가 주장하는 전체 CTest 13/13의 현재 실행 transcript가 없고, CTest `-N`은 등록만 확인한다. `final-*` build는 0.9.4라서 최신 소스의 증거가 아니다.
- Evidence: `CMakeLists.txt:1-4,85-102,257-299`; `spec.md:552-569`; `BUILD_GUIDE.md:29-55`; 위 4.3의 CMakeCache/stat/CTest/direct-run 결과; `IMPLEMENTATION_SUMMARY.md:72-80`.
- Expected Basis: Debug/Release build와 등록 CTest, Linux package smoke가 모두 통과해야 한다는 `spec.md:38,552-569` 및 BUILD_GUIDE 계약.
- Actual: 소스와 최신 Debug/Release binary의 시점 정렬 및 핵심 target 직접 실행은 확인됐지만, 현재 CTest가 13개 모두 실행되어 성공했다는 증거와 UI raster CTest 성공 transcript는 확인되지 않았다. `final-*`는 0.9.4이다.
- Impact: `IMPLEMENTATION_SUMMARY.md`의 13/13 주장을 현재 source에 대한 독립 release gate로 승격할 수 없다. 전체 PASS 계열 판정을 차단한다.
- Suggested Fix: clean 별도 Debug/Release build에서 문서 명령을 그대로 실행하고, `ctest --output-on-failure --no-tests=error` 전체 결과와 package/resource/UI evidence를 보존한다. `BUILD_GUIDE.md:3`의 0.9.4 기준일/버전 drift도 0.10.0 lane에 맞춰 정리한다.
- Re-audit Method: source path/version 및 source-to-binary dependency를 재확인한 뒤 fresh Debug/Release build, 13/13 CTest, CPack/resource smoke의 실제 exit code와 transcript를 대조한다.
- Owner: Coder / Release owner
- Confidence: High
- Notes: 직접 실행 성공은 유효한 부분 증거지만 CTest orchestrator 실행과 동일하지 않다. 이 supplement에서는 새 build나 제품 산출물을 만들지 않았다.

### [S04-F002] 4인 생성부터 개별 사망·TPK·복구까지 production-linked E2E가 없음

- Pass: Implementation / Debug
- Pattern: `IMP-001`, `IMP-003`, `TEST-001`
- Area: CharacterCreationState, DungeonState, CombatState, GameOverState, checkpoint recovery
- Severity: Major
- Status: Confirmed
- Summary: 현재 controller transcript는 한 명의 생성·저장만 검증하고, TPK는 저장된 단일 fixture 캐릭터를 전투 시작 전에 죽인 뒤 checkpoint를 복구한다. 4명을 실제 생성 UI로 연속 확정하고 저장한 뒤 Dungeon/Combat에서 한 명씩 죽고 마지막 TPK 후 GameOver/checkpoint/후속 복구까지 닫는 조합은 없다.
- Evidence: `tests/test_controller_contracts.cpp:143-165`는 `Saved` 한 명을 저장한 뒤 `savedMember->takeDamage(...)`로 전투 전에 사망시킨다. `:288-330`은 `testCharacterCreationRequiresIdentityPointsAndConfirm` 한 명만 생성한다. `tests/test_hud_contracts.cpp:86-115`의 4 slot 검증은 fixture/model HUD snapshot이며 생성 UI 경로가 아니다. `tests/test_ui_state_raster.cpp:124-135,164-186`은 fixture party를 저장하고 creation confirm 직전 화면을 캡처할 뿐이다. TPK 구현은 `src/controller/CombatState.cpp:367-373`에 있다.
- Expected Basis: `spec.md:28`, `spec.md:139-145`, `spec.md:110-132`, `designs.md:32-53,326-349`의 4인 생성/확정, Dungeon·Combat 전이, TPK root recovery 계약.
- Actual: 한 명 생성과 단일 fixture TPK는 통과하지만, 4인 production creation, 개별 사망 상태가 HUD/turn order/생존 파티에 반영되는 흐름, 실제 Dungeon 진입을 포함한 TPK 후 복구의 단일 E2E 증거가 없다.
- Impact: 캐릭터 시스템의 핵심 수명주기와 4인 파티 제약, 사망 중간 상태, checkpoint 복구를 함께 보장할 수 없다. 모델·부분 controller 테스트의 녹색 결과로 이 공백을 대체할 수 없다.
- Suggested Fix: controller contract에 4개 `CharacterCreationState` event transcript를 추가하고 각 확정 저장을 확인한다. 저장된 4인 파티로 Dungeon에 진입해 실제 Combat에서 한 명씩 사망하는 상태와 dead member 제외를 확인하고, 마지막 TPK가 `GameOverState` root를 만들며 저장된 정상 checkpoint를 복구하는지 확인한다. GameOver 이후 Continue/Temple 등 제품이 정의한 복구 경로도 같은 fixture로 닫는다.
- Re-audit Method: 새 테스트가 fixture 직접 삽입이 아닌 production creation/controller 경로를 사용하는지 확인하고, Debug/Release에서 4인 생성→저장→Dungeon→Combat→individual death→TPK→checkpoint recovery를 반복 실행한다.
- Owner: Coder / Test owner
- Confidence: High
- Notes: 구현 함수의 존재를 결함으로 단정한 것이 아니라, 사용자 핵심 목표에 대한 end-to-end coverage gap 판정이다.

### [S04-F003] 세 목적형 quest의 Castle report·반복 무보상·report rollback 조합이 닫히지 않음

- Pass: Implementation / Debug
- Pattern: `IMP-001`, `IMP-003`, `TEST-001`
- Area: objective quest lifecycle, Castle board, persistence and reward ledger
- Severity: Major
- Status: Confirmed
- Summary: 세 quest의 field resolve와 snapshot load는 controller 테스트에 있으나 quest를 Castle UI에서 accept하거나 Castle UI에서 세 건 모두 report하지 않는다. save failure rollback과 재보고 성공/중복 차단은 moon seal 한 건만 검증한다.
- Evidence: `tests/test_controller_contracts.cpp:461-502`는 `party.acceptQuest(...)`로 세 quest를 직접 수주하고 object helper로 field resolve한 뒤 `Party loaded`를 검사한다. 같은 파일 `:504-532`는 `qst_recover_moon_seal`만 Castle report, save failure rollback, 재보고 성공과 completed ID를 검사한다. `tests/test_content_contracts.cpp:82-104`는 legacy KILL 한 건의 model report 중복 차단이다. UI raster의 Castle 캡처는 `tests/test_ui_state_raster.cpp:187-195`에서 생성될 뿐 입력 transcript가 아니다. Runtime report 경로는 `src/controller/TownState.cpp:262-291`, 보상/ledger는 `src/model/Party.cpp:424-477`이다.
- Expected Basis: `spec.md:196-202`, `spec.md:513-520`, `designs.md:62-77,318-322`: 세 목적형 quest의 accept→field resolve→Castle report, save commit 시 정확히 한 번 보상, 중요품 소비, 취소/실패 보존.
- Actual: retrieval/boss/NPC의 현장 해결 및 일부 save/load는 확인되지만, 세 quest 각각에 대해 보드 수주, 마을 귀환/보고, save/load 후 상태, 두 번째 보고 no-reward, report save failure rollback을 모두 묶은 증거는 없다.
- Impact: boss/NPC reward item·XP·gold ledger와 completed ID 중복 차단, 실패 시 active/ready/world state rollback이 retrieval과 동일하게 동작한다고 주장할 수 없다.
- Suggested Fix: 각 canonical quest에 대해 Castle에서 accept하고 저장한 뒤 Dungeon에서 field resolve, Castle report, save/load, 동일 항목 재선택 no-reward를 수행하는 production-linked transcript를 추가한다. report 직전 `.tmp` blocking 또는 주입된 IoError를 세 quest 각각에 적용해 gold/XP/item/key item/active/completed/world object bytes/state가 rollback되는지 단언한다.
- Re-audit Method: 보드 입력이 `TownState`를 통과하는지 확인하고 세 ID 각각의 success/failure/repeat 경로를 Debug와 Release에서 실행해 저장 전후 JSON과 메모리 상태를 비교한다.
- Owner: Coder / Test owner
- Confidence: High
- Notes: 현재 코드의 공통 report 함수가 모든 유형을 호출한다는 사실은 유형별 E2E 증거를 대신하지 않는다.

### [S04-F004] 자동 이동 시 quest boss/BossGate 활성화에 대해 `spec.md`와 `designs.md`가 충돌함

- Pass: Cross-Pass / Implementation
- Pattern: `IMP-002`, `SPEC-GAP-001`
- Area: auto movement activation contract, boss transition authority
- Severity: Major
- Status: Needs Clarification
- Summary: `spec.md`는 활성 quest boss 칸에 진입하면 도주 불가 고정 전투가 시작된다고 정의한다. 반면 `designs.md`는 자동 이동이 계단과 목표를 자동 활성화하지 않는다고 정의한다. 현재 코드는 `stepAutoMove`가 매 칸 `checkCurrentTileLog`를 호출하고, 그 함수가 quest boss와 BossGate에서 즉시 `CombatState`를 push하므로 spec 쪽 entry-trigger semantics를 따른다.
- Evidence: `spec.md:196-208` 및 master-plan 선언 `spec.md:4-7`; `designs.md:55-60`; `src/controller/DungeonState.cpp:245-280`의 auto step→`checkCurrentTileLog`; `src/controller/DungeonState.cpp:312-345`의 quest boss/BossGate combat push; `src/controller/DungeonState.cpp:407-440`의 `E` item/NPC/stairs 처리. 자동 이동 테스트는 `tests/test_controller_contracts.cpp`에 없다.
- Expected Basis: 두 문서가 서로 다른 기대를 명시하므로 권위 결정이 먼저 필요하다. `spec.md`가 주된 제품 계약이면 entry-trigger 코드는 일치하고 `designs.md` 문구가 drift다. `designs.md`의 자동 이동 계약을 우선하는 UX 결정이면 코드가 자동 활성화를 멈추고 사용자의 `E` 입력을 요구해야 한다.
- Actual: 코드가 `spec.md`의 활성 quest boss 진입 전투와 일치하며 `designs.md:60`의 “자동 이동은 계단과 목표를 자동 활성화하지 않는다”와 불일치한다. BossGate도 동일한 공통 검사로 자동 이동 중 전투를 시작한다. quest item/NPC는 자동 이동에서 발견만 되고 `E`가 필요하다.
- Impact: 자동 이동 중 갑작스러운 고정 전투/최종 전투가 제품 의도와 다를 수 있고, 어느 계약을 기준으로 테스트해야 하는지 결정할 수 없다. 문서·코드·테스트가 닫히기 전에는 이 동작을 PASS할 수 없다.
- Suggested Fix: `spec.md`와 `designs.md` 중 activation authority를 명시적으로 하나로 확정하고 다른 문서를 동기화한다. entry-trigger를 유지하면 designs 문구를 “quest boss/BossGate는 진입 시 활성화, 계단/item/NPC는 E 필요”로 보정하고, 아니면 auto step에서 activation을 차단한다. 두 경우 모두 자동 이동으로 boss와 BossGate에 도달하는 controller regression을 추가한다.
- Re-audit Method: 문서 authority와 event transcript를 재확인하고 auto path가 boss/BossGate/item/NPC 각각에서 기대한 상태 전이를 만드는지 Xvfb production test로 검증한다.
- Owner: Architect / Product owner
- Confidence: High
- Notes: 이 finding은 어느 문서가 맞는지 임의로 결정하지 않는다. 현재 코드가 어느 쪽인지와 충돌 자체만 확정한다.

### [S04-F005] 월드 round-trip과 migration은 있으나 production re-entry·반복 save/load·재생성 경계가 부분 검증임

- Pass: Implementation / Debug
- Pattern: `IMP-001`, `IMP-003`, `DBG-002`, `TEST-001`
- Area: `DungeonWorld`, `DungeonMap`, save/load idempotency, destruction/regeneration, controller lifecycle
- Severity: Major
- Status: Confirmed
- Summary: model 수준의 snapshot equality, v3 deterministic migration, New Game reset과 일부 production floor/object interaction은 확인했다. 그러나 Continue/re-entry를 통해 v4 월드 상태를 다시 여는 경로, 반복 `save→load→save`의 저장 bytes/semantic equality, item/NPC/floor transition save failure rollback, 동일 seed 재생성·기존 월드 소멸 경계를 모두 묶은 증거는 없다.
- Evidence: `tests/test_content_contracts.cpp:260-312`는 한 번의 `DungeonWorld::toJson`→`fromJson` equality를 검사한다. `:314-349`는 같은 v3 입력의 두 migration world equality와 v4 저장을 검사한다. `:351-372`는 `startNewGame`으로 seed/world/object/quest reset을 검사한다. `tests/test_controller_contracts.cpp:461-502`는 floor/object 일부와 load를, `:589-613`은 boss 전투 save failure 일부를 검사한다. Runtime 경계는 `src/model/DungeonWorld.cpp`의 `generate`, `toJson`, `fromJson`, `src/model/Party.cpp:390-413`의 reset/new game, `src/controller/TitleState.cpp:68-100`의 Continue이다.
- Expected Basis: `spec.md:203-208,344-354`, `DESIGN_DECISIONS.md:163-168`, `tasks/plan.md:5-20`: New Game 전까지 지형·fog·stepped·object state 유지, Continue/마을 재입장은 1층 입구, full snapshot/migration, 실패 시 checkpoint 보존.
- Actual: 생성 연결성·snapshot 한 번의 semantic equality·migration deterministic·New Game reset은 Partially Covered다. 그러나 Title Continue로 저장된 v4 지형/방문/목표를 재진입해 확인하는 controller path, 여러 번 저장/로드 후 동일성, all-object/floor failure rollback, generation/destruction/re-generation 반복 테스트는 없다. helper가 좌표를 직접 teleport하므로 실제 BFS/auto-move traversal도 닫히지 않는다.
- Impact: 월드 persistence의 장기 멱등성, 재생성 시 이전 상태 소멸, 중간 저장 실패 후 참조 수명과 state rollback을 배포 후보의 전체 계약으로 보증할 수 없다.
- Suggested Fix: production controller test를 추가해 동일 save에서 Town 재입장과 Title Continue를 반복하고 floors/fog/stepped/object state를 비교한다. 각 cycle에서 `save→load→save` JSON/bytes와 world semantic hash를 비교하고, 동일 seed regenerate, New Game replacement, floor transition 및 item/NPC/boss save failure rollback을 별도 fixture로 검증한다. 직접 좌표 설정 helper만 쓰지 말고 deterministic path/auto path도 포함한다.
- Re-audit Method: clean save directory에서 seed를 고정해 생성→탐험→목표 해결→저장→load→재저장→Continue→New Game을 반복하고, 이전 월드의 seed/state가 새 월드에 남지 않는지와 실패 시 마지막 정상 snapshot이 유지되는지 확인한다.
- Owner: Coder / Test owner
- Confidence: High
- Notes: `DungeonWorld::fromJson`의 한 번의 `toJson()==serialized`는 좋은 model evidence이나, 사용자 요청의 전체 lifecycle gate를 닫지는 않는다.

### [S04-F006] 저장된 `DISCOVERED` object가 미방문 타일을 가리키는 불일치가 로드·렌더 경계에서 차단되지 않음

- Pass: Implementation / Debug
- Pattern: `IMP-001`, `TEST-001`
- Area: world snapshot invariants, fog-of-war marker visibility
- Severity: Minor
- Status: Confirmed
- Summary: 월드 loader는 object 위치가 walkable인지와 canonical 계약은 확인하지만 `DISCOVERED`이면 해당 floor tile이 `visited`여야 한다는 불변조건을 검사하지 않는다. renderer도 `state == DISCOVERED`와 active quest만 확인하고 `map.isVisited`를 다시 확인하지 않아, 일관성이 깨진 local save가 목표 marker를 fog 전에 노출할 수 있다.
- Evidence: `src/model/DungeonWorld.cpp`의 `DungeonWorld::fromJson` object 검증은 interior/unique/walkable/canonical을 확인하나 object state와 floor `visited`의 관계를 확인하지 않는다. `src/model/Party.cpp:314-329`의 load 검증은 resolved/active/completed 관계를 검사하지만 discovered/visited 관계를 검사하지 않는다. `src/view/DungeonRenderer.cpp:185-214`는 `DISCOVERED` active object를 marker로 그리며 `map.isVisited(object.x, object.y)` 조건이 없다. 기대 UI는 `designs.md:55-60`의 “시야로 발견하기 전에는 미니맵에 표시하지 않는다”다.
- Expected Basis: fog-of-war와 object discovery의 문서 계약 및 `DungeonState::discoverQuestObjects`의 정상 경로가 `visited`를 근거로 discovery를 만드는 불변조건.
- Actual: 정상 runtime에서는 `discoverQuestObjects`가 visited를 보고 state를 변경하지만, snapshot load가 불일치 조합을 허용하고 renderer가 방어적으로 차단하지 않는다.
- Impact: 손상/수동 변조된 save가 탐험 전 퀘스트 위치를 노출하고 world state semantics를 깨뜨린다. 일반 게임 진행의 정상 경로보다 save integrity와 renderer defense-in-depth 문제다.
- Suggested Fix: loader에서 `DISCOVERED => map.isVisited(x,y)`를 거부하고, renderer에서도 marker를 그릴 때 visited를 함께 요구한다. discovered-but-unvisited malformed snapshot 회귀 테스트를 추가한다.
- Re-audit Method: malformed v4 world snapshot을 load해 거부되는지, visited를 지운 뒤 renderer가 marker를 그리지 않는지 production raster/headless test로 확인한다.
- Owner: Coder / Test owner
- Confidence: Medium-High
- Notes: 현재 정상 생성·탐험 경로가 이 조합을 만들지 않는다는 점은 완화 요소지만, persistence boundary가 허용하는 상태 조합 자체는 계약과 맞지 않는다.

## 6. Coverage Gap Matrix

| Work Surface / Audit Question | Evidence | Coverage | Follow-up |
| --- | --- | --- | --- |
| current working tree와 Debug/Release binary 정렬 | CMakeCache version/path, source-vs-binary mtimes, Debug/Release 핵심 direct binaries | Partially Covered | fresh build와 full CTest 13/13 transcript, package smoke |
| Debug/Release/CTest가 current source를 실제 검증하는가 | CTest `-N` 13개 등록, LastTest.log에 outcome 없음, direct target 일부 exit 0 | Partially Covered | CTest 전체 실행 결과를 보존하고 UI raster/fixture 결과 포함 |
| 4인 character creation→save→Dungeon/Combat→individual death→TPK→recovery | 한 명 creation, 단일 fixture TPK, 4-slot HUD model snapshot | Not Covered | production controller 4인 수명주기 transcript |
| 세 objective quest accept→field resolve→Castle report→save/load→repeat no-reward→report rollback | 세 field resolve/load, moon seal 한 건 report rollback | Partially Covered | 세 canonical ID별 Castle-driven success/failure/repeat |
| auto-move quest boss/BossGate activation contract | spec/design 문구와 `stepAutoMove`/`checkCurrentTileLog` 정적 대조 | Partially Covered | authority 확정 후 auto path runtime regression |
| world generation/snapshot/migration/New Game reset | model round-trip, v3 deterministic migration, New Game reset | Partially Covered | Continue/re-entry, repeated save/load, transition failure, regenerate/destroy cycle |
| fog/object snapshot invariant | 정상 discovery 경로와 renderer/source 정적 대조 | Partially Covered | malformed discovered/unvisited load/raster regression |

## 7. Uncertainties and Clarifications Needed

- `S04-F004`: 자동 이동이 quest boss와 최종 BossGate에 도달했을 때 즉시 전투를 시작하는 것이 제품 의도인지, 모든 목표/게이트에 `E`를 요구하는 것이 의도인지 `spec.md`와 `designs.md`를 하나의 authority로 정리해야 한다.
- `IMPLEMENTATION_SUMMARY.md`의 2026-09-04 `build/debug`·`build/release` 13/13 주장은 binary 직접 실행 결과와 모순되지는 않지만, 현재 파일만으로 CTest 전체 실행의 exact transcript를 재구성할 수 없다.
- UI raster PNG 420개씩의 존재는 산출물 생성 사실의 부분 증거일 뿐, 각 파일이 현재 source의 fresh CTest 실행에서 생성됐고 시각적으로 판독됐다는 독립 증거는 아니다.
- 외부 hosted gate, clean Windows runtime, OS high-DPI, 장시간 플레이와 실제 3층 traversal은 이 보완에서 판정하지 않았다.

## 8. Perspective Decision

**Partially Covered — HOLD**

현재 Debug/Release 최신 binary와 핵심 직접 실행은 current working tree와 정렬되어 있고 다수 domain/controller contract가 통과했다. 그러나 (a) 전체 CTest 13/13 current-run evidence가 없고, (b) 4인 캐릭터 수명주기 production E2E가 `Not Covered`이며, (c) 세 objective quest의 Castle-driven report/repeat/rollback 조합이 `Partially Covered`이고, (d) 월드 lifecycle 재진입·반복 멱등성 및 자동 이동 계약이 닫히지 않았다. 따라서 이 supplement 범위에서 PASS 계열 판정은 금지한다.

## 9. Coder Handoff

`/mnt/Projects_SSD/cpp/crawlmaster/docs/multi_audit/3/sub_audit_04_contract_docs_supplement_1.md`를 먼저 읽고, 각 finding을 현재 프로젝트 문서와 실제 코드에 대조하여 우선순위대로 보완하세요. 계약 충돌이 있는 `S04-F004`는 문서를 먼저 정렬하고, 수정 후 fresh Debug/Release CTest·production-linked E2E·재감사 증거를 기록하세요.

