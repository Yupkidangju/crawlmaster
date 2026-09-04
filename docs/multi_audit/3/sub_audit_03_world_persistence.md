# Sub Audit Report

## 1. Audit Metadata

- Audit Turn: 3
- Perspective: 월드 생성·던전 맵·세이브/로드·마이그레이션·월드 생명주기
- User Goal: 캐릭터/퀘스트와 연결된 3층 월드의 생성부터 플레이·사망·재입장·New Game까지, 생성 결정성·snapshot·멱등성·소멸/재생성·무결성·문서 정합성을 독립 점검한다.
- Audit Basis: Standard-backed
- Standard Path: `/mnt/Projects_SSD/cpp/crawlmaster/AI_AUDIT_DOC_STANDARD.md`; `/home/eunho1/.codex/skills/multi-audit/references/report-contract.md`
- Baseline: `HEAD 927753278f46b92a015197ee229edce4f52e0657` + 현재 working tree
- Write Boundary: 이 보고서만 작성했다. 소스·테스트·설정·제품 문서는 수정하지 않았다.

## 2. Assigned Scope

- `DungeonMap` 20x20 DFS/loop 생성, 파생 floor seed, 계단·Door·BossGate와 도달성
- `DungeonWorld` 3층 canonical object, object state, snapshot serialize/deserialize
- `Party` schema v4 저장, v1~v3 migration, world/party/quest/key-item 관계, RNG checkpoint
- `Persistence` atomic save, backup/quarantine, 손상·변조 입력과 실패 경계
- `DungeonState`, `CombatState`, `TitleState`, `TownState`, `GameOverState`, `VictoryState`의 같은 world 인스턴스 사용, 층 이동·TPK·Continue·재입장·New Game 경계
- 문서·테스트가 위 월드 계약을 실제 구현·실행 증거와 함께 닫는지

## 3. Excluded and Uninspected Scope

- 캐릭터 생성 규칙 자체와 퀘스트 보드/보상 전체 설계는 각 담당 관점의 범위로 두고, 저장·월드 관계를 검증하는 데 필요한 경계만 읽었다.
- 다른 감사 보고서(`docs/audit/**`, `docs/multi_audit/1/**`, `docs/multi_audit/2/**`, `docs/multi_audit/3/`의 다른 sub-report)는 읽지 않았다.
- Windows/macOS 실기, high-DPI, 장시간 플레이, 실제 30~60분 완주, 패키지 배포 환경은 실행하지 않았다. 프로젝트 문서의 `UNVERIFIED` 표기를 독립 PASS로 승격하지 않았다.
- 새 빌드·새 제품 산출물은 만들지 않았다. 이미 존재하던 `build/debug`·`build/release` 테스트 실행 파일만 직접 실행했고, 테스트가 만드는 임시 파일은 `/tmp` 아래에서 자체 정리됐다.
- 실제 키 입력으로 모든 3층을 이동하는 GUI 실기와 2초 dirty-save 타이머의 물리적 시간을 재현하지 못했다. 현재 controller 계약 테스트는 일부 좌표를 test access로 직접 설정한다.

## 4. Evidence Examined

### Product documents

- `spec.md:49-55,203-207,345-422,446-454,522-550`
- `DESIGN_DECISIONS.md:46-58,129-153,163-168`
- `IMPLEMENTATION_SUMMARY.md:29-44,60-80`
- `README.md:9-26,47-61`
- `BUILD_GUIDE.md:29-55,106-120`
- `audit_roadmap.md:21-40,55-80`
- `tasks/plan.md:5-21`, `tasks/todo.md:1-9`, `CHANGELOG.md:7-18,26-34,44-61`
- `tests/fixtures/save_v1.json:1-9`, `tests/fixtures/README.md:1-5`

### Source and test evidence

- `src/model/DungeonMap.cpp:29-70,233-363,365-470`
- `include/model/DungeonMap.hpp:13-98`
- `src/model/DungeonWorld.cpp:12-209`, `include/model/DungeonWorld.hpp:11-49`
- `src/model/Party.cpp:102-385,388-411,527-584`, `include/model/Party.hpp:61-100`
- `src/core/Persistence.cpp:164-253`, `include/core/Persistence.hpp:9-50`
- `src/core/SessionRng.cpp:10-69`, `include/core/SessionRng.hpp:9-26`
- `src/controller/DungeonState.cpp:15-81,230-243,312-453`
- `src/controller/CombatState.cpp:337-411,684-785`
- `src/controller/TitleState.cpp:41-106`, `src/controller/Game.cpp:60-69`
- `src/view/DungeonRenderer.cpp:128-263`
- `src/model/Quest.cpp:120-175`
- `tests/test_content_contracts.cpp:184-244,260-385`
- `tests/test_controller_contracts.cpp:143-226,461-613`
- `src/test_harness.cpp:357-367,398-534,559-667,1176-1223,1391-1464`

