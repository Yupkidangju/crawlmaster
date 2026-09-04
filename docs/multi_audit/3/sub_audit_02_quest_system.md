# Sub Audit Report

## 1. Audit Metadata

- Audit Turn: 3
- Perspective: A02 — Quest System 전체 (계약·상태·보상·저장·UI/i18n)
- User Goal: `$multi-audit 추가한 기능이 많음. 1. 캐릭터 시스템을 생성부터 플레이 사망까지 모두 전체 점검 2. 퀘스트 시스템을 전체 점검 3. 맵 생성 및 세이브/로드시 멱등성, 소멸, 재생성 기타 모든 부분 싹다 점검 4. 문서상 요청된 내용이 잘 구현되고 모순되거나 문제되는 점이 있으면 점검`
- Audit Basis: Standard-backed
- Standard Path: `/mnt/Projects_SSD/cpp/crawlmaster/AI_AUDIT_DOC_STANDARD.md`
- Report Contract: `/home/eunho1/.codex/skills/multi-audit/references/report-contract.md`
- Target HEAD: `927753278f46b92a015197ee229edce4f52e0657`
- Target Tree: 위 HEAD와 현재 working tree의 퀘스트 관련 변경을 함께 조사함. 보고서 외 제품 파일은 수정하지 않음.

## 2. Assigned Scope

다음 퀘스트 계약과 실제 소비 경로를 독립적으로 대조했다.

- `QuestType` 및 canonical ID/type/target/targetFloor/reward 계약
- 신규 `qst_recover_moon_seal`, `qst_defeat_crypt_warden`, `qst_find_missing_scout`의 수주, 동시 활성화, 현장 달성, 보고, 정확히 한 번의 보상
- legacy `qst_clear_kobolds`, `qst_collect_maces`, `qst_hunt_spiders`의 v1~v3 로드·활성·완료/보고 경로
- 중요품의 일반 인벤토리 분리, 판매·사용·장착 방지와 보고 시 소비
- 퀘스트 월드 오브젝트(`PRESENT|DISCOVERED|RESOLVED`), 보스/NPC/item 조건, inactive 상태 상호작용, 발견 후 미니맵 표식
- `Party` 저장·로드·migration·checkpoint/rollback 및 `CommittedDurabilityUnknown`
- Town quest board, `QuestJournalState`, Dungeon HUD/minimap, 5 locale와 placeholder
- dead/TPK, campaign completion, New Game/Continue와 퀘스트 상태의 상호작용
- CMake의 production-linked 테스트 편입 및 결정적 테스트 증거

## 3. Excluded and Uninspected Scope

- 사용자 지시대로 `docs/audit/**`와 `docs/multi_audit/1/**`, `docs/multi_audit/2/**`, 그리고 `docs/multi_audit/3/`의 다른 보고서는 읽지 않았다.
- 생성·벤더·의존성 트리와 기존 `build/**` 산출물 내부 소스는 조사하지 않았다. 기존 build에 대해서는 구성 상태 확인만 했다.
- Windows/macOS 실기, high-DPI, 장시간 플레이, 패키지/법률 gate는 이 개별 관점의 직접 실행 범위가 아니다.
- 현재 working tree가 build 산출물보다 새롭고, 사용자 계약이 새 산출물 생성·변경을 금지하므로 fresh CMake build/CTest와 GUI raster 실행은 하지 않았다. `ctest --test-dir build -N`은 `Total Tests: 0`을 반환했다.
- 보고서 작성 전후 product source/test/config/document 변경은 수행하지 않았다.

## 4. Evidence Examined

### 권위 문서

- `spec.md`: 10.4 퀘스트 구조(196-201), 10.4.1 월드 상태(203-208), schema/migration·내구성·RNG 정책(344-358), save contract(359-422), item/quest registry(492-520), Phase 11 및 검증 기준(547-584)
- `designs.md`: Dungeon/quest marker·input(55-77), Turn 1 UI/저장 실패·i18n 계약(326-357)
- `DESIGN_DECISIONS.md`: 3층 영속 월드와 목적형 퀘스트 결정(163-168)
- `IMPLEMENTATION_SUMMARY.md`: 파일 책임 및 현재 구현·검증 주장(25-90)
- `CHANGELOG.md`: v0.10.0 영속 월드·목적형 퀘스트 및 legacy migration 주장(7-18, 36-61)
- `README.md`, `BUILD_GUIDE.md`, `audit_roadmap.md`, `tasks/plan.md`, `tasks/todo.md`, `LESSONS_LEARNED.md`
- `tests/fixtures/README.md`, `tests/fixtures/save_v1.json`

### 구현

- `include/model/Quest.hpp`, `src/model/Quest.cpp`
- `include/model/Party.hpp`, `src/model/Party.cpp`
- `include/model/DungeonWorld.hpp`, `src/model/DungeonWorld.cpp`
- `include/model/DungeonMap.hpp`, `src/model/DungeonMap.cpp`
- `include/controller/TownState.hpp`, `src/controller/TownState.cpp`
- `include/controller/DungeonState.hpp`, `src/controller/DungeonState.cpp`
- `include/controller/QuestJournalState.hpp`, `src/controller/QuestJournalState.cpp`
- `include/controller/CombatState.hpp`, `src/controller/CombatState.cpp`, `src/controller/CombatStateActions.cpp`
- `src/view/DungeonRenderer.cpp`, `src/core/SessionRng.cpp`, `src/core/Persistence.cpp`, `src/core/Game.cpp`, `src/controller/TitleState.cpp`
- `assets/lang/{ko,en,ja,zh_tw,zh_cn}.json`

### 테스트·설정

- `tests/test_content_contracts.cpp`
- `tests/test_controller_contracts.cpp`
- `tests/test_localization_contracts.cpp`
- `tests/test_ui_state_raster.cpp`, `tests/test_font_raster.cpp`, `src/test_harness.cpp`
- `CMakeLists.txt`, `CMakePresets.json`

