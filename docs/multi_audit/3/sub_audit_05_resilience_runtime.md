# Sub Audit Report

## 1. Audit Metadata

- Audit Turn: 3
- Perspective: 오류 복원력·런타임 데이터 무결성 (world persistence 중심, 캐릭터·퀘스트 교차 상태 포함)
- User Goal: 캐릭터 생성부터 플레이·사망까지, 퀘스트 전체, 맵 생성과 save/load의 멱등성·소멸·재생성 및 문서-구현 모순을 전수 점검한다.
- Audit Basis: Standard-backed
- Standard Path: `AI_AUDIT_DOC_STANDARD.md`; `/home/eunho1/.codex/skills/multi-audit/references/report-contract.md`
- Repository Basis: `HEAD 927753278f46b92a015197ee229edce4f52e0657 + current working tree`
- Assigned Report: `docs/multi_audit/3/sub_audit_05_resilience_runtime.md`

## 2. Assigned Scope

- `Persistence`의 atomic write, backup/quarantine, typed `PersistenceResult`, path/symlink 경계
- `Party`의 schema-v4 serialization/deserialization, v1~v3 migration, RNG checkpoint, 메모리 원자성
- `DungeonWorld`/`DungeonMap` 생성·snapshot round-trip, 층 이동·FOW·오브젝트 상태, New/Continue/TPK 경계
- `Quest`의 현장 달성·성 보고·보상·중요품·완료 원장과 저장 실패 rollback
- `Character` 상태·사망/전투 보상 저장 및 Party/Quest/World 교차 상태
- `Game`/Title/Town/Dungeon/Combat state 호출 경로와 상태 수명·stale reference 위험
- 기존 build artifacts에 링크된 Debug/Release 실행 증거와 등록 테스트의 production-link/실패모드 범위

## 3. Excluded and Uninspected Scope

- 요청대로 `docs/audit/**`, `docs/multi_audit/1/**`, `docs/multi_audit/2/**` 및 본 디렉터리의 다른 sub-audit 보고서는 읽지 않았다.
- 제품 소스, 테스트, 설정, 문서 및 새 build 산출물은 수정하지 않았다. 지정 보고서만 작성 대상이다.
- clean Windows 10/11 런타임, 실제 high-DPI/장시간 플레이, macOS, 법률·지원 승인과 외부 hosted gate는 이 관점에서 직접 실행하지 않았다.
- 다중 프로세스 동시 writer/실제 전원 차단 crash 실험은 실행하지 않았다. 기존 failure injection과 정적 호출 경로로 판정했다.

## 4. Evidence Examined

### 통제 문서

- `spec.md:47-73, 136-208, 344-422, 513-584` — 캐릭터·던전·퀘스트·save schema v4·TPK·RNG·내구성·검증 기준
- `DESIGN_DECISIONS.md:46-58, 129-160, 163-168` — town checkpoint, TPK/New Game 분리, RNG raw draw, 영속 월드 결정
- `IMPLEMENTATION_SUMMARY.md:6-76` — 현재 파일 책임과 완료 주장
- `audit_roadmap.md:21-40, 55-80`, `tasks/plan.md:5-21`, `tasks/todo.md:3-9` — rollback·migration·월드/퀘스트 gate
- `README.md:13-24, 49-51`, `BUILD_GUIDE.md:40-56`, `CHANGELOG.md:9-66`, `tests/fixtures/README.md:1-5`

### 구현과 호출 경로

- `include/core/Persistence.hpp:9-50`, `src/core/Persistence.cpp:28-263`
- `include/model/Party.hpp:20-101`, `src/model/Party.cpp:15-586`
- `include/model/Character.hpp:67-185`, `src/model/Character.cpp:16-585`
- `include/model/Quest.hpp:14-79`, `src/model/Quest.cpp:10-177`
- `include/model/DungeonMap.hpp:31-98`, `src/model/DungeonMap.cpp:17-472`
- `include/model/DungeonWorld.hpp:11-50`, `src/model/DungeonWorld.cpp:12-211`
- `src/core/SessionRng.cpp:19-69`
- `src/core/Game.cpp:13-115`, `src/controller/TitleState.cpp:16-119`, `src/controller/TownState.cpp:32-293`, `src/controller/DungeonState.cpp:15-455`, `src/controller/CombatState.cpp:26-785`, `src/controller/CharacterInfoState.cpp:377-607`, `src/controller/GameOverState.cpp:9-54`

### 테스트·빌드·실행 증거