### Commands and results

- `./build/debug/ContentContractTests` — exit 0, `Content contract tests passed.`
- `./build/release/ContentContractTests` — exit 0, `Content contract tests passed.`
- `./build/debug/ControllerContractTests` — exit 0, `Controller contract tests passed.`
- `./build/release/ControllerContractTests` — exit 0, `Controller contract tests passed.`
- `./build/debug/TestHarness --run-all` — exit 0, all unit tests passed.
- `git diff --check` (관련 파일 제외 규칙 적용) — 출력 없음.

## 5. Findings

### [A03-F001] v1/v2 seed 부재 migration이 ambient RNG에 의존해 동일 입력에서 다른 월드를 만든다

- Area: legacy migration, world determinism, session seed
- Severity: Major
- Status: Confirmed
- Summary: 저장 입력에 `lastSessionSeed`가 없거나 0인 v1/v2는 현재 프로세스의 `SessionRng::global()` seed로 3층 월드를 생성한다. 따라서 동일한 legacy bytes를 다른 프로세스 또는 다른 global seed에서 로드하면 지형과 objective 좌표가 달라진다.
- Evidence:
  - `tests/fixtures/save_v1.json:1-9`는 `schemaVersion`과 `lastSessionSeed`가 없고, `src/test_harness.cpp:357-367`의 v2 fixture도 `lastSessionSeed: 0`이다.
  - `src/model/Party.cpp:287-302`는 legacy의 seed를 0으로 만들고 `lastSessionSeed == 0`이면 `SessionRng::global().seed()`를 migration seed로 사용한다.
  - `src/core/SessionRng.cpp:10-29`의 global 기본 seed는 `random_device`에서 온다.
  - `tests/test_content_contracts.cpp:314-349`는 저장 seed가 있는 v3만 두 번 로드해 같은 world JSON인지 확인하며, v1 test(`src/test_harness.cpp:441-506`)에는 cross-process/world equality 검사가 없다.
- Expected Basis: `spec.md:345-346`의 “v1~v3 ... 저장된 session seed에서 3층 월드를 결정론적으로 생성”, `tasks/plan.md:9-10`, 사용자 목표의 “legacy migration이 동일 입력에서 동일 world”. seed가 없는 구형 입력에 대한 deterministic fallback은 현재 문서에 닫혀 있지 않지만 ambient process state는 결정론적 fallback이 아니다.
- Actual: 같은 v1 bytes를 global seed A/B에서 읽으면 `DungeonWorld::generate(A/B)`가 실행되어 floor snapshot과 object 좌표가 달라진다. Title Continue에서도 첫 load 후 `savedSeed == 0`이면 `src/controller/TitleState.cpp:70-85`가 다시 새 entropy seed를 만든 뒤 저장하므로, 실패 후 재시도마다 legacy world가 달라질 수 있다.
- Impact: 구형 사용자 세이브의 월드가 세션/재시도마다 재생성되어 “저장에 귀속된 월드”와 migration replay를 보장하지 못한다. 완료 전 위치·안개·objective를 복구할 수 없고, 이후 저장된 v4 world도 처음 선택된 ambient seed에 종속된다.
- Suggested Action: seed가 없는 legacy schema에 대해 문서화된 고정/콘텐츠-파생 deterministic seed를 정의하고 migration에 사용한다. migration memory model의 `lastSessionSeed`와 world seed를 그 값으로 함께 채워 Title의 0-sentinel 분기를 거치지 않게 한다. v1/v2 동일 bytes를 서로 다른 global seed의 별도 프로세스에서 로드하는 회귀 테스트를 추가한다.
- Re-audit Method: v1 fixture와 seed 없는 v2 fixture를 두 개의 별도 process에서 서로 다른 `SessionRng::reseedGlobal`로 로드하고 `world.toJson()` 및 첫 v4 save를 byte/JSON 비교한다. migration 후 `lastSessionSeed == world.seed`와 party/active legacy quest 보존도 확인한다.
- Confidence: High
- Notes: 저장 seed가 있는 v3 경로와 v4 snapshot 경로 자체는 별도 테스트에서 결정적으로 보인다. 문제는 seed 부재 legacy branch에 한정된다.

### [A03-F002] schema v4 필수 필드와 v4 quest 필드를 생략해도 로드되어 진행 상태를 조용히 잃는다