### 실행·정적 확인 명령

- `git rev-parse HEAD`: 지시된 HEAD 확인
- `git diff --check`: whitespace 오류 없음
- `g++ -std=c++20 -Wall -Wextra -Werror -pedantic -Iinclude -Ibuild/_deps/json-src/include -Ibuild/_deps/sfml-src/include -fsyntax-only ...` (퀘스트·월드·Town/Dungeon/Combat 소스): 성공
- 5개 locale JSON 정합성 스크립트: 각 452 key, quest 관련 72 key, key/placeholder parity mismatch 없음
- `rg`로 기존 `TOWN_MSG_QUEST_{KOBOLD,MACE,SPIDER}_*` 9개 key의 `src/include/tests` 소비자를 확인: 소비자 없음
- `ctest --test-dir build -N`: 기존 구성 build는 `Total Tests: 0`; 현재 working tree 대상 실행 증거로 승격하지 않음

### 확인된 정상 범위

- `Quest::getCanonicalIds()`에는 legacy 3개와 신규 3개가 있고 `getOfferableIds()`에는 신규 3개만 있다(`src/model/Quest.cpp:59-66`).
- `TownState`는 offerable 3개를 보드에 내고 active legacy만 추가하며, `DungeonState`는 inactive object를 `DUNGEON_QUEST_REQUIRED`로 차단한다(`src/controller/TownState.cpp:32-40,262-291`, `src/controller/DungeonState.cpp:316-328,421-445`).
- 신규 item/NPC와 고정 boss의 정상 현장 달성은 controller transcript에서 확인되는 경로가 있다(`tests/test_controller_contracts.cpp:461-502`).
- 중요품은 `m_inventory`와 별개의 `m_keyItems`에 저장되고 일반 ItemFactory 경로에 들어가지 않는다(`src/model/Party.cpp:222-233,538-551`, `src/controller/DungeonState.cpp:433-436`).
- 정상 보고는 `completedQuestIds`를 먼저 재수주 차단 기준으로 사용한다(`src/model/Party.cpp:418-435,475-482,555-557`).

## 5. Findings

### Pass 1: Implementation Compliance Findings

### [A02-F001] seed 없는 v1/v2/v3 이관이 프로세스 entropy에 의존해 동일 legacy save의 월드가 달라짐

- Pass: Implementation
- Pattern: IMP-002, IMP-003
- Area: legacy migration / worldSeed / quest-object placement
- Severity: Major
- Status: Confirmed (Needs Fix)
- Summary: 저장된 seed가 없는 legacy save를 읽을 때 월드 생성 seed를 현재 process의 global entropy에 의존한다. 따라서 동일한 v1/v2 save를 다른 process에서 Continue하면 지형과 세 목표 위치가 달라지고, Title 경로에서는 migration 전에 생성한 월드 seed와 새로 기록한 `lastSessionSeed`가 서로 달라질 수 있다.
- Evidence:
  - `tests/fixtures/save_v1.json:1-9`에는 `schemaVersion`, `lastSessionSeed`, `world`가 없다.
  - `src/model/Party.cpp:286-302`는 schema 1~3의 `lastSessionSeed`가 0이면 `SessionRng::global().seed()`를 `migrationSeed`로 사용한다.
  - `src/core/SessionRng.cpp:10-15,26-28`의 global 초기 seed는 `std::random_device` entropy다.
  - `src/controller/TitleState.cpp:68-87`는 seed가 0인 save를 로드한 뒤 `startNewGlobalSession()`을 호출하고 이미 생성된 `m_world`를 다시 생성하지 않은 채 `saveToFile()`한다. `Party::saveToFile()`는 `src/model/Party.cpp:102-110`에서 현재 global seed를 `lastSessionSeed`에 기록한다.
  - `spec.md:204-205,345,356`는 session seed/worldSeed 및 저장 seed 기반의 결정론적 v1~v3 이관을 요구한다.
  - `tests/test_content_contracts.cpp:314-349`는 명시적 non-zero seed를 가진 v3만 같은 process에서 비교하며, seed 없는 v1/v2 cross-process 증거가 없다.
- Expected Basis: `spec.md`의 v1~v3 결정론적 이관, New Game의 session seed=`worldSeed`, save checkpoint의 seed 계약. 기대되는 동일 seed 외의 구체적인 fallback 알고리즘은 문서에 없다.
- Expected: seed가 없는 legacy 입력에도 문서화된 안정적인 migration seed를 정하고, 같은 byte 입력은 process와 실행 시점이 달라도 같은 3층 월드/목표 위치로 이관되어야 한다. 이관 후 `world.seed`와 `lastSessionSeed` 관계도 일관되어야 한다.
- Actual: v1 fixture 및 seed가 0인 v2/v3 입력은 process entropy로 월드를 생성한다. 이후 Title Continue는 다른 entropy seed를 session checkpoint로 저장할 수 있어 world snapshot과 RNG checkpoint가 분리된다.
- Impact: legacy active quest를 보고하기 전까지의 월드와 오브젝트 위치가 실행마다 달라진다. 구세이브의 map/quest migration을 재현할 수 없고, 첫 Continue 이후 저장된 seed만 보고 월드 재현을 시도하면 서로 다른 월드가 된다.
- Suggested Action: legacy 파일에 seed가 없을 때 사용할 deterministic derivation(예: canonical legacy bytes hash 등)을 제품 문서에서 확정하고, world 생성 전에 같은 값을 `lastSessionSeed`/migration metadata로 고정한다. Title은 migration world가 이미 생성된 경우 별도 entropy로 RNG seed를 덮어쓰지 않도록 조정한다. v1/v2/v3 active KILL/COLLECT fixture를 별도 process에서 비교하고 `world.seed`·`lastSessionSeed` 불변식을 단언한다.
- Re-audit Method: seed 없는 v1 fixture와 seed=0 v2 fixture를 두 독립 process에서 로드하여 `world.toJson()` byte/semantic equality를 비교한다. Title Continue 후 v4의 `world.seed`, `lastSessionSeed`, active legacy quest를 재로드하고 동일성을 확인한다.
- Owner: Architect / Coder
- Confidence: High
- Notes: 명시적 seed가 있는 v3 경로 자체는 `tests/test_content_contracts.cpp:317-339`에서 정상으로 보인다. 결함 범위는 seed sentinel/부재 migration이다.

