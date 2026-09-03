# Sub Audit Report

## 1. Audit Metadata

- Audit Turn: 1
- Perspective: 제품 목표·계약·문서·구현·제품 포지셔닝 정합성
- User Goal: `$multi-audit` 위저드리 스타일 게임으로서의 인터페이스, 게임요소, 컨텐츠가 상용수준인지 감사하고 부족한 부분의 개선방법을 확인한다.
- Audit Basis: Standard-backed / Goal-driven
- Standard Path: `/mnt/Projects_SSD/cpp/crawlmaster/AI_AUDIT_DOC_STANDARD.md`
- Companion Contract: `/home/eunho1/.codex/skills/multi-audit/references/report-contract.md`
- Evidence date: 2026-09-03

## 2. Assigned Scope

제품 계약 관점에서 다음을 독립적으로 대조했다.

- `spec.md`, `designs.md`, `README.md`, `IMPLEMENTATION_SUMMARY.md`, `DESIGN_DECISIONS.md`, `BUILD_GUIDE.md`, `CHANGELOG.md`, `LESSONS_LEARNED.md`, `audit_roadmap.md`
- `CMakeLists.txt`, 전체 `include/`·`src/`의 상태 전이, 파티/세이브, 던전·전투·퀘스트·아이템·스킬·렌더링 호출 경로
- `src/test_harness.cpp`와 테스트 타깃 구성
- `assets/lang/*.json`, `assets/fonts/*`, 루트 `save.json`
- 빌드·실행·세이브 경로와 제품 주장(신규 게임/이어하기, 완결 목표, 콘텐츠 도달성, i18n, 출시 산출물)

## 3. Excluded and Uninspected Scope

- `build/**`의 FetchContent 소스, 생성물, 벤더 코드는 제품 증거에서 제외했다. 빌드 결과 바이너리와 명령 결과만 실행 가능성 확인에 사용했다.
- 기존 `audit_report_*.md`, `docs/audit/**`, 다른 `docs/multi_audit/**` 보고서는 읽지 않았다. 배정된 `audit_run.json`과 본 보고서 경로만 예외로 확인했다.
- Windows/macOS 실기 실행, 스토어 백엔드, 네트워크·유료 외부 호출, 파괴적 저장소 시험, 장시간 완주 플레이는 수행하지 않았다.
- 전체 GUI 조작·화면 캡처는 수행하지 않았다. Linux에서 실행 파일을 2초 제한으로 기동했으며, 프로세스가 유지된 뒤 종료된 것은 시간 제한에 의한 것이다.
- Git 메타데이터는 루트에 존재하지 않아 변경 이력은 프로젝트 문서와 현재 트리만으로 판정했다.

## 4. Evidence Examined

문서상 마스터 기준은 `spec.md`다. 제품 목표와 완료 정의는 `spec.md:9-31`, 상태 흐름은 `spec.md:93-123`, 저장 계약은 `spec.md:289-350`, 콘텐츠 표와 획득 제약은 `spec.md:385-432`, Phase/검증 명령은 `spec.md:434-471`에 있다. UI 계약은 `designs.md:29-89`, `designs.md:228-281`을 확인했다.

핵심 구현 증거는 다음과 같다.

- Title/Town/Dungeon 상태 전이: `src/controller/TitleState.cpp:18-42`, `src/controller/TownState.cpp:30-73`, `src/controller/DungeonState.cpp:13-27`, `src/controller/DungeonState.cpp:177-203`
- Dungeon lifecycle/reward: `src/controller/DungeonState.cpp:205-267`, `src/controller/CombatState.cpp:699-746`, `src/model/DungeonMap.cpp:24-50`
- 저장 및 복원: `include/model/Party.hpp:53-66`, `src/model/Party.cpp:11-16`, `src/model/Party.cpp:88-202`, `src/model/Character.cpp:384-471`
- 콘텐츠 등록/상점/퀘스트: `src/model/ItemFactory.cpp:10-109`, `src/model/MonsterFactory.cpp:10-77`, `src/controller/TownState.cpp:121-263`
- 실제 HUD/UI: `src/view/DungeonRenderer.cpp:205-300`, `src/controller/TownState.cpp:318-510`, `src/controller/CharacterInfoState.cpp:100-238`, `src/controller/CombatState.cpp:790-879`
- 테스트 범위: `CMakeLists.txt:61-113`, `src/test_harness.cpp:52-72`, `src/test_harness.cpp:347-404`, `src/test_harness.cpp:795-848`

실행 명령과 핵심 결과:

```text
cmake --build build --parallel 2
-> Built target TestHarness
-> Built target Crawlmaster

./build/TestHarness --run-all
-> exit=0; 모든 단위 테스트 검증 완료.

ctest --test-dir build -N
-> Total Tests: 0

timeout 2s ./build/Crawlmaster
-> exit=124 (시간 제한); 창 생성 및 게임 루프는 유지됨.
-> Failed to create input context for window ...
-> Setting vertical sync not supported
```

테스트 전후 루트 `save.json` SHA-256은 `6c19075f10fbc2e242b0a6bbb588828f83b185b0d73b4897c48fa7ffb9a48518`로 동일했다. 이는 해당 테스트가 루트 세이브를 보존했다는 증거이지, 제품 세이브 계약 전체가 검증됐다는 뜻은 아니다.

## 5. Findings

### Pass 1: Implementation Compliance

### [A01-F001] 상용 출시 포지셔닝과 release scope가 명세에 닫혀 있지 않음

- Pass: Implementation
- Pattern: `SPEC-GAP-001`
- Area: 제품 포지셔닝, release scope, acceptance authority
- Severity: Major
- Status: Needs Clarification
- Summary: 장르·기술·기본 루프는 정의되어 있지만 누가 어떤 형태로 구매하는 제품인지, 어느 범위를 v0.9.4 출시로 약속하는지, 캠페인 완주 조건과 지원 플랫폼/산출물이 무엇인지가 닫혀 있지 않다.
- Evidence:
  - `spec.md:9-31`은 장르, 데스크톱 기술, 기능 및 완료 정의를 설명하지만 대상 사용자, 가격 모델(정확한 가격은 TBD로 둘 수 있음), 제품 형태(데모/EA/정식), 예상 플레이 길이, 완주 조건을 정의하지 않는다.
  - `README.md:9-22`는 완성된 데스크톱 RPG처럼 소개하지만 release lane, 지원 OS 범위, 패키지 산출물은 없다.
  - `audit_roadmap.md:75-79`는 다층 던전·사운드·비주얼을 “Post-Launch”로 부르지만 현재 정식 출시 범위와의 경계나 gate를 제시하지 않는다.
