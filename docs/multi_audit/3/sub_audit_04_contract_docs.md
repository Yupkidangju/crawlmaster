# Sub Audit Report

## 1. Audit Metadata

- Audit Turn: 3
- Perspective: Contract / Documentation / Implementation Compliance (Pass 1 중심, 필요한 Debug 교차)
- User Goal: `$multi-audit 추가한 기능이 많음. 1. 캐릭터 시스템을 생성부터 플레이 사망까지 모두 전체 점검 2. 퀘스트 시스템을 전체 점검 3. 맵 생성 및 세이브/로드시 멱등성, 소멸, 재생성 기타 모든 부분 싹다 점검 4. 문서상 요청된 내용이 잘 구현되고 모순되거나 문제되는 점이 있으면 점검`
- Audit Basis: Standard-backed
- Standard Path: `AI_AUDIT_DOC_STANDARD.md`
- Report Contract: `/home/eunho1/.codex/skills/multi-audit/references/report-contract.md`
- Baseline: `HEAD 927753278f46b92a015197ee229edce4f52e0657 + current working tree`
- Audit Date: 2026-09-04 (Asia/Seoul)

## 2. Assigned Scope

- `spec.md`, `designs.md`, `DESIGN_DECISIONS.md`, `IMPLEMENTATION_SUMMARY.md`, `README.md`, `CHANGELOG.md`, `BUILD_GUIDE.md`, `audit_roadmap.md`, `tasks/*`, `LESSONS_LEARNED.md`의 현재 계약과 supersede 계보.
- 캐릭터 생성·저장·장비·상태이상·전투·TPK 경로: `Character`, `RecruitmentDraft`, `CharacterCreationState`, `CharacterInfoState`, `CombatState`, `Party`.
- 목적형 퀘스트·중요품·성 보고·일지 경로: `Quest`, `TownState`, `QuestJournalState`, `DungeonState`, `CombatState`.
- `DungeonMap`/`DungeonWorld` 생성, 계층 이동, FOW/object state, snapshot 및 v1~v4 save/load/migration.
- `CMakeLists.txt`, `CMakePresets.json`, locale JSON, CTest 등록, 설치/CPack 문서와 현재 검증 증거의 정합성.
- 사용자가 명시한 캐릭터 생명주기, 전체 퀘스트, 월드 생성·영속성·소멸·재생성·멱등성은 제외하지 않았다.

## 3. Excluded and Uninspected Scope

- 생성기·벤더·외부 의존성 소스는 소스 감사에서 제외했다. CMake의 FetchContent 선언과 프로젝트가 소유한 packaging 문서만 확인했다.
- `docs/audit/**`, `docs/multi_audit/1/**`, `docs/multi_audit/2/**`, `docs/multi_audit/3/`의 다른 보고서는 읽지 않았다. 현재 배정된 보고서와 `audit_run.json` 외 보고서 결론을 사용하지 않았다.
- Windows hosted 실행, clean Windows 10/11 runtime, OS high-DPI, IME, 장시간/30~60분 플레이, 법률·지원 주체는 이 환경에서 재실행하지 않았다. 문서가 인용한 과거 증거의 source SHA와 현재 tree의 관계만 대조했다.
- 새 소스·테스트·설정·제품 문서는 수정하지 않았다. 이 보고서 파일만 생성 대상이다.
- 새 configure/build는 수행하지 않고 기존 `build/release` 산출물을 사용했다. 따라서 현재 working tree가 그 산출물과 완전히 동일한 build provenance인지는 별도 finding으로 남겼다.

## 4. Evidence Examined

- 문서: `spec.md`, `designs.md`, `DESIGN_DECISIONS.md`, `IMPLEMENTATION_SUMMARY.md`, `README.md`, `CHANGELOG.md`, `BUILD_GUIDE.md`, `audit_roadmap.md`, `tasks/plan.md`, `tasks/todo.md`, `LESSONS_LEARNED.md`, `tests/fixtures/save_v1.json`, `tests/fixtures/README.md`.
- 핵심 구현: `src/model/{Character,RecruitmentDraft,Quest,Party,DungeonMap,DungeonWorld}.cpp`, `include/model/{Character,Quest,Party,DungeonMap,DungeonWorld}.hpp`, `src/controller/{CharacterCreationState,CharacterInfoState,DungeonState,TownState,QuestJournalState,CombatState,CombatStateActions}.cpp`, `src/view/DungeonRenderer.cpp`, `src/core/{Game,Persistence,SessionRng}.cpp`.
- 검증 코드: `tests/test_content_contracts.cpp`, `tests/test_controller_contracts.cpp`, `tests/test_agency_contracts.cpp`, `tests/test_combat_contracts.cpp`, `tests/test_localization_contracts.cpp`, `tests/test_ui_state_raster.cpp`, `src/test_harness.cpp`.
- 빌드/등록: `CMakeLists.txt:1-340`, `CMakePresets.json:1-36`, `assets/lang/*.json` (5개 파일 모두 452 keys).
- 실행 명령 및 결과:
  - `git status --short`: v0.10 캐릭터/월드/퀘스트 소스와 문서가 현재 working tree에서 변경·추가 상태임을 확인했다.
  - `git diff --stat 927753278f46b92a015197ee229edce4f52e0657`: 46개 tracked 파일, 2320 insertions/434 deletions의 변경을 확인했다.
  - `ctest --test-dir build/release -N`: 13개 CTest 등록을 확인했다.
  - `ctest --test-dir build/release --output-on-failure`: `LastTest.log`에서 13개 test 모두 `Test Passed`를 확인했다(최장 `UiStateRasterTests` 약 38초).
  - `rg`로 `findPath`/`stepAutoMove`/`checkCurrentTileLog`, quest report, world generation 호출 경로와 locale key를 역추적했다.
  - `git show 4f988483bf5cbcfdce4c79a6aabab4a67a7043f9:CMakeLists.txt`: hosted 증거가 가리키는 source가 `project(Crawlmaster VERSION 0.9.4)`임을 확인했다.
  - 기존 `build/release/ui-state-raster-evidence/en-scale-100-character-details.png`를 육안 판독해 poison fixture가 있어도 캐릭터 상태/버프 행이 표시되지 않음을 확인했다.