- `CMakeLists.txt:85-101, 103-127, 129-246, 257-302` — runtime library와 개별 테스트 링크/등록
- `tests/test_controller_contracts.cpp:143-286, 461-613` — TPK, Continue RNG, 월드·퀘스트 정상/단일 save failure 경로
- `tests/test_content_contracts.cpp:246-401` — 월드 round-trip, v3(nonzero seed) migration, New Game 경계, 제한적인 malformed snapshot
- `tests/test_rng_process_replay.cpp:23-67` — 정상 save/Continue raw RNG 재현
- `src/test_harness.cpp:398-667, 1097-1221, 1391-1465` — schema·backup·quarantine·oversize·일부 rollback과 전체 하네스
- 실행 명령: `./ContentContractTests`, `xvfb-run -a ./ControllerContractTests`, `./TestHarness --run-all`, `./CombatContractTests`, `./AgencyContractTests`, `./HudContractTests` in `build/debug` and `build/release`; 모두 exit 0.
- 실행 명령: `ctest --test-dir build/debug --output-on-failure`; 13/13 완료를 기존 `build/debug/Testing/Temporary/LastTest.log`의 최종 `Test Passed`로 확인했다. raster test는 5 locale × 3 scale 산출에 약 2분이 걸렸으나 완료했다.
- 직접 변조 probe (기존 `build/debug/RngProcessReplayTests`, 모든 입력은 `/tmp` 임시 디렉터리):
  - primary save를 외부 정규 파일 symlink로 대체하고 `RngProcessReplayTests continue ...` 실행: `rc=0`, symlink target을 로드하고 RNG replay 성공.
  - v4 `.world.floors[0].tiles`에서 `D`를 `.`로 모두 치환 후 Continue: `rc=0`, `door_count=0`인 snapshot을 로드.
  - v4 `activeQuests`에 legacy `qst_clear_kobolds`를 삽입 후 Continue: `rc=0`, 해당 legacy quest를 로드.
  - `keyItems=["key_moon_seal"]`, unresolved `qst_recover_moon_seal`/`obj_moon_seal`을 삽입 후 Continue: `rc=0`, cross-state 불일치를 로드.
  - corrupt primary + valid `.bak` + 복구용 `.tmp` 충돌을 넣고 Continue: `rc=0`이지만 `primary_exists=no`, `backup_exists=yes`; 백업 복구 실패가 UI 성공 경로로 승격됨.

## 5. Findings

### [A05-F001] pre-commit save rollback이 Party는 되돌려도 전역 RNG stream을 되감지 않는다

- Pass: Debug / Engineering Quality
- Pattern: `DBG-002`, `TEST-001`
- Area: combat reward·level-up·quest progress rollback, RNG checkpoint
- Severity: Major
- Status: Confirmed
- Summary: 저장 전 보상 계산이 소비한 raw RNG draw가 save failure 후에도 전역 `SessionRng`에 남는다. `Party`의 저장된 draw count만 과거 checkpoint로 돌아가므로 같은 전투를 재시도하면 I/O 성공 여부에 따라 다른 보상과 다음 난수가 나온다.
- Evidence:
  - `spec.md:356-358`, `tasks/plan.md:18-20`, `DESIGN_DECISIONS.md:148-153`은 seed와 raw draw count를 checkpoint로 삼고 rollback에서 보상 상태를 함께 되돌릴 것을 요구한다.
  - `src/controller/CombatState.cpp:704-715, 726-773`에서 gold, XP/level-up, kill quest progress, loot, objective/campaign state를 변경하면서 `rollGoldReward`와 drop RNG를 소비한 뒤 save한다. `src/controller/CombatState.cpp:779-782`의 실패 처리는 `party.loadFromFile()`만 호출한다.
  - `src/model/Party.cpp:327-338`은 JSON의 `m_sessionRngDrawCount`를 Party 필드에 대입하지만 `SessionRng::global()`을 seed/draw count로 복원하지 않는다. 전역 복원은 `src/controller/TitleState.cpp:68-85`의 Continue 경로에만 있다.
  - 동일한 reload-only rollback이 `src/controller/DungeonState.cpp:401-404`, `src/controller/CharacterInfoState.cpp:443-447, 544-547, 598-601`, `src/controller/TownState.cpp:63-74`에도 존재한다.
  - `tests/test_controller_contracts.cpp:504-531, 589-613`은 save failure 후 gold/quest/world reference만 확인하고 전역 draw count와 다음 난수열을 확인하지 않는다. `tests/test_rng_process_replay.cpp:23-67`은 성공한 Continue만 다룬다.
- Expected Basis: `spec.md`의 RNG checkpoint 계약, 저장 실패 시 transaction invariant, `tasks/plan.md`의 보상·중요품·완료 원장 rollback.
- Actual: pre-save draw count가 `n`이어도 보상 계산 후 실패하면 전역은 `n+k`, Party를 `n` 상태로 load한 뒤 retry는 `n+k`에서 시작한다. 다음 성공 save는 그 달라진 count를 기록한다.
- Impact: 동일 입력의 재시도가 비멱등적이며, 전투 보상·레벨업 HP·drop·후속 encounter가 디스크 failure 유무에 따라 달라진다. TPK/floor rollback도 동일하게 global stream을 보존하지 않는다.
- Suggested Action: pre-commit transaction에 seed/raw draw state를 포함해 실패 시 전역 stream을 복원하거나, 보상 계산을 checkpoint 복사본에서 수행한 뒤 durable commit 시에만 전역 state를 publish한다. `CommittedDurabilityUnknown`은 rollback 대상에서 분리한다.
- Re-audit Method: 저장 직전 seed/draw count와 다음 N개 raw 결과를 기록하고, 보상 RNG를 소비한 뒤 `.tmp` write failure를 주입한다. Party/world/quest/key item/ledger와 전역 다음 N개가 failure 없는 baseline과 같은지, retry 성공 시에도 같은지 확인한다.
- Confidence: High
- Notes: 정상 process replay는 PASS이나 이 finding을 반증하지 않는다. 정상 경로와 pre-commit rollback 경로의 목적이 다르다.