- Area: schema validation, corruption/tamper rejection, progress durability
- Severity: Major
- Status: Confirmed
- Summary: canonical v4에 필수인 quest/key-item/campaign/RNG 필드가 `contains`/`value` 기본값으로 처리된다. v4 active quest entry도 여러 canonical field가 없어도 허용된다. 부분적으로 잘린 JSON이 `Corrupt`가 아니라 정상 로드되고, 다음 save가 누락된 진행을 정식 checkpoint로 덮어쓴다.
- Evidence:
  - `spec.md:359-422`는 v4의 `inventory`, `keyItems`, `members`, `activeQuests`, `completedQuestIds`, `campaignCompleted`, `lastSessionSeed`, `sessionRngDrawCount`, `world`를 canonical 구조로 열거하고 `spec.md:422`는 정확한 3층/목표 배열을 요구한다.
  - `src/model/Party.cpp:222-234`는 v4 `keyItems`를 선택 필드로, `src/model/Party.cpp:247-259`는 `activeQuests`를 선택 필드로, `src/model/Party.cpp:261-274`는 `completedQuestIds`를 선택 필드로 읽는다.
  - `src/model/Party.cpp:286-293`는 `campaignCompleted`, `lastSessionSeed`, `sessionRngDrawCount`를 `value(..., default)`로 읽고, `src/model/Party.cpp:295-302`에서 `world`만 `at`으로 요구한다.
  - `src/model/Quest.cpp:126-173`는 v4에서도 `type`, target/reward fields, `currentCount`, `readyToReport`, `targetFloor`를 대부분 `contains`/`value`로 선택 처리한다.
  - `src/model/Party.cpp:327-337`는 이런 결과를 즉시 memory state로 대입하고 `m_loadedSchemaVersion = schemaVersion`으로 설정한다. v4이므로 `needsSaveMigration()`도 false다.
- Expected Basis: `spec.md:345-358`의 손상 파일 격리·자동 초기화 금지 및 v4 canonical save contract, `audit_roadmap.md:73-77`의 변조 입력 거부/월드·진행 보존. 제품이 생성한 v4에는 위 필드가 항상 존재하므로 누락은 허용된 migration 형태가 아니다.
- Actual: 예를 들어 유효한 v4의 `activeQuests`, `completedQuestIds`, `keyItems`, `campaignCompleted`, RNG fields를 삭제하고 world objects를 모두 `PRESENT`로 둔 입력은 파싱되어 빈 quest/key state·false campaign·0 checkpoint로 로드된다. `activeQuests` entry가 `id`와 `readyToReport` 일부만 남아도 `Quest::fromJson`이 canonical 객체를 생성해 진행도 0/default로 받아들인다. 이후 `saveToFile()`이 손실 상태를 v4로 저장한다.
- Impact: 파일 truncation/변조가 quarantine되지 않고 기존 완료·중요품·퀘스트·campaign/RNG 진행을 되돌릴 수 없는 정상 save로 덮는다. `lastSessionSeed` 누락은 world snapshot은 그대로 둔 채 Continue RNG만 새로 선택하게 만든다.
- Suggested Action: v4는 top-level required keys와 JSON type을 모두 `at`/엄격 schema validator로 요구하고, v4 `Quest::fromJson`도 모든 canonical fields와 registry 값을 required로 검증한다. 누락/추가 허용 정책이 필요한 legacy는 v1~v3 branch로만 격리한다. 누락 field별 no-mutation/quarantine 회귀 테스트를 추가한다.
- Re-audit Method: 유효한 v4 save에서 각 required key를 하나씩 제거하고 `loadFromFile()`이 `Corrupt`를 반환하며 quarantine path를 만들고 Party memory/원본 backup을 보존하는지 확인한다. 최소 quest entry(`{"id": ...}`)와 missing RNG/campaign/keyItems도 동일하게 검사한다.
- Confidence: High
- Notes: `world` 자체는 `at("world")`로 요구되므로 이 finding은 top-level world가 존재하고 나머지 핵심 필드가 정상인 부분 save에도 적용된다.

### [A03-F003] world snapshot loader가 Door/BossGate/계단의 canonical semantic과 입구 탐험 invariant를 검증하지 않는다

- Area: map snapshot integrity, reserved tile validation, reachability semantics
- Severity: Major
- Status: Confirmed
- Summary: loader는 행 크기·외벽·계단/관문 개수·전체 연결성만 확인하고, generated map의 Door 존재/개수, DOWNSTAIRS/BOSS_GATE가 canonical 최장 거리 tile인지, 입구의 visited/stepped 초기 invariant를 확인하지 않는다.
- Evidence:
  - `src/model/DungeonMap.cpp:335-363`는 생성 시 Door를 하나 배치하고 최장 거리 tile을 floor 1/2의 `DOWNSTAIRS`, floor 3의 `BOSS_GATE`로 지정한다.
  - `src/model/DungeonMap.cpp:57-65`는 `(1,1)`을 `UPSTAIRS`, `visited=true`, `stepped=true`, player spawn으로 만든다.
  - `src/model/DungeonMap.cpp:451-467`의 `fromJson` 검사는 `upCount/downCount/bossCount`, `(1,1)`의 U, 모든 walkable cell의 도달성만 확인한다. Door count나 Door/관문의 거리 invariant, `(1,1)` visited/stepped는 검사하지 않는다.
  - `tests/test_content_contracts.cpp:223-243`는 생성 결과에서 Door 1개와 BossGate 최장 거리를 검사하지만, `tests/test_content_contracts.cpp:374-385`의 malformed test는 외벽만 바꾼 한 사례다.