### [A02-F004] 중요품이 어떤 active/completed objective와도 연결되지 않은 상태로 로드되어 회수 퀘스트를 영구 정지시킬 수 있음

- Pass: Implementation
- Pattern: IMP-001, IMP-003
- Area: key-item ownership / objective-world consistency
- Severity: Major
- Status: Confirmed (Needs Fix)
- Summary: v4 loader는 `keyItems`에 알려진 문자열인지와 중복 여부만 검사하고, `key_moon_seal`이 회수 퀘스트의 `READY_TO_REPORT` 상태와 연결되는지 역방향으로 검사하지 않는다. orphan 중요품이 로드되면 같은 quest를 새로 수주해도 `addKeyItem()`이 false가 되어 현장 달성이 영구히 막힌다.
- Evidence:
  - `src/model/Party.cpp:222-233`은 `key_moon_seal`만 허용하며 active/completed quest 관계를 확인하지 않는다.
  - `src/model/Party.cpp:304-325`는 active `RETRIEVE_KEY_ITEM`이 ready일 때 key item이 있는지만 검사한다. key item이 있을 때 해당 active-ready quest가 있어야 한다는 역방향 검사는 없다.
  - `src/controller/DungeonState.cpp:421-435`는 active quest를 확인한 후 `addKeyItem()`이 false이면 목표를 완료하지 않고 반환한다.
  - `src/model/Party.cpp:538-545`의 `addKeyItem`은 이미 보유한 `key_moon_seal`에 대해 false를 반환한다.
  - `spec.md:200-201,498`은 중요품이 회수 목표의 현장 결과이고 보고 시 소비된다고 정의한다. `spec.md:351`은 신규 초기 중요품이 없음을 정의한다.
- Expected Basis: key item은 회수 quest의 현장 결과로만 존재하고, ready 상태에서 보고되면 소비되어야 한다. 저장 변조 입력은 `audit_roadmap.md:73-78`의 v0.10.0 gate에 따라 거부되거나 복구 불가능한 orphan을 만들지 않아야 한다.
- Expected: `key_moon_seal`은 (a) active `qst_recover_moon_seal`가 ready이고 world object가 resolved인 경우에만 보유되거나, (b) 제품이 별도 허용 상태를 명시하는 경우 그 상태와 함께 검증되어야 한다. 그 외 standalone key item은 Corrupt로 격리되어야 한다.
- Actual: standalone key item save가 로드된다. 이후 사용자는 보드에서 `qst_recover_moon_seal`을 수주할 수 있지만 object 상호작용은 `addKeyItem` false로 멈추고, UI상 key item을 제거할 경로가 없다.
- Impact: 저장 변조/부분 손상 또는 잘못된 외부 호출이 퀘스트를 완료 불능으로 만든다. 현재 loader의 semantic validation이 이름·타입 검증보다 약해 영속 상태가 게임 진행을 고아화한다.
- Suggested Action: key item의 허용 상태를 canonical quest/world relation으로 고정하고, loader에서 key→active-ready quest→resolved object 및 completed quest→no key를 양방향 검증한다. orphan fixture와 수주 후 재진입 회귀 테스트를 추가한다.
- Re-audit Method: 유효한 v4 save에서 `keyItems`만 `key_moon_seal`로 설정하고 active/completed quest를 비운 입력, active non-ready 입력, completed 입력을 각각 로드한다. 모두 Corrupt/quarantine되는지와 정상 ready save만 round-trip되는지 확인한다.
- Owner: Architect / Coder
- Confidence: High
- Notes: 정상 controller 경로는 quest를 먼저 active로 만든 뒤 key를 추가하므로 통상 플레이에서는 드러나지 않는다. 이 finding은 저장 경계와 고장 복구를 포함한다.

### [A02-F005] 완료 원장이 unknown/duplicate quest ID를 canonical registry 없이 수용함

- Pass: Implementation
- Pattern: IMP-001
- Area: completedQuestIds / canonical ID contract
- Severity: Minor
- Status: Confirmed (Needs Fix)
- Summary: active quest는 중복 ID와 completed overlap을 검사하지만, `completedQuestIds`는 문자열 길이만 검사하고 canonical registry 소속 여부와 중복 array를 검사하지 않는다.
- Evidence:
  - `src/model/Party.cpp:261-273`은 non-empty/최대 길이만 확인하고 `unordered_set::insert` 결과를 무시해 duplicate를 deduplicate한다.
  - 같은 파일 `276-283`은 active quest에 대해서는 별도 duplicate와 completed overlap을 검사한다.
  - `src/model/Quest.cpp:59-66,68-100`은 canonical/offerable registry를 별도로 제공하지만 completed ledger parser가 사용하지 않는다.
  - `spec.md:498,520`, `spec.md:575`는 완료 ID가 canonical quest의 중복 보상을 차단하는 원장임을 정의한다.