- Expected Basis: 사용자 감사 계약의 “소규모 유료 PC 게임의 신뢰 가능한 출시 후보” 가정, `AI_AUDIT_DOC_STANDARD.md`의 `SPEC-GAP-001` 및 `IMP-003`. 정확한 가격이나 Wizardry의 10층을 새 요구사항으로 창작하는 것이 아니라, 제품이 무엇을 약속하는지 명시해야 한다.
- Actual: 문서는 기능 목록과 기술 버전을 완료처럼 기술하지만 대상 고객·콘텐츠 길이·완주·출시 형태·지원 플랫폼·패키지 acceptance의 권위를 제공하지 않는다.
- Impact: “상용 수준”과 v0.9.4 완료/포스트런치 주장을 객관적으로 판정할 기준이 없어 전체 PASS 계열 판정이 불가능하다. 이후 구현 gap을 의도된 미지원인지 결함인지 분류하기도 어렵다.
- Suggested Action: `spec.md`에 제품 포지셔닝 섹션을 추가해 대상 플레이어, 유료/EA/데모/정식 중 release lane, 가격 TBD 정책, 지원 OS, 최소 플레이·완주 기준, 세이브·패키지 acceptance를 명시한다. 결정 전 항목은 `가정`/`미확정`으로 표시하고 README/CHANGELOG/로드맵의 완료 문구를 같은 범위로 맞춘다.
- Re-audit Method: 갱신된 제품 계약에서 각 README 기능과 Phase 결과를 도메인 구현·UI·패키지·테스트로 매핑하고, 미확정 항목이 별도 gate로 남아 있는지 확인한다.
- Owner: Architect / Human
- Confidence: High
- Notes: 이 finding은 특정 가격이나 10층 캠페인을 요구하지 않는다. 최소한 유료 출시 후보의 명시적 범위와 완주 기준이 필요하다는 계약 공백이다.

### [A01-F002] 완주 가능한 목표·진행 곡선·엔딩이 없음

- Pass: Implementation
- Pattern: `IMP-001`, `IMP-002`
- Area: 핵심 루프, 캠페인 완결성, progression
- Severity: Major
- Status: Confirmed
- Summary: 현재 제품은 마을-단일 랜덤 미로-무작위 전투-마을 반복 루프만 구현되어 있고 플레이어가 도달할 최종 목표, 진행 단계, 승리/엔딩 상태가 없다.
- Evidence:
  - `spec.md:93-123`의 FSM에는 `TitleState`, `TownState`, `DungeonState`, `CombatState`, `GameOverState`만 있고 캠페인 승리·보스·완주 상태가 없다.
  - `src/controller/DungeonState.cpp:13-16`은 던전 상태가 생성될 때마다 새 20x20 맵을 생성한다.
  - `src/model/DungeonMap.cpp:24-50`에는 한 맵과 입구 계단만 있고 깊이/층/최종 목표 데이터가 없다.
  - `src/model/MonsterFactory.cpp:52-77`은 위치·진행도와 무관한 단일 무작위 몬스터 분포만 제공한다.
  - `audit_roadmap.md:75-79`의 다층 던전은 후속 항목으로 남아 있다.
- Expected Basis: 사용자 감사 계약의 일반 상용 불변조건 “완결 가능한 목표와 진행 곡선”. Wizardry 공식 포털의 10층을 그대로 요구하는 것이 아니라, 프로젝트가 스스로 약속한 유한한 완주 경로가 있어야 한다는 판단이다.
- Actual: 전투 승리는 `src/controller/CombatState.cpp:213-227`에서 기존 던전으로 돌아가는 분기일 뿐이며, 두 퀘스트를 완료해도 별도 캠페인 종료가 없다. TPK만 타이틀로 돌아간다.
- Impact: 사용자는 제품을 완료할 수 없고, 반복 탐험의 보상·난이도·목표 곡선이 닫히지 않는다. 현재 상태를 정식 유료 게임으로 포지셔닝하면 기대 불일치와 조기 이탈 위험이 크다.
- Suggested Action: 현재 문서 범위에 맞는 최소한의 유한 캠페인(층 수는 별도 결정), 목표/보스 또는 대체 엔딩, 해금·난이도·보상 곡선을 명시하고 상태/세이브/테스트에 연결한다. 이를 구현하지 않을 경우 제품을 프로토타입/EA로 재포지셔닝하고 “완결성 보장” 및 정식 출시 표현을 제거한다.
- Re-audit Method: 새 세이브에서 파티 구성→탐험→전투→퀘스트→최종 목표까지 스크립트 입력으로 진행해 승리 상태/엔딩/재시작을 확인하고, 동일 경로의 저장·복원을 검증한다.
- Owner: Architect / Coder
- Confidence: High
- Notes: 장르의 특정 층 수나 보스 설계를 감사자가 창작하지 않고, 프로젝트 계약이 최소 하나의 완주 경로를 닫도록 요구한다.

### [A01-F003] Title의 “신규 게임”과 “이어하기” 계약이 실제 동작과 다름

- Pass: Implementation
- Pattern: `IMP-001`
- Area: title UX, new game/load semantics, progression reset
- Severity: Major
- Status: Confirmed
- Summary: 번역 리소스와 안내 문구에는 신규 게임/이어하기가 모두 존재하지만 실제 타이틀에는 신규 게임만 있고, 그 신규 게임도 기존 `save.json`을 자동 로드한다.
- Evidence:
  - `assets/lang/en.json` 등 5개 리소스에 `TITLE_NEW_GAME`과 `TITLE_LOAD_GAME`이 모두 있고, 각 리소스의 `TITLE_PRESS_KEY`도 생성 또는 로드를 안내한다.
  - `src/controller/TitleState.cpp:20-37`은 메뉴를 3개로 순환하며 index 0에서만 `TownState`를 생성하고, index 1은 Settings, index 2는 종료다.
  - `src/controller/TitleState.cpp:89-91`은 `TITLE_NEW_GAME`, `TITLE_SETTINGS`, `TITLE_EXIT`만 그린다. `TITLE_LOAD_GAME`의 제품 호출 경로는 없다.
  - `src/controller/TownState.cpp:30-35`와 `src/model/Party.cpp:11-16`은 Town/Party 생성 시 기존 기본 세이브를 로드한다.