- Expected Basis: `spec.md:175-178,203-207`, generated map의 reserved tile 계약, `tests/test_content_contracts.cpp:223-243`의 현재 acceptance invariant, 사용자 목표의 canonical tile/배열 validation complete. snapshot은 생성기의 결과를 보존해야 하며 단순히 연결된 임의 맵으로 대체하지 않아야 한다.
- Actual: v4 snapshot에서 Door를 `.`로 바꾸거나, 기존 B/V를 `.`로 바꾸고 가까운 EMPTY tile을 B/V로 바꾸어 개수·연결성을 유지하면 `DungeonMap::fromJson()`이 통과한다. `(1,1)`의 visited/stepped를 0으로 해도 통과한다. 이후 B/V 위치는 진행 거리와 encounter tier를 바꾸고, 새 `DungeonState`가 constructor `revealFogOfWar()`에서 입구 비트를 다시 바꾸므로 load→state construction의 snapshot 상태도 달라진다.
- Impact: 변조/손상 snapshot이 canonical 생성 결과가 아닌 다른 경로·난이도·탐험 상태로 정상 취급된다. 특히 gate를 입구 근처로 옮기면 최종 경로 설계가 무너지고, 입구 bit 변경은 load-save 전후 idempotence를 깨뜨린다.
- Suggested Action: 중앙 world/map validator에서 각 floor의 exact Door count, reserved tile role/거리 invariant, 시작 tile state를 검증한다. 생성기가 변경되어도 snapshot은 저장된 semantic을 그대로 복원하되, 최소 canonical shape를 벗어난 입력은 quarantine한다.
- Re-audit Method: generated v4 JSON에 Door 삭제, B/V 위치 교체·근접 이동, entrance visited/stepped 삭제를 각각 적용해 모두 `Corrupt`/quarantine 되는지 확인하고, 정상 snapshot은 `DungeonWorld::fromJson(...).toJson()`과 `Party` round-trip에서 동일한지 확인한다.
- Confidence: High
- Notes: object 좌표의 “walkable·unique” 검사는 별도로 존재한다(`DungeonWorld.cpp:197-204`). 이 finding은 floor landmark semantic 검사의 공백이다.

### [A03-F004] serializer가 mutable world를 검증 없이 `Saved`로 커밋할 수 있다

- Area: serialization boundary, model invariant ownership
- Severity: Minor
- Status: Confirmed
- Summary: `DungeonWorld`의 mutable floor/object API와 public `WorldObject` fields가 canonical invariant를 우회하며, `toJson()`/`Party::saveToFile()`는 `m_generated` 외 검증 없이 결과를 저장한다.
- Evidence:
  - `include/model/DungeonWorld.hpp:14-23,33-43`은 `WorldObject` fields를 공개하고 mutable `getFloor()`/`getObjects()`를 제공한다.
  - `src/model/DungeonWorld.cpp:145-156`의 `toJson()`은 generated 여부만 확인하고 floors/objects를 직렬화한다.
  - `src/model/Party.cpp:106-110,151-156`은 world가 generated이면 바로 world JSON을 save payload에 넣는다.
  - 반대로 `src/model/DungeonWorld.cpp:167-205`의 loader는 3층·object 수·canonical ID·좌표·중복을 검사하므로, 저장 시점과 다음 로드 시점의 계약 강도가 다르다.
- Expected Basis: `spec.md:359-422` canonical v4 snapshot, `tasks/plan.md:18-20`의 전체 snapshot 검증·저장 실패 안전성, 데이터 무결성 high-risk 범위.
- Actual: 내부/호출자가 object를 추가·삭제·중복시키거나 floor tile을 바꿔도 `Party::saveToFile()`이 `Saved`를 반환하고 malformed snapshot을 디스크에 남긴다. 일부 변조는 다음 load에서 거부되지만 Door 누락 같은 loader 공백은 계속 살아남을 수 있다.
- Impact: 저장 성공 결과가 “다음 load 가능한 canonical checkpoint”를 의미하지 않게 되고, runtime bug가 발생하면 현재 memory와 durable save가 서로 다른 invariant를 가진다.
- Suggested Action: serializer 직전에 동일한 canonical validator를 호출해 실패를 `IoError`/`Corrupt`로 구분하고, world mutation을 명시적 command로 좁히거나 검증된 mutation API로 감싼다. `toJson()` 자체도 generated state만 보는 대신 invariant를 닫는다.
- Re-audit Method: test-only mutable mutation으로 object duplicate/missing, Door missing, invalid position을 만든 뒤 `saveToFile()`이 성공하지 않는지와 기존 정상 save가 보존되는지 확인한다.
- Confidence: High
- Notes: 현재 제품 UI가 임의 object를 직접 편집하는 것은 아니므로 즉시 사용자 경로는 제한적이다. 그러나 production controller도 `getObjects()`를 통해 state를 바꾸므로 ownership 경계가 열린 상태다.