## 5. Findings

### [A04-F001] 마스터 스펙의 town-only checkpoint/FOW 문구와 v0.10 full-world persistence가 동시 유효함

- Pass: Implementation
- Pattern: `IMP-004`, `SPEC-GAP-001`
- Area: 권위 문서, save/checkpoint/FOW 계약, supersede 계보
- Severity: Major
- Status: Needs Clarification
- Summary: `spec.md`는 자신을 절대적인 master plan으로 선언하면서도, 동결 결정에서는 마을·종결 결과에서만 checkpoint를 기록하고 활성 던전 FOW를 재개하지 않는다고 적고, 뒤의 v0.10 섹션에서는 월드 snapshot과 FOW를 저장하고 최대 2초마다 합쳐 저장한다고 적는다. `DESIGN_DECISIONS.md`는 구 결정에 v0.10 대체 주석을 달았지만 master `spec.md:53`에는 같은 supersede 표시가 없다.
- Evidence:
  - `spec.md:3-6`은 이 문서를 모든 구현·설정의 절대 master plan으로 정의한다.
  - `spec.md:53-54`는 “마을과 종결 결과에서만 checkpoint” 및 “활성 던전 좌표/FOW/전투 중간 재개는 지원하지 않는다”고 명시한다.
  - `spec.md:203-208`은 새 게임 귀속 3층 월드, FOW/object state 보존, Continue/마을 재입장 시 월드 유지 등을 명시한다.
  - `spec.md:345-353`은 schema v4 full snapshot과 탐험 변경의 최대 2초 저장을 명시한다.
  - `DESIGN_DECISIONS.md:47`은 구 월드 저장 범위가 15번 결정으로 대체됐다고 적지만, 구 `town checkpoint` 문장 자체는 `DESIGN_DECISIONS.md:53-55`에 남아 있다.
  - 구현은 `src/controller/DungeonState.cpp:230-243,363-377`에서 dirty world를 주기적으로 저장하고, `src/model/Party.cpp:102-156`에서 v4 world snapshot을 쓴다.
- Expected Basis: 최신 v0.10 사용자 목표와 `spec.md:203-208,345-353`에 따라 full world/FOW persistence가 effective contract인지, 또는 `checkpoint`와 autosave를 구분하는지 master 문서가 명시해야 한다. 기대 동작을 임의로 확정하지 않는다.
- Actual: 코드·README·IMPLEMENTATION_SUMMARY·v0.10 roadmap은 full world snapshot을 구현 사실로 취급하지만, master spec의 동결 핵심 결정은 여전히 town-only/FOW non-resume처럼 읽힌다.
- Impact: 코더와 감사자가 탐험 FOW 손실을 허용할지, TPK/정상 종료에서 어느 snapshot을 authoritative로 볼지 다르게 해석할 수 있다. 이 모순을 해소하지 않으면 persistence 및 TPK gate의 PASS 기준을 재현할 수 없다.
- Suggested Action: `spec.md:53`에 checkpoint와 2초 world autosave의 차이, TPK가 복구하는 checkpoint 범위, FOW의 저장/재개 범위를 명시하고 v0.10 결정으로 supersede 표시를 추가한다. `audit_roadmap.md`, `DESIGN_DECISIONS.md`, `README.md`, `IMPLEMENTATION_SUMMARY.md`의 같은 용어를 한 정의로 맞춘다.
- Re-audit Method: 모든 제품 문서에서 `checkpoint`, `autosave`, `FOW`, `world snapshot`, `Continue`, `TPK`를 검색하고 한 계약만 남겼는지 확인한 뒤, 이동·층 이동·목표 해결·TPK·Continue의 save/load transcript를 해당 계약과 대조한다.
- Confidence: High
- Notes: Classification `Spec Gap / Design authority conflict`; 이 finding은 문서 권위 충돌이므로 `Needs Spec Clarification`을 유지한다.

### [A04-F002] 자동 이동이 퀘스트 보스와 최종 BossGate를 E 확인 없이 자동 활성화함

- Pass: Implementation
- Pattern: `IMP-001`, `IMP-003`
- Area: Dungeon automove, objective interaction, player agency
- Severity: Major
- Status: Confirmed
- Summary: 자동 이동은 발견된 바닥을 BFS로 이동하되 계단·목표를 자동 활성화하지 않아야 한다. 현재 경로 탐색은 모든 visited walkable 특수 타일을 허용하고, 한 칸 이동 뒤 `checkCurrentTileLog()`를 호출한다. 그 함수는 퀘스트 보스와 BossGate에서 즉시 CombatState를 push한다.
- Evidence:
  - `spec.md:31`은 자동 이동 대상을 “탐험된 빈 바닥”으로 정의한다.
  - `designs.md:60`은 자동 이동이 계단과 목표를 자동 활성화하지 않는다고 명시한다.
  - `src/model/DungeonMap.cpp:119-120,150-153`은 target 및 모든 이웃을 `isWalkable && isVisited`만으로 path에 넣어 `DOOR/UPSTAIRS/DOWNSTAIRS/BOSS_GATE`와 목표 좌표를 배제하지 않는다.
  - `src/controller/DungeonState.cpp:269-277`은 자동 이동 각 단계에서 `checkCurrentTileLog()`를 호출한다.
  - `src/controller/DungeonState.cpp:316-325`는 퀘스트 보스 칸에서 고정 전투를 push하고, `src/controller/DungeonState.cpp:334-341`은 BossGate에서 최종 전투를 push한다.
  - 퀘스트 보스 world object 좌표는 `src/model/DungeonWorld.cpp:68-76,94-100`에서 `EMPTY` 타일에 배치되므로 자동 이동 target으로 직접 선택 가능하다.
  - `src/test_harness.cpp:736-805`의 BFS 검사는 `DungeonMap` path만 확인하고 `DungeonState`의 특수 타일 자동 활성화를 검사하지 않는다.