- Expected Basis: `spec.md:93-100`의 “신규 게임 생성 / 세이브 로드”, `README.md:19`, UI의 두 title 키. 신규 시작과 이어하기는 서로 다른 사용자 의도를 보존해야 한다.
- Actual: 이전 진행이 존재해도 “Start New Game”은 이를 초기화하지 않고, 유효한 세이브가 없어도 별도 이어하기 경로가 없다. 새 캠페인을 시작하려면 파일을 외부에서 지워야 한다.
- Impact: 새 게임 시작의 의미가 깨지고 기존 진행을 덮어쓰거나 계속하는 사용자 흐름을 제어할 수 없다. 하드코어 리셋 게임에서는 특히 치명적인 UX 혼선이다.
- Suggested Action: Title에 `Continue`와 `New Game`을 분리하고 유효 세이브가 있을 때만 Continue을 활성화한다. New Game은 명시적 확인 후 새 기본 세이브를 만들고, 기존 세이브는 백업/슬롯/삭제 정책을 문서화한다.
- Re-audit Method: 기존 세이브가 있는 상태와 없는 상태에서 각 title 메뉴 입력을 이벤트로 주입해 상태 전이, 세이브 바이트, 파티 구성 결과를 비교한다.
- Owner: Architect / Coder
- Confidence: High
- Notes: title에서 load 키를 의도적으로 제거한 것이라면 locale/README/spec를 그 결정에 맞춰 정리해야 한다.

### [A01-F004] 저장 파일 스키마와 초기 기본값이 마스터 스펙과 드리프트함

- Pass: Implementation
- Pattern: `IMP-001`, `IMP-003`
- Area: save contract, schema compatibility, new-player defaults
- Severity: Major
- Status: Confirmed
- Summary: `spec.md`가 정의한 필드명·중첩 구조·초기 마나 물약과 실제 직렬화·기본 세이브가 서로 다르다.
- Evidence:
  - `spec.md:289-340`은 초기 인벤토리를 `pot_heal` 2개와 `pot_mana` 1개로 정의하고, 캐릭터에 `maxHp`, `spellSlots`, `maxSpellSlots`, `poisonTurns`, `paralysisTurns`, `abilities.strength` 등과 `equipment.weapon` 중첩 구조를 예시한다.
  - `src/model/Party.cpp:191-201`은 기본 인벤토리에 `pot_heal` 2개만 넣는다. 현재 `save.json:1-9`도 골드 100, 치유 물약 2개, 멤버 없음이다.
  - `src/model/Character.cpp:384-414`는 `max_hp`, `spell_slots`, `max_spell_slots`, `poison_turns`, `abilities.str`, `eq_weapon` 등 다른 스키마를 쓴다. `src/model/Character.cpp:417-469`도 이 비표준 키만 읽는다.
  - `src/test_harness.cpp:52-72`는 실제 `Party::saveToFile()`나 스펙 스키마가 아니라 `party_gold`/`depth`라는 임의 JSON을 검증한다.
- Expected Basis: `spec.md:302-350`의 명시적 Save File Contract와 `IMP-001`/`IMP-003`. 마스터 스펙 예시를 제품 입력으로 취급할지 다른 canonical schema를 정할지 문서와 코드가 같아야 한다.
- Actual: 스펙 예시 파일은 `fromJson()`이 요구하는 `max_hp`·`abilities.str`·`eq_weapon` 등을 찾지 못해 예외 복구 경로로 갈 수 있고, 신규 파티는 문서에 약속된 마나 물약 없이 시작한다. 호환 마이그레이션도 없다.
- Impact: 문서화된 세이브를 사용한 복원과 외부 도구/QA fixture가 실패하거나 기본값으로 리셋된다. 신규 플레이어의 초반 자원·튜토리얼 기대도 문서와 달라진다.
- Suggested Action: 하나의 버전 있는 canonical schema를 선택해 스펙·구현·fixture를 맞춘다. 기존 short/snake_case 형식이 유지되어야 하면 명시적 migration을 추가하고, 기본 세이브에 `pot_mana`를 넣을지 제품 계약을 먼저 확정한다. 필드 존재·타입·범위와 round-trip을 실제 Party 테스트로 잠근다.
- Re-audit Method: 스펙 canonical fixture와 실제 `saveToFile()` 산출물을 모두 새 Party로 로드하고, 필드/기본 인벤토리/장비/퀘스트 round-trip을 바이트·구조 비교한다.
- Owner: Architect / Coder
- Confidence: High
- Notes: 현재 자체 저장 후 자체 로드는 일부 동작할 수 있으나, 그것은 마스터 Save File Contract 정합성 증거가 아니다.

### [A01-F005] README가 약속한 던전 좌표·진행 저장이 구현되지 않음

- Pass: Implementation
- Pattern: `IMP-001`, `IMP-003`
- Area: dungeon persistence, resume flow, save/load product claim
- Severity: Major
- Status: Confirmed
- Summary: README는 파티 데이터뿐 아니라 던전 진행/좌표를 저장한다고 홍보하지만 Party save에는 던전 상태가 없고, 던전 재진입 시 매번 새 맵이 생성된다.
- Evidence:
  - `README.md:18-20`은 안정적 즉시 저장과 `save.json`의 “던전 진행 상황”을, `README.md:56-58`은 좌표(serializes ... coordinates)를 약속한다.
  - `spec.md:302-340`의 Save File Contract에도 map seed/layout, depth, position, direction, visited/stepped 상태가 없다.
  - `src/model/Party.cpp:88-118`은 gold/inventory/members/active_quests만 쓴다.
  - `src/controller/DungeonState.cpp:13-16`은 생성 시 `m_map.generate()`를 호출하고, `src/controller/DungeonState.cpp:177-183`은 계단에서 `changeState(TownState)`로 현재 던전 상태를 제거한다.
  - `src/controller/DungeonState.cpp:69-203`의 이동/회전 경로에는 파티 세이브나 DungeonMap 직렬화 호출이 없다.