### [A05-F002] 퀘스트·보상·월드 rollback이 디스크 재로드에 의존하고 재로드 실패 시 메모리 변경을 남긴다

- Pass: Debug / Engineering Quality
- Pattern: `TEST-001`, `DBG-001`
- Area: quest report transaction, combat reward, object interaction, floor transition
- Severity: Major
- Status: Confirmed
- Summary: 보고/보상/목표 해결은 먼저 Party와 World를 mutate한 뒤 save하고, failure 시 같은 save 파일을 다시 읽어 복원한다. 복원 파일이 없거나 손상되거나 복원 자체의 I/O가 실패하면 in-memory snapshot이 없어 gold·inventory·key item·completed ledger와 world/floor 상태가 부분 커밋된다.
- Evidence:
  - `src/controller/TownState.cpp:262-287`은 `party.completeQuest()` 후 `persistTownChange()`를 호출한다. `src/controller/TownState.cpp:63-74`는 save failure 때 `party.loadFromFile()`만 시도하며 그 결과가 실패하면 mutated Party를 보존한다.
  - `Party::completeQuest`의 실제 순서는 `src/model/Party.cpp:435-481`에서 gold/XP 지급, 일반 수집품·중요품 제거, 보상 추가, 완료 원장 삽입, active 제거다. 이 순서 중간 상태를 별도 snapshot으로 보존하지 않는다.
  - `src/controller/CombatState.cpp:726-772`도 gold/XP/kill progress/drop/object/campaign을 먼저 변경하고, `src/controller/CombatState.cpp:779-782`에서 reload 결과를 무시한다.
  - `src/controller/DungeonState.cpp:431-440`은 key item 추가·objective 완료·object RESOLVED 후 save하며 failure 뒤 reload 결과를 무시한다. `src/controller/DungeonState.cpp:379-404`의 floor transition도 restore load 결과를 무시한다.
  - `src/model/Party.cpp:174-385`는 malformed/no-file candidate가 `NotFound`/`Corrupt`를 반환할 수 있고, load 시작에서 `m_hasActiveSaveSession`까지 false로 바꾼다.
  - `tests/test_controller_contracts.cpp:504-531`은 유효한 primary가 남아 있고 `.tmp`만 막힌 경우만 검증한다. primary 부재·corrupt·restore failure 조합은 없다.
- Expected Basis: `tasks/plan.md:20`의 gold/inventory/key item/completed ledger 동시 rollback, `spec.md:349-350, 358`의 마지막 정상 원본 유지와 typed failure.
- Actual: rollback 원본을 읽지 못하면 함수는 UI에 restore failure를 표시할 수 있지만 메모리 변경을 원자적으로 취소하지 않는다. `changeFloor`/`interactCurrentTile`/`distributeRewards`는 실패한 load를 무시한 채 상태 객체를 계속 사용한다.
- Impact: 드물지만 실제 파일 삭제·손상·권한 race에서 보상이 중복되거나 중요품/월드 해결 상태만 남는 교차 상태가 발생한다. 이후 정상 save가 그 부분 상태를 영속화할 수 있다.
- Suggested Action: Party/World/RNG를 포함한 in-memory transaction snapshot 또는 domain command transaction을 도입하고 durable commit 전에만 publish한다. rollback source를 읽지 못하면 mutated 상태를 성공처럼 사용하지 말고 명시적 복구 대기 상태로 멈춘다. 모든 호출자가 restore `PersistenceResult`를 분기하도록 한다.
- Re-audit Method: valid checkpoint를 만든 뒤 primary를 제거/손상시키고 `.tmp`/권한 failure를 주입한 상태에서 quest report, combat reward, object interaction, floor transition을 각각 실행한다. memory와 bytes가 모두 pre-state인지, 실패 결과가 후속 save를 허용하지 않는지 확인한다.
- Confidence: High
- Notes: 기존 단일 save failure 테스트가 통과하는 것은 정상 primary reload가 가능한 좁은 경우만 증명한다.

### [A05-F003] 별도 town checkpoint가 없어 dungeon autosave가 TPK 복원 원본을 덮어쓴다

