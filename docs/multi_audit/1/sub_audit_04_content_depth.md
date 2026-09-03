# Sub Audit Report

## 1. Audit Metadata

- Audit Turn: 1
- Perspective: A04 — 콘텐츠 깊이·던전/레벨·내러티브·리텐션
- User Goal: 유료 데스크톱 Wizardry 스타일 게임으로서 인터페이스, 게임요소, 콘텐츠가 상용 출시 후보 수준인지 감사하고 부족분의 개선방법을 확인한다.
- Audit Basis: Standard-backed / Goal-driven
- Standard Path: `/mnt/Projects_SSD/cpp/crawlmaster/AI_AUDIT_DOC_STANDARD.md`; `/home/eunho1/.codex/skills/multi-audit/references/report-contract.md`
- Evidence Cutoff: 2026-09-03, 현재 작업 트리

## 2. Assigned Scope

이번 배정에서는 콘텐츠 디렉터·레벨 디자인·내러티브·리텐션 관점에서 다음을 실제 호출 경로와 함께 확인했다.

- 던전 층 구조, 맵 생성, 특수 타일, 이벤트·함정·비밀·보상, 재진입 및 저장 지속성
- 몬스터·스킬·아이템·퀘스트·직업의 선언 수량, 팩토리 등록, 스폰·드롭·소비·성장 경로
- 시작-중반-종반 아크, 목표, 보스, 엔딩, 퀘스트 완결성
- 튜토리얼·도감/베스티어리·재플레이 동기와 반복 조합의 실질적 변주
- 테스트가 위 콘텐츠를 UI/런타임 수준에서 닫는지 여부