- Expected Basis: README의 명시적 좌표·진행 저장 주장, `spec.md:21`, 사용자 계약의 저장 안전성. 구현하지 않는 run-only 설계라면 해당 마케팅 문구를 제거해야 한다.
- Actual: 프로세스를 던전에서 닫거나 계단으로 돌아온 뒤 재진입하면 좌표·맵·안개·방문 경로가 복원되지 않는다. 동일 세이브는 파티만 복구한다.
- Impact: 긴 탐험 진행이 손실되고 던전 진행 저장이라는 제품 주장이 허위가 된다. 단일 층 반복과 결합되어 핵심 루프의 지속성이 약해진다.
- Suggested Action: `DungeonProgress`를 세이브 계약에 추가해 seed/layout 또는 안정적인 map identity, depth, position/direction, visited/stepped와 version을 원자적으로 저장하고 재진입 시 복원한다. 저장하지 않을 계획이면 README/spec를 파티-only 저장으로 좁히고 종료 시 사용자에게 명시한다.
- Re-audit Method: 임시 세이브 경로에서 탐험·회전·안개 해제 후 저장/프로세스 재시작/재진입을 수행해 같은 map identity, 좌표, 방향, FOW와 퀘스트가 복원되는지 확인한다.
- Owner: Architect / Coder
- Confidence: High
- Notes: 현재 환경에서 장시간 플레이는 제외했지만, 저장 필드와 lifecycle 호출 경로만으로 누락을 확정할 수 있다.

### [A01-F006] 문서화된 콘텐츠가 실제 플레이 획득·퀘스트 경로에 연결되지 않음

- Pass: Implementation
- Pattern: `IMP-002`, `IMP-003`
- Area: content reachability, loot loop, quest catalog
- Severity: Major
- Status: Confirmed
- Summary: 아이템 18종과 퀘스트 3종을 계약했지만 실제 플레이에서는 기본 상점 8종과 퀘스트 2종만 노출되고, 던전/전투의 loot 경로가 없다.
- Evidence:
  - `spec.md:387-408`은 18개 아이템을, `spec.md:410-413`은 기본 8종만 상시 상점에 두고 고급 장비·특수 소모품은 던전 파밍/퀘스트 보상으로만 획득하도록 동결한다.
  - `spec.md:427-432`에는 `qst_hunt_spiders`가 추가되어 있다.
  - `src/controller/TownState.cpp:136-143`의 실제 구매 입력은 8개 기본 ID뿐이고, `src/controller/TownState.cpp:213-261`에는 `qst_clear_kobolds`와 `qst_collect_maces`만 있다. `qst_hunt_spiders`는 source 호출 경로가 없다.
  - `src/controller/CombatState.cpp:699-745`의 승리 보상은 gold/XP/kill progress/save뿐이며 `addItem()` loot가 없다. `src/controller/DungeonState.cpp`에도 아이템 pickup 경로가 없다.
  - `src/model/ItemFactory.cpp:88-109`의 `getShopCatalog()`은 18개를 반환하지만 호출처가 없고, 기본 8종 상점 계약과 분리된 고아 구현이다.
- Expected Basis: `spec.md:29-30`, 콘텐츠 기준표와 상점 획득 제약. 문서에 있는 콘텐츠는 의도된 후속 항목인지 현재 release 콘텐츠인지 명시하고, 현재 release로 주장한다면 도달 가능한 경로가 있어야 한다.
- Actual: 일반 사용자는 고급 아이템·특수 소모품을 상점에서 구매할 수 없고 던전/퀘스트에서도 얻을 수 없다. 거미 사냥 퀘스트는 수락·진행·보고 자체가 불가능하다. 메이스 수집은 기본 상점 구매로만 강제 주입할 수 있다.
- Impact: 콘텐츠 반복성이 높은 단일 루프가 되고, 문서화된 전략 요소와 보상 선택이 실질적으로 작동하지 않는다. v0.8 확장 완료 주장은 실제 플레이 커버리지를 과대평가한다.
- Suggested Action: loot table/드롭 조건/퀘스트 보상과 `qst_hunt_spiders` Castle UI·진행·보고를 구현하거나, 이들을 명시적 후속 Phase로 격리한다. 상점 catalog는 한 canonical source에서 기본 8종과 파밍 전용 목록을 분리하고, 아이템·퀘스트별 reachable route를 표로 남긴다.
- Re-audit Method: clean save에서 각 퀘스트를 수락하고 목표 아이템/몬스터를 정상 플레이 경로로 얻어 완료·보상·소모·저장을 검증한다. 각 18개 아이템과 3개 퀘스트에 최소 하나의 호출 가능한 획득/사용 경로가 있는지 source+scripted scenario로 확인한다.
- Owner: Architect / Coder
- Confidence: High
- Notes: `ItemFactory` 객체 생성 가능성만으로 콘텐츠가 shipped되었다고 보지 않았다. 실제 소비 경로와 상태 변화가 기준이다.

### [A01-F007] 동결된 전투 골드 보상 공식이 구현과 다름

- Pass: Implementation
- Pattern: `IMP-003`
- Area: economy, reward progression, content balance
- Severity: Major
- Status: Confirmed
- Summary: 스펙은 몬스터 레벨에 비례한 `1d10 * monster level`을 동결했지만 구현은 몬스터 정체성·레벨을 무시한 5~15 범위와 적 수만 사용한다.
- Evidence:
  - `spec.md:352-362`의 동결 공식은 전투 종료 골드를 `(1d10 * 몬스터 레벨)`로 정의한다.
  - `include/model/Monster.hpp:50-56`와 `src/model/MonsterFactory.cpp:10-47`에는 monster level 필드/인자가 없다.
  - `src/controller/CombatState.cpp:710-713`은 `uniform_int_distribution<>(5, 15)`을 굴려 `foes.size()`만 곱한다.