- Expected Basis: canonical registry에 없는 완료 ID는 orphan ledger이므로 변조 입력으로 거부되어야 한다. 완료 ID 배열의 duplicate도 canonical 저장 형식에 맞지 않으므로 거부하거나 명시적 canonicalization 계약이 있어야 한다.
- Expected: 모든 completed ID가 `Quest::getCanonicalIds()`에 속하고 배열 내 중복이 없으며, active/completed overlap도 현재처럼 거부되어야 한다.
- Actual: `completedQuestIds:["qst_future_orphan","qst_future_orphan"]` 같은 입력이 길이 조건만 만족하면 로드되고, unknown ID는 계속 `m_completedQuestIds`에 남는다.
- Impact: 현재 UI에서 즉시 보상 중복은 막히지만, 완료 원장에 orphan이 남아 향후 같은 ID가 registry에 추가될 때 정상 quest를 영구 차단할 수 있다. save/load가 malformed input을 조용히 정규화한다.
- Suggested Action: completed ID마다 canonical registry 검사를 추가하고 duplicate는 Corrupt 처리한다. legacy 세 ID는 registry에 계속 포함하므로 호환성을 잃지 않는다.
- Re-audit Method: unknown ID, duplicate canonical ID, active/completed overlap을 각각 v4 fixture로 로드하고 unknown/duplicate만 Corrupt, 정상 canonical set만 round-trip되는지 확인한다.
- Owner: Coder
- Confidence: High

### [A02-F006] v4 active legacy quest의 currentCount 누락을 0으로 조용히 복원해 진행도를 잃음

- Pass: Implementation
- Pattern: IMP-001, IMP-003
- Area: v4 quest schema / mutable progress validation
- Severity: Major
- Status: Confirmed (Needs Fix)
- Summary: `Quest::fromJson()`은 schema v4에서도 `currentCount`가 없으면 0으로 기본값을 사용한다. 저장 중 필드가 손상되거나 삭제된 active 수량형 quest가 Corrupt로 격리되지 않고 로드되어 기존 처치 진행도를 잃는다.
- Evidence:
  - `src/model/Quest.cpp:120-132`에서 canonical ID로 정의를 복원한다.
  - `src/model/Quest.cpp:134-155`는 type/target/reward 값이 있을 때만 canonical mismatch를 검사한다.
  - `src/model/Quest.cpp:158-165`는 `j.value(currentCountKey, 0)`를 사용하고 범위만 검사한다. 따라서 v4 `{"id":"qst_clear_kobolds"}`는 current count 0으로 수용된다.
  - `src/model/Quest.cpp:166-173`도 `readyToReport`가 없으면 false를 기본값으로 둔다.
  - `Quest::toJson()`은 `src/model/Quest.cpp:102-117`에서 currentCount/readyToReport를 항상 쓴다.
  - v4 save contract의 수량 필드는 `spec.md:395-404`에 있고 변조 입력 거부 gate는 `audit_roadmap.md:73-78`에 있다. 기존 tamper test는 값 mismatch만 확인한다(`src/test_harness.cpp:1204-1221`).
- Expected Basis: v4 canonical wire shape의 mutable progress는 필수이며, 누락은 기본 진행도 추정이 아니라 Corrupt/quarantine이어야 한다. v1~v3 alias/legacy tolerance와 v4 필수 필드는 분리되어야 한다.
- Expected: v4 active quest에서 `currentCount`와 objective의 `readyToReport`가 누락되면 로드 실패로 처리하고, legacy schema에 대해서만 문서화된 migration default를 허용해야 한다.
- Actual: 누락 필드는 canonical definition의 값으로 채워지고, 특히 legacy KILL/COLLECT 진행도가 0으로 조용히 바뀐다. target/type/reward도 존재할 때만 검증된다.
- Impact: 부분 저장/수동 변조/향후 serializer drift가 quest 진행도 손실로 이어진다. 사용자는 이미 처치한 legacy 임무를 다시 처음부터 수행해야 하고, 데이터 손상 사실도 알 수 없다.
- Suggested Action: schemaVersion >= 4에 대해 mutable fields의 presence/type/range를 명시적으로 검사하고, 누락 fixture를 추가한다. immutable canonical fields를 ID로 재구성할지 wire에서 요구할지는 spec에 기록하되 currentCount는 재구성하지 않는다.
- Re-audit Method: 유효한 v4 save에서 active legacy quest의 currentCount만 삭제하고 로드한다. Corrupt/quarantine 및 기존 primary/backup 비파괴가 확인되어야 한다. ready objective에서 readyToReport 삭제도 별도 확인한다.
- Owner: Architect / Coder
- Confidence: High

### [A02-F007] DISCOVERED 상태가 map visited와 묶이지 않아 fog 안에 목표 표식이 나타날 수 있음

- Pass: Implementation
- Pattern: IMP-001
- Area: world-object state / discovery marker
- Severity: Minor
- Status: Confirmed (Needs Fix)
- Summary: loader는 object 좌표가 walkable인지와 quest 관계만 검사하고 `DISCOVERED|RESOLVED` 상태가 해당 층의 visited tile과 일치하는지 검사하지 않는다. renderer도 `DISCOVERED`와 active quest만 확인하므로, unvisited tile에 조작된 discovered object가 있으면 검은 fog 위에 표식이 그려진다.
- Evidence:
  - `src/model/DungeonWorld.cpp:180-205`는 canonical object ID/quest/target/kind/floor, 위치·중복만 검사하며 map `isVisited(object.x, object.y)`를 검사하지 않는다.
  - `src/model/Party.cpp:304-325`는 object state와 active/completed/ready 관계만 확인하고 discovered→visited 불변식을 확인하지 않는다.
  - `src/view/DungeonRenderer.cpp:185-214`는 `object.state == DISCOVERED`와 `party.hasQuest`만으로 marker를 draw하며 map visited를 재확인하지 않는다.
  - `spec.md:207`, `designs.md:58-60`은 목표가 시야로 발견된 뒤에만 표식이 남는다고 정의한다.
  - raster fixture는 반대로 object 위치를 discovered로 만들면서 같은 tile을 visited로 설정한다(`tests/test_ui_state_raster.cpp:127-135`); hidden/unvisited negative case는 없다.