- Pass: Implementation / Debug
- Pattern: `IMP-001`, `DBG-001`
- Area: TPK, world autosave, town/dungeon checkpoint separation
- Severity: Major
- Status: Confirmed
- Summary: 문서와 설계 결정은 마지막 town checkpoint 복원을 말하지만 구현에는 save 하나만 있고, DungeonState의 2초 world autosave가 그 파일을 계속 갱신한다. 따라서 TPK가 읽는 파일은 별도 town checkpoint가 아니라 가장 최근 dungeon autosave다.
- Evidence:
  - `spec.md:53-54, 352-354`와 `DESIGN_DECISIONS.md:48, 53, 58`은 checkpoint를 town/종결에 두고 TPK에서 마지막 정상 town checkpoint를 복구한다고 선언한다.
  - `src/controller/DungeonState.cpp:230-243`은 `m_worldDirty`가 있으면 2초마다 `persistWorldCheckpoint()`를 호출하고, `src/controller/DungeonState.cpp:363-376`은 결국 `Party::saveToFile()`를 기본 단일 경로로 실행한다.
  - `include/model/Party.hpp:69-75`에는 save/load 경로가 하나뿐이며 town checkpoint 전용 파일·메모리 snapshot이 없다. `src/controller/CombatState.cpp:367-373`의 TPK는 같은 기본 `loadFromFile()`을 호출한다.
  - `rg -n -i 'town checkpoint|checkpoint|last.*town|persistWorld' src include` 결과에도 별도 town 저장 경계는 없고 `persistWorldCheckpoint`만 존재한다.
- Expected Basis: `DESIGN_DECISIONS.md:53, 58`의 “town checkpoint만 저장” 및 TPK restore 계약.
- Actual: 탐험 중 visited/stepped/object DISCOVERED/RESOLVED와 함께 전체 Party save bytes가 갱신된다. TPK restore에는 어느 버전이 town에서 만들어졌는지 식별할 정보가 없다.
- Impact: 엄격한 TPK 정책을 적용하면 사망 후 in-run world progress를 되돌리지 못한다. 반대로 dungeon progress 보존이 의도라면 현재 `spec.md`의 “town checkpoint”와 autosave 문장이 서로 모순되어 운영자가 복원 결과를 예측할 수 없다.
- Suggested Action: (A) 별도 town checkpoint snapshot/path를 유지하고 TPK/재시도에서 그 원본을 선택하거나, (B) dungeon autosave 보존을 제품 계약으로 채택하고 TPK·README·설계 결정을 같은 의미로 갱신한다. 어느 경우든 TPK 전후 world/object/FOW 테스트를 추가한다.
- Re-audit Method: town save 후 dungeon에서 2초 이상 autosave를 유발해 world/object/FOW를 변경하고 TPK를 실행한다. 복원된 bytes가 town baseline인지 최신 autosave인지 확인한 뒤 문서 계약과 일치시키고 Continue/New Game에도 반복한다.
- Confidence: High for “별도 checkpoint 부재”; intended result is additionally a specification conflict.
- Notes: `spec.md:353`의 탐험 autosave 문장은 town-only checkpoint와 충돌하므로 제품 결정이 필요한 명세 공백도 함께 남긴다.

### [A05-F004] v1/v2 save의 seed 부재 migration이 프로세스 RNG에 의존해 월드가 결정론적이지 않다

- Pass: Implementation / Debug
- Pattern: `IMP-003`, `DBG-002`, `TEST-001`
- Area: legacy migration, world seed, independent-process replay
- Severity: Major
- Status: Confirmed
- Summary: v1/v2 입력에는 저장된 session seed가 없거나 0인데 migration이 현재 프로세스 전역 RNG seed를 world seed로 사용한다. 같은 legacy bytes를 첫 save 전에 다른 프로세스에서 읽으면 서로 다른 3층 월드가 생성된다.
- Evidence:
  - `spec.md:344-347`, `tasks/plan.md:9-10`, `IMPLEMENTATION_SUMMARY.md:62-64`는 v1~v3를 저장된 seed에서 결정론적으로 월드 이관한다고 기술한다.
  - `tests/fixtures/save_v1.json:1-9`는 `schemaVersion`, `lastSessionSeed`, world가 없는 실제 fixture다. `src/test_harness.cpp:357-367`의 v2 root도 `lastSessionSeed: 0`을 만든다.
  - `src/model/Party.cpp:286-302`는 schema<4에서 `lastSessionSeed == 0`이면 `SessionRng::global().seed()`를 migration seed로 사용한다. 이 값은 파일에서 오지 않는다.
  - `src/controller/TitleState.cpp:68-85`는 seed 0 load 뒤 새 global session을 시작하고 save하므로, 첫 migration world가 이미 프로세스 seed에 따라 만들어진 뒤 session metadata만 새 값으로 바뀐다.
  - `tests/test_content_contracts.cpp:314-349`는 nonzero v3 seed 두 번을 비교할 뿐 v1/v2 seed-less independent process case를 검증하지 않는다. `tests/test_rng_process_replay.cpp:53-65`도 Continue 시작 seed를 고정 7로 하므로 이 결손을 덮지 못한다.