- Expected Basis: 스펙의 “동결된 공식”과 `IMP-003`의 완료/검증 기준. 숫자 설계를 바꾸는 것은 허용될 수 있지만 문서와 단일 경제 계약을 먼저 맞춰야 한다.
- Actual: 약한 코볼트와 새끼 용이 같은 전투 수 기준으로 비슷한 골드 범위를 공유하며, 레벨별 진행 경제가 존재하지 않는다.
- Impact: 구매·파밍·레벨업 곡선을 예측하거나 밸런싱할 수 없고, 유료 게임의 보상 신뢰성과 콘텐츠 난이도 gate가 닫히지 않는다.
- Suggested Action: 각 몬스터에 명시적 level/tier와 공식 적용을 추가하거나, 현재 5~15×count를 승인된 경제 계약으로 재정의한다. 보상 계산을 단일 함수로 분리하고 monster별/encounter별 fixture 및 경계 테스트를 추가한다.
- Re-audit Method: 모든 몬스터 ID와 1~3마리 encounter fixture에서 보상 범위·공식·세이브 결과를 검증하고, 문서의 공식과 테스트 기대값이 일치하는지 확인한다.
- Owner: Architect / Coder
- Confidence: High
- Notes: 몬스터 처치 XP/퀘스트 진행은 별도 코드 경로이며, 이 finding은 골드 보상 공식만 다룬다.

### [A01-F008] 던전 HUD가 실제 파티가 아닌 고정 샘플 데이터를 표시함

- Pass: Implementation
- Pattern: `IMP-001`
- Area: interface feedback, party state visibility, dungeon UX
- Severity: Major
- Status: Confirmed
- Summary: 디자인이 요구하는 던전 파티 상태/피드백 영역이 실제 `Party`를 읽지 않고 네 명의 가상 캐릭터와 고정 HP를 표시한다.
- Evidence:
  - `designs.md:31-58`은 미니맵 좌표/Level과 실제 파티 상태를 표시하는 레이아웃을 정의한다.
  - `src/view/DungeonRenderer.cpp:241-267`은 `[PARTY STATUS]`와 `1. WARRIOR HP:18/18` 등 고정 문자열을 `tempMembers`로 그리며 `m_game.getParty()`를 호출하지 않는다.
  - `TownState`만 `src/controller/TownState.cpp:440-470`에서 파티를 동적으로 읽는다. 던전 렌더러에는 동일한 동적 경로가 없다.
- Expected Basis: `designs.md`의 HUD 역할, 사용자 목표의 조작/피드백 불변조건, `IMP-001`의 UI 레이어 완료 기준. 레트로 미니멀 그래픽을 결함으로 보는 것이 아니라, 실제 상태와 시각 피드백이 일치해야 한다는 판정이다.
- Actual: 파티가 0명이거나 사망/레벨업/장비 변경을 해도 던전 HUD에는 같은 네 명과 HP가 남는다. 플레이어가 전투 위험과 현재 파티 상태를 신뢰할 수 없다.
- Impact: 조작 결과와 생존 여부를 잘못 안내하며, 하드코어 TPK와 결합될 때 사용자 행동을 오도한다. “상용수준 인터페이스”의 핵심 피드백 조건을 충족하지 못한다.
- Suggested Action: DungeonRenderer가 현재 Party를 읽어 멤버 이름/직업/HP/레벨/상태를 동적으로 그리게 하고, 미니맵에 실제 좌표·층을 표시하거나 해당 디자인을 제거한다. 빈 파티·사망·4인·긴 이름·다국어 레이아웃을 별도 fixture로 검증한다.
- Re-audit Method: 0/1/4인, HP 변동, TPK 직전, 장비/레벨 변경 상태를 렌더링해 HUD 문자열이 Party snapshot과 동일한지 화면 캡처 또는 test seam으로 비교한다.
- Owner: Coder
- Confidence: High
- Notes: 이 finding은 그래픽 품질이나 ASCII 스타일이 아니라 잘못된 런타임 데이터 표시를 다룬다.

### [A01-F009] 5개 국어 완료 주장이 전체 사용자 문자열을 포괄하지 않음

- Pass: Implementation
- Pattern: `IMP-001`
- Area: localization contract, content presentation, user-facing text
- Severity: Major
- Status: Confirmed
- Summary: 번역 JSON 키와 타이틀 회귀는 존재하지만 핵심 화면에 하드코딩된 영어/한국어 레이블·로그·콘텐츠 이름이 남아 있어 5개 국어 “모든 UI 텍스트” 계약이 닫히지 않는다.
- Evidence:
  - `spec.md:444-449`는 모든 UI 텍스트를 `LocalizationManager`에서 가져오도록 요구한다. `README.md:20`도 5개 국어 실시간 전환을 제품 기능으로 약속한다.
  - `src/controller/TownState.cpp:341-365,391-393`에는 `Guild Desk`, `Shop Menu`, `Shop Catalog (Buy)`, `Temple Sanctuary`와 영어 아이템명이 직접 있다.
  - `src/controller/CharacterInfoState.cpp:131-141`에는 `Name:`·`Class:`가 직접 있고, `src/view/DungeonRenderer.cpp:241-263`에는 고정 영어 HUD/조작 안내가 있다.
  - `src/controller/CombatState.cpp:31-32,842-854`와 `src/controller/DungeonState.cpp:87-175,244-265`에는 영어·한국어 전투/이동 로그와 `Slot 1`, `No Cost`가 직접 있다.
  - `src/model/ItemFactory.cpp:14-67`, `src/model/MonsterFactory.cpp:13-46`, `include/model/ConcreteSkills.hpp:16-18`의 이름/설명은 한국어 단일 값이며 locale key가 아니다.
- Expected Basis: `spec.md:444-449`, `designs.md:264-267`, `AI_AUDIT_DOC_STANDARD.md`의 Forward Sync. key 존재/글리프 존재는 전체 사용자-facing 문자열 번역의 증거가 아니다.
- Actual: EN/JA/ZH 설정에서도 화면 일부와 전투/던전 로그, 아이템·몬스터·스킬 명칭이 영어 또는 한국어로 혼합된다. 현재 테스트 `testTownHubLocalizationKeyCoverage`는 Town 허브 9개 키만 확인한다.
- Impact: 언어 선택이 일관된 사용자 경험을 제공하지 못하고, CJK 폰트/레이아웃 검증도 실제 content surface를 다루지 않는다. README의 다국어 완료 주장과 출시 신뢰도가 떨어진다.
- Suggested Action: 사용자-facing 문자열을 key/placeholder 기반 catalog로 외부화하고 아이템·몬스터·스킬·로그에도 locale ID를 연결한다. 고정 HUD와 안내 문구를 동적/번역 데이터로 교체하고, 모든 상태·행동·오류 메시지의 locale matrix를 만든다.
- Re-audit Method: 소스의 직접 UI 문자열 검색 결과를 허용 목록과 대조하고, 5개 언어에서 Title→Town→Character→Dungeon→Combat→Settings 전 경로를 렌더링해 raw key/타언어 잔존/잘림을 캡처로 확인한다.
- Owner: Architect / Coder
- Confidence: High
- Notes: 레트로 단색/미니멀 미학은 유지할 수 있다. 문제는 스타일이 아니라 언어 계약과 실제 문자열 데이터의 불일치다.