- Expected Basis: 명세·설계의 탐험된 빈 바닥 자동 이동 및 E/목표 명시 활성화 계약.
- Actual: 발견된 quest boss marker 또는 BossGate까지 클릭하면 `stepAutoMove()` 후 CombatState가 바로 시작된다. 계단은 전이하지 않고 로그만 남지만, 보스 목표는 자동 활성화된다.
- Impact: 플레이어가 취소·E 확인 전에 고정 보스/최종 전투에 진입하며, 설계의 명시적 상호작용·자동 이동 안전 경계가 깨진다. 자동 이동 destination/중간 경로가 특수 타일을 통과하는 경우에도 같은 문제가 발생할 수 있다.
- Suggested Action: 자동 이동 target과 각 path node를 `TileType::EMPTY` 및 비목표 조건으로 제한하거나 특수 타일 직전에서 경로를 멈추고 E prompt를 유지한다. quest boss/BossGate 전용 production controller transcript에서 클릭만으로 state가 변하지 않고 E/명시 입력 후에만 전투가 시작되는지 잠근다.
- Re-audit Method: discovered quest item/NPC/boss/BossGate/stairs 각각을 destination 및 중간 node로 설정한 실제 `DungeonState` event/update transcript를 재실행하고 state stack, quest state, world object state를 확인한다.
- Confidence: High
- Notes: Classification `Design Drift / Needs Fix`; 현재 aggregate CTest green은 이 호출 경로를 다루지 않는다.

### [A04-F003] seed가 없는 v1/v2 및 seed 누락 v3 migration이 프로세스별로 다른 월드를 만든다

- Pass: Implementation
- Pattern: `IMP-001`, `DBG-002`, `TEST-001`
- Area: Legacy save migration, world determinism, idempotence
- Severity: Major
- Status: Confirmed
- Summary: 문서는 v1~v3 save가 저장된 session seed로 3층 월드를 결정론적으로 이관된다고 선언하지만, 실제 v1 fixture에는 seed가 없고 parser는 seed가 없으면 각 프로세스의 global entropy seed를 사용한다.
- Evidence:
  - `spec.md:345` 및 `IMPLEMENTATION_SUMMARY.md:62-64`는 v1~v3 migration이 저장 seed에서 결정론적으로 world를 생성한다고 선언한다.
  - `tests/fixtures/save_v1.json:1-7`은 `schemaVersion`과 `lastSessionSeed`가 없는 구형 입력이다.
  - `src/model/Party.cpp:295-302`는 v1~v3에서 `lastSessionSeed == 0`이면 `SessionRng::global().seed()`를 migration seed로 사용한다.
  - `src/core/SessionRng.cpp:10-15,26-29`의 global 초기화는 프로세스별 `std::random_device` entropy seed를 만든다.
  - `tests/test_content_contracts.cpp:314-349`가 결정성을 검증하는 케이스는 명시적 `lastSessionSeed`가 있는 schema 3뿐이다.
  - `src/test_harness.cpp:441-504`의 v1 migration은 canonical v4 필드만 확인하고 독립 프로세스 간 world snapshot equality를 확인하지 않는다.
- Expected Basis: 같은 legacy save는 같은 world snapshot으로 이관되어야 한다는 `spec.md:345` 및 `tasks/plan.md:10,18-20`의 deterministic migration 계약. seed 부재 시 fallback은 문서로 먼저 결정해야 한다.
- Actual: seedless legacy save를 두 프로세스에서 읽으면 서로 다른 random global seed로 `DungeonWorld::generate()`가 호출된다. 첫 Continue 이후 저장하면 새 v4 snapshot이 생겨 이후에는 고정되지만, migration 자체가 멱등하지 않고 최초 world가 process-dependent다.
- Impact: 같은 기존 save의 지형·목표 위치·탐험 경험이 실행 프로세스마다 달라진다. v1 fixture를 계속 지원한다고 문서화한 이상 save-bound world의 핵심 불변조건이 깨진다.
- Suggested Action: seedless legacy 입력에 대한 versioned deterministic fallback(예: 명시된 고정 migration seed 또는 legacy payload hash)을 `spec.md`에 정의하고 구현한다. migration 시 chosen seed를 `lastSessionSeed`로 커밋하며, v1/v2/seedless-v3 fixture를 독립 프로세스에서 두 번 load해 `world.toJson()`과 migrated seed가 같은지 검증한다.
- Re-audit Method: 동일 fixture를 서로 다른 process seed로 두 번 `Party::loadFromFile()`하고 world snapshot/derived object coordinates/seed를 비교한 뒤, 한 번 저장하고 다시 Continue해 값이 변하지 않는지 확인한다.
- Confidence: High
- Notes: Classification `Design Drift / Needs Fix`; fallback 선택 자체는 명세 보완이 필요하지만 현재 process-random fallback은 deterministic contract와 직접 충돌한다.

### [A04-F004] v4 floor snapshot validator가 Door landmark와 완전한 shape를 검증하지 않는다

- Pass: Implementation
- Pattern: `IMP-001`, `IMP-003`, `TEST-001`
- Area: World snapshot validation, malformed save rejection, map idempotence
- Severity: Major
- Status: Confirmed
- Summary: 생성기는 각 층에 정확히 하나의 Door landmark를 만들지만 v4 load validator는 U/down/BossGate 개수만 검사한다. 따라서 Door가 사라지거나 여러 개가 된 snapshot도 로드·재저장된다.
- Evidence:
  - `spec.md:174-181,203-208,422`는 20x20 map, Door/계단/BossGate, 3층 full snapshot과 변조 입력 거부를 요구한다.
  - `src/model/DungeonMap.cpp:335-363`는 `placeLandmarks()`에서 Door 하나와 farthest gate를 생성한다.
  - `src/model/DungeonMap.cpp:409-453`의 `fromJson()`은 `upCount`, `downCount`, `bossCount`만 세고 Door count를 세지 않는다.
  - `src/model/DungeonMap.cpp:455-469`는 U 위치·연결성은 검사하지만 Door 존재/유일성은 검사하지 않는다.
  - `tests/test_content_contracts.cpp:223-243`의 Door count 검사는 새로 생성된 map에만 적용되고, `tests/test_content_contracts.cpp:374-385`의 malformed case는 외벽을 깨는 경우 하나뿐이다.