- Expected Basis: schema v4 migration의 결정론적 world 생성 및 raw-seed replay 계약.
- Actual: seed-less legacy save의 world seed는 `SessionRng::global()` 초기화 상태에 따라 달라진다. 파일 자체만으로 world snapshot을 재현할 수 없다.
- Impact: 구버전 사용자 진행의 지형·목표 좌표가 최초 migration 프로세스마다 달라져 지원 재현·회귀 비교·플레이어 기대가 깨진다. migration 직후 저장하면 그 임의 결과가 v4 canonical state로 굳어진다.
- Suggested Action: legacy bytes와 명시적 migration version에서 deterministic hash seed를 도출하거나, 고정 fallback seed를 계약으로 정하고 v1/v2의 session seed 부재를 문서화한다. migration world seed와 saved session metadata 관계도 명시한다.
- Re-audit Method: 동일한 v1 fixture와 seed-less v2 fixture를 서로 다른 초기 global seed의 독립 프로세스에서 load하고 `world.toJson()` hash를 비교한다. 동일해야 하며, migration save 후 v4 Continue에서도 snapshot과 RNG checkpoint가 일치해야 한다.
- Confidence: High
- Notes: nonzero v3 path 자체는 `ContentContractTests`에서 통과했다. 결손은 seed가 없는 v1/v2 분기다.

### [A05-F005] save primary symlink를 load가 따라가 per-user path 경계를 우회한다

- Pass: Security / Resilience
- Pattern: `SEC-004`, `SEC-007`
- Area: save path, symlink boundary, backup/quarantine
- Severity: Major
- Status: Confirmed
- Summary: write path에서는 leaf `save.json` symlink만 거부하지만 load path에서는 symlink 검사가 없어 외부 파일을 정상 save처럼 읽는다. per-user 저장 경계와 changelog의 symlink rejection 주장이 양방향으로 닫혀 있지 않다.
- Evidence:
  - `src/core/Persistence.cpp:164-183`은 `atomicWriteText`에서 `is_symlink(path)`만 확인한다. `src/model/Party.cpp:178-196`의 `loadCandidate`는 `exists` 후 `ifstream`으로 열며 symlink 여부를 검사하지 않는다.
  - `src/core/Persistence.cpp:219-249`의 quarantine은 corrupt path를 rename할 뿐, 정상 symlink primary/backup을 load 단계에서 거부하지 않는다.
  - 직접 probe 명령(기존 `build/debug/RngProcessReplayTests`): 외부에 생성한 정상 `save.json`을 `$tmp/data/save.json` symlink로 연결한 뒤 `RngProcessReplayTests continue $tmp/data/save.json ...`를 실행했다. 결과는 `rc=0`, `Load .../data/save.json`, `Independent-process RNG checkpoint replay passed.`였다.
  - `CHANGELOG.md:60`은 “심볼릭 링크 대상 거부”를 수정 사실로 기록하지만 load 테스트는 `tests`/`src` 검색에서 확인되지 않았다.
- Expected Basis: per-user save boundary, symlink 대상 거부 정책, 정상 primary/backup만 load한다는 데이터 무결성 불변조건.
- Actual: 외부 파일 내용을 game Party로 수용한다. 이후 save는 leaf symlink를 거부하므로 읽기 원본과 쓰기 원본의 경계가 서로 다르다. Windows `src/core/Persistence.cpp:47-72`의 temp open에도 POSIX `O_NOFOLLOW`에 대응하는 검사가 없다.
- Impact: 외부/예상 밖 파일이 진행 상태로 주입되고, backup symlink를 통한 recovery도 같은 경계를 우회한다. Windows에서는 pre-existing `.tmp` symlink가 open 경로에서 별도 검증되지 않는다.
- Suggested Action: primary, backup, temp, backup-temp와 parent data directory의 symlink/reparse-point 정책을 플랫폼별로 명시하고 load/write 양쪽에서 동일하게 거부한다. canonicalized per-user root confinement과 dedicated symlink regression을 추가한다.
- Re-audit Method: Linux에서 primary/backup/temp 각각 정상·dangling symlink를 만들고 load/save/quarantine 결과가 모두 typed `IoError` 또는 명시적 거부인지 확인한다. hosted Windows에서도 junction/reparse-point와 temp/backup 대상을 같은 방식으로 확인한다.
- Confidence: High for primary load acceptance; Windows temp impact requires hosted execution.

### [A05-F006] schema v4가 v1~v3 legacy active quest를 새 보드 quest처럼 수용한다

- Pass: Implementation Compliance / Security of persisted state
- Pattern: `IMP-001`, `IMP-002`, `TEST-001`
- Area: quest registry, schema gate, save tampering
- Severity: Major
- Status: Confirmed
- Summary: v4에는 신규 3개 목적형 quest만 허용되어야 하지만 parser가 legacy `qst_clear_kobolds`/`qst_collect_maces`/`qst_hunt_spiders`도 canonical ID로 받아들인다. Town은 active ID를 보드에 추가하므로 v4 변조 save가 legacy quest를 실제 플레이 경로에 노출한다.
- Evidence:
  - `spec.md:196-201, 513-520`은 v4 신규 보드를 3개로 제한하고 legacy registry는 v1~v3 호환용으로만 남긴다.
  - `src/model/Quest.cpp:59-99, 120-175`의 `createCanonical`/`fromJson`은 schema>=4에서도 6개 canonical ID 모두를 허용한다.
  - `src/model/Party.cpp:247-284`는 active ID 중복과 active/completed 겹침만 검사하며 `Quest::getOfferableIds()` 또는 schema별 allowed set을 검사하지 않는다.
  - `src/controller/TownState.cpp:32-39, 262-287`은 offerable ID에 active ID를 추가하고 기존 quest를 수주/보고한다.
  - 직접 probe: 정상 v4 save의 `activeQuests`에 canonical shape `qst_clear_kobolds`를 삽입하고 `RngProcessReplayTests continue ...`를 실행했다. 결과 `rc=0`, `Load ...save.json`, RNG replay 성공, `active_ids=["qst_clear_kobolds"]`였다.