### [A03-F005] discovered object와 fog bit의 관계를 로드/렌더 양쪽에서 닫지 않아 숨은 목표가 표시될 수 있다

- Area: object state, FOW, renderer contract
- Severity: Minor
- Status: Confirmed
- Summary: 정상 runtime은 발견 시 `visited`를 먼저 세우지만, loader는 `DISCOVERED ⇒ visited`를 검사하지 않고 renderer도 visited를 다시 확인하지 않는다.
- Evidence:
  - `src/controller/DungeonState.cpp:353-360`은 `PRESENT` object를 `map().isVisited(object.x, object.y)`일 때만 `DISCOVERED`로 바꾼다.
  - `src/model/DungeonWorld.cpp:174-205`은 object state와 좌표/종류를 확인하지만 해당 floor의 `visited` bit와 `DISCOVERED` state의 관계는 확인하지 않는다.
  - `src/view/DungeonRenderer.cpp:185-187`은 `floor`, `DISCOVERED`, `party.hasQuest`만 확인하고 `map.isVisited(object.x, object.y)`를 확인하지 않은 채 marker를 그린다.
  - `spec.md:207`, `designs.md:59`는 시야로 발견한 뒤에만 목표 표식이 나타나는 흐름을 요구한다.
- Expected Basis: FOW/object state contract와 사용자 목표의 `visited/stepped/fog/목표/object 상태` 정합성.
- Actual: object state를 `DISCOVERED`로, 해당 floor visited row를 0으로 조작한 snapshot은 world loader를 통과한다. active quest가 있으면 renderer가 안개 속 좌표에 marker를 표시한다. 정상 입력에서는 `revealFogOfWar()`가 순서를 지키므로 현재 테스트가 놓치는 malformed/inconsistent state다.
- Impact: 숨겨진 objective 위치가 노출되고, state snapshot이 설명하는 탐험 상태와 화면이 불일치한다.
- Suggested Action: load validator에서 `DISCOVERED` object의 좌표가 visited인지 요구하고, renderer에도 `map.isVisited()` 방어 조건을 둔다. resolved object와 quest/key state 관계도 같은 validator에서 한 곳에 둔다.
- Re-audit Method: active quest + discovered object fixture에서 해당 visited bit를 끈 입력을 로드해 quarantine되는지, renderer production raster에서 marker가 안개에 나타나지 않는지 확인한다.
- Confidence: High
- Notes: 이는 일반 사용자가 UI로 임의 수정하는 경로보다 손상/변조 save에서 직접 드러나는 무결성 finding이다.

### [A03-F006] load/rollback이 RNG checkpoint를 실제 global stream과 원자적으로 함께 복원하지 않는다

- Area: save/load idempotence, RNG checkpoint, failure rollback
- Severity: Major
- Status: Confirmed
- Summary: `Party::loadFromFile()`는 seed/draw count를 Party fields에만 적재하고 `SessionRng::global()`은 바꾸지 않는다. 반대로 `saveToFile()`은 호출 시점의 global stream으로 metadata를 덮어쓴다. Title의 Continue branch만 별도로 global을 복원하므로 직접 load·rollback 경로가 self-contained하지 않다.
- Evidence:
  - `src/model/Party.cpp:102-105`는 save 시 매번 현재 global seed/draw count를 `m_lastSessionSeed/m_sessionRngDrawCount`에 기록한다.
  - `src/model/Party.cpp:287-293,333-337`는 저장된 checkpoint를 읽어 Party fields에만 대입한다.
  - `src/controller/TitleState.cpp:68-85`가 유일하게 `SessionRng::global() = SessionRng(savedSeed, drawCount)`를 명시한다.
  - `src/controller/DungeonState.cpp:401-404,437-440`와 `src/controller/CombatState.cpp:371-373,773-782`는 save 실패 rollback에서 `party.loadFromFile()`만 호출한다.
  - `DESIGN_DECISIONS.md:148-153`와 `spec.md:356`은 seed/raw draw count를 session checkpoint로 저장하고 Continue가 실제 stream을 이어가야 한다고 명시한다.