- Expected Basis: 생성된 v4 snapshot의 canonical landmark shape 및 `audit_roadmap.md:75`의 변조 입력 거부 계약.
- Actual: Door code를 `.`로 바꾸거나 내부 `.`를 `D`로 바꾼 v4 floor는 다른 invariant가 모두 맞으면 `DungeonWorld::fromJson()`이 받아들인다. 이후 `toJson()`도 malformed map을 canonical처럼 보존한다.
- Impact: landmark가 소멸하거나 중복되어 문서가 약속한 던전 구조가 달라진다. round-trip equality가 “유효한 canonical snapshot”을 보증하지 못한다.
- Suggested Action: floor validator에 Door 정확히 1개, 올바른 walkable/reachable 조건을 추가하고 필요하면 spawn/visited/stepped invariants도 명시한다. Door 삭제·중복·특수 타일 변조를 각각 reject하는 negative fixture와 full Party save→load→save semantic equality를 추가한다.
- Re-audit Method: 생성 snapshot의 각 tile code를 단일 변조한 뒤 `DungeonWorld::fromJson()` status를 확인하고, 정상 snapshot의 3층 tile/visited/stepped/object state를 save/load/re-save해 비교한다.
- Confidence: High
- Notes: Classification `Design Drift / Needs Fix`; current test verifies generated Door but not load boundary.

### [A04-F005] 완료된 회수 퀘스트에 중요품이 남은 불가능한 save를 parser가 허용한다

- Pass: Implementation
- Pattern: `IMP-001`, `IMP-003`
- Area: Quest one-time report invariant, key-item persistence
- Severity: Major
- Status: Confirmed
- Summary: 보고 시 Moon Seal key item을 소비해야 하지만, v4 parser는 completed retrieve quest의 `key_moon_seal` 잔존을 거부하지 않는다. 반대로 active ready retrieve quest에 key가 있어야 한다는 한 방향 검사만 있다.
- Evidence:
  - `spec.md:198-201`은 현장 달성 후 성 보고 시 보상을 정확히 한 번 지급하고 중요품은 보고 시 소비한다고 명시한다.
  - `src/model/Party.cpp:222-233`는 `key_moon_seal`의 형식·중복만 검사한다.
  - `src/model/Party.cpp:304-325`는 active retrieve가 ready이면 key가 있어야 하고 completed object는 RESOLVED여야 함을 검사하지만, completed retrieve에서 key가 없어야 한다거나 key가 active ready retrieve에 귀속되어야 한다는 역방향 조건은 검사하지 않는다.
  - `src/model/Party.cpp:471-473`에서 정상 `completeQuest()` 실행 시에만 key를 제거한다.
  - `tests/test_content_contracts.cpp:95-103`은 동일 프로세스의 정상 direct completion 후 key 제거만 확인하고, completed+key v4 load reject를 확인하지 않는다.
- Expected Basis: v4 save canonical postcondition과 `audit_roadmap.md:75-77`의 변조 입력 거부/완료 원장·중요품 일회성 계약.
- Actual: `completedQuestIds`에 `qst_recover_moon_seal`, `keyItems`에 `key_moon_seal`, 해당 object state `resolved`를 넣은 payload는 현재 consistency checks를 통과한다.
- Impact: 저장 상태가 보고 완료와 중요품 소비를 동시에 표현해 quest journal/key count 및 향후 migration의 의미를 깨뜨린다. parser가 canonical state를 보장하지 못해 one-time report invariant가 저장 경계에서 약해진다.
- Suggested Action: key item의 생명주기 양방향 invariant를 추가한다. key가 있으면 active retrieve quest가 READY_TO_REPORT이고 object가 RESOLVED여야 하며, completed retrieve quest 또는 보고 후에는 key가 없어야 한다. 정상 보고·재로드와 불가능한 completed+key/ orphan-key payload를 각각 regression test한다.
- Re-audit Method: active ready, active unresolved, completed report, orphan key, completed+key의 v4 fixture를 load하고 status·quest·object·key를 비교한다.
- Confidence: High
- Notes: Classification `Design Drift / Needs Fix`; key item은 일반 inventory가 아니므로 정상 드롭/판매 경로로 정당화할 수 없다.

### [A04-F006] 세 목적형 퀘스트의 production 성 보고 및 정확히 한 번 보상은 테스트로 닫히지 않았다

- Pass: Debug / Engineering Quality
- Pattern: `TEST-001`, `IMP-003`
- Area: Quest end-to-end coverage, completion claims
- Severity: Major
- Status: Confirmed
- Summary: 현재 문서와 tasks는 세 목적형 퀘스트·보고 rollback·일회성 보상을 구현 완료로 표시하지만, 실제 production `TownState` report 경로는 Moon Seal 한 건의 rollback만 테스트된다. Crypt Warden과 Missing Scout의 성 보고, 각 보상/XP/완료 원장/중복 보고는 검증되지 않았다.
- Evidence:
  - `spec.md:198-201`, `audit_roadmap.md:73-78`, `tasks/todo.md:4-9`는 세 퀘스트와 현장 달성·성 보고·one-time 보상을 완료 기준으로 둔다.
  - `IMPLEMENTATION_SUMMARY.md:76`은 “세 목적형 퀘스트, 보고 rollback”을 production-linked 계약 테스트로 확인했다고 주장한다.
  - `tests/test_controller_contracts.cpp:461-502`는 세 object를 현장에서 resolve하고 ready 상태와 world snapshot을 확인하지만 Castle로 돌아가 세 퀘스트를 보고하지 않는다. helper `:45-68`은 실제 경로 대신 map position을 직접 설정한다.
  - `tests/test_controller_contracts.cpp:504-532`의 production Town report/rollback은 `qst_recover_moon_seal` 한 건뿐이다.
  - `tests/test_content_contracts.cpp:82-104`는 retrieve direct API 한 건, `:106-129`는 legacy KILL quest의 duplicate reward만 확인한다.
  - 실제 report mutation은 `src/controller/TownState.cpp:262-291`, reward/key/completed mutation은 `src/model/Party.cpp:424-483`에 있다.