- Expected Basis: v4 schema/offerable quest boundary와 legacy migration 범위.
- Actual: v4 변조 save가 legacy quantity quest를 active 상태로 로드하고 Town에서 보고/진행 가능한 상태가 된다.
- Impact: 현 월드에 대응 object가 없는 legacy quest의 kill 진행과 gold/XP/item 보상을 새 캠페인에서 취득할 수 있어 저장 계약과 one-time ledger 범위를 우회한다.
- Suggested Action: schema>=4 parser에서 `Quest::getOfferableIds()` 외 ID를 reject하고, v1~v3 migration에서만 legacy active quest를 별도 호환 상태로 허용한다. completed ledger도 canonical/legacy 허용 범위를 같은 규칙으로 검증한다.
- Re-audit Method: v4 각 legacy ID·unknown ID·offerable ID를 각각 load하고, legacy는 `Corrupt`/`UnsupportedVersion`으로 거부되는지, v1~v3는 완료·보고 호환을 유지하는지 확인한다.
- Confidence: High

### [A05-F007] unresolved retrieve object와 이미 보유한 key item의 불일치를 load가 허용해 영구 soft-lock을 만든다

- Pass: Implementation Compliance / Debug
- Pattern: `IMP-001`, `TEST-001`
- Area: Quest–World–KeyItem cross-state invariant
- Severity: Major
- Status: Confirmed
- Summary: active retrieve quest가 READY/RESOLVED가 아닌데 `key_moon_seal`이 이미 key item set에 있어도 parser가 이를 거부하지 않는다. 실제 상호작용은 중복 key item에서 즉시 실패해 목표를 해결할 수 없는 상태를 만든다.
- Evidence:
  - `src/model/Party.cpp:304-325`는 active quest가 ready일 때만 retrieve key item 존재를 확인한다. active-not-ready + key item present + object PRESENT/DISCOVERED 조합에 대한 검사가 없다.
  - 직접 probe: v4 save에 `keyItems=["key_moon_seal"]`, active `qst_recover_moon_seal` (`currentCount=0`, `readyToReport=false`), `obj_moon_seal.state="present"`를 삽입하고 `RngProcessReplayTests continue ...`를 실행했다. 결과 `rc=0`과 해당 cross-state JSON 출력으로 load acceptance를 확인했다.
  - `src/controller/DungeonState.cpp:421-440`은 object interaction에서 `addKeyItem`을 먼저 호출하고 false면 line 434-435에서 바로 반환한다. `src/model/Party.cpp:538-545`의 set insert는 이미 가진 key를 false로 만든다.
  - key item을 제거하는 정상 경로는 `src/model/Party.cpp:471-473`의 successful quest report뿐이며, ready가 되지 않은 이 상태에서는 그 경로에 도달할 수 없다.
- Expected Basis: `spec.md:200-201, 207-208`, `tasks/plan.md:11-12, 20`의 중요품/현장 달성/성 보고 불변조건.
- Actual: load는 허용하지만 Dungeon interaction은 중복 key를 “새로 추가할 수 없음”으로 처리하여 objective complete와 RESOLVED 전이를 하지 않는다.
- Impact: 변조·손상된 save 하나가 해당 quest와 오브젝트를 영구 진행 불가로 만들고, abandon/reaccept로도 기존 key item을 정상 제거할 수 없다.
- Suggested Action: load에서 `keyItems`와 retrieve object/quest 상태를 상호 검증해 reject하거나, 동일 quest·object의 이미 보유한 key를 idempotent success로 처리해 objective를 resolve한다. 양방향 state transition test를 추가한다.
- Re-audit Method: key item/object/quest의 3×3 상태 조합을 load하고, 정상 조합만 통과시키며 accepted 조합은 E interaction 후 한 번에 RESOLVED/READY가 되는지 확인한다.
- Confidence: High

### [A05-F008] v4 world snapshot parser가 필수 Door landmark의 누락을 허용한다