- Expected Basis: 저장·로드·rollback이 같은 checkpoint의 party/world/RNG를 함께 복원해야 한다는 spec/design 및 사용자 질문의 “전투 RNG와 독립”, “load-save-load 멱등성”.
- Actual: world save가 `(seed=S, draw=D)`일 때 global을 다른 seed/count로 둔 뒤 `loadFromFile(); saveToFile();`하면 world snapshot은 S로 남지만 metadata는 현재 global로 교체된다. Combat reward save 실패 후 `loadFromFile()`이 party/world를 되돌려도 global draw는 이미 보상 RNG만큼 소비된 상태라 재시도 결과가 달라지고, 다음 save가 그 advanced checkpoint를 기록한다.
- Impact: 저장 직후 재로드가 동일한 RNG continuation을 보장하지 않고, 실패 rollback이 gameplay 결과를 재현하지 못한다. world와 RNG가 서로 다른 세션 identity를 가질 수 있다.
- Suggested Action: checkpoint load/rollback을 담당하는 단일 API에서 Party snapshot과 global `SessionRng`를 함께 복원하거나, save API가 현재 session checkpoint ownership을 명시적으로 확인하도록 한다. 보상 실패 시 RNG도 pre-action checkpoint로 복원하고, direct `loadFromFile` 후 save의 metadata 불변/의도적 갱신 정책을 문서화한다.
- Re-audit Method: 서로 다른 global seed/count로 v4 save를 load→save해 seed/draw count와 world seed가 보존되는지 확인한다. Combat reward save failure injection에서 party/world/RNG를 pre-action JSON 및 다음 raw draws와 비교하고, retry가 같은 결과인지 검사한다.
- Confidence: High
- Notes: 정상 Title Continue test(`tests/test_controller_contracts.cpp:258-286`)는 Title branch를 거치므로 통과한다. 이 finding은 그 branch 밖의 public persistence/rollback seam에 대한 것이다.

### [A03-F007] primary 부재 + 손상 backup에서는 `.bak`가 quarantine되지 않는다

- Area: backup recovery, corruption handling, repeated load
- Severity: Minor
- Status: Confirmed
- Summary: primary가 없고 `.bak`만 존재할 때 backup parse가 `Corrupt`여도 early return하여 backup quarantine을 시도하지 않는다. primary가 corrupt이고 backup도 corrupt인 경우에도 primary만 quarantine되고 malformed `.bak`는 남는다.
- Evidence:
  - `src/model/Party.cpp:347-361`의 `recoverBackup()`은 backup `loadCandidate()`가 실패하면 즉시 그 결과를 반환한다.
  - `src/model/Party.cpp:364-369`는 primary `NotFound` + backup exists이면 `recoverBackup()`을 바로 반환한다. `src/model/Party.cpp:371-385`의 quarantine은 primary 결과가 `Corrupt`일 때만 실행된다.
  - `src/core/Persistence.cpp:219-252`는 호출받은 단일 path만 quarantine하므로 backup path를 자동으로 처리하지 않는다.
  - `spec.md:350`은 손상 파일을 `.corrupt-<timestamp>.json`으로 격리하고 자동 초기화하지 않도록 한다.
- Expected Basis: corrupt primary/backup 모두 반복 재시도 가능한 quarantine 경계를 가져야 하며, 손상 입력을 recoverable save로 계속 표시하지 않아야 한다.
- Actual: `save.json`을 없애고 malformed `save.json.bak`만 두면 `loadFromFile()`은 `Corrupt`를 반환하지만 `.bak`가 그대로 남는다. 다음 load도 같은 파일을 다시 읽는다. 두 파일 모두 손상이면 primary만 이동되고 backup은 남는다.
- Impact: 복구 불능 파일이 계속 recoverable 후보로 남아 Continue/New Game UI를 반복적으로 막고, 운영자가 원인 파일을 격리·추적하기 어렵다. 자동 overwrite는 하지 않으므로 직접 진행 덮어쓰기보다는 recovery dead-end 위험이다.
- Suggested Action: candidate별 parse failure를 quarantine하고, primary/backup 각각의 result/path를 보존해 UI에 전달한다. backup quarantine 실패도 typed result로 드러내되 정상 New Game/reset으로 자동 전환하지 않는다.
- Re-audit Method: primary absent/corrupt, backup malformed/oversized 조합을 각각 만들고 load 후 `.corrupt-*` 경로·원본 bytes·status를 검사한다. 반복 load가 같은 malformed candidate를 다시 시도하지 않는지 확인한다.
- Confidence: High

### [A03-F008] unordered set을 직접 dump해 save bytes가 canonical/idempotent하지 않다