- Expected Basis: `TEST-001`의 구체적 실패 모드 회귀 기준과 사용자 핵심 목표의 전체 quest lifecycle. 테스트가 없는 부분은 완료로 승격하지 않는다.
- Actual: CTest 13/13은 통과하지만 각 canonical quest에 대해 `accept -> field resolve -> return Castle -> report -> save/load -> repeat report`를 production event로 검증하지 않는다. 현장 resolve에서 reward가 지급되지 않는다는 assertion도 세 퀘스트 전체에 없다.
- Impact: 특정 quest type의 report selection, 두 개 reward item, XP 분배, key 소비, duplicate prevention 또는 save failure rollback이 깨져도 현재 gate가 녹색일 수 있다. `tasks/todo.md`의 all-x와 summary의 완료 주장이 증거보다 강하다.
- Suggested Action: 세 ID 각각에 production `TownState` keyboard transcript를 추가해 field 단계 reward 불변, Castle report 보상/XP/key/completed ID, 두 번째 report no-op, save failure rollback을 검증한다. 실제 이동/층 전이는 helper direct set 대신 가능한 범위에서 controller event 또는 별도 headless driver로 잠근다.
- Re-audit Method: 세 quest를 독립 seed/temp save에서 순서별로 수행하고 report 전후 JSON bytes/state, reload 후 state, repeat report 결과를 비교한다. 각 save failure injection도 모든 quest type에 적용한다.
- Confidence: High
- Notes: Classification `Spec-complete claim unsupported / Needs Fix`; direct model tests와 production controller coverage를 혼합해 PASS로 올리지 않는다.

### [A04-F007] 캐릭터 생성→실제 플레이→사망/TPK 생명주기 E2E 증거가 없다

- Pass: Debug / Engineering Quality
- Pattern: `TEST-001`, `IMP-003`
- Area: Character lifecycle, creation persistence, play/death gate
- Severity: Major
- Status: Confirmed
- Summary: 명세의 성공 기준은 4명 생성 후 저장, 던전 플레이, 전투 사망/TPK 복구까지지만 테스트는 한 명 Aria 생성과 모델 단위 poison death, 이미 죽인 캐릭터를 넣은 TPK stack test로 분리되어 있다. 생성된 4인 파티가 실제 Dungeon/Combat/Save/TPK 경로를 통과하는 증거가 없다.
- Evidence:
  - `spec.md:27-38`은 4명 생성·저장, 던전 전투, 상태 저장, New/Continue/TPK를 완료 정의로 둔다.
  - `spec.md:139-172`는 identity/point-buy/class trait/status/death 수명주기를 정의한다.
  - `tests/test_controller_contracts.cpp:288-330`은 `Aria` 한 명만 CharacterCreationState로 생성하고 `party.getMemberCount()==1`만 확인한다.
  - `tests/test_agency_contracts.cpp:66-115`은 한 개 `RecruitmentDraft`/Rogue의 direct API만 확인한다.
  - `src/test_harness.cpp:925-935`는 별도 새 Rogue에 poison을 직접 넣어 death를 확인한다.
  - `tests/test_controller_contracts.cpp:143-165`는 TPK 전에 저장 member를 `savedMember->takeDamage(savedMember->getMaxHp())`로 이미 죽여 놓고 전투 update가 GameOver로 바뀌는지만 확인한다.
  - 실제 production 연결은 `src/controller/CharacterCreationState.cpp:181-197`, `src/controller/DungeonState.cpp:125-160,312-341`, `src/controller/CombatState.cpp:367-373`에 있다.
- Expected Basis: 사용자 핵심 목표와 `spec.md:27-38`, `tasks/plan.md:9-14`의 production-linked deterministic verification.
- Actual: class별 creation gear/skills와 실제 전투 피해로 인한 individual death→TPK→checkpoint restore의 한 런 transcript가 없다. 취소/잘못된 identity 후 bytes 불변도 state UI에서 직접 확인되지 않는다.
- Impact: creation save failure, class별 초기 상태, combat에서 생성 캐릭터의 death, last town checkpoint restore 및 stale DungeonState 제거의 연결 결함이 aggregate model tests에 숨을 수 있다.
- Suggested Action: 별도 temp save에서 4개 class를 실제 CharacterCreationState 입력으로 생성·confirm하고 저장한 뒤 DungeonState→CombatState에서 deterministic damage/poison으로 한 명 및 전원을 죽이는 transcript를 추가한다. TPK 후 stack root, restored HP/status/world/quest bytes와 Continue 경로를 확인하고 creation cancel/save failure도 별도 검증한다.
- Re-audit Method: Debug/Release에서 같은 controller transcript를 독립 실행하고 4인 identity/class/gear/skills, saved JSON, combat death, GameOver root, reloaded checkpoint를 비교한다.
- Confidence: High
- Notes: Classification `Spec-complete claim unsupported / Needs Fix`; direct model poison test는 actual gameplay lifecycle의 대체 증거가 아니다.

### [A04-F008] v4 save loader가 생성 계약보다 느슨해 잘못된 캐릭터 이름을 영속화한다

- Pass: Implementation
- Pattern: `IMP-001`, `IMP-003`
- Area: Character identity boundary, Unicode/name validation, save migration
- Severity: Major
- Status: Confirmed
- Summary: 신규 생성은 Unicode 1~16자, 앞뒤 공백 제거, 제어문자 금지여야 하지만 save loader는 이름의 empty/byte length만 검사한다. 또한 draft validator도 ASCII space 양끝만 거부하고 trim/Unicode whitespace semantics를 구현하지 않는다.
- Evidence:
  - `spec.md:139-145`는 name trim, Unicode 1~16자, control character 금지 및 identity validation을 요구한다.
  - `src/model/RecruitmentDraft.cpp:26-29,123-161`은 draft 입력을 검사하지만 `name.front/back() == ' '`만 양끝 공백으로 처리하고 문자열을 trim하지 않는다.
  - `src/controller/CharacterCreationState.cpp:172-179,122-129`는 입력을 draft에 전달할 뿐, 제출 시 `setName()`이 false이면 거부한다.
  - `src/model/Character.cpp:460-480`의 `fromJson()`은 name이 empty이거나 `name.size()>64`인지만 거부하고 Unicode code point 수, 제어문자, 양끝 Unicode whitespace를 검사하지 않는다.
  - `tests/test_agency_contracts.cpp:117-135`는 ASCII empty/leading/trailing space와 17 ASCII만 검사하고, `src/test_harness.cpp:536-557`는 v3 gender/age만 검사한다.