- Expected Basis: discovered marker는 field-of-view/visited 상태와 함께 유지되어야 하며, resolved object도 정상 progression에서 방문된 좌표에 있다.
- Expected: `DISCOVERED` 이상 상태는 해당 object 좌표가 visited인 경우에만 로드되거나 draw되어야 한다. 불일치 save는 Corrupt로 격리하거나 renderer에서 방어적으로 숨겨야 한다.
- Actual: map tile이 unvisited여도 object state만 DISCOVERED이면 loader가 수용하고 renderer가 marker를 그린다.
- Impact: 목표 위치가 fog 경계를 무시하고 노출되며, object state와 탐험 상태의 round-trip 의미가 달라진다. 저장 변조 시 문서의 discovery gate를 우회한다.
- Suggested Action: loader와 renderer 양쪽에서 discovered/resolved→visited 불변식을 잠그고, PRESENT/DISCOVERED/RESOLVED와 active/completed quest 조합의 negative fixture를 추가한다.
- Re-audit Method: 유효한 world snapshot에서 각 object 좌표의 visited bit를 0으로 만들고 state를 discovered로 둔 뒤 Party load와 minimap raster를 실행한다. 로드 거부 또는 marker 미출력을 확인한다.
- Owner: Coder
- Confidence: High

### [A02-F008] runtime 수주 API가 canonical registry를 강제하지 않아 unsaveable/custom reward quest를 허용함

- Pass: Implementation
- Pattern: IMP-001, IMP-002
- Area: quest construction/acceptance boundary
- Severity: Info
- Status: Needs Clarification
- Summary: production Town은 `Quest::createCanonical()`만 사용하지만 `Quest` 생성자와 `Party::acceptQuest()`는 public이고 ID/type/target/reward 일치 여부를 검사하지 않는다. 따라서 현재 문서가 custom quest extension을 허용하는지, 모든 runtime quest를 canonical registry로 제한하는지 결정이 필요하다.
- Evidence:
  - `include/model/Quest.hpp:26-28`의 arbitrary constructor가 public이다.
  - `src/model/Party.cpp:418-421`은 null/중복/완료 ID만 검사하고 canonical definition과 비교하지 않는다.
  - `src/controller/TownState.cpp:275-276`의 현재 사용자 경로는 `Quest::createCanonical()`을 호출한다.
  - `src/model/Quest.cpp:120-151`은 저장 로드 시에만 canonical ID 및 type/target/reward mismatch를 거부한다.
  - `tests/test_content_contracts.cpp:108-128`, `src/test_harness.cpp:681-687`은 public constructor로 canonical payload를 만들어 API를 직접 exercise한다.
- Expected Basis: `spec.md:197-200,513-520` 및 Turn 1 계약은 신규 보드와 legacy registry를 canonical 목록으로 정의하지만 custom quest serialization 정책은 정의하지 않는다.
- Expected: 정책이 canonical-only라면 수주 시 definition을 canonicalize/검증해 unsaveable payload를 막아야 한다. custom quest를 지원한다면 registry·locale·reward serialization 계약을 문서화하고 loader도 같은 정책을 가져야 한다.
- Actual: in-memory custom quest는 수주·진행·보상이 가능하지만 canonical mismatch가 포함된 save는 다음 load에서 Corrupt가 된다.
- Impact: 현재 UI에서는 노출되지 않으나 도구/향후 호출자가 quest ID를 spoof하거나 저장 불가능한 reward를 생성할 수 있다. 계약이 닫히지 않아 확장 시 orphan 목표·보상이 생길 수 있다.
- Suggested Action: canonical-only와 extensible registry 중 하나를 spec/설계에 명시하고 acceptance, save/load, locale, reward 테스트를 같은 정책으로 맞춘다.
- Re-audit Method: 정책 확정 후 custom ID, canonical ID with wrong type/reward, valid legacy/new payload를 acceptance→save→load까지 테스트한다.
- Owner: Architect
- Confidence: High
- Notes: 제품 결함으로 단정하지 않고 문서 authority가 부족한 항목으로 분류했다.

### [A02-F009] 5 locale에 구 legacy quest 메시지가 남아 있으나 runtime 소비자가 없는 orphan 번역

- Pass: Implementation
- Pattern: IMP-004, DOC-BACKFILL-001
- Area: i18n catalog / legacy compatibility
- Severity: Info
- Status: Confirmed (Documentation/cleanup)
- Summary: `TOWN_MSG_QUEST_KOBOLD_*`, `TOWN_MSG_QUEST_MACE_*`, `TOWN_MSG_QUEST_SPIDER_*` 9개 key가 5 locale에 모두 존재하지만 현재 source/include/tests에는 소비자가 없다. 현재 legacy quest도 `QUEST_QST_*` 이름/설명과 generic report key를 사용한다.
- Evidence:
  - `assets/lang/{ko,en,ja,zh_tw,zh_cn}.json:209-217`에 9개 구 legacy 메시지가 존재한다.
  - `rg`로 각 9개 key를 `src include tests`에서 검색한 결과 asset catalog 외 소비자가 없었다.
  - `src/model/Quest.cpp:69-83`은 legacy quest에 `QUEST_QST_*_NAME/DESC`를 사용한다.
  - `src/controller/TownState.cpp:273-285`는 `TOWN_MSG_QUEST_ALREADY_COMPLETED`, `TOWN_MSG_QUEST_ACCEPTED`, `TOWN_MSG_QUEST_REPORTED`, `TOWN_MSG_QUEST_IN_PROGRESS`만 사용한다.
  - locale parity 자체는 5개 파일 각 452 key로 일치했고 quest 관련 placeholder mismatch는 없었다.