### [A01-F010] 완료 문서와 현재 구현의 authority/Phase가 서로 충돌함

- Pass: Implementation
- Pattern: `IMP-004`
- Area: documentation authority, phase claims, reverse synchronization
- Severity: Major
- Status: Confirmed
- Summary: v0.9.4가 현재 무엇을 완료했는지 문서마다 다르고, 보조 구현 요약에는 존재하지 않는 renderer 파일과 이전 Phase 설명이 남아 있다.
- Evidence:
  - `IMPLEMENTATION_SUMMARY.md:3`은 “Phase 1 개발의 시작점”이라고 쓰면서 `IMPLEMENTATION_SUMMARY.md:120-135`에서는 Phase 4~7 최종 플레이어블과 v0.9.4 수정을 완료처럼 기술한다.
  - `spec.md:455-458`에는 Phase 10/v0.9.4가 있지만 `audit_roadmap.md:35-71`에는 Phase 9와 Phase 1~8만 있고 Phase 10 섹션이 없다.
  - `spec.md:52-55`와 `IMPLEMENTATION_SUMMARY.md:55-59`는 `TownRenderer`, `CombatRenderer`, `UIRenderer` 책임/경로를 언급하지만 현재 `rg --files` 및 `CMakeLists.txt:61-113`에는 `DungeonRenderer`만 있다. `src/view/UIRenderer.cpp`, `include/view/UIRenderer.hpp`, `src/view/TownRenderer.cpp`, `src/view/CombatRenderer.cpp`는 존재하지 않는다.
  - `CHANGELOG.md:7-17`과 `CHANGELOG.md:196-202`에 동일한 `[0.9.4]` 섹션이 두 번 있다. `DESIGN_DECISIONS.md:108-112`는 폰트 provenance를 Human Review Required로 남기지만 README는 완료 기능처럼 소개한다.
- Expected Basis: `AI_AUDIT_DOC_STANDARD.md`의 `IMP-004` 및 `DOC-BACKFILL-001`: primary authority, snapshot 시점, 현재 소스와의 양방향 동기화가 필요하다.
- Actual: 구현·문서·로드맵의 Phase/파일 책임/출시 gate가 같은 현재 상태를 가리키지 않으며, 완료 문구가 unresolved gate와 공존한다.
- Impact: 코더와 후속 감사자가 잘못된 파일/Phase를 기준으로 작업하고, v0.9.4 “완료”를 실제 출시 범위로 오인할 수 있다. 감사 재현성과 release decision이 훼손된다.
- Suggested Action: `spec.md`를 승인 범위 authority로 유지할지 명시하고 모든 보조 문서에 기준 시점·snapshot·UNVERIFIED를 표시한다. 존재하지 않는 renderer 책임을 현재 구조에 맞춰 복구하거나 문서에서 제거하고, Phase 9/10·중복 CHANGELOG·README 완료 문구를 한 릴리즈 ledger로 정리한다.
- Re-audit Method: 갱신된 문서의 모든 파일 링크·Phase claim·완료 문장을 현재 `rg --files`, CMake sources, 테스트 명령과 대조하고, unresolved font/release gate가 PASS 문구로 승격되지 않았는지 확인한다.
- Owner: Architect / Human
- Confidence: High
- Notes: 이는 과거 문서를 보존하지 말라는 finding이 아니다. 과거 기록은 snapshot으로 남기되 현재 상태처럼 읽히지 않게 해야 한다.

### Pass 2: Debug / Engineering Quality

### [A01-F011] 녹색 TestHarness가 제품 완료·런타임 경로를 증명하지 않음

- Pass: Debug
- Pattern: `TEST-001`, `IMP-003`
- Area: acceptance evidence, integration coverage, runtime reproducibility
- Severity: Major
- Status: Confirmed
- Summary: `--run-all`은 모델 단위 테스트만 수행하며 실제 State 입력·렌더링·세이브 resume·제품 완주 경로를 호출하지 않는다. 따라서 “100% 통과”와 v0.9.4 완료 주장을 제품 acceptance 증거로 사용할 수 없다.
- Evidence:
  - `CMakeLists.txt:96-110`의 `TEST_SOURCES`에는 `src/test_harness.cpp`, LocalizationManager와 model만 있고 `Game`, `TitleState`, `TownState`, `DungeonState`, `CombatState`, `CharacterInfoState`, `SettingsState`, `DungeonRenderer`가 없다.
  - `src/test_harness.cpp:52-72`는 실제 Party schema가 아닌 임의 `party_gold`/`depth` JSON을 검증한다.
  - `src/test_harness.cpp:347-404`와 `src/test_harness.cpp:493-575`는 UI 이벤트를 호출하지 않고 장착/스왑을 직접 모사한다.
  - `ctest --test-dir build -N` 결과는 `Total Tests: 0`이며, `./build/TestHarness --run-all`은 exit 0으로 모델 테스트 성공만 보고한다.