- Expected Basis: 신규 character contract와 v3/v4 canonical save boundary. Legacy identity unknown(age=0/gender=unspecified)는 허용하되 name의 유효성 규칙은 별도 예외로 정의되지 않았다.
- Actual: v4 JSON에 17개 Unicode code point, 제어문자/개행 또는 U+3000 양끝 공백을 넣어도 `Character::fromJson()`이 통과할 수 있고, 이후 `Party::saveToFile()`가 이를 canonical v4로 재저장한다.
- Impact: 저장 파일이 UI/log multiline injection 및 identity contract drift를 일으킬 수 있고, 생성 화면과 Continue 후 캐릭터 의미가 달라진다.
- Suggested Action: 공통 UTF-8/code-point name validator를 draft와 `fromJson()`에서 재사용하고 trim 정책(삭제인지 거부인지)을 spec에 명시한다. legacy save도 허용할 예외와 canonical v4 normalization을 분리하고 Unicode whitespace/control/17-codepoint negative tests를 추가한다.
- Re-audit Method: valid 1/16 code point, 17, malformed UTF-8, control/newline, ASCII/Unicode leading/trailing whitespace를 draft와 v4 load 양쪽에 넣어 동일한 결과와 no-mutation을 확인한다.
- Confidence: High
- Notes: Classification `Design Drift / Needs Fix`; byte length 64 검사는 Unicode 1~16 계약을 대체하지 않는다.

### [A04-F009] CharacterInfoState가 문서화된 독·마비·버프 상태를 그리지 않는다

- Pass: Implementation
- Pattern: `IMP-001`
- Area: Character sheet UI, status/buff visibility
- Severity: Major
- Status: Confirmed
- Summary: 설계 문서는 CharacterInfoState에 현재 상태이상과 버프를 표시하도록 요구하지만, 현재 normal/large layout은 HP/XP/AC/ability/equipment만 그리고 status/buff API를 사용하지 않는다.
- Evidence:
  - `designs.md:291-302`는 `[OK]`, `[POISONED]`, `[PARALYZED]`, STR buff 등의 상태/버프 표시와 상태별 색상을 명시한다.
  - `src/controller/CharacterInfoState.cpp:188-225`의 normal detail layout은 HP/XP/AC, abilities, equipment만 렌더링한다.
  - `src/controller/CharacterInfoState.cpp:321-347`의 large layout도 identity/class/HP/XP/AC/abilities/equipment만 렌더링한다.
  - `src/controller/CharacterInfoState.cpp`에는 `getPoisonTurns()`, `getParalysisTurns()`, `getBlessTurns()`, `getStrBuffAmount()`, `getDexBuffAmount()` 호출이 없다. 해당 값은 `src/view/DungeonRenderer.cpp:325-331` HUD에서만 사용된다.
  - `tests/test_ui_state_raster.cpp:69-79`의 sample character는 `poisonTurns: 2`를 갖지만, 기존 `build/release/ui-state-raster-evidence/en-scale-100-character-details.png`에는 poison/status/buff 행이 없다.
- Expected Basis: `designs.md:291-302`의 명시적 CharacterInfo UI 계약. Dungeon HUD 표시만으로 별도 캐릭터 시트 요구를 충족했다고 보지 않는다.
- Actual: CharacterInfoState의 상태 알림(`m_statusMsg`)은 아이템 사용/운영 메시지일 뿐 selected character의 poison/paralysis/bless/STR/DEX buff 상태를 표시하지 않는다.
- Impact: 플레이어가 캐릭터 시트에서 치료·버프·사망 상태를 확인할 수 없고, 문서의 status visibility와 실제 UI가 어긋난다.
- Suggested Action: normal/large layout에 locale key 기반 상태/버프 행을 추가하고 turns/active values를 표시한다. 건강/독/마비/버프 fixture의 production raster와 text assertion을 추가한다.
- Re-audit Method: 상태 조합(OK, poison, paralysis, bless, STR/DEX buff, dead)을 CharacterInfoState draw에 넣어 100/200%와 5 locale에서 expected labels/turns/color/overflow를 확인한다.
- Confidence: High
- Notes: Classification `Design Drift / Needs Fix`; current HUD status coverage is a separate surface.

### [A04-F010] active roadmap와 CHANGELOG가 v3/1층/0.9.4와 v4/3층/0.10을 섞어 gate authority를 흐린다

- Pass: Implementation
- Pattern: `IMP-002`, `IMP-004`, `DOC-BACKFILL-001`
- Area: Release roadmap, changelog chronology, schema/version authority
- Severity: Major
- Status: Confirmed
- Summary: `audit_roadmap.md`는 현재 gate 문서라고 선언하지만 P0/Turn 2 완료 항목에 schema v3·한 층을 남긴 채 같은 문서 아래 v0.10 schema v4·3층 gate를 추가했다. Unreleased CHANGELOG도 0.10 항목을 중복 작성하고 v3/old one-floor 문장을 같은 Unreleased 섹션에 남겼다.
- Evidence:
  - `audit_roadmap.md:6,11`은 과거 목록이 아닌 현재 판정 문서이며 현재 Needs Fix 0이라고 주장한다.
  - `audit_roadmap.md:23-29`의 P0 gate는 `FIN-F010: save schema v3`를 요구한다.
  - `audit_roadmap.md:55-64`의 Turn 2 완료는 `save schema v3`와 “seeded 한 층”을 현재 완료 증거처럼 적는다.
  - `audit_roadmap.md:73-80`은 같은 문서에서 v0.10 schema v4/3층 snapshot을 새 gate 및 local 13/13 결과로 적는다.
  - 현재 authority인 `spec.md:13,17,345-353`, `CMakeLists.txt:2-4`, `IMPLEMENTATION_SUMMARY.md:3-4,62-78`은 v0.10/3층/v4를 사용한다.
  - `CHANGELOG.md:9-17`에는 동일한 `0.10.0 persistent world and objective quests` heading이 두 번 있고, `CHANGELOG.md:39,46,49`에는 한 층·schema v3·town-checkpoint 문장이 동일한 `[Unreleased]` 아래 남아 있다.
  - `spec.md:64-65`의 Controller 목록은 현재 CMake에 등록된 `CharacterCreationState`, `QuestJournalState`, `GameOverState`, `VictoryState` 책임을 완전하게 열거하지 않는다(`CMakeLists.txt:58-82`).