Wizardry 비교점은 공식 포털의 설명을 사용했다. 공식 설명은 `Proving Grounds`를 10층 미로로 묘사하고, 최하층의 부적과 이를 지키는 Werdna를 명시하며, 파티 관리·탐색·주문·전투 QoL을 별도 개선 영역으로 설명한다: [Wizardry 공식 포털](https://wizardry.info/en/product/game/wizardry-proving-grounds-of-the-mad-overlord/). 10층을 기계적으로 요구하지 않고, 현재 게임이 그에 상응하는 완결 아크와 변주를 제공하는지 판단했다.

## 3. Excluded and Uninspected Scope

- 다른 감사보고서(`audit_report_*.md`, `docs/audit/**`, 다른 `docs/multi_audit/**`)는 읽지 않았다. 예약 메타데이터와 이 배정 보고서만 사용했다.
- 소스·테스트·설정·제품 문서는 수정하지 않았고, 이 보고서만 새로 작성했다.
- 장시간 수동 완주, 실제 GUI 플레이 캡처, 여러 세션에 걸친 유료 플레이 시간·리텐션 측정은 실행하지 않았다. 따라서 장시간 완주 성공을 주장하지 않는다.
- 가격, 목표 플레이 시간, Early Access 범위, 정식 출시 시 층 수가 통제 문서에 수치로 정의되어 있지 않아 그 항목의 절대 점수는 확정하지 않았다. 이는 `A04-F009`의 명세/증거 공백이다.
- 상점의 가격 수학과 전체 UI 접근성은 배정 범위를 넘는 별도 심층 감사 대상이다. 다만 콘텐츠 획득 경로를 판정하는 데 필요한 상점 카탈로그와 구매 호출은 확인했다.
- 실행은 `./build/TestHarness --run-all` 모델/리소스 하네스에 한정했다. 새로 빌드하거나 `Crawlmaster` GUI 바이너리를 실행하지 않았다.

## 4. Evidence Examined

### 통제 문서 및 제품 문서

- `spec.md` — 게임 정체성·성공 기준(`:9-31`), 20×20 던전/타일(`:151-158`), 보상 공식(`:360-362`), 아이템 기준표·획득 제약(`:385-413`), 몬스터 기준표(`:415-425`), 퀘스트 기준표(`:427-432`)
- `designs.md` — 던전/전투/TUI 레이아웃과 조작 가이드(`:1-237`)
- `README.md` — 사용자 기능 목록과 게임 루프(`:7-22`, `:45-60`)
- `CHANGELOG.md` — v0.8.0~v0.9.4 기능 주장 및 완료 표현
- `BUILD_GUIDE.md` — 실행·테스트 명령과 현재 릴리즈 체크리스트(`:63-81`)
- `IMPLEMENTATION_SUMMARY.md` — 모델/컨트롤러 책임 및 플레이어블 범위(`:39-71`, `:120-135`)
- `DESIGN_DECISIONS.md`, `LESSONS_LEARNED.md`, `audit_roadmap.md` — 현재 설계 결정과 다층 던전이 향후 로드맵으로 남아 있는지 확인(`audit_roadmap.md:75-79`)
- `assets/lang/en.json` — 실제 사용자 노출 퀘스트·튜토리얼·전투 문자열(`:1-153`)
- `save.json` — 현재 저장 파일의 실제 키 구조(`:1-8`)

### 콘텐츠 모델·팩토리·호출 경로

- 던전: `include/model/DungeonMap.hpp`, `src/model/DungeonMap.cpp`, `include/controller/DungeonState.hpp`, `src/controller/DungeonState.cpp`, `src/view/DungeonRenderer.cpp`
- 아이템: `include/model/ConcreteItems.hpp`, `include/model/ItemFactory.hpp`, `src/model/ItemFactory.cpp`, `src/controller/TownState.cpp`, `src/controller/CharacterInfoState.cpp`
- 몬스터: `include/model/Monster.hpp`, `include/model/MonsterFactory.hpp`, `src/model/MonsterFactory.cpp`, `src/controller/CombatState.cpp`
- 스킬·성장: `include/model/ConcreteSkills.hpp`, `include/model/SkillFactory.hpp`, `src/model/SkillFactory.cpp`, `src/model/Skill.cpp`, `include/model/Character.hpp`, `src/model/Character.cpp`
- 퀘스트·저장: `include/model/Quest.hpp`, `src/model/Quest.cpp`, `include/model/Party.hpp`, `src/model/Party.cpp`, `src/controller/TownState.cpp`
- 시작/온보딩: `src/controller/TitleState.cpp`, `src/controller/TownState.cpp`, `assets/lang/en.json`

### 수량 산출

| 콘텐츠 | 문서상 수량 | 현재 코드/실제 연결 | 근거 |
| --- | ---: | ---: | --- |
| 던전 층 | 20×20 단일 배열, 층 수 미정 | `DungeonState`에 `DungeonMap` 1개; 층/깊이 필드 0개 | `spec.md:151-155`, `include/controller/DungeonState.hpp:27-29` |
| 아이템 ID | 19 | `ItemFactory::createItem` 19개 | `spec.md:390-408`, `src/model/ItemFactory.cpp:12-82` |
| 상점 구매 | 8종 | Town 입력 경로 8종; `getShopCatalog()`는 19종을 반환하지만 호출되지 않음 | `spec.md:410-413`, `src/controller/TownState.cpp:132-149`, `src/model/ItemFactory.cpp:88-108` |
| 몬스터 ID | 8 | 8종 모두 하나의 전역 랜덤 풀에서 스폰 | `spec.md:418-425`, `src/model/MonsterFactory.cpp:10-76` |
| 스킬/주문 | 12 | 4직업×3레벨 자동 매핑 | `spec.md:436-443`, `src/model/SkillFactory.cpp:10-67` |
| 직업 | 4 | Warrior/Mage/Rogue/Cleric | `spec.md:176-180`, `include/model/Character.hpp:20-26` |
| 퀘스트 | 3 | Castle UI·호출 경로 2개 | `spec.md:430-432`, `src/controller/TownState.cpp:213-261`, `assets/lang/en.json:79-86` |

### 실행한 명령 및 결과

- `./build/TestHarness --run-all` → exit code `0`; 14개 테스트 함수가 모두 통과했고, 한 번의 미로 생성에서 `169 / 169` 통로 연결성이 출력됐다.
- `rg` 기반 ID/호출 산출 → ItemFactory 19, MonsterFactory 8, SkillFactory 12, spec 퀘스트 3, Town 연결 퀘스트 2.
- 런타임 `addItem` 호출 검색 → 기본 치유 물약, Town 구매, 장비 교체/해제, 저장 로드만 확인되며 전투 승리 후 드롭 호출은 확인되지 않았다.
- `DOOR`, `BOSS`, `ending`, `tutorial`, `bestiary`, `codex`, `secret`, `trap`, `loot`, `Werdna`, `amulet` 검색 → 선언 주석·열거형·비콘텐츠 문구를 제외하면 실제 기능 연결이 확인되지 않았다.

## 5. Findings

### [A04-F001] 단일 맵 생성과 재진입 리셋으로 던전 아크·층 진행이 닫히지 않음

- Pass: Implementation Compliance
- Pattern: IMP-002, IMP-003, SPEC-GAP-001
- Area: Dungeon floors, run continuity, progression state
- Severity: Major
- Status: Confirmed
- Summary: 현재 구현은 층 개념 없이 `DungeonMap` 1개를 생성하고, 계단에서 Town으로 나갈 때 던전 상태를 버린다. 20×20이라는 면적은 확인되지만 시작-중반-종반을 전달할 층별 진행이나 저장 가능한 원정 상태가 없다.
- Evidence:
  - `spec.md:151-155`는 20×20 2차원 배열과 `Wall/Empty/Door/UpStairs`만 정의한다.
  - `include/controller/DungeonState.hpp:27-29`에는 `DungeonMap m_map` 하나만 있고 floor/depth/run-state 필드가 없다.
  - `src/controller/DungeonState.cpp:13-20`은 상태 생성 시 매번 `m_map.generate()`를 호출한다.
  - `src/model/DungeonMap.cpp:24-50`은 배열을 리셋하고 `m_tiles[1][1] = UPSTAIRS`만 지정한다. 하층 계단이나 최종 공간 생성은 없다.
  - `src/controller/DungeonState.cpp:177-183`은 입구에서 ESC를 누르면 `changeState(TownState)`로 던전 상태를 제거한다. 다음 입장은 새 맵 생성으로 시작한다.
  - `src/model/Party.cpp:88-118`와 현재 `save.json:1-8`에는 골드·인벤토리·멤버·퀘스트만 있고 맵 seed, floor, 좌표, 목표 상태가 없다. `src/test_harness.cpp:57-69`의 `depth`는 Party 저장 계약을 검증하지 않는 별도 모의 JSON이다.
- Expected Basis: 사용자 목표의 “시작-중반-종반-결말의 명확한 아크”와 던전 층 평가 요구. 공식 비교점은 10층 미로와 최하층 목표를 제시하지만, 10층 수량 자체를 요구사항으로 강제하지 않았다.
- Actual: 한 번의 절차적 20×20 맵과 입구 회귀만 있으며, 완주 목표·하층 전환·원정 저장/복원 경로가 없다.
- Impact: 플레이어는 한 층을 반복해서 걷고 전투할 뿐, 어디까지 왔는지·무엇을 향하는지·재진입 시 무엇이 보존되는지를 경험할 수 없다. 상용 후보의 캠페인 완결성과 장기 진행을 판정할 수 없다.
- Suggested Action:
  - 구현 전에 `spec.md`에 층 수(수치는 제품 결정), 층별 진입/탈출 규칙, 원정 저장/재개 의미, 최종 목표를 명시한다.
  - 최소 수직 슬라이스는 한 층이라도 entry → 탐험 목표 → 종결 전투/보상 → 명시적 귀환/종료 → 저장/재로드가 하나의 닫힌 경로가 되게 한다.
  - Early Access에서는 여러 진행 구간과 층별 랜드마크를 추가하고, 정식 출시 전에는 모든 층·목표·재진입 상태를 동일한 저장 계약으로 연결한다. 정확한 층 수와 플레이 시간은 명세 확정 후 정한다.
- Re-audit Method: clean save에서 층/목표 시작, 전투 후 진행, 층 전환, 중단 후 재로드, 최종 귀환을 실제로 수행한다. save JSON의 floor/seed/position/objective가 재개 전후 동일하고 최종 완료 상태가 한 번만 지급되는지 확인한다.
- Owner: Architect / Coder; 층 수와 완주 정의는 Human/Product
- Confidence: High
- Notes: 다층 던전은 현재 `audit_roadmap.md:77`의 향후 로드맵으로 기록되어 있어, 이미 완료된 상용 콘텐츠로 승격해 해석하지 않았다. D3D disposition: Needs Fix / PASS 차단.

### [A04-F002] 생성된 벽/바닥과 랜덤 전투 외에 이벤트·함정·비밀·던전 보상 표면이 없음

- Pass: Implementation Compliance
- Pattern: IMP-001, IMP-003
- Area: Dungeon authored content, events, hazards, loot
- Severity: Major
- Status: Confirmed
- Summary: `TileType::DOOR`는 선언만 있고 생성되지 않으며, 맵에는 이벤트·함정·비밀·상자·보상 노드가 없다. 전투 승리도 XP/골드와 퀘스트 kill-count만 갱신하고 아이템 드롭을 만들지 않는다.
- Evidence:
  - `include/model/DungeonMap.hpp:11-17`은 DOOR를 선언하지만 `src/model/DungeonMap.cpp:24-43,204-267`의 생성 경로는 WALL/EMPTY를 만들고 시작점만 UPSTAIRS로 바꾼다. 실제 `TileType::DOOR` 대입은 없다.
  - `src/controller/DungeonState.cpp:107-155,216-267`의 이동 경로는 충돌·FOW·10% 인카운터만 처리하며 이벤트/함정/비밀/아이템 보상 분기가 없다.
  - `src/controller/CombatState.cpp:699-745`의 `distributeRewards()`는 XP, `5..15 × 적 수` 골드, kill quest 진행만 처리한다. `party.addItem()` 또는 loot table 호출이 없다.
  - 제품 코드에서 확인된 `addItem` 호출은 `src/controller/TownState.cpp:148`, `src/controller/CharacterInfoState.cpp:381,385,423`, `src/model/Party.cpp:154,196-197`에 한정된다. 이는 상점 구매, 장비 교체/해제, 저장 로드, 기본 물약이다.
  - `spec.md:29-30,410-413`은 수집 아이템과 특수 아이템의 던전 파밍/퀘스트 보상 경로를 성공 기준으로 적는다.
- Expected Basis: 사용자 배정 범위의 이벤트·함정·비밀·보상과 `spec.md`의 아이템 획득 계약. 레트로 단순성은 존중하되, 선언된 콘텐츠가 실제 소비 경로를 가져야 한다는 D3D 양방향 정합성 기준을 적용했다.
- Actual: 실제 던전 변주는 절차적 통로와 이동 중 랜덤 전투뿐이다. 특수 아이템 9종(`wpn_greatsword`, `wpn_rapier`, `arm_plate`, `shd_tower`, `pot_greater_heal`, `pot_mana`, `pot_strength`, `pot_dexterity`, `scr_cure`)은 현재 정상 플레이의 드롭/퀘스트 보상 경로가 없다. `wpn_staff`와 `arm_robe`는 Mage 생성 시 시작 장비로만 들어온다.
- Impact: 플레이어가 던전에서 탐색·위험을 감수할 이유와 발견 기억이 없다. 인벤토리/퀘스트 콘텐츠는 표면상 수량보다 실제 플레이 가능 수량이 작고, 특수 효과와 수집·파밍 루프를 정상적으로 경험할 수 없다.
- Suggested Action:
  - 층 seed에서 파생되는 authored node/event registry를 만들고, 최소 수직 슬라이스에 목표 지점 1개·위험/비밀 1개·보장 보상 1개를 배치한다. 이는 새 요구사항을 확정하는 것이 아니라 현재 스펙의 콘텐츠 획득 공백을 닫는 제안이다.
  - 전투 결과에서 선언된 드롭/퀘스트 보상으로 이어지는 명시적 loot 경로를 추가하고, 아이템별 acquisition source를 `shop/starter/dungeon/quest`처럼 문서화한다.
  - Early Access에는 이벤트·위험·비밀 조합을 진행 구간별로 늘리고, 정식 출시에는 모든 선언 아이템과 퀘스트가 clean save에서 도달 가능한지 완결성 매트릭스로 잠근다.
- Re-audit Method: seed 고정 시 각 이벤트/함정/비밀/보상 노드가 실제 맵에 존재하는지 검사하고, UI를 통한 전투 승리만으로 특수 아이템을 획득한 뒤 인벤토리·퀘스트·저장 파일의 상태 변화를 확인한다. 수동 save 편집 없이 수집/소비까지 완료한다.
- Owner: Architect / Coder
- Confidence: High
- Notes: `qst_collect_maces` 자체는 현재 상점의 기본 메이스 구매로 진행할 수 있지만, 던전 수집/파밍 경험을 제공하지 않는다. D3D disposition: Needs Fix / PASS 차단.

### [A04-F003] 아이템 19종의 canonical catalog와 실제 획득 계약이 분리됨

- Pass: Implementation Compliance
- Pattern: IMP-001, IMP-003
- Area: Item content registry, acquisition reachability
- Severity: Minor
- Status: Confirmed
- Summary: 아이템 정의는 19종으로 일치하지만 상점 카탈로그 helper는 19종을 반환하고, 실제 Town 입력은 별도 하드코딩 8종을 사용한다. 이 이중 정의는 현재 UI가 우연히 8종으로 동작하는 상태이며, 특수 아이템 획득 경로와 향후 콘텐츠 확장을 검증할 단일 진실원이 없다.
- Evidence:
  - `spec.md:390-408`의 기준표에는 19개 ID가 있다. `src/model/ItemFactory.cpp:12-82`의 `createItem()`도 19개 ID를 등록한다.
  - `spec.md:410-413`은 구매 상시 진열을 8종으로 제한하고, 나머지 고급 장비/특수 소모품을 던전 파밍 또는 퀘스트 보상으로 둔다.
  - `src/model/ItemFactory.cpp:88-108`의 `getShopCatalog()`는 19개를 모두 반환한다.
  - `src/controller/TownState.cpp:132-149`는 `Num1..Num8`에 8개 ID를 직접 매핑한다. 저장소 검색에서 `getShopCatalog()`의 제품 호출자는 확인되지 않았다.
  - `src/model/Character.cpp:35-49`는 일부 비상점 아이템(`wpn_staff`, `arm_robe`)을 Mage 시작 장비로 직접 지급한다.
- Expected Basis: `spec.md`의 19개 데이터 표와 8종 상점/특수 획득 분리 계약, 그리고 사용자 요청의 정확한 수량·실제 호출 경로 확인.
- Actual: 사용자에게 보이는 구매 수량은 8종이지만, helper의 19종 반환은 사용되지 않는다. 19개 중 9개는 정상적인 드롭/퀘스트/상점 획득 경로가 없고, 2개는 Mage 시작 장비라는 별도 예외다.
- Impact: 새 아이템을 추가할 때 factory/helper/Town/drop/quest 중 일부만 수정해도 “정의됨”과 “플레이 가능함”이 쉽게 갈라진다. 카탈로그만 보면 19종 콘텐츠가 있는 것처럼 보이지만 실제 도달 가능성이 보장되지 않는다.
- Suggested Action: 아이템별 획득 출처와 상점 노출을 하나의 data registry로 통합하거나 생성 검증을 추가한다. 수직 슬라이스에서 정의→획득→장착/소비→판매→저장까지 최소 대표 아이템을 닫고, Early Access/정식 출시에서는 19개 ID 전부에 reachability 표와 테스트를 둔다.
- Re-audit Method: canonical registry에서 19개를 열거하고 shop/starter/dungeon/quest source를 하나씩 판정한다. clean save에서 각 경로를 실제로 수행해 인벤토리 추가·소비·판매·재로드가 일치하는지 확인한다.
- Owner: Architect / Coder
- Confidence: High
- Notes: 현재 8종 Town 동작 자체는 스펙의 상점 제한과 일치한다. finding은 현재 판매 목록보다 분리된 canonical data와 도달성 증거의 부재에 대한 것이다. D3D disposition: Needs Fix 또는 명시적 후속 Phase 이관.

### [A04-F004] 스펙 퀘스트 3개 중 2개만 Castle UI와 런타임 경로에 연결됨

- Pass: Implementation Compliance
- Pattern: IMP-001, IMP-002
- Area: Quest content completeness, objective loop
- Severity: Major
- Status: Confirmed
- Summary: 스펙 기준표의 `qst_hunt_spiders`가 현재 코드·Castle UI·번역 리소스·진척도 호출에 연결되어 있지 않다. 따라서 몬스터 8종 중 거미의 특수 전투는 있어도 그에 대응하는 선언 퀘스트 콘텐츠는 실제 플레이에서 제공되지 않는다.
- Evidence:
  - `spec.md:427-432`는 `qst_clear_kobolds`, `qst_collect_maces`, `qst_hunt_spiders` 3개를 명시한다.
  - `src/controller/TownState.cpp:213-261`의 CASTLE 입력·보고 분기는 `qst_clear_kobolds`와 `qst_collect_maces`만 처리한다.
  - `src/controller/TownState.cpp:396-434`의 Castle 화면도 두 퀘스트만 렌더링한다.
  - `assets/lang/en.json:79-86,136-141`에도 두 퀘스트 문자열만 있다.
  - 저장소 검색에서 `qst_hunt_spiders`는 스펙 기준표 외의 제품 연결을 찾지 못했다.
- Expected Basis: `spec.md`의 기본 퀘스트 목록과 사용자 목표의 퀘스트 완결성·실제 수주/보고/성장 소비 경로 확인.
- Actual: runtime quest count is 2, not the declared 3. `Quest` generic model can represent an arbitrary ID, but no content registry or UI path creates the spider quest.
- Impact: 현재 퀘스트 아크는 두 개의 반복 심부름으로 끝나고, 거미를 처치해도 목적·보상·완료 피드백이 없다. 스펙 수량과 실제 콘텐츠 볼륨의 차이가 사용자 진행에서 드러난다.
- Suggested Action: 제품 방향을 먼저 선택한다. 거미 퀘스트를 현재 Phase에 포함할 경우 Castle 수주/진척/보고/보상/저장/UI를 연결하고, 제외할 경우 `spec.md`와 번역/요약 문서에서 현재 Phase 밖으로 명시한다. 새 퀘스트 체인이나 목표 수량은 별도 제품 결정으로 남긴다.
- Re-audit Method: clean save에서 각 스펙 퀘스트를 Castle에서 수주하고, 실제 전투/아이템 획득으로 진행도를 채운 뒤 보고·보상·활성 목록 제거·재로드를 확인한다. 모든 선언 ID에 연결된 호출과 번역 키가 있는지 다시 열거한다.
- Owner: Architect / Coder; 현재 Phase 포함 여부는 Human/Product
- Confidence: High
- Notes: D3D disposition: Needs Fix 또는 명시적 Scope/Spec Clarification; 현재 콘텐츠 완결성 gate는 차단.

### [A04-F005] 전투 풀과 보상에 층별 난이도·보스 정체성이 없어 pacing이 랜덤 스파이크에 의존함

- Pass: Implementation Compliance
- Pattern: IMP-002, IMP-003
- Area: Encounter pacing, difficulty bands, reward economy
- Severity: Major
- Status: Confirmed
- Summary: 8종 몬스터가 층·진행도와 무관한 하나의 전역 풀에서 1~3마리씩 등장한다. 드문 새끼 용도 보스가 아니며, 보상은 스펙의 몬스터 레벨 기반 공식 대신 그룹 수 기반 일반 골드와 XP만 지급된다.
- Evidence:
  - `spec.md:418-425`에는 8종의 HP/AC/XP와 특수 행동이 정의되어 있으나 층별 출현표나 boss role은 없다.
  - `src/model/MonsterFactory.cpp:10-48`은 8종을 생성하고, `:52-76`은 코볼트 20%, 고블린 20%, 스켈레톤 15%, 거미 15%, 오크 10%, 주술사 10%, 구울 7%, 새끼 용 3%의 전역 가중치만 사용한다.
  - `src/controller/CombatState.cpp:166-174`는 모든 인카운터에서 1~3마리를 같은 factory로 스폰한다. floor/biome/encounter tier 인자가 없다.
  - `src/controller/CombatState.cpp:699-745`는 `totalXp`와 `uniform_int_distribution(5,15) * foes.size()` 골드만 지급하고 item drop은 만들지 않는다.
  - `spec.md:360-362`는 골드를 `(1d10 * 몬스터 레벨)`로 정의하지만, `Monster`/`ConcreteMonster` 계약에는 몬스터 레벨 필드가 없고 현재 보상 계산도 이를 사용하지 않는다.
- Expected Basis: 사용자 목표의 시작-중반-종반 변주·보상·보스 평가와 `spec.md`의 보상 공식. 3% 새끼 용을 보스라고 창작하지 않고, 코드상 boss 상태가 없는 사실만 판정했다.
- Actual: 첫 전투부터 3% 확률로 HP 35/AC 14 새끼 용이 등장할 수 있고, 후반에도 동일 풀과 동일 encounter count가 유지된다. 승리 보상은 generic XP/gold뿐이며 층/보스/드롭에 따른 상승 곡선이 없다.
- Impact: 초반 난이도는 운에 따라 급변하고, 중반 이후 학습·대비·보상 기대가 형성되지 않는다. 레벨 3 이후 전투를 반복할 실질적 이유도 약해진다.
- Suggested Action:
  - `spec.md`에서 진행 구간별 encounter band, 보스 여부/전용 규칙, 보상 공식의 canonical source를 먼저 확정한다.
  - 수직 슬라이스에는 플레이어가 학습 가능한 일반 전투와 종결 전투를 분리하고, Early Access에는 진행 구간별 조합·특수 행동·보상 상승을 추가한다. 정식 출시에서는 모든 전투가 적절한 위험/보상 band에 속하는지 표로 검증한다.
  - 몬스터 레벨을 유지할지, 스펙을 그룹/층 보상으로 바꿀지 결정한 뒤 XP·골드·드롭을 한 계산 경로로 통합한다.
- Re-audit Method: 각 floor/band seed로 1~3마리 스폰 결과를 재현하고, 첫 전투·중반·종결 전투의 HP/AC/행동/보상 분포를 비교한다. 보스 encounter가 명시적 상태와 종료 보상을 가지며, 스펙 공식 또는 승인된 대체 공식과 일치하는지 확인한다.
- Owner: Architect / Coder; 보상·난이도 계약은 Human/Product
- Confidence: High
- Notes: 8종 자체의 수량과 특수 행동 구현 여부와, 이를 상용 캠페인에 배치하는 pacing은 별개다. D3D disposition: Needs Fix / PASS 차단.

### [A04-F006] 이야기·최종 목표·보스·엔딩 상태가 제품 표면에 없음

- Pass: Implementation Compliance
- Pattern: IMP-001, SPEC-GAP-001
- Area: Narrative arc, objective, boss, ending
- Severity: Major
- Status: Confirmed
- Summary: 현재 제품에는 Waterdeep 캠프와 일반 던전 진입/귀환 문구는 있으나 플레이어 목표, 이야기 사건, 최종 보스, 엔딩을 표현하거나 호출하는 데이터·상태·UI가 없다. 개별 전투의 victory는 캠페인 종결이 아니다.
- Evidence:
  - `spec.md:9-13`은 장르와 Town→Dungeon→Combat 루프만 설명하며 story/goal/boss/ending acceptance 기준을 정의하지 않는다.
  - `assets/lang/en.json:76-86`은 환영, 어둡고 추운 던전 진입, 계단 귀환, 두 퀘스트 설명 정도만 제공한다.
  - `src/controller/DungeonState.cpp:21-27`은 입장·조작·ESC 안내 로그만 추가하고, `:177-185`는 계단 귀환만 처리한다.
  - `src/controller/CombatState.cpp:679-684`의 `checkVictory()`는 현재 `m_foes`가 모두 죽었는지만 검사한다. 최종 목표나 campaign completion 상태가 없다.
  - 제품 문서·소스에서 `boss`, `ending`, `Werdna`, `amulet`, `objective`, `lore`, `story`의 기능 연결을 확인하지 못했다.
- Expected Basis: 사용자 목표의 “명확한 시작-중반-종반-결말” 요구와 공식 Wizardry 비교점의 최하층 목표/보스 설명. 공식 작품의 이름이나 설정을 복제하라는 요구로 해석하지 않았다.
- Actual: 플레이어가 도달해야 할 최종 장소·회수해야 할 대상·쓰러뜨릴 보스·보여줄 엔딩이 없다. 가능한 종료는 계단을 통한 마을 귀환, 개별 전투 승리, 퀘스트 보고뿐이다.
- Impact: 완주를 정의할 수 없고, 상용 구매자가 첫 실행 후 기대할 “무엇을 위해 탐험하는가”와 “언제 끝나는가”가 없다. 콘텐츠 수량을 늘려도 결말 없는 반복 루프로 남는다.
- Suggested Action:
  - 코드보다 먼저 최소 narrative contract를 `spec.md`에 기록한다: 주인공/목표의 최소 설명, 진행 단계, 최종 encounter, 성공/실패 종료, 최종 보상, 저장·재로드 규칙.
  - 수직 슬라이스는 짧더라도 하나의 목표와 보스/종결 encounter, 명시적 결과 화면 또는 로그를 포함한다.
  - Early Access에서는 단계별 목표와 중간 결과를 추가하고, 정식 출시에서는 clean save에서 최종 목표와 엔딩까지 도달하는 경로를 닫는다. 세부 설정은 창작으로 확정하지 말고 제품 결정으로 남긴다.
- Re-audit Method: 새 저장에서 목표 수락/갱신 → 종결 encounter → 보스/최종 보상 → 엔딩 → 재로드 후 완료 상태 보존을 수행한다. `checkVictory()`가 일반 전투 승리와 campaign completion을 구분하는지 확인한다.
- Owner: Human/Product for narrative contract; Architect / Coder for implementation
- Confidence: High
- Notes: 이는 레트로 그래픽 단순성 finding이 아니라 완결 목표 상태 부재 finding이다. D3D disposition: Needs Fix 또는 먼저 Spec Clarification; 현재 PASS 차단.

### [A04-F007] 온보딩은 조작 로그 수준이며 튜토리얼·도감/베스티어리가 없음

- Pass: Implementation Compliance
- Pattern: IMP-003, SPEC-GAP-001
- Area: Tutorial, discoverability, bestiary
- Severity: Minor
- Status: Confirmed
- Summary: 첫 실행/첫 전투/상태이상/스킬·아이템 사용을 단계적으로 가르치는 튜토리얼과 몬스터 도감은 없고, 조작 가이드와 전투 중 이름·HP 목록만 있다. 레트로 단순성으로 허용될 여지는 있으나 사용자 목표가 명시적으로 튜토리얼·도감을 점검하므로 개선 여지를 기록한다.
- Evidence:
  - `src/controller/TitleState.cpp:54-104`는 신규 게임·설정·종료 메뉴와 점멸 안내만 그린다.
  - `assets/lang/en.json:7,13-18,76-78`은 “Create a new hero or load a saved game”, 조작/설정/계단 귀환 안내만 제공한다.
  - `src/controller/DungeonState.cpp:21-27`은 입장, 설정 키, 이동, ESC 안내 로그를 추가하지만 파티 구성·역할·첫 전투·상태이상 학습 흐름은 없다.
  - `src/controller/CombatState.cpp:805-820`은 적 이름과 HP를, `:881-920`은 대부분 공통 ASCII 아트(오크·스켈레톤만 별도)를 보여준다. 설명·발견 기록·약점/특수 행동 도감이 없다.
  - `tutorial`, `bestiary`, `codex`, `journal` 기능 연결은 확인되지 않았다.
- Expected Basis: 사용자 배정 범위의 튜토리얼·도감/재플레이 평가. `spec.md`에 구체적인 튜토리얼/도감 acceptance 기준은 없어 필수 수량을 창작하지 않았다.
- Actual: 플레이어는 조작 키는 알 수 있지만 왜 거미의 독을 해독해야 하는지, 어떤 직업/스킬을 선택했는지, 새 몬스터를 어떻게 학습했는지 게임 내 누적 지식이 없다.
- Impact: 첫 세션 이탈과 전투 특수 행동의 발견성 저하가 예상된다. 도감은 핵심 루프를 막지는 않으므로 Major가 아닌 Minor로 분류한다.
- Suggested Action: 수직 슬라이스에 건물/전투/상태이상/아이템을 한 번씩 안내하는 짧은 contextual help와 첫 전투 telegraph를 추가한다. Early Access에서 발견한 몬스터·스킬·아이템을 기록하는 최소 도감 탭을 도입하고, 정식 출시에서 내용·언어·저장 지속성을 확정한다.
- Re-audit Method: clean save 첫 실행부터 첫 전투까지 신규 사용자 관점으로 수행하고, 각 핵심 조작과 상태이상 설명을 도움 없이 발견 가능한지 확인한다. 모든 8종 몬스터와 12종 스킬/19종 아이템이 도감에 들어가는지 제품 결정 후 검증한다.
- Owner: Human/Product for shipped scope; Architect / Coder for implementation
- Confidence: High
- Notes: 도감 수량과 설명 깊이는 현재 spec gap이다. D3D disposition: Known Issue 후보 또는 Spec Clarification.

### [A04-F008] 재플레이 변주는 절차 맵·주사위·랜덤 직업에 한정되고 의미 있는 선택/빌드가 없음

- Pass: Implementation Compliance
- Pattern: IMP-002, IMP-003
- Area: Replayability, build diversity, retention
- Severity: Major
- Status: Confirmed
- Summary: 반복 플레이의 차이는 동일한 20×20 DFS+loop 생성, 전역 몬스터 확률, 주사위, 랜덤 직업/능력치에 주로 의존한다. 플레이어가 파티 구성·스킬·성장 경로·난이도·목표를 선택하거나 다른 결말을 만드는 구조는 없다.
- Evidence:
  - `src/model/DungeonMap.cpp:24-50,204-267`은 고정 크기·고정 알고리즘으로 새 맵을 만든다. seed를 저장/주입하는 API가 없다.
  - `src/controller/DungeonState.cpp:13-20`은 던전 재진입마다 맵을 새로 만든다.
  - `src/controller/TownState.cpp:77-102`는 Guild에서 직업을 `rand() % 4`로 정하고 8개 이름을 순환한다. 플레이어의 직업/이름/능력치 선택 화면은 없다.
  - `src/model/SkillFactory.cpp:39-67`은 직업과 레벨에 따라 각 3개 스킬을 자동으로 고정한다. `src/model/Character.cpp:141-169`는 최대 레벨 3과 자동 습득을 구현한다.
  - `src/model/MonsterFactory.cpp:52-76`과 `src/controller/TownState.cpp:213-261`은 고정 전역 인카운터 풀과 두 개의 고정 퀘스트만 제공한다.
- Expected Basis: 사용자 목표의 재플레이 동기와 “반복 조합의 실질 변주” 평가. 레트로 단순성을 이유로 선택지 수를 임의로 늘리지는 않고, 현재 반복에서 의미 있는 결과 차이가 있는지만 판정했다.
- Actual: 맵과 전투 주사위는 달라지지만, 모든 run이 같은 한 층·같은 전역 적 풀·같은 두 퀘스트·같은 직업별 스킬 순서를 공유한다. 레벨 3 이후 성장 소비와 새로운 목표가 없다.
- Impact: 첫 완주 이후 재방문 이유가 절차적 운에만 남고, 플레이어의 숙련·실험·수집·대안 빌드가 장기 리텐션으로 이어지지 않는다.
- Suggested Action: `spec.md`에 허용할 replay levers를 먼저 선택한다(예: 목표/경로/파티 선택/seed/보상 변주 중 필요한 범위). 수직 슬라이스는 하나의 명확한 선택이 결과를 바꾸는지 증명하고, Early Access에는 진행 구간별 조합/목표/보상 변주를, 정식 출시에는 반복 완주 후에도 의미 있는 선택과 소비처를 검증한다.
- Re-audit Method: 동일 seed와 다른 seed의 clean run을 각각 수행하고 맵 모양뿐 아니라 목표·전투 조합·보상·엔딩 결과가 어떻게 달라지는지 기록한다. 네 직업과 레벨 1~3의 선택 가능한 경로, 레벨 3 이후 소비처가 제품 계약과 일치하는지 확인한다.
- Owner: Human/Product for replay contract; Architect / Coder for implementation
- Confidence: High
- Notes: 레벨 3 상한과 4직업 자체를 결함으로 보지 않았다. 문제는 이를 보완할 목표/경로/보상 변주가 현재 연결되지 않은 점이다. D3D disposition: Needs Fix 또는 명시적 후속 Phase 이관.

### [A04-F009] 상용 콘텐츠 완주 판정을 위한 시간·완결·도달성 증거와 수치 기준이 없음

- Pass: Debug / Engineering Quality
- Pattern: IMP-003, DBG-002, TEST-001
- Area: Content acceptance evidence, duration, release gate
- Severity: Major
- Status: Needs Clarification
- Summary: 현재 하네스는 모델 규칙과 일부 저장/리소스 테스트는 닫지만, 실제 던전→전투→보상→퀘스트→최종 목표를 완주하는 콘텐츠 시나리오를 실행하지 않는다. 가격·목표 플레이 시간·층 수·Early Access/정식 출시 기준도 문서화되어 있지 않아 “상용 수준”의 절대 판정은 증거상 불가능하다.
- Evidence:
  - `src/test_harness.cpp:818-831`은 14개 테스트를 호출하지만 `CombatState`/`DungeonState`를 생성해 완주하거나 loot, boss, ending, tutorial, replay를 검증하는 테스트는 없다.
  - `src/test_harness.cpp:57-69`의 JSON 테스트는 `party_gold`와 임의 `depth`를 모의 객체에 넣을 뿐 실제 `Party::saveToFile()`의 던전 진행 계약을 검증하지 않는다.
  - `./build/TestHarness --run-all`은 exit code 0이지만, 이는 위 모델/리소스 범위가 통과했다는 뜻이며 장시간 완주 증거가 아니다.
  - `BUILD_GUIDE.md:76-81`은 실행 전 체크리스트를 제공하지만, 콘텐츠 완주 시간·층별 커버리지·최종 엔딩 증거 기준은 없다.
- Expected Basis: 사용자 지시의 “장시간 완주는 미검증 명시”, 표준의 결정적 검증 기준과 상용 출시 후보 판정 원칙. 가격과 수량을 임의로 정하지 않는다.
- Actual: 현재 보고서에서 확인 가능한 것은 14개 하네스 테스트와 정적 호출 경로뿐이다. 실제 campaign completion과 플레이 시간은 `UNVERIFIED`다.
- Impact: 테스트가 green이어도 핵심 사용자 가치와 완주 가능성을 보장하지 못한다. 이 공백을 닫기 전에는 콘텐츠 관점 `PASS` 또는 `PASS WITH KNOWN RISKS`를 선언할 수 없다.
- Suggested Action:
  - `spec.md`에 최소 수직 슬라이스, Early Access, 정식 출시의 콘텐츠 종료 조건을 분리한다. 층 수·가격·목표 플레이 시간·완주 정의는 제품 결정으로 명시한다.
  - seed/fixture 기반 headless scenario runner를 만들어 각 선언 콘텐츠의 spawn/drop/quest/level-up/save/reload/ending 경로를 실행한다.
  - 수직 슬라이스는 한 번의 완결 가능한 경로 증거, Early Access는 선언 ID 도달성·진행 구간 커버리지 증거, 정식 출시 전에는 fresh build와 clean save의 장시간 완주 기록을 요구한다.
- Re-audit Method: 명세가 확정된 뒤 clean save 및 fresh build에서 수직 슬라이스/EA/정식 출시 시나리오를 각각 실행한다. 시작부터 최종 엔딩까지 시간·스테이트 전이·획득 ID·보상·재로드 결과를 기록하고, 실패 시 해당 finding을 유지한다.
- Owner: Human/Product for release criteria; Architect / Coder for test harness
- Confidence: High (증거 공백 자체에 대해); 실제 완주 실패 여부는 미검증
- Notes: D3D disposition: Needs Spec Clarification + Hold. `./build/TestHarness --run-all`의 green 결과를 상용 콘텐츠 PASS로 승격하지 않는다.

## 6. Uncertainties and Clarifications Needed

1. `spec.md`는 던전 맵 크기는 20×20으로 고정하지만 층 수·캠페인 길이·최종 목표를 정하지 않는다. 한 층 완결형으로 출시할지 다층 캠페인으로 확장할지 제품 결정이 필요하다.
2. 가격, 목표 플레이 시간, Early Access 시작/종료 범위, 정식 출시 acceptance가 없다. 이 값을 정하기 전에는 “상용 수준”을 객관 수치로 비교할 수 없다.
3. `spec.md`에는 `qst_hunt_spiders`가 있으나 현재 Phase에서 의도적으로 지연한 것인지, 구현 누락인지 기록이 없다. `A04-F004`를 수정하기 전에 scope를 확정해야 한다.
4. 도감/튜토리얼이 반드시 shipped 기능인지, 선택적 QoL인지 통제 문서가 없다. 현재는 사실상 부재를 Minor로 기록했으며, 제품 대상에 따라 우선순위를 확정해야 한다.
5. 장시간 GUI 완주와 반복 플레이의 실제 시간/이탈률은 측정되지 않았다. 현재 결과는 코드·데이터·하네스 기반이며 플레이 시간에 대한 추정이 아니다.

## 7. Perspective Decision

### 판정

`HOLD — 콘텐츠 관점 REWORK REQUIRED`

현재 구현은 Wizardry풍 1인칭 던전 전투의 기술 프로토타입으로서 20×20 생성 맵, 8종 몬스터, 19종 아이템 정의, 12종 스킬, 4직업, 일부 퀘스트/성장 루프를 갖춘다. 그러나 콘텐츠 디렉터 관점에서 상용 출시 후보로 닫히려면 다음 Major 공백을 먼저 해결해야 한다.

- 층/원정 지속성과 명확한 최종 아크 부재 (`A04-F001`)
- 이벤트·함정·비밀·던전 loot 부재 (`A04-F002`)
- 스펙 아이템의 실제 획득 계약과 canonical catalog 분리 (`A04-F003`)
- 선언 퀘스트 3개 중 런타임 2개 (`A04-F004`)
- 층별 pacing·보스·보상 곡선 부재 및 보상 공식 drift (`A04-F005`)
- 이야기·목표·보스·엔딩 부재 (`A04-F006`)
- 실질적 재플레이 선택/빌드 변주 부재 (`A04-F008`)
- 장시간 완주와 상용 acceptance 증거/수치 부재 (`A04-F009`)

### 개선 우선순위 제안

1. **최소 상용 수직 슬라이스:** 먼저 명세를 닫고, 한 던전 구간 안에서 시작 목표·탐험 landmark/event·획득 가능한 보상·종결 encounter·결과/엔딩·저장/재로드를 완결한다. 현재 8종 몬스터와 19종 아이템 전체를 한 번에 늘리는 것보다, 선언된 대표 콘텐츠의 실제 소비 경로를 닫는 것이 우선이다.
2. **Early Access 기준:** 진행 구간을 여러 개로 나누고, 각 구간의 적 조합·위험·퀘스트·획득 경로·보상을 연결한다. 스펙에 선언된 모든 ID를 reachability matrix로 검증하며, 각 구간을 clean save로 시작해 목표 완료까지 재현한다.
3. **정식 출시 기준:** 층 수·가격·목표 시간·최종 목표를 제품 문서에 확정하고, 모든 층의 차별화된 이벤트/보상과 최종 보스/엔딩을 연결한다. fresh build에서 장시간 완주와 중단/재개를 증명하고, 두 번째 run에서 절차적 모양 이상으로 달라지는 선택/목표/보상 결과를 기록한다.

### Accepted Risks

- 레트로 와이어프레임 그래픽, 20×20 고정 맵, 간소화된 4직업/레벨 3 규칙 자체는 이 배정에서 결함으로 판단하지 않았다.
- 오디오 부재나 상세 상점 UI 품질은 이 보고서의 콘텐츠 깊이 판정 범위 밖이다.
- `./build/TestHarness --run-all`의 모델·리소스 테스트 14개 통과는 사실로 인정하되, 콘텐츠 완주 증거로 해석하지 않았다.

## 8. Coder Handoff

```text
`/mnt/Projects_SSD/cpp/crawlmaster/docs/multi_audit/1/sub_audit_04_content_depth.md`를 먼저 읽고, 각 finding을 프로젝트 문서와 실제 코드에 대조하여 검토한 뒤 우선순위대로 수정하세요. 계약 변경이 필요하면 관련 문서를 먼저 갱신하고, 수정 후 콘텐츠 spawn/drop/quest/성장/save-reload/ending 시나리오 테스트와 빌드·재감사 증거를 기록하세요.
```