- Expected Basis: 사용자 목표의 orphan 번역 확인 및 `designs.md:352`의 실제 화면 key 계약. 과거 key를 호환 목적으로 보존할지에 대한 별도 문서는 없다.
- Expected: 소비하지 않는 과거 key는 제거하거나 `legacy-only` 보존 사유/소유 경로를 문서화해야 한다. key parity 통과만으로 runtime 소비를 증명하지 않아야 한다.
- Actual: 현재 catalog에는 사용되지 않는 구체적 quest 메시지가 남아 있고, 어떤 migration/UI 경로도 이를 읽지 않는다.
- Impact: 번역 유지 비용과 향후 key 변경 시 drift가 생기며, catalog completeness 검사에서 실제 제품 표면과 역사적 잔재가 구분되지 않는다. 즉시 gameplay 차단은 아니다.
- Suggested Action: legacy compatibility가 필요 없으면 제거하고, 필요하면 `legacy-only` metadata/테스트를 둔다. unused-key 검사 결과를 shipped surface와 분리한다.
- Re-audit Method: source reference scan과 legacy active quest UI transcript를 다시 실행하여 보존 사유 또는 제거 상태를 확인한다.
- Owner: Coder / Documentation
- Confidence: High

### Pass 2: Debug / Engineering Quality Findings

### [A02-F002] 보상 보고 저장 실패 rollback이 primary/backup 부재 시 domain state를 복구하지 못함

- Pass: Debug
- Pattern: DBG-001, TEST-001
- Area: quest report transaction / reward ledger / failure recovery
- Severity: Major
- Status: Confirmed (Needs Fix)
- Summary: Town 보고와 combat reward는 gold/XP/item/key/ledger를 먼저 메모리에 반영한 뒤 save한다. 저장이 실패하면 `loadFromFile()`에 의존해 복구하지만 primary와 `.bak`이 모두 없거나 둘 다 손상된 경우 복구 결과를 검사하지 않고 변이된 domain state를 남긴다.
- Evidence:
  - `src/model/Party.cpp:424-482`는 `completeQuest()`에서 gold, alive XP, collect item 제거, key 제거, reward item 추가, completed ledger 삽입, active 제거를 순서대로 수행한다.
  - `src/controller/TownState.cpp:63-74,275-285`는 보고 후 save 실패 시 `party.loadFromFile()`을 호출하지만 restore 실패 시 snapshot/in-memory undo를 수행하지 않는다.
  - `src/controller/CombatState.cpp:704-783`도 전투 gold/XP/kill progress/drop/quest object/campaign을 먼저 변경하고 save 실패 시 `party.loadFromFile()` 결과를 무시한다.
  - `src/model/Party.cpp:347-385`는 primary/backup이 없거나 복구 불가하면 실패 결과만 반환하고 이전 memory state를 재구성하지 않는다.
  - `src/core/Game.cpp:63-67`와 이후 Town/Combat 저장 경로는 남은 메모리 state가 다른 정상 저장에서 뒤늦게 commit될 수 있는 경계를 만든다. Combat에서는 dead foes가 남아 `nextTurn()`이 보상을 다시 시도할 수 있다(`src/controller/CombatState.cpp:337-365,443-503`).
  - `tasks/plan.md:16-21`과 `spec.md:349-350,358`은 저장 실패 시 마지막 정상 checkpoint 유지와 보고 rollback을 요구한다.
- Expected Basis: 보고 저장 실패는 보상·중요품·완료 원장을 함께 rollback하고, durable commit unknown만 rollback하지 않는다는 `tasks/plan.md:18-21` 및 `spec.md:358`의 구분.
- Expected: 성공 checkpoint가 없거나 복구 실패인 경우에도 보상 operation은 원자적으로 중단되어 domain state가 pre-report/pre-reward로 유지되거나, 추가 진행을 차단한 명시적 오류 상태가 되어야 한다. recovery 실패를 무시한 채 변이 state로 계속 진행하면 안 된다.
- Actual: `loadFromFile()` 실패 시 Town은 이미 완료된 quest/reward/key 소비를 memory에 남긴다. Combat은 defeated foes와 증가된 보상을 남기므로 재시도·후속 save 시 이중 보상 가능성이 있다.
- Impact: 저장 실패를 사용자에게 알린 뒤에도 보상/원장이 나중에 commit되거나, 동일 전투 재진입 시 gold/XP/drop이 중복된다. 정확히 한 번 보상과 checkpoint rollback이 보장되지 않는다.
- Suggested Action: report/reward를 pre-state snapshot 또는 transaction object로 감싸고 save 성공 후에만 commit한다. 복구 실패 시 memory snapshot으로 되돌리고 UI를 재시도/중단 상태로 고정한다. Combat retry는 encounter state도 재구성하거나 one-shot reward attempt marker를 둔다. `CommittedDurabilityUnknown`은 별도 unknown 상태로 유지하되 명시된 대로 rollback하지 않는다.
- Re-audit Method: valid save 및 primary/backup 모두 제거·손상된 save에서 Town objective report와 quest-boss victory를 각각 force IoError로 실행한다. gold/XP/inventory/key/active/completed/world/foes를 pre-state와 비교하고, 후속 save·재시도에서도 reward가 정확히 한 번인지 확인한다.
- Owner: Architect / Coder
- Confidence: High
- Notes: 현재 controller test는 valid primary가 있고 `.tmp` directory로 save만 실패하는 경로를 확인한다(`tests/test_controller_contracts.cpp:504-532,589-613`). restore 불능 경로는 확인하지 않는다.

### [A02-F003] rollback load가 Party의 RNG checkpoint 필드만 복원하고 실제 global mt19937을 되돌리지 않음