- Pass: Implementation Compliance / Debug
- Pattern: `IMP-001`, `TEST-001`
- Area: malformed world snapshot validation
- Severity: Minor
- Status: Confirmed
- Summary: 생성 계약과 content test는 각 맵에 Door landmark가 있음을 전제하지만 `DungeonMap::fromJson`은 up/down/boss 개수만 세고 Door 개수를 검사하지 않는다. `D`를 모두 `.`로 바꾼 malformed snapshot도 full save load가 성공한다.
- Evidence:
  - `spec.md:174-178, 204-207`은 Door와 층별 예약 타일을 던전 계약으로 정의한다.
  - `src/model/DungeonMap.cpp:408-454`는 `upCount`, `downCount`, `bossCount`만 검증하며 `doorCount`가 없다. 외벽·입구·도달성은 검사하지만 Door 존재는 검사하지 않는다.
  - `tests/test_content_contracts.cpp:223-243`은 생성 결과만 Door 1개를 확인하고, `tests/test_content_contracts.cpp:374-385` malformed test는 외벽 변조만 검사한다.
  - 직접 probe: `.world.floors[0].tiles`의 `D`를 `.`로 치환하고 `RngProcessReplayTests continue ...`를 실행했다. 결과 `missing_door_continue_rc=0`, `door_count_after_mutation=0`.
- Expected Basis: generated world snapshot이 저장·복원 후 같은 landmark contract를 유지해야 한다.
- Actual: Door가 0개인 연결 가능한 map도 `DungeonWorld::fromJson`과 Party load를 통과한다.
- Impact: 중앙 landmark와 진행 안내가 사라진 상태가 영속화되며, map generator와 snapshot validation의 계약이 서로 달라진다. 연결성은 남아도 gameplay landmark가 결손된다.
- Suggested Action: floor별 Door 정확히 1개와 위치/타일 계약을 검증하고 duplicate/missing Door 변조 회귀를 추가한다.
- Re-audit Method: Door 0/2, 각 특수 타일 swap, object 위치 swap snapshot을 load해 정확한 typed rejection과 정상 round-trip을 확인한다.
- Confidence: High

### [A05-F009] 정상 창 닫기에서 save `PersistenceResult`를 무시해 실패를 사용자에게 전달하지 않는다

- Pass: Debug / Engineering Quality
- Pattern: `DBG-001`, `TEST-001`
- Area: shutdown durability and user-visible failure handling
- Severity: Major
- Status: Confirmed
- Summary: `Game::processEvents`의 Closed 경로는 active save가 있으면 save를 호출하지만 결과를 버리고 창을 닫는다. pre-commit IoError는 최신 Party 변경을 잃을 수 있고 `CommittedDurabilityUnknown`도 사용자에게 유지 경고를 보여주지 않는다.
- Evidence:
  - `src/core/Game.cpp:60-69`은 `if (m_party.hasActiveSaveSession()) static_cast<void>(m_party.saveToFile()); m_window.close();`로 끝난다.
  - `spec.md:349, 353, 358`은 실패 typed result를 UI에 전달하고 정상 창 종료에서 즉시 checkpoint를 기록해야 한다고 정의한다.
  - 기존 tests에는 Closed event를 보내고 `IoError`/`CommittedDurabilityUnknown`에서 window close를 보류하거나 사용자 banner를 확인하는 케이스가 없다 (`rg -n -i 'Closed|window.close|processEvents' tests src`).
- Expected Basis: 정상 종료 시 save failure가 조용한 데이터 손실이 되지 않고 typed result가 표시·재시도 가능해야 한다.
- Actual: `.tmp` collision/권한/파일시스템 failure가 있어도 창을 닫고 caller는 result를 관찰하지 않는다. in-memory session은 프로세스 종료와 함께 사라진다.
- Impact: 마지막 town/world/quest/character 상태가 저장되지 않고 사용자에게 복구 선택권도 없다. durability unknown의 실제 commit 여부도 UI에 나타나지 않는다.
- Suggested Action: Closed 경로에서 result를 검사해 성공/unknown/실패별 banner와 retry/leave decision을 제공하고, 실패 시 window를 유지하거나 마지막 bytes와 복구 경로를 명시한다. shutdown failure injection을 추가한다.
- Re-audit Method: active save + dirty Party를 준비하고 temp/dir sync/write/commit-unknown을 각각 주입해 close event를 보낸다. window/state 유지, result banner, bytes와 다음 Continue 결과를 확인한다.
- Confidence: High

### [A05-F010] 고위험 failure mode와 production-link 범위를 13개 CTest가 직접 잠그지 못한다