- Expected Basis: `AI_AUDIT_DOC_STANDARD.md`의 `IMP-003`, `TEST-001`, `DBG-002`. 완료 주장은 실제 호출 경로와 결정적 acceptance fixture에 연결되어야 한다.
- Actual: aggregate green은 능력치, 모델 save/load, BFS, item effect, locale key/glyph 일부만 증명한다. 새 게임→파티→상점→퀘스트→던전→전투→복귀→재시작과 완주/오류 복구는 검증하지 않는다.
- Impact: F002~F009 같은 제품 결함이 모두 녹색 gate를 통과하며, 배포 전 신뢰도를 과대평가한다. 수동 장시간 플레이가 제외된 현재 환경에서는 핵심 제품 coverage가 `Not Covered`다.
- Suggested Action: UI 상태를 포함하는 headless/scripted event seam 또는 실제 입력 harness를 추가하고 canonical save fixture·상태 invariant·완주 시나리오를 CTest에 등록한다. Unit, integration, packaged smoke 결과를 별도 보고하며 `--run-all`을 전체 제품 gate로 부르지 않는다.
- Re-audit Method: clean build에서 CTest가 등록한 scripted scenario를 실행해 각 상태 전이, 실제 renderer data, save/resume, completion/TPK, locale 변경의 결과를 재현하고 실패 모드별 assertion을 확인한다.
- Owner: Coder / Auditor
- Confidence: High
- Notes: 모델 테스트가 무가치하다는 뜻이 아니다. 현재 완료 주장에 필요한 제품 레이어 증거가 빠졌다는 finding이다.

### [A01-F012] 저장 쓰기가 원자적이지 않고 UI·던전 변경이 종료 전에 자동 저장되지 않음

- Pass: Debug
- Pattern: `DBG-001`, `TEST-001`
- Area: save durability, mutation persistence, user-data path
- Severity: Major
- Status: Confirmed
- Summary: 저장은 기존 파일을 직접 truncate하는 단일 `ofstream`이고, CharacterInfo의 장비/소모품 변경과 DungeonState의 위치/FOW 변경에는 저장 호출이 없다. 창 닫기 경로도 세이브하지 않는다.
- Evidence:
  - `include/model/Party.hpp:53-66`의 기본 경로는 상대 경로 `./save.json`이다.
  - `src/model/Party.cpp:120-128`은 임시 파일·fsync·rename·backup 없이 `std::ofstream file(filePath)`로 직접 쓰고, stream write 상태를 확인하지 않은 채 true를 반환할 수 있다.
  - `src/controller/CharacterInfoState.cpp:240-426`은 consumable/equip/unequip으로 Party와 Character를 변경하지만 `party.saveToFile()` 호출이 없다(`rg -n saveToFile src/controller/CharacterInfoState.cpp` 결과 없음).
  - `src/controller/DungeonState.cpp:69-203`의 이동/회전/FOW 경로에도 저장 호출이 없다.
  - `src/core/Game.cpp:60-64`는 `Closed` 이벤트에서 곧바로 창을 닫고 종료한다.
- Expected Basis: 사용자 계약의 “저장 안전성”, `spec.md:289-340`, `designs.md:269-274`, `AI_AUDIT_DOC_STANDARD.md`의 `DBG-001` 및 일반 상용 데이터 보존 불변조건.
- Actual: 장비 장착·물약 사용·위치/탐험 변경 직후 종료하면 마지막 명시 저장 이후 상태가 유실될 수 있다. 전원 장애/중단 중 직접 truncate가 발생하면 유효 세이브가 빈/부분 파일이 될 수 있다.
- Impact: 진행 손실과 세이브 신뢰성 저하가 발생하며, F002의 하드코어 루프와 결합해 사용자 피해가 크다. 테스트는 임시 경로만 사용해 이 내구성·종료 경로를 검증하지 않는다.
- Suggested Action: 저장 정책을 명시해 mutation 후 autosave 또는 안전한 Save/Quit을 제공하고, 임시 파일 write/flush/fsync/atomic rename, backup/rollback, schema version/validation을 구현한다. 게임 데이터는 OS별 사용자 데이터 경로를 사용하고 실패 시 사용자에게 명확히 알린다.
- Re-audit Method: 사용자 파일을 파괴하지 않는 임시 경로/fault-injection으로 write 실패·중단·부분 파일·재시작을 시험하고, CharacterInfo와 Dungeon mutation 직후 재로드한 상태가 동일한지 확인한다.
- Owner: Architect / Coder
- Confidence: High
- Notes: 루트 `save.json`을 테스트 전후 보존한 사실은 테스트 격리 증거이며 제품 save durability의 PASS 증거가 아니다.

### [A01-F013] 유료 데스크톱 출시용 패키징 산출물과 release gate가 없음

- Pass: Debug
- Pattern: `BUILD-001`
- Area: packaging, deployability, clean-machine execution
- Severity: Major
- Status: Confirmed
- Summary: 프로젝트에는 개발용 Debug 빌드와 실행 파일·assets 복사만 있고, 유료 데스크톱 제품을 배포할 install/package/release 경로가 정의되어 있지 않다.
- Evidence:
  - `CMakeLists.txt:61-113`은 `add_executable`, `Assets` copy target, 라이브러리 링크만 제공하며 `install()`, CPack 설정, release preset, 패키지 manifest가 없다.
  - `README.md:24-40,62-69`와 `BUILD_GUIDE.md:33-72`는 `cmake -DCMAKE_BUILD_TYPE=Debug`, `./Crawlmaster`와 raw assets 실행만 안내한다.
  - `file build/Crawlmaster` 결과는 `ELF ... with debug_info, not stripped`이며, CI/CD·CTest 등록·설치 산출물은 없다.
  - `BUILD_GUIDE.md:76-81`의 배포 체크리스트도 미완료 checkbox이며, Windows/macOS artifact/clean install 기준이 없다.
- Expected Basis: 사용자 목표의 “유료 데스크톱 출시 후보”, `spec.md:58-61`, `AI_AUDIT_DOC_STANDARD.md`의 `BUILD-001` 및 배포 가능성 불변조건. Windows/macOS 실기 시험을 요구하는 것이 아니라 release artifact 계약의 존재를 확인한다.
- Actual: 현재 트리에서 재현 가능한 것은 개발자가 빌드 디렉터리에서 실행하는 Linux Debug binary뿐이다. 설치 위치·사용자 데이터 위치·플랫폼별 자산/런타임 묶음·삭제/업데이트 경로가 없다.
- Impact: 구매자에게 전달할 수 있는 검증된 산출물과 clean-machine smoke가 없어 정식 출시 후보로 판정할 수 없다.
- Suggested Action: 지원 플랫폼별 Release preset과 install/package target(CPack 또는 동등 도구), binary/assets/dependency manifest, clean install smoke, versioned artifact/checksum, 사용자 데이터 migration 경로를 정의한다. Debug unit gate와 release package gate를 분리한다.
- Re-audit Method: fresh checkout/clean directory에서 문서된 Release 명령으로 패키지를 만들고, 패키지 안의 바이너리·fonts·locales·런타임 라이브러리만으로 새 사용자 데이터 경로에서 실행·저장·업데이트 smoke를 수행한다.
- Owner: Architect / Coder / Release Owner
- Confidence: High
- Notes: 생성된 `build/CPackConfig.cmake`는 현재 CMakeLists가 설치/패키징을 정의했다는 증거로 승격하지 않았다.