- Area: canonical serialization, byte-level idempotence
- Severity: Minor
- Status: Needs Clarification
- Summary: `keyItems`와 `completedQuestIds`가 `std::unordered_set` 순회 순서 그대로 JSON array에 들어간다. 논리 상태는 보존되지만, 동일한 논리 save의 byte 순서가 implementation/process/build에 따라 달라질 수 있다.
- Evidence:
  - `include/model/Party.hpp:91-92`는 두 collection을 `unordered_set`으로 소유한다.
  - `src/model/Party.cpp:123-149`는 정렬 없이 range-for로 JSON array를 만든다.
  - `tests/test_content_contracts.cpp:303-312`는 `DungeonWorld` JSON object equality만 검사하고, 전체 Party save bytes의 반복 load→save equality는 검사하지 않는다.
- Expected Basis: 사용자 목표의 “save/load 멱등성”과 canonical snapshot이라는 `spec.md:359-422`. 다만 계약이 semantic equality인지 byte equality인지 문서에 명시되어 있지 않아 status를 clarification으로 둔다.
- Actual: 완료 quest가 둘 이상인 Party를 load/save할 때 array ordering은 표준상 보장되지 않는다. 현재 libstdc++에서 같은 실행 파일의 논리 equality가 대개 같을 수는 있지만, 이를 cross-platform/hash/byte checkpoint 기준으로 사용할 수 없다.
- Impact: exact bytes 비교, sidecar/hash, backup diff 또는 deterministic fixture를 기준으로 삼으면 불필요한 drift가 발생할 수 있다. JSON semantic load에는 직접 영향이 없다.
- Suggested Action: byte-level canonical save가 요구사항이면 IDs를 정렬해 dump하고, semantic-only라면 문서에 명시해 acceptance를 구분한다. 여러 completed IDs를 가진 fresh-process round-trip test를 추가한다.
- Re-audit Method: 2개 이상의 completed quest를 만든 뒤 별도 process에서 load→save를 반복하고 raw bytes와 parsed JSON을 각각 비교해 정해진 계약에 맞는지 확인한다.
- Confidence: Medium

### [A03-F009] legacy no-arg `DungeonMap::generate()`가 global combat RNG를 소비하는 잔여 공개 경로

- Area: generation/RNG boundary, legacy API surface
- Severity: Info
- Status: Confirmed
- Summary: 실제 3층 runtime은 `DungeonWorld::generate(seed)`의 local RNG 경로를 사용하지만, 공개된 no-arg `DungeonMap::generate()`는 global RNG로 seed를 뽑는다.
- Evidence:
  - `src/model/DungeonMap.cpp:29-35`의 no-arg/one-arg overload 중 no-arg는 `SessionRng::global().rollRange(0, INT_MAX)`를 호출한다.
  - `src/model/DungeonWorld.cpp:82-101`은 floor마다 local `SessionRng`를 사용하고, `src/controller/DungeonState.cpp:17-23`은 world 경로만 호출한다.
  - 잔여 overload는 `src/test_harness.cpp:90-95,736-742`와 `tests/test_content_contracts.cpp:184-187`에서 계속 사용된다.
- Expected Basis: `spec.md:203-205`, `tasks/plan.md:18-19`, 사용자 질문의 “생성이 전투 RNG와 독립”.
- Actual: no-arg API 호출자는 map 생성 전에 global draw count가 증가한다. 현재 shipped DungeonState 경로에서는 관찰되지 않았지만, 동일 production library의 public API가 generation independence invariant를 보장하지 않는다.
- Impact: 향후 runtime/도구가 overload를 재사용하면 전투 encounter/reward replay가 seed에서 이탈한다. 현재 scope에서는 즉시 runtime failure가 아닌 legacy API drift다.
- Suggested Action: no-arg overload를 deprecated/remove하고 world seed를 명시적으로 받게 하거나, 별도 generation RNG source를 사용한다. 모든 generation entrypoint의 global draw count 불변 test를 둔다.
- Re-audit Method: 각 generate overload 전후 global draw count를 비교하고, runtime call graph에서 no-arg overload가 shipped path에 남아 있지 않은지 재검색한다.
- Confidence: High
- Notes: `DungeonWorld::generate` local RNG independence 자체는 `src/model/DungeonWorld.cpp:88-99`와 현재 content tests 결과로 긍정적이다.

### [A03-F010] full lifecycle의 핵심 world 회귀가 headless 직접 좌표 설정에 의존해 coverage가 부분적이다

- Area: runtime evidence, regression coverage, lifecycle gate
- Severity: Minor
- Status: Confirmed
- Summary: 현재 green 테스트는 3층 생성/일부 state round-trip을 확인하지만, 실제 movement/FOW/2초 checkpoint/repeated Continue·town re-entry·TPK world snapshot을 모두 production event 경로로 닫지 않는다.
- Evidence:
  - `tests/test_controller_contracts.cpp:57-69`의 `ControllerTestAccess`는 object/stair 좌표를 `setPlayerPos()`로 직접 옮긴다.
  - `tests/test_controller_contracts.cpp:461-502`의 3층 흐름은 위 test access로 item/boss/NPC와 계단에 teleport한 뒤 object state와 loaded world 일부만 검사한다.
  - `tests/test_content_contracts.cpp:260-312`는 `DungeonWorld::toJson()/fromJson()` equality를 확인하지만 Party 전체 load-save-load bytes, fog snapshot, repeated town re-entry는 확인하지 않는다.
  - `BUILD_GUIDE.md:43-55`의 13개 CTest 목록에도 legacy v1/v2 cross-process deterministic world와 dirty-save timer/lifecycle 전용 test가 없다.