- Pass: Debug
- Pattern: DBG-002, TEST-001
- Area: save rollback / RNG checkpoint / quest-boss reward and level-up
- Severity: Major
- Status: Confirmed (Needs Fix or Spec Clarification)
- Summary: save 실패 후 `Party::loadFromFile()`는 `m_lastSessionSeed`와 `m_sessionRngDrawCount` 값을 파일에서 복원하지만 `SessionRng::global()`을 재생성하지 않는다. 따라서 quest report 중 XP level-up 주사위나 quest boss combat reward의 gold/drop draw가 실패 transaction 뒤에도 실제 global stream에서 소비된 채 남는다.
- Evidence:
  - `src/controller/CombatState.cpp:704-716,740-765`는 victory 전에 `rollGoldReward`와 drop RNG를 global stream에서 소비한다.
  - `src/model/Party.cpp:444-449`의 quest XP 분배는 `Character::addXp()`를 호출하고, level-up 시 `src/model/Character.cpp:174-197`에서 HP 증가 주사위를 global RNG로 굴린다.
  - `src/controller/CombatState.cpp:773-782`는 save 실패 후 `party.loadFromFile()`만 호출하며 global RNG restore가 없다.
  - `src/model/Party.cpp:327-336`는 load 시 memory fields만 설정한다. `src/core/SessionRng.cpp:21-24,31-33`의 `(seed,drawCount)` 재구성은 `TitleState`의 Continue 분기 `src/controller/TitleState.cpp:82-84`에서만 직접 사용된다.
  - `src/model/Party.cpp:102-110,151-152`는 이후 save 때 현재 global seed/draw count를 다시 기록한다.
  - `spec.md:356`은 seed와 raw draw count checkpoint로 Continue를 재현한다고 정의한다.
- Expected Basis: rollback이 보상/보고 operation을 취소한다면 operation에서 소비한 RNG도 checkpoint와 일치해야 한다. RNG 소비를 실패 transaction에도 보존하려는 정책이라면 그 예외와 retry semantics를 명세해야 한다.
- Expected: rollback 후 global seed/draw count와 Party checkpoint가 같은 pre-state를 가리키고, 같은 다음 roll이 fresh Continue와 in-process retry에서 일치해야 한다.
- Actual: Party fields는 old checkpoint처럼 보이지만 global stream은 combat/reward/level-up draw 이후다. 다음 save는 advanced draw count를 기록하고, fresh Continue(old file)와 현재 process retry가 서로 다른 난수를 사용한다.
- Impact: 저장 실패 후 재시도 결과와 Continue 결과가 달라지고, 보상/레벨업 HP·다음 encounter의 결정성이 깨진다. 특히 quest boss save rollback이 성공한 것처럼 보여도 RNG checkpoint가 오염된다.
- Suggested Action: rollback transaction에서 pre-seed/pre-draw를 캡처하고 성공적인 restore와 함께 `SessionRng(seed, drawCount)`로 global을 되돌린다. 만약 실패한 combat의 RNG draw를 의도적으로 소비한다면 `spec.md`에 명시하고 retry/save/test를 그 정책에 맞춰 설계한다.
- Re-audit Method: 고정 seed에서 quest-boss victory 직전 checkpoint를 저장하고 save failure를 강제한다. rollback 직후 global draw count와 다음 N개 roll을 pre-checkpoint fresh `SessionRng`와 비교한다. quest report는 XP threshold 직전 캐릭터로 HP level-up draw도 함께 검사한다.
- Owner: Architect / Coder
- Confidence: High
- Notes: 사용자 요청의 rollback 범위에 RNG를 포함할지 문서가 완전히 명시하지 않으므로 정책 선택 자체는 Clarification 대상이다. 현재 코드의 두 stream 불일치는 확인된 사실이다.

### [A02-F010] legacy active quest·inactive object·durability-unknown·quest UI의 결정적 회귀 증거가 부족하고 현재 build는 CTest 0개

- Pass: Debug
- Pattern: TEST-001, DBG-002, BUILD-001
- Area: regression coverage / executable evidence
- Severity: Minor
- Status: Confirmed (Unverified execution)
- Summary: 구현 문서와 roadmap은 v1~v3 active legacy quest 완료, 세 objective의 rollback, inactive interaction, 5 locale quest UI/raster를 완료했다고 주장하지만 현재 테스트는 일부 정상 경로와 파일 생성만 잠근다. 현재 제공된 build는 working tree보다 오래되어 CTest가 0개로 구성되어 fresh source 실행 증거가 없다.
- Evidence:
  - `IMPLEMENTATION_SUMMARY.md:70-90`, `audit_roadmap.md:73-80`은 13/13, legacy migration, rollback, quest board/journal/raster를 완료로 주장한다.
  - `src/test_harness.cpp:441-504`의 v1 테스트는 active KILL fixture의 shape migration과 v4 save shape만 확인하고 report/reward/duplicate/다른 process를 확인하지 않는다.
  - `tests/test_content_contracts.cpp:314-349`의 deterministic migration은 explicit-seed v3이고 active legacy quest가 없다.
  - `tests/test_controller_contracts.cpp:461-502`는 세 신규 quest를 active로 두고 정상 object/boss/NPC 경로를 확인하지만 inactive object, key orphan, missing checkpoint, durability-unknown report는 없다.
  - `tests/test_controller_contracts.cpp:504-532`는 valid checkpoint가 있는 Town report save failure만 확인한다. `589-613`도 valid checkpoint가 있는 combat retry다.
  - `tests/test_ui_state_raster.cpp:127-208`은 quest를 수동 구성하고 PNG 생성 성공 및 cyan player marker만 검사한다. board/journal/quest marker의 text/status/hidden negative pixel 또는 각 locale 의미 assertion은 없다.
  - `CMakeLists.txt:101-299`에는 현재 13 test registration이 보이나 `ctest --test-dir build -N` 결과는 `Total Tests: 0`; 기존 `build/CMakeCache.txt`와 binary timestamp가 current source보다 이전이다.