- Pass: Debug / Engineering Quality
- Pattern: `TEST-001`, `IMP-003`
- Area: verification authority, production linkage, coverage gap
- Severity: Major
- Status: Confirmed
- Summary: Debug/Release 정상 계약은 통과하지만 RNG rollback, rollback reload failure, town-vs-dungeon checkpoint, symlink load, v4 legacy quest, key/object mismatch, Door 누락, shutdown save failure를 직접 회귀 잠그는 테스트가 없다. 또한 앱과 동일한 `CrawlmasterRuntime` library를 링크하는 것은 Controller/RNG/UI target에 한정되고 대부분 model harness는 소스 목록을 다시 컴파일한다.
- Evidence:
  - `CMakeLists.txt:85-91`에서 앱은 `CrawlmasterRuntime`을 링크한다. `CMakeLists.txt:103-246`의 `TestHarness`, `HudContractTests`, `CombatContractTests`, `ContentContractTests`, `AgencyContractTests`, `LocalizationContractTests`, `FontRasterTests`는 각자 `src/*.cpp`를 소스 목록에 복제한다.
  - `CMakeLists.txt:257-285`에서 `ControllerContractTests`, `RngProcessReplayTests`, `UiStateRasterTests`만 `CrawlmasterRuntime`을 링크한다.
  - 기존 high-risk test 범위는 `tests/test_controller_contracts.cpp:504-531, 589-613`, `tests/test_content_contracts.cpp:314-401`, `src/test_harness.cpp:559-667, 1176-1221`의 정상·좁은 failure case다. 위 F001~F009에서 직접 probe와 정적 경로로 확인한 공백은 테스트 이름/실패 assertion에 없다.
  - 실행 결과는 Debug/Release 개별 계약과 Debug CTest 13/13 PASS였으나, 이는 위 미검증 failure mode의 부재를 반증하지 않는다.
- Expected Basis: `AI_AUDIT_DOC_STANDARD.md`의 `TEST-001`, `IMP-003`, `DBG-002`; 완료 주장에는 실제 책임 구현과 결정적 failure regression이 연결되어야 한다.
- Actual: aggregate green gate가 atomic/rollback/legacy/world cross-state를 정상 경로로만 통과시킨다. model test source duplication은 runtime archive와 drift를 자동 차단하지 않는다.
- Impact: 핵심 데이터 무결성 결함이 release-safe CTest 13/13 뒤에 숨을 수 있고, 구현 변경 시 앱과 contract test가 서로 다른 source/link 경로를 통과할 수 있다.
- Suggested Action: high-risk integration target을 `CrawlmasterRuntime`에만 링크하도록 정리하고, 최소한 F001~F009 각 failure를 이름 붙인 production-linked test로 추가한다. bytes, typed status, Party/World/Quest/RNG state와 post-failure retry를 직접 assert한다.
- Re-audit Method: fresh Debug/Release configure/build에서 target linkage를 확인하고, failure injection matrix와 malformed save matrix를 모두 실행한다. test binary가 runtime archive를 실제 링크하며 각 finding의 pre/post invariant를 fail-fast로 잠그는지 확인한다.
- Confidence: High for coverage gap; source duplication becomes a defect when target/source lists drift.

## 6. Uncertainties and Clarifications Needed

- `spec.md:53-54, 352-354`와 `DESIGN_DECISIONS.md:53, 58`의 town-only/last-town-checkpoint 문장이 `spec.md:353`의 2초 dungeon exploration autosave와 충돌한다. A05-F003의 구조적 사실(별도 checkpoint 없음)은 확인되지만, TPK에서 world/FOW/object를 rollback할지 보존할지는 제품 결정이 필요하다.
- v1/v2에 seed가 없는 경우의 deterministic fallback은 현재 권위 문서에 정의되어 있지 않다. 저장 bytes hash, 고정 seed, migration 거부 중 하나를 명시해야 한다.
- 실제 power-loss 중 rename/dir fsync 결과, hosted Windows symlink/reparse-point 동작, multi-writer race는 이 실행에서 검증하지 않았다. 이 범위는 PASS로 승격하지 않는다.
- `CommittedDurabilityUnknown`은 의도상 commit을 되돌리지 않는 것이 맞지만, backup recovery의 primary restore 실패는 현재 `RecoveredFromBackup` truthy 상태로 합쳐져 caller가 구분하지 못한다. 이는 A05-F002/F009와 연결된 추가 재감사 항목이다.

## 7. Perspective Decision

- Decision: `HOLD`
- Rationale: A05-F001, A05-F002, A05-F003, A05-F004, A05-F005, A05-F006, A05-F007, A05-F009, A05-F010의 Major finding이 현재 코드와 호출 경로로 확인되었다. 정상 Debug/Release 계약의 green 결과는 pre-commit rollback, town checkpoint, malformed/cross-state, shutdown failure를 보장하지 않는다. A05-F008은 Minor이지만 snapshot contract gap이다.
- Verified strengths (non-gate): 정상 nonzero-seed 3층 generation/round-trip, v3 migration, New Game reset, valid backup quarantine/recovery, valid save parse-before-assign, forced boss no-escape path, TPK `replaceAll` root replacement, normal process RNG Continue replay는 실행·소스·테스트가 같은 결론을 지지했다.
- Required before this perspective can pass: (1) town checkpoint 의미를 문서/구현에서 확정, (2) transaction에 Party/World/Quest/key item/RNG rollback을 포함, (3) v1/v2 deterministic seed 정책 확정, (4) load/write symlink boundary와 v4 cross-state validation 강화, (5) shutdown result handling과 production-linked failure tests 추가.
- Re-audit trigger: 위 finding 관련 source/spec/test/build target 중 하나라도 변경되면 해당 호출 경로·문서·실패 주입·independent process evidence를 다시 실행한다.