- Expected Basis: 현재 활성 gate는 한 schema/version/phase를 사용하고, 과거 상태는 명확히 historical/superseded로 표시해야 한다(`IMP-002`, `IMP-004`).
- Actual: 같은 현재 문서 묶음에서 코더가 v3/1층 기준으로 gate를 닫을 수도, v4/3층 기준으로 gate를 닫을 수도 있다. duplicate Unreleased heading은 chronology와 변경 의도를 더 흐린다.
- Impact: 잘못된 migration/phase gate를 PASS로 올리거나 v0.10 검증 누락을 “Needs Fix 0”으로 오판할 수 있다. 문서의 완료 주장과 실제 tree를 양방향으로 추적하기 어렵다.
- Suggested Action: roadmap의 v1~v2 baseline을 `Historical / Superseded by v0.10`으로 격리하고 active P0/P1을 v4/3층/현 테스트 기준으로 재작성한다. CHANGELOG의 중복 0.10 heading을 하나로 합치고 v3/1층 문장은 해당 historical release로 이동하거나 superseded 표기를 붙인다. spec의 controller roster와 책임표도 보강한다.
- Re-audit Method: active 문서에서 `schema v3`, `schemaVersion: 3`, `한 층`, `town-only`, `0.9.4` 검색 결과가 historical context 외에 남지 않는지 확인하고, active claims마다 source/test/command를 연결한다.
- Confidence: High
- Notes: Classification `Documentation Drift / Needs Fix`; CHANGELOG historical sections may retain old facts, but these instances are in an active Unreleased/current gate context.

### [A04-F011] 현재 v0.10 working tree의 hosted Windows/SLSA 증거가 구 source 4f를 가리킨다

- Pass: Implementation
- Pattern: `IMP-003`, `BUILD-001`
- Area: Release evidence provenance, packaging, current-tree claims
- Severity: Major
- Status: Confirmed
- Summary: 현재 문서가 hosted MSVC/package/SLSA gate를 Verified로 서술하면서 인용하는 source `4f988...`는 `project VERSION 0.9.4` 및 한 층 code 상태다. 현재 감사 기준은 HEAD 927 + v0.10 uncommitted working tree이므로 그 hosted artifact가 새 CharacterCreation/QuestJournal/DungeonWorld/schema v4 변경을 증명하지 않는다.
- Evidence:
  - `spec.md:15`, `BUILD_GUIDE.md:118-119`, `IMPLEMENTATION_SUMMARY.md:85-86`, `CHANGELOG.md:34,61`은 run `33786241695`/source `4f988...` hosted evidence를 인용한다.
  - `git show 4f988483bf5cbcfdce4c79a6aabab4a67a7043f9:CMakeLists.txt:1-4`는 해당 source가 `project(Crawlmaster VERSION 0.9.4)`임을 보여준다.
  - `git show 4f988483bf5cbcfdce4c79a6aabab4a67a7043f9:spec.md`의 lane/schema는 한 층·schema v2이고, 현재 `CMakeLists.txt:1-4`, `spec.md:13,345`, `IMPLEMENTATION_SUMMARY.md:4,62`는 v0.10/3층/v4다.
  - `git status --short`와 `git diff --stat HEAD`는 v0.10 source/docs/tests가 current tree 변경으로 남아 있음을 보여준다.
- Expected Basis: `IMP-003`/`BUILD-001`의 완료·배포 증거는 같은 source tree/commit의 artifact, test, package를 가리켜야 한다.
- Actual: 현재 local `build/release`에서 13 CTest는 통과했지만, 문서가 Verified로 인용하는 hosted MSVC/SLSA evidence는 v0.10 diff 이전 source에 귀속된다. 현재 v0.10 Windows gate는 이 보고서 범위에서 `UNVERIFIED`다.
- Impact: 새 캐릭터/퀘스트/월드 persistence가 Windows package와 attestation에 포함됐다고 과대주장할 수 있다. release readiness 및 SBOM/provenance 판단이 source mismatch로 오염된다.
- Suggested Action: v0.10 변경이 포함된 immutable commit을 만들고 hosted MSVC build/test/package/SLSA/SPDX를 그 SHA로 재실행하거나, current docs에서 v0.10 hosted claims를 `UNVERIFIED`로 낮춘다. artifact manifest/checksum에 exact source SHA를 필수로 연결한다.
- Re-audit Method: hosted artifact의 embedded/version/test list/package tree/attestation subject SHA가 current v0.10 commit과 일치하는지 검증하고, Windows CTest/package/startup을 재실행한다.
- Confidence: High
- Notes: Classification `Evidence Provenance Drift / Needs Fix`; 과거 0.9.4 gate가 통과했다는 사실 자체를 기각하는 finding이 아니다.

### [A04-F012] UI 최소 글자 크기 계약과 실제 scaling clamp가 다르다