- Expected Basis: `audit_roadmap.md:75-78`, `spec.md:575-582`, `AI_AUDIT_DOC_STANDARD.md`의 TEST-001/DBG-002 및 `IMPLEMENTATION_SUMMARY`의 결정적 검증 주장.
- Expected: 핵심 실패 모드별 production-linked test가 current tree에서 실행되고, legacy active completion, inactive interaction, malformed semantic save, report rollback/unknown, hidden marker, locale UI 상태를 직접 assertion해야 한다.
- Actual: 정상 흐름을 확인하는 테스트는 있으나 위 negative/high-risk cases가 비어 있고, 이 턴의 현 build에서는 CTest를 실행할 수 없다.
- Impact: F001~F007 같은 결함이 aggregate green 또는 PNG 생성 성공만으로 숨을 수 있다. 현재 실행 결과만으로 구현 문서의 13/13 완료 주장을 검증할 수 없다.
- Suggested Action: fresh Debug/Release configure/build 후 current CTest를 실행하고, v1/v2 active KILL/COLLECT cross-process, no-backup rollback, RNG restore, key/object/visited semantic rejection, inactive/duplicate/report unknown transcript, 5 locale quest UI assertions를 추가한다.
- Re-audit Method: 사용자 금지 범위를 넘지 않는 fresh build 산출물 준비 후 `ctest --test-dir build/<fresh> --output-on-failure --no-tests=error`를 실행한다. 각 negative fixture의 status/bytes/state와 UI transcript/raster assertion 결과를 report에 기록한다.
- Owner: Coder / Auditor
- Confidence: High
- Notes: 이는 current source의 기능 부재를 단정하는 finding이 아니라, 현재 tree에 대한 실행 증거와 고위험 회귀 coverage가 부족하다는 finding이다.

### Pass 3: Security Findings

퀘스트 전용 네트워크/셸/인증 surface는 없었다. 저장 파일 변조와 semantic integrity는 Pass 1/2의 F004~F007로 다뤘으며, 별도 보안 finding으로 중복 기록하지 않는다.

## 6. Uncertainties and Clarifications Needed

- seed가 없는 v1/v2 legacy save의 deterministic migration seed derivation은 문서에 없다. 결정론적이어야 한다는 결과 계약은 명확하지만 hash/fixed derivation/metadata 중 선택은 Architect가 확정해야 한다(F001).
- `SessionRng` draw가 실패한 보상 transaction에서도 소비되어야 하는지 문서가 명시하지 않는다. 현재 stream과 Party checkpoint가 갈라지는 사실은 확인되며, rollback semantics 또는 intentional consumption policy를 확정해야 한다(F003).
- public arbitrary `Quest` constructor가 미래 custom quest extension인지 legacy 테스트 편의 API인지 authority가 없다(F008). canonical-only 또는 extensible registry 중 하나를 결정해야 한다.
- 9개 legacy-specific `TOWN_MSG_QUEST_*` key를 이전 실행 호환용으로 보존할 의도가 문서에 없다(F009). 현재 runtime 소비자는 없다.
- fresh current-tree Debug/Release/GUI 실행은 산출물 변경 금지로 수행하지 못했다. 기존 build의 `Total Tests: 0`은 current source PASS/FAIL 증거가 아니다(F010).

## 7. Perspective Decision

- Decision: `HOLD` (assigned quest scope)
- Rationale:
  - 정상 신규 3종 quest의 registry, 동시 활성, field/report 분리, key item 분리, completed ledger 재수주 차단, 5 locale key/placeholder parity는 정적·기존 테스트 경로에서 확인했다.
  - 그러나 seed 없는 legacy migration이 결정론적이지 않고(F001), checkpoint 부재 시 보고/전투 보상 rollback이 불완전하며(F002), rollback 후 RNG checkpoint가 실제 stream과 분리된다(F003).
  - orphan key item(F004), orphan completed ID(F005), malformed v4 progress(F006), discovery marker semantic gap(F007)가 영속 상태 integrity를 약화한다.
  - F001~F007 중 Major가 포함되고 current-tree executable evidence도 없으므로 퀘스트 scope의 PASS 계열 판정은 허용하지 않는다.

### Re-audit Checklist

- [ ] v1/v2/v3 seed 부재 fixture를 독립 process에서 반복 로드하고 migration world/seed equality 확인
- [ ] Town report와 quest-boss reward를 valid checkpoint, no primary/backup, corrupt backup, durability unknown 각각에서 검사
- [ ] rollback 후 `SessionRng::global()`의 seed/draw count와 next-roll sequence 확인; quest XP level-up draw 포함
- [ ] standalone/duplicate/unknown key item·completed ID와 malformed v4 currentCount fixture를 Corrupt/quarantine 확인
- [ ] discovered object의 unvisited tile을 로드·렌더링하여 reject 또는 marker hidden 확인
- [ ] canonical-only/custom quest 정책 확정 후 acceptance/save/load/locale/reward contract 확인
- [ ] legacy-specific translation key의 보존 사유 또는 제거 확인
- [ ] fresh current-tree Debug/Release CTest와 legacy/inactive/UI negative regression 결과 기록

### Coder Handoff

`/mnt/Projects_SSD/cpp/crawlmaster/docs/multi_audit/3/sub_audit_02_quest_system.md`를 먼저 읽고, 각 finding을 프로젝트 문서와 실제 코드에 대조하여 검증한 뒤 우선순위대로 수정하세요. 계약 변경이 필요한 경우 관련 문서를 먼저 갱신하고, 수정 후 테스트·빌드·재감사 증거를 기록하세요.