### [A01-F014] 번들 폰트의 재배포 provenance와 실제 혼합 문자열 gate가 미해소 상태

- Pass: Debug
- Pattern: `BUILD-001`
- Area: asset provenance, multilingual release gate
- Severity: Major
- Status: Needs Clarification
- Summary: 폰트 자산은 제품에 포함되어 있지만 유료 재배포 권한과 실제 CJK/ASCII 혼합 가독성이 문서상 명시적으로 미해결이다.
- Evidence:
  - `DESIGN_DECISIONS.md:108-112`는 외부 폰트 재배포 허가와 provenance를 `Human Review Required`로 남긴다.
  - `designs.md:18-22`는 CJK 혼합 가독성과 재배포 근거를 감사 미해결 게이트로 기록한다.
  - `CHANGELOG.md:7-17,196-202`와 `BUILD_GUIDE.md:76-81`은 `hasGlyph`가 실제 화면 가독성/허가를 보증하지 않으며 별도 확인이 필요하다고 말한다.
  - 프로젝트 소유 파일 목록에 `LICENSE`, `NOTICE`, 폰트별 license/provenance manifest가 없다. `src/core/Game.cpp:99-112`는 시스템 Noto 경로와 번들 fallback을 혼합한다.
- Expected Basis: 사용자 범위의 번역/폰트 에셋과 유료 출시, `spec.md:61,455-458`, `BUILD-001`. 법적 허가나 화면 품질을 감사자가 추측해 PASS 처리하지 않는다.
- Actual: 현재 상태는 코드포인트 `hasGlyph`와 파일 복사까지만 확인되며, 어떤 폰트를 어떤 조건으로 재배포할 수 있는지와 실제 5개 언어 혼합 UI의 판독 결과가 없다.
- Impact: 라이선스 위반·배포 차단 또는 구매자 화면의 대체 사각형 위험이 남는다. 이 gate가 닫히기 전에는 유료 release PASS를 줄 수 없다.
- Suggested Action: 각 폰트의 출처·버전·라이선스·재배포 허가를 Human Review로 확정하고 artifact에 license/NOTICE manifest를 포함한다. 대표 혼합 문자열을 실제 Release package의 선택 폰트로 렌더링해 캡처/판독 gate를 추가한다.
- Re-audit Method: release artifact와 provenance 문서를 대조하고, 5개 locale에서 ASCII+CJK 제목/메뉴/로그/아이템명을 실제 창 또는 재현 가능한 render test로 확인해 대체 glyph·잘림·레이아웃 겹침이 없는지 기록한다.
- Owner: Human / Release Owner / Coder
- Confidence: High
- Notes: 이 finding은 레트로 폰트 미학을 거부하지 않는다. 문서가 스스로 선언한 법적·시각적 미해결 gate를 현재 완료 주장과 구분하라는 의미다.

## 6. Uncertainties and Clarifications Needed

- `spec.md:31,375-376`은 Settings `O`를 Town/Dungeon/Combat에서 요구하고 `README.md:20`은 “언제든”이라고 표현하지만, `src/controller/CharacterInfoState.cpp:21-93`에는 `O` 처리 경로가 없다. CharacterInfo overlay를 “언제든” 범위에 포함할지 결정하고, 포함하면 해당 state에도 동일 push/pop 계약을 추가한다.
- `assets/lang/*`에는 `COMBAT_SELECT_ITEM` 키가 있으나 `src/controller/CombatState.cpp:394-529`의 Item 액션은 목록 선택 없이 해독→고급치유→치유→마나→버프 우선순위로 자동 소비한다. 전투 Item이 자동 정책인지 플레이어 선택 UI인지 `designs.md:62-88`와 `spec.md:30`에서 확정해야 한다. 선택 UI가 제품 목표라면 target/member selection과 취소 경로를 구현한다.
- `spec.md:302-340`의 예시 저장 스키마와 실제 short/snake_case 스키마 중 어느 쪽이 canonical인지, 초기 `pot_mana` 지급이 의도인지 문서 authority가 확정해야 한다. 이는 A01-F004에서 결함으로 기록했지만, 호환 정책 자체는 Human/Architect 결정이 필요하다.
- `spec.md`의 `Door` 타일(`spec.md:152-157`)은 현재 생성기에서 사용되지 않는다. 단순 미래 확장인지 현 release 콘텐츠인지 명시해야 한다.
- `BUILD_GUIDE.md:80-81`의 그래픽/X11 및 실제 CJK 화면 검증, Windows/macOS 실기와 장시간 완주 플레이는 이 환경에서 판정하지 못했다. 미검증을 PASS로 승격하지 않는다.

## 7. Perspective Decision

**HOLD — 현재 트리는 Linux 개발 빌드와 모델 단위 테스트는 재현되지만, 유료 Wizardry 스타일 정식 출시 후보로는 판정할 수 없다.**

우선순위는 다음과 같다.

1. **P0 계약/완결:** A01-F001의 release scope 확정, A01-F002의 유한 완주·진행 설계, A01-F005의 던전 진행 저장, A01-F012의 안전 저장 정책.
2. **P1 구현/제품 신뢰:** A01-F003 신규/이어하기, A01-F004 canonical save schema, A01-F006 콘텐츠 도달성, A01-F007 보상 공식, A01-F008 실제 HUD, A01-F009 전체 i18n, A01-F010 현재 문서 authority.
3. **P1 출시 증거:** A01-F011 제품 레이어 acceptance 테스트, A01-F013 Release package/clean install, A01-F014 폰트 provenance·혼합 문자열 gate.

이 관점에서 `Critical`은 확인하지 않았지만 `Major` finding과 미확정 hard release gate가 남아 있어 PASS 계열 판정은 금지된다. 위 항목을 수정하거나 명시적으로 후속 Phase/EA 범위로 재정의한 뒤 관련 pass를 재감사해야 한다.