- Expected Basis: 사용자 핵심 질문 전체, `audit_roadmap.md:73-80`의 v0.10.0 gate(3층 snapshot, 변조 거부, 동일 save 재입장/Continue, 여러 lifecycle 상태).
- Actual: `ContentContractTests`, `ControllerContractTests`, `TestHarness`는 모두 통과했지만, 그 결과만으로 실제 event-driven stair traversal, fog persistence after re-entry, TPK restoring latest world checkpoint, save failure retry와 같은 핵심 경로를 PASS라 할 수 없다.
- Impact: F001~F007과 같은 failure mode가 aggregate green test 아래에서 회귀할 수 있다. 특히 direct teleport는 `revealFogOfWar()`, `m_worldDirty`, 2초 save timer와 실제 입력 순서를 우회한다.
- Suggested Action: production event transcript/headless seam으로 실제 walk/turn/auto-move→encounter→combat pop, floor up/down, Town re-entry, Continue, TPK를 실행하고, pre/post world JSON·fog rows·object states·RNG checkpoint를 비교하는 failure-specific tests를 추가한다.
- Re-audit Method: clean temporary save에서 위 event sequence를 별도 process와 failure injection 조합으로 실행하고, `load-save-load`의 semantic/byte acceptance를 문서에서 정한 기준으로 확인한다. Debug/Release CTest 양쪽에서 새 회귀를 실행한다.
- Confidence: High
- Notes: 이는 구현 결함의 직접 판정보다 현재 evidence coverage gap이다. full world persistence scope의 PASS를 제한한다.

## 6. Uncertainties and Clarifications Needed

- `spec.md:49-55`의 “마을과 종결 결과에서만 checkpoint, 활성 dungeon FOW 미지원” 문구는 `spec.md:203-207,345-353`의 v0.10.0 full-world snapshot과 직접 충돌한다. `DESIGN_DECISIONS.md:47`은 v0.10.0 결정이 이전 결정을 대체한다고 적었지만, 이전 섹션의 save schema v3 문장(`DESIGN_DECISIONS.md:53-58`)은 그대로 남아 있다. 구현을 곧바로 오작동으로 분류하지 않고 문서 authority/삭제·supersede 범위를 명시할 필요가 있다.
- `spec.md:204-205`의 `splitmix32(worldSeed + constant * floorNumber)`가 내부에서 상수 increment를 다시 더하는 현재 `splitmix32` 구현(`src/model/DungeonWorld.cpp:12-17`)과 어떤 exact test vector를 요구하는지 명시되어 있지 않다. 현재 code는 unsigned deterministic이지만, floor seed reference vector가 없어 독립 계산으로 formula conformance를 확정하지 않았다.
- “멱등성”이 parsed semantic state인지 exact save bytes인지 현재 문서에서 분리되지 않는다. F008은 이 결정이 내려지기 전까지 Needs Clarification으로 유지한다.
- `DungeonWorld` mutable API가 의도적 controller seam인지 public model contract인지 문서가 정하지 않는다. API를 유지할 경우 serializer validator를 source of truth로 둘지 결정해야 한다.

## 7. Perspective Decision

**HOLD (월드 영속성 범위).**

현재 evidence로 다음은 긍정적이다.

- `DungeonWorld::generate()`는 floor별 local seeded RNG를 사용하고 global combat RNG draw를 직접 소비하지 않는다(`src/model/DungeonWorld.cpp:82-101`).
- v4 `DungeonWorld::fromJson()`은 generator를 다시 호출하지 않고 저장된 3층 tile/fog/object snapshot을 복원하며, `tests/test_content_contracts.cpp:303-312`의 world JSON round-trip은 통과한다.
- generated world의 3층 수·계단 수·목표 object 수, v3(seed 보유) migration, New Game reset, controller의 기본 층 이동/목표 interaction, TPK root replacement는 현재 실행 테스트에서 통과했다.

그러나 F001/F002/F003/F006은 사용자 핵심 데이터 무결성·결정성 계약을 직접 제한하는 Confirmed Major finding이다. F004/F005/F007과 F010의 coverage gap도 함께 남아 있어 이 관점에서 `PASS` 또는 `PASS WITHIN STATED SCOPE`로 판정하지 않는다.