- Pass: Implementation
- Pattern: `IMP-001`, `TEST-001`
- Area: Accessibility typography, raster gate
- Severity: Minor
- Status: Confirmed
- Summary: 최신 designs remediation 계약은 본문 16px·보조문 14px 미만을 금지하지만, `LocalizationManager`는 최소 14px로 clamp하고 CharacterInfo/CharacterCreation 등은 10~14 base size를 사용한다.
- Evidence:
  - `designs.md:350`은 본문 16px, 보조문 14px를 최소값으로 명시한다.
  - `include/core/LocalizationManager.hpp:51-57`의 `getScaledTextSize()`는 `std::max(14U, ...)`만 보장한다.
  - `src/controller/CharacterInfoState.cpp:188-225,259-282`는 base 10/11/12/14를 사용한다.
  - `src/controller/CharacterCreationState.cpp:253-262`는 body 12/14, status 10/12를 사용한다.
  - `tests/test_localization_contracts.cpp:88-95`는 75/100/200% 산술 결과만 검사하고 16px body minimum을 검사하지 않는다.
- Expected Basis: `designs.md:350` 및 audit roadmap의 최소 text size/accessibility gate.
- Actual: 100%에서 base 12는 14px로, base 10은 14px로 그려져 본문 16px 계약을 충족하지 않는다. 75%에서도 clamp 14px가 적용되어 문서의 scale policy와 최소 body policy가 분리되어 있지 않다.
- Impact: 문서가 약속한 접근성 최소치와 실제 화면이 다르고, 200% compact layout 판단 기준도 일관되지 않다.
- Suggested Action: 본문/보조/장식 역할별 scale helper를 분리해 body minimum 16, secondary minimum 14를 강제하거나, 제품 계약을 실제 최소치로 명시적으로 낮춘다. raster test에 role별 effective pixel size와 overflow/wrap assertion을 추가한다.
- Re-audit Method: 각 State/substate의 text role과 effective character size를 75/100/200%에서 계산하고 representative raster bounds/overlap을 확인한다.
- Confidence: High
- Notes: Classification `Design Drift / Needs Fix`; 문서를 낮추는 경우 접근성 gate의 사람 승인/범위를 함께 갱신해야 한다.

### [A04-F013] 사용되지 않는 `ItemFactory::getShopCatalog()`가 8종 구매 계약과 충돌할 가능성이 있다

- Pass: Implementation
- Pattern: `IMP-001`, `DOC-BACKFILL-001`
- Area: Item acquisition API, shop catalog, orphan implementation
- Severity: Minor
- Status: Needs Clarification
- Summary: spec/design은 직접 구매를 8종으로 고정하지만, `getShopCatalog()`라는 공개 API는 고급 장비와 특수 소모품까지 반환한다. 현재 `TownState`는 이를 사용하지 않고 8종 배열을 별도로 하드코딩하므로 두 catalog source가 분리되어 있다.
- Evidence:
  - `spec.md:494`, `designs.md:320-322`는 구매 catalog를 기본 8종으로 제한하고 나머지는 drop/quest 보상으로만 얻도록 한다.
  - `include/model/ItemFactory.hpp:19-21`은 `getShopCatalog()`를 “상점에서 판매 가능한 모든 아이템 리스트”로 공개한다.
  - `src/model/ItemFactory.cpp:88-109`은 `wpn_greatsword`, `wpn_staff`, `wpn_rapier`, `arm_plate`, `shd_tower`, 고급/특수 소모품 등 18개를 반환한다.
  - `src/controller/TownState.cpp:391-405` 및 `:161-172`는 `getShopCatalog()`가 아니라 별도 8종 array/keys를 사용한다.
  - `rg -n 'getShopCatalog' src include tests`에서 정의 외 production/test consumer를 찾지 못했다.
- Expected Basis: 직접 구매 catalog와 inventory sell/drop registry의 의미를 하나의 canonical source로 정해야 한다. 제품 요구가 ambiguous하면 창작하지 않는다.
- Actual: 현재 runtime Town 구매는 8종이지만, 이름·주석상 구매 catalog로 해석 가능한 dead API는 금지된 direct acquisition을 노출한다. 함수가 sellable-all 의미라면 이름/문서가 그 의미를 명시하지 않는다.
- Impact: 향후 caller가 이 API를 사용하면 고급 아이템 direct purchase가 열려 acquisition contract가 깨질 수 있고, 현재도 code-only orphan과 문서 drift가 남는다.
- Suggested Action: `getShopCatalog()`를 canonical 8종 구매 목록으로 수정하고 `getRegisteredIds()`/별도 sellable registry를 분리하거나, 이 API가 판매 가능 전체를 뜻한다면 이름·주석·문서·테스트를 명확히 바꾼다. Town의 hard-coded array를 canonical API와 하나로 만든다.
- Re-audit Method: 모든 item ID에 shop buy/starter/drop/quest reward/sell source를 표로 만들고 runtime API와 tests가 동일한 8종/19종 구분을 사용하는지 확인한다.
- Confidence: Medium
- Notes: Classification `Intentional but Undocumented or Orphan Code`; current Town buy path itself is 8종, so this finding is not claiming an already reachable forbidden UI path.

## 6. Uncertainties and Clarifications Needed

- `spec.md:53`의 `checkpoint`가 full world autosave와 다른 개념인지, 아니면 v0.10에서 완전히 supersede된 것인지 명시가 필요하다(A04-F001).
- seed 없는 v1/v2 및 seed 누락 v3 save를 지원할 때 deterministic fallback seed의 정책을 문서로 결정해야 한다(A04-F003). 현재 process entropy fallback은 확인됐지만 요구되는 fallback 값은 창작하지 않았다.
- `getShopCatalog()`의 “판매 가능한 모든 아이템”이 구매 목록인지 sellable inventory registry인지 문서/이름으로 확정되지 않았다(A04-F013).
- 이 보고서에서는 Windows hosted gate, clean OS runtime/high-DPI, IME, 장기 플레이를 실행하지 못했으므로 current v0.10 shipping PASS를 판정하지 않았다.

## 7. Perspective Decision

**HOLD for this perspective.** Current `build/release` CTest 13/13 is green, and locale key sets are parity-complete, but the perspective retains Major contract/coverage findings: master checkpoint/FOW authority is unresolved, automove violates the no-auto-activation contract, seedless migration is non-deterministic, v4 validation accepts malformed/impossible states, and the claimed character/quest lifecycle evidence is incomplete. The current v0.10 hosted release claims are not proven by the cited older source SHA.
