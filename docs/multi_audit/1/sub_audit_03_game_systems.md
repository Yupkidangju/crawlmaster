# Sub Audit Report

## 1. Audit Metadata

- Audit Turn: 1
- Perspective: 게임 설계·밸런스·진행·핵심 루프(Game Systems)
- User Goal: `$multi-audit` 위저드리 스타일 게임으로서의 인터페이스, 게임요소, 컨텐츠가 상용수준인지 감사하고 부족한 부분의 개선방법을 확인한다.
- Audit Basis: Standard-backed / Goal-driven
- Standard Path: `/mnt/Projects_SSD/cpp/crawlmaster/AI_AUDIT_DOC_STANDARD.md`; `/home/eunho1/.codex/skills/multi-audit/references/report-contract.md`
- Independent boundary: 다른 감사 보고서의 결론을 사용하지 않고 현재 소스·테스트·제품 문서·실행 결과만 사용했다.

## 2. Assigned Scope

다음 시스템의 선언부터 호출·소비 경로까지를 확인했다.

- `Character`, `Party`, `DungeonMap`, `CombatState`, `Skill`/`SkillFactory`, `Monster`/`MonsterFactory`, `Item`/`ItemFactory`, `Quest`
- `TownState`, `DungeonState`, `CharacterInfoState`의 파티 구성·탐험·전투·장비·상점·퀘스트 호출 경로
- `TestHarness`와 CMake 테스트 소스 범위
- 파티 구성의 의미, 탐험 의사결정, 전투 선택과 적 다양성, 성장·장비·경제·퀘스트·드롭, 난이도·실패 복구, 저장 계약, 난수/결정성

상용수준은 소규모 유료 PC Wizardry 스타일 던전 RPG 출시 후보라는 사용자 기준으로 해석했다. 2024 공식 Wizardry 제품의 파티 관리·탐색·주문·전투 QoL 및 10층/최종 목표는 비교 관찰에만 사용하며 세부 기능을 강제하지 않았다.

## 3. Excluded and Uninspected Scope

- `audit_report_*.md`, `docs/audit/**`, 다른 `docs/multi_audit/**` 보고서는 읽지 않았다.
- 생성·벤더 트리, 외부 서비스, 패키지 배포물의 상용 인증, 사운드·텍스처 3D는 명시된 비목표 또는 배정 범위 밖이다.
- 장시간 플레이를 통한 실제 승률·골드 유입·레벨업 시간·희귀 드롭 확률은 실행하지 못했다. 해당 항목은 `Needs Clarification` 또는 미검증으로 남겼다.
- GUI의 수동 키 시퀀스, 화면 가독성·접근성·다국어 시각 QA는 이 관점에서 직접 수행하지 않았다. `xvfb-run` 2초 부팅 smoke만 실행했으며 상호작용 검증은 아니다.

## 4. Evidence Examined

제품 문서와 구현:

- `spec.md:9-46, 93-171, 289-432` — 제품 목표, FSM, 캐릭터·전투·퀘스트 계약, 저장·리셋·보상 공식, 아이템·몬스터·퀘스트 기준표
- `designs.md:228-280` — 탐험·전투·인벤토리·스킬 선택·상점 UX 계약
- `IMPLEMENTATION_SUMMARY.md:1-135`, `DESIGN_DECISIONS.md:1-116`, `audit_roadmap.md:33-79`, `README.md:9-22` — 구현 책임, 완료 주장, 향후 로드맵 및 사용자 기능 주장
- `src/model/Character.cpp`, `src/model/Party.cpp`, `src/model/DungeonMap.cpp`, `src/model/Quest.cpp`, `src/model/Skill.cpp`, `src/model/SkillFactory.cpp`, `src/model/ItemFactory.cpp`, `src/model/MonsterFactory.cpp`
- `src/controller/TownState.cpp`, `src/controller/DungeonState.cpp`, `src/controller/CombatState.cpp`, `src/controller/CharacterInfoState.cpp`
- `include/model/{Character,Party,DungeonMap,Monster,Item,Equipment,Quest,Skill,ConcreteItems,ConcreteSkills}.hpp`
- `src/test_harness.cpp`, `CMakeLists.txt`, 현재 `save.json`

명령 및 결과:

- `cmake --build build --config Debug --verbose` — 최종 실행은 `Crawlmaster`, `TestHarness` 모두 링크 성공.
- `./build/TestHarness --run-all` — 모든 모델/i18n 테스트 통과. 같은 명령을 연속 2회 실행해 모두 통과했다.
- `timeout 2s xvfb-run -a ./Crawlmaster` — 창 초기화·세이브 로드는 확인했으나 의도적인 timeout(`exit=124`)으로 종료되어 플레이 흐름은 미검증.
- `rg` 부재·호출 검색 — `getBlessTurns`는 선언/정의만 존재하고, `getDamageDiceCount`, `TileType::DOOR`, `qst_hunt_spiders`, `setSeed`/`seed`의 게임 호출 경로가 없음을 확인했다.

검증된 긍정 증거:

- `TestHarness` 출력에서 DFS 연결성 `169 / 169`, BFS 방문/벽 차단, 캐릭터 능력치·레벨업, 상태이상·물약, 저장 파싱, 상점 판매, 번역 키 테스트가 통과했다.
- 이는 모델 불변조건의 일부를 확인하지만, 실제 State 입력 흐름·전투 턴 전이·보상 드롭·장시간 밸런스의 통과를 의미하지 않는다.

## 5. Findings

### Pass 1: Implementation Compliance

### [A03-F001] 길드가 플레이어가 의도한 파티 구성을 만들 수 없는 무작위 생성 버튼으로 고정됨

- Pass: Implementation
- Pattern: `IMP-002`, `SPEC-GAP-001`
- Area: 파티 구성·직업 역할·길드 호출 경로
- Severity: Major
- Status: Confirmed
- Summary: 네 직업의 역할과 직업별 스킬은 문서화되어 있지만, 실제 길드에는 클래스·이름·능력치 선택/확정 UI가 없고 `rand() % 4`로 직업을 정한다. 네 번 생성해도 원하는 전열/회복/마법 조합을 계획할 수 없다.
- Evidence:
  - `spec.md:131-135`는 Warrior/Mage/Rogue/Cleric의 주 능력치, 장비, 스킬 성장 차이를 파티 구성의 기반으로 정의한다.
  - `src/controller/TownState.cpp:76-103`의 Guild `Num1` 경로는 `rand() % 4` 직업과 순환 고정 이름만 생성한다. 플레이어 입력으로 클래스·이름·능력치·순서를 선택하는 경로가 없다.
  - `src/controller/TownState.cpp:104-119`는 마지막 멤버만 해제할 수 있어 조합 교체도 제한된다.
- Expected Basis: 사용자 감사 기준의 “파티 구성의 의미”와 `spec.md`의 직업별 역할 계약. 특정 직업 선택 UX의 세부 방식은 새로 창작하지 않는다.
- Actual: 파티 구성은 최대 4명의 무작위 직업을 반복 생성하는 결과에 의존하며, `rand()`도 게임 전체 RNG와 연결되지 않는다.
- Impact: 파티 전략이 플레이어 선택이 아닌 운에 의해 결정된다. Cleric/Mage 부재, 역할 중복, 레벨별 스킬 접근 불가가 반복 플레이와 난이도·경제 계획을 좌우하여 핵심 루프의 선택성이 약해진다.
- Suggested Action: 길드에 클래스·이름·초기 능력치 확정 또는 재굴림, 멤버 교체·순서 변경을 제공하고, 직업/스킬/장비 규칙을 데이터 기반으로 연결한다. 무작위 모집을 의도한 경우에도 모집 후보와 클래스 표시·거부·재모집 규칙을 명세로 먼저 확정한다.
- Re-audit Method: 입력 fixture로 4개 직업을 각각 원하는 순서로 생성·교체하고 `save.json` 저장/재로드 후 순서·클래스·초기 장비·스킬이 그대로 유지되는지 확인한다.
- Confidence: High
- Owner:
  - Architect / Coder
- Notes: 레트로 난이도나 무작위 능력치 자체를 문제 삼는 finding이 아니다. 문제는 역할을 소비하는 플레이어 선택 경로가 없다는 점이다.

### [A03-F002] 탐험은 이동·안개·랜덤 전투만 있고 문, 층 목표, 상호작용이 없다

- Pass: Implementation
- Pattern: `IMP-002`, `SPEC-GAP-001`
- Area: 탐험 의사결정·던전 진행
- Severity: Major
- Status: Needs Clarification
- Summary: 현재 구현의 20x20 맵은 연결된 이동 공간이지만, 실제로는 시작 계단으로 돌아오기 전까지 FOW를 걷고 전투를 반복하는 구조다. `Door`는 타입만 있고 생성·상호작용이 없으며 층 완료/최종 목표도 정의·호출되지 않는다.
- Evidence:
  - `spec.md:151-158`은 `Wall`, `Empty`, `Door`, `UpStairs` 셀과 DFS/루프 생성을 문서화하지만, 층 완료 조건이나 던전 상호작용 계약은 정의하지 않는다.
  - `src/model/DungeonMap.cpp:24-50,204-265`는 모든 셀을 `WALL`로 초기화한 뒤 DFS와 loop에서 `EMPTY`만 열고 `[1][1]`만 `UPSTAIRS`로 지정한다. `DOOR` 할당은 없다(`rg -n 'TileType::DOOR' src` 결과 없음).
  - `src/controller/DungeonState.cpp:69-203,205-267`는 이동/회전, FOW, BFS 자동 이동과 10% 인카운터만 처리한다. 보물·문·방·목표 상호작용 호출이 없다.
  - `audit_roadmap.md:75-79`는 다층 던전 레이어링을 Post-Launch 항목으로 남긴다.
- Expected Basis: 사용자 목표의 탐험 의사결정·난이도 곡선·퀘스트/드롭 상호작용. 사용자 요청은 10층을 강제하지 않으므로 “몇 층/어떤 목표가 상용 제품 범위인지”는 명세 확인이 필요하다.
- Actual: 현재 코드에는 단일 맵·단일 입구 계단·전투 외 탐험 사건·최종 목적지가 없다.
- Impact: 탐험 위험을 감수할 이유와 전투 외 의사결정이 없어 핵심 루프가 짧고 반복적으로 소비된다. 사용자가 의도한 제품이 수직 슬라이스인지 완결형 던전 RPG인지 판정할 통제 문서가 부족하다.
- Suggested Action: 먼저 이번 출시 범위를 `vertical slice` 또는 완결형 캠페인으로 확정한다. 완결형이면 depth/entrance/exit/clear condition, 문·방·함정·보물·퀘스트 목적지와 귀환 규칙을 정의하고, 각 요소를 저장·드롭·난이도 표와 연결한다.
- Re-audit Method: 고정 seed 맵에서 비전투 상호작용과 층 목표를 최소 1회 완료하는 headless 시나리오, 목표 실패·귀환·재진입 상태를 저장/복구하는 시나리오를 추가한다.
- Confidence: High for absence; Medium for release severity because final product scope is unspecified.
- Owner:
  - Architect / Human
- Notes: 와이어프레임·고정 해상도·사운드 비목표는 결함으로 판정하지 않았다. 여기서는 탐험 시스템이 실제 장기 루프에서 소비되는지에 대한 범위 공백을 기록한다.

### [A03-F003] 전투 승리에는 아이템 드롭 경로가 없어 고급 콘텐츠와 COLLECT 루프가 도달 불가

- Pass: Implementation
- Pattern: `IMP-001`, `IMP-003`
- Area: 전투 보상·아이템 획득·퀘스트/경제 연결
- Severity: Major
- Status: Confirmed
- Summary: 아이템 팩토리는 18개 ID를 만들지만 전투 승리 보상은 XP·골드·KILL 진행만 처리한다. 상점은 문서상 기본 8종만 팔고, 퀘스트 완료도 골드·XP만 주므로 고급 장비·특수 소모품의 정상 플레이 획득원이 없다.
- Evidence:
  - `spec.md:410-413`은 고급 장비 및 특수 소모품을 “던전 탐험 파밍이나 퀘스트 완료 보상”으로만 획득하도록 동결한다.
  - `src/controller/CombatState.cpp:699-745`의 `distributeRewards()`는 `totalXp`, `totalGold`, `updateQuestKillProgress()`와 저장만 수행하며 `party.addItem()` 또는 드롭 테이블 호출이 없다.
  - `src/controller/TownState.cpp:136-149`에서 `addItem()`은 상점 구매에만 사용되고, 그 외 `addItem` 호출은 장비 스왑/해제 반환(`src/controller/CharacterInfoState.cpp:376-425`)뿐이다.
  - `src/model/ItemFactory.cpp:10-85`의 고급/특수 ID 생성은 가능하지만 플레이어 획득 호출과 연결되지 않는다.
- Expected Basis: `spec.md:23-31,410-413`의 수집·드롭·퀘스트 루프 및 완료 주장. 아이템 생성 존재만으로 획득 가능성을 인정하지 않는다.
- Actual: 정상 플레이에서 `wpn_greatsword`, `wpn_rapier`, `arm_plate`, `shd_tower`, `pot_mana`, 버프 물약, `scr_cure` 등을 얻을 보상/드롭 경로가 없다. `wpn_staff`/`arm_robe`는 Mage 생성 시 초기 장비로만 제한적으로 주어질 뿐, 진행 보상/드롭은 없다. `qst_collect_maces`는 아이템을 요구하지만 전투 드롭과 연결되지 않는다.
- Impact: 장비·소모품의 장기 진행 가치가 사라지고, 던전 리스크와 보상이 연결되지 않는다. 문서의 “확장 콘텐츠”는 팩토리 단위에만 존재하고 플레이어 경험으로 소비되지 않는다.
- Suggested Action: 몬스터·층·퀘스트별 loot table과 보장/확률·중복·저장 규칙을 정의하고 `distributeRewards()`의 단일 보상 pipeline에 연결한다. 획득 가능한 모든 콘텐츠에 최소 한 개의 정상 플레이 source와 회귀 fixture를 둔다.
- Re-audit Method: 고정 seed 전투에서 각 몬스터/퀘스트의 드롭이 기대 분포와 ID로 생성되고 인벤토리·퀘스트 진행·저장/재로드에 반영되는지 확인한다.
- Confidence: High
- Owner:
  - Architect / Coder
- Notes: 수동 save 편집이나 테스트에서 `Party::addItem()`을 호출하는 것은 제품 획득 경로의 증거가 아니다.

### [A03-F004] 명세의 거미 사냥 퀘스트가 실제 퀘스트 보드에 없다

- Pass: Implementation
- Pattern: `IMP-001`, `IMP-004`
- Area: 퀘스트 콘텐츠·보드 호출 경로
- Severity: Major
- Status: Confirmed
- Summary: `spec.md`의 기본 퀘스트 3종 중 `qst_hunt_spiders`는 생성·수락·보고 경로가 없고, `TownState`에는 코볼트와 메이스 퀘스트만 하드코딩되어 있다.
- Evidence:
  - `spec.md:427-432`는 `qst_clear_kobolds`, `qst_collect_maces`, `qst_hunt_spiders`를 기본 목록으로 정의한다.
  - `src/controller/TownState.cpp:212-261`에는 `qst_clear_kobolds`와 `qst_collect_maces`만 수락/진행/보고된다.
  - `rg -n 'qst_hunt_spiders' src include` 결과가 없다.
- Expected Basis: 명세의 기본 콘텐츠 목록과 README/로드맵의 퀘스트 연동 완료 주장. 추가 퀘스트를 실제 출시 범위에서 제외하려면 문서에 명시해야 한다.
- Actual: 플레이어는 거미를 처치해도 그 처치가 해당 기본 퀘스트로 추적되는지 알 수 없고, 보드에서 수락할 수도 없다.
- Impact: 콘텐츠 표와 실제 퀘스트 소비량이 다르며, 적 다양성의 일부가 장기 목표로 연결되지 않는다.
- Suggested Action: 거미 퀘스트를 보드 데이터/번역/수락·보고·저장·보상 pipeline에 연결하거나, 이번 제품 범위에서 제외하고 `spec.md`·README·로드맵의 기본 목록과 완료 주장을 동기화한다.
- Re-audit Method: 보드에서 수락 → 거미 처치 → 진행도 저장 → 보고 → 보상/재수락 정책까지 한 이벤트 시퀀스를 실행한다.
- Confidence: High
- Owner:
  - Architect / Coder
- Notes: 난수로 거미가 스폰되는 것과 거미 퀘스트가 제품에 존재하는 것은 별개다.

### [A03-F005] 퀘스트가 무한 재수락되고 COLLECT 목표는 상점 구매로 즉시 우회된다

- Pass: Implementation
- Pattern: `IMP-002`
- Area: 퀘스트 상태·경제 악용·보상 회수
- Severity: Major
- Status: Confirmed
- Summary: 완료 퀘스트를 이력에 남기지 않고 활성 목록에서 지운 뒤 같은 키로 즉시 다시 수락할 수 있다. 동시에 메이스 수집 퀘스트는 상점에서 판매하는 메이스를 그대로 세어 완료할 수 있어 던전 수집 목표와 보상 루프가 분리된다.
- Evidence:
  - `src/model/Party.cpp:215-265`에서 완료 시 골드·XP를 지급하고 `m_activeQuests.erase(it)`로 제거한다. 완료 이력, 일회성 보상, cooldown, repeatable 정책이 없다.
  - `src/controller/TownState.cpp:214-243`은 `!party.hasQuest(qId)`이면 같은 코볼트/메이스 퀘스트를 다시 생성·수락한다.
  - `src/controller/TownState.cpp:136-149`은 상점에서 `wpn_mace`를 구매해 인벤토리에 넣고, `src/controller/TownState.cpp:245-251`은 인벤토리의 해당 ID 개수만으로 `qst_collect_maces`를 완료시킨다.
  - `spec.md:410-413,427-432`는 메이스를 COLLECT 목표로 두지만 획득 provenance 또는 상점 구매 제한을 정하지 않아 코드의 우회가 가능하다.
- Expected Basis: 사용자 기준의 경제 악용·퀘스트와 드롭 상호작용. 반복 퀘스트가 의도라면 보상·회수율·반복 제한을 명시해야 한다.
- Actual: 플레이어가 코볼트 5마리 또는 메이스 2개를 반복적으로 보고하여 골드/XP를 누적할 수 있고, 메이스는 던전 위험 없이 구매 후 보고할 수 있다.
- Impact: 상점 가격·몬스터 골드·레벨업 곡선을 우회하는 무한 보상 루프가 된다. 전투·탐험·드롭의 장기 의사결정이 경제적으로 무의미해진다.
- Suggested Action: 퀘스트에 `repeatable`, completion history, cooldown/상한과 보상 예산을 명시하고, COLLECT 아이템에 `source`/quest-bound provenance를 부여해 상점 구매품을 제외하거나 메이스의 획득원을 조정한다. 퀘스트 완료는 보상 지급과 상태 전이를 하나의 원자적 명령으로 묶는다.
- Re-audit Method: 동일 퀘스트의 2회 보고, 구매 메이스 2개 보고, 던전 드롭 메이스 보고를 각각 실행해 허용/거부·골드·XP·인벤토리 차감 결과를 확인한다.
- Confidence: High
- Owner:
  - Architect / Coder
- Notes: 반복 퀘스트를 제품 기능으로 채택할 수는 있으나 현재는 그 정책과 경제 budget이 문서·코드·테스트에 없다.

### [A03-F006] 저장 파일의 승인 스키마와 구현 스키마가 달라 정상 계약 데이터도 리셋될 수 있다

- Pass: Implementation
- Pattern: `IMP-001`, `IMP-004`
- Area: 저장 계약·세이브/로드 진행 보존
- Severity: Major
- Status: Confirmed
- Summary: `spec.md`의 캐릭터 저장 예시는 camelCase·중첩 `equipment`·긴 능력치 키를 사용하지만, `Character::toJson/fromJson`은 snake_case·평면 `eq_*`·짧은 능력치 키를 사용한다. 계약 형식으로 작성된 데이터는 로드 시 예외를 일으켜 기본 파티로 리셋될 수 있다.
- Evidence:
  - `spec.md:302-339`는 `maxHp`, `spellSlots`, `poisonTurns`, `paralysisTurns`, `abilities.strength`, `equipment.weapon` 등의 구조를 계약 예시로 제시한다.
  - `src/model/Character.cpp:384-412`는 `max_hp`, `spell_slots`, `poison_turns`, `eq_weapon`, `abilities.str` 등을 저장한다.
  - `src/model/Character.cpp:417-469`는 구현 스키마의 키만 직접 읽으며 schema version/migration 분기가 없다.
  - `src/model/Party.cpp:142-187`는 필드 타입/키 예외를 잡으면 `resetToDefault()`를 호출하고 `true`를 반환한다.
- Expected Basis: `spec.md:4-6,302-341`의 master save contract와 사용자 기준의 저장 데이터 무결성.
- Actual: 문서 계약과 실제 파일 형식이 양립하지 않는다. 현재 루트 `save.json:1-8`도 명세의 초기 inventory인 heal 2개 + mana 1개가 아니라 heal 2개만 보유한다.
- Impact: 수동/이전 버전/도구가 문서 계약을 따르는 경우 캐릭터·장비·진행이 통째로 초기화될 수 있다. 형식 드리프트를 정상 로드로 오인하기 어렵고 지원/마이그레이션도 불가능하다.
- Suggested Action: canonical schema를 하나로 결정하고 `schema_version`과 명시적 migration을 구현한다. 저장·로드 round-trip fixture를 문서 예시와 동일한 키로 고정하고, migration 실패는 새 세이브로 조용히 대체하지 말고 사용자 복구 경로로 반환한다. 초기 기본 inventory도 heal 2 + mana 1인지 문서와 코드가 동일하게 맞춘다.
- Re-audit Method: 문서 예시 JSON과 현재 생성 JSON을 각각 로드하고, 모든 캐릭터 필드·장비·퀘스트·초기 아이템이 손실 없이 round-trip 되는지 확인한다.
- Confidence: High
- Owner:
  - Architect / Coder
- Notes: nlohmann/json 자체의 파싱 성공은 애플리케이션 schema 정합성을 보장하지 않는다.

### Pass 2: Debug / Engineering Quality

### [A03-F007] 모든 게임 난수가 호출별 random_device에 묶여 seed 재현이 불가능하다

- Pass: Debug
- Pattern: `DBG-002`
- Area: RNG·seed·결정적 검증
- Severity: Major
- Status: Confirmed
- Summary: 명세는 `std::mt19937`를 요구하지만 중앙 RNG나 seed 주입/기록 API가 없고, 맵·캐릭터·스킬·몬스터·전투 컨트롤러가 각 호출마다 `std::random_device`로 새 엔진을 만든다. 길드만 별도 C `rand()`를 사용한다.
- Evidence:
  - `spec.md:40-46`은 PRNG 기반 판정을 동결하지만 seed 공개·저장·재생 계약은 없다.
  - `src/model/DungeonMap.cpp:216-219,237-264`, `src/model/Character.cpp:55-75,150-156,298-305`, `src/model/Skill.cpp:13-29`, `include/model/Monster.hpp:64-80,102-107`, `src/controller/CombatState.cpp:166-183,321-323,571-573,710-713`에서 호출마다 `random_device`가 생성된다.
  - `src/controller/TownState.cpp:82-84`는 `rand() % 4`를 사용한다.
  - `rg -n 'setSeed|seed' src include`에서 게임 제어용 seed API가 확인되지 않았다.
- Expected Basis: 사용자가 명시한 “랜덤성/시드” 및 표준 `DBG-002`의 동일 fixture·seed 재현 기준.
- Actual: 같은 입력·같은 파티를 재현할 seed가 없고, 주사위·드롭·맵·AI 타겟 선택의 난수 순서도 한 스트림으로 관찰할 수 없다.
- Impact: 밸런스 실패, 희귀 드롭, TPK, AI 행동을 버그 리포트에서 재생할 수 없다. 현재 `TestHarness`의 녹색 결과도 특정 난수 시나리오의 회귀를 잠그지 못한다.
- Suggested Action: `GameRng`를 Game/State/Model에 주입하고 production seed와 debug seed를 분리한다. seed와 필요한 RNG stream/version을 run metadata 또는 debug replay에 기록하며, 맵·전투·드롭·능력치가 같은 seed에서 동일 로그/상태 hash를 만드는 테스트를 추가한다. C `rand()`를 제거한다.
- Re-audit Method: seed 하나로 맵 hash, 파티 생성, 인카운터, initiative, 공격/피해, 보상과 AI 로그를 두 번 생성해 byte/hash 동일성을 확인하고 다른 seed의 분기를 확인한다.
- Confidence: High
- Owner:
  - Architect / Coder
- Notes: 난수가 충분히 랜덤한가의 통계 문제와 별개로, 감사·지원 가능한 결정성 표면이 없다는 finding이다.

### [A03-F008] 전투 일반 공격과 스킬이 무기 주사위 개수를 무시한다

- Pass: Debug
- Pattern: `DBG-002`
- Area: 전투 피해·장비 성장·스킬 수치
- Severity: Major
- Status: Confirmed
- Summary: `wpn_greatsword`는 2d6으로 정의되고 `Equipment`도 `getDamageDiceCount()`를 제공하지만 실제 플레이어 일반 공격·Slash·Cleave는 sides만 읽고 항상 1회 굴린다. 치명타도 2회 굴림으로 고정되어 2d6 무기가 1d6처럼 동작한다.
- Evidence:
  - `spec.md:393`과 `src/model/ItemFactory.cpp:24-27`은 그레이트소드를 2d6으로 정의한다.
  - `include/model/Equipment.hpp:32-36`, `include/model/ConcreteItems.hpp:151-164`에 주사위 개수 계약이 존재한다.
  - `src/controller/CombatState.cpp:339-362`는 `getDamageDiceSides()`만 가져와 일반 명중은 `dmgDist(gen)` 1회, 치명타는 2회 호출한다.
  - `src/model/Skill.cpp:58-66,135-152`도 `rollDice(1, dmgDice)`만 사용한다.
- Expected Basis: 아이템 표의 2d6 계약과 전투 피해 공식 `spec.md:163-167`.
- Actual: 그레이트소드의 일반 피해는 1d6+능력치, Slash/Cleave 역시 1회 주사위로 처리된다. `getDamageDiceCount()`는 소비되지 않는다.
- Impact: 고급 양손 무기의 구매·드롭 가치와 빌드 선택이 사라지고, 치명타·스킬 밸런스 계산이 문서 수치와 어긋난다.
- Suggested Action: 공통 `rollWeaponDamage(count, sides, modifier)`를 만들고 일반 공격·스킬·치명타가 같은 무기 사양을 사용하게 한다. 무기별 최소/최대·평균 피해와 치명타를 고정 seed 테스트로 검증한다.
- Re-audit Method: 1d4, 1d8, 2d6 무기를 동일 seed로 공격해 굴림 횟수·범위·치명타 배수와 로그가 item contract와 일치하는지 확인한다.
- Confidence: High
- Owner:
  - Coder
- Notes: Slash/Cleave에 자체 피해 주사위를 우선할지 장착 무기 주사위를 우선할지는 문서에 명시한 뒤 한 정책으로 통일해야 한다.

### [A03-F009] 적 다양성 계약인 스켈레톤 저항과 적 자연 치명타가 전투 엔진에 없다

- Pass: Debug
- Pattern: `DBG-002`, `IMP-001`
- Area: 적 방어·피해 타입·몬스터 전투 규칙
- Severity: Major
- Status: Confirmed
- Summary: 스켈레톤은 참격/관통 감쇄와 메이스 권장이 문서화되어 있지만 피해 타입이 모델·스킬·무기 API에 없고 `takeDamage(int)`가 모든 피해를 동일 처리한다. 적 기본 공격도 원시 d20 결과를 노출하지 않아 자연 20 치명타/자연 1 무조건 실패가 적용되지 않는다.
- Evidence:
  - `spec.md:163-167,418-425`는 자연 20/1 규칙과 스켈레톤의 피해 감쇄·메이스 권장을 정의한다.
  - `include/model/Monster.hpp:21-26,49-82`는 `takeDamage(int)`와 총합만 반환하는 `getAttackRoll()`만 제공하고, 피해 타입·원시 주사위·치명타 정보가 없다.
  - `src/controller/CombatState.cpp:636-641`은 적 `attackRoll >= targetAc`만 검사하고 자연값 분기나 치명타 피해를 처리하지 않는다.
  - `src/model/Monster.hpp:60-62`의 `takeDamage()`는 모든 정수 피해를 그대로 HP에서 차감한다. `src/model/Skill.cpp`의 모든 공격도 타입 없는 정수 피해다.
- Expected Basis: 문서화된 적 다양성과 사용자 기준의 “전투 선택/적 다양성”. 레트로 단순화는 허용하지만 문서에 적힌 전술 차이는 실제 소비되어야 한다.
- Actual: 메이스를 들어도 스켈레톤에 별도 효과가 없고, 적 자연 20은 평범한 명중일 뿐이다.
- Impact: 적별 카운터·무기 선택·치명타 긴장감이 장식에 그치며, 8종 몬스터의 체력/AC 숫자 외 전투 의사결정이 줄어든다.
- Suggested Action: `DamageType`/저항·취약도와 공격 결과(`rawRoll`, `isCritical`, `isFumble`)를 공통 전투 결과 타입으로 도입한다. 스켈레톤 감쇄와 모든 몬스터 공격의 자연값 정책을 데이터/규칙으로 고정하고 고정 seed 회귀를 추가한다.
- Re-audit Method: 스켈레톤에 메이스·검·스킬을 같은 피해 seed로 적용해 감쇄가 확인되는지, 적 raw d20 1/20 fixture에서 miss/crit이 일관되게 적용되는지 확인한다.
- Confidence: High
- Owner:
  - Architect / Coder
- Notes: 스켈레톤의 감쇄 수치를 문서가 정하지 않았으므로 정확한 수치는 `Needs Spec Clarification`으로 분리해야 한다. 여기서 확정된 것은 해당 규칙의 소비 경로 부재다.

### [A03-F010] Bless 상태는 기록·감소되지만 공격 명중에 연결되지 않는 고아 상태다

- Pass: Debug
- Pattern: `DBG-002`, `IMP-003`
- Area: Cleric 지원 선택·버프 소비·전투 판정
- Severity: Major
- Status: Confirmed
- Summary: Bless 주문은 3턴 상태를 부여하고 만료시키지만 일반 공격과 12개 스킬의 명중 계산 어디에서도 `getBlessTurns()`를 읽지 않는다. 문서의 +2 명중 지원 선택이 실질적으로 무효다.
- Evidence:
  - `spec.md:143-149`와 `include/model/ConcreteSkills.hpp:202-211`은 Bless가 3턴 동안 모든 공격 명중에 +2라고 정의한다.
  - `src/model/Skill.cpp:433-453`는 `applyBless(3)`만 호출한다.
  - `src/model/Character.cpp:282-295,336-349`는 상태 저장·감소·초기화를 수행한다.
  - `src/controller/CombatState.cpp:321-333,355-370`와 `src/model/Skill.cpp:45-49,97-100,146-151,275-279,322-324,359-362`의 명중 계산에는 Bless 보정이 없다. `rg -n 'getBlessTurns' src`는 정의 외 사용이 없다.
- Expected Basis: `spec.md:149,163-165`의 Bless +2 공격 명중 계약.
- Actual: Bless를 시전하면 로그와 턴 카운터만 변하고 이후 공격 roll은 동일하다.
- Impact: Cleric 레벨 2 주문의 핵심 전술 선택이 낭비되며, 주문 슬롯·힐·공격의 장기 밸런스 평가가 왜곡된다.
- Suggested Action: 공통 attack-roll resolver가 active Bless를 +2로 읽도록 하고 일반 공격·모든 명중형 스킬·적용 범위를 한 규칙으로 통일한다. Bless 전/후 동일 raw roll fixture를 두어 정확히 +2가 적용되는지 검증한다.
- Re-audit Method: 1개 파티에서 Bless 전후 동일 raw d20/대상 AC로 일반 공격과 각 명중형 스킬을 비교하고, 3턴 이후 보정이 사라지는지 확인한다.
- Confidence: High
- Owner:
  - Coder
- Notes: 상태를 저장하는 것과 그 상태가 게임 결과를 바꾸는 것은 별도 검증 대상이다.

### [A03-F011] Cure Wounds와 전투 아이템 행동이 플레이어의 대상·아이템 선택을 소비하지 않는다

- Pass: Debug
- Pattern: `DBG-002`, `IMP-001`
- Area: 전투 UX와 파티 의사결정·지원 행동
- Severity: Major
- Status: Confirmed
- Summary: `CureWounds`는 `targetIdx`를 무시하고 최저 HP 비율 아군을 자동 선택한다. `CombatState::performUseItem()`도 인벤토리 선택 화면 없이 숨은 우선순위로 아이템·대상을 정하고, 효과가 없어도 아이템을 제거한다.
- Evidence:
  - `include/model/ConcreteSkills.hpp:184-199`는 `SINGLE_ALLY` 대상 타입과 선택 가능한 스킬 계약을 선언한다.
  - `src/model/Skill.cpp:393-430`의 `CureWoundsSpell::execute()`는 `targetIdx`를 사용하지 않고 `lowestRatio` 아군을 선택하며, 모두 풀피이면 `allies[0]`을 선택한다.
  - `src/controller/CombatState.cpp:394-505`는 해독→고급 힐→일반 힐→마나→버프→첫 소모품 순으로 자동 선택한다.
  - `src/controller/CombatState.cpp:514-529`는 `applyEffect()` 결과와 무관하게 `party.removeItem(itemIdx)`를 수행한다. 풀피에서 힐, 독이 없을 때 해독, 비캐스터에게 마나 물약 등이 소모될 수 있다.
  - `designs.md:259-262`는 스킬 선택 팝업과 타겟팅 연동을 요구하고, 사용자 기준은 전투 선택이 실제 장기 선택이어야 한다.
- Expected Basis: `SkillTargetType::SINGLE_ALLY`, `designs.md`의 스킬/전투 선택 UX 및 사용자 기준의 파티 의사결정. 자동 보조를 채택하려면 실패·취소·대상 규칙을 명세해야 한다.
- Actual: Cleric이 특정 부상자를 선택할 수 없고, 전투 Item 명령은 플레이어가 어떤 물약을 누구에게 쓸지 결정하지 못한다. no-op 사용도 행동과 아이템을 잃는다.
- Impact: 4인 파티의 위치·HP·상태이상·자원 관리가 선택으로 소비되지 않아 전투가 자동화된 숫자 소모에 가까워진다. 귀중한 드롭이 무효 대상 때문에 사라질 수 있다.
- Suggested Action: 대상 선택 상태와 아이템 목록/취소를 추가하고, 효과 전 검증 실패는 행동·아이템을 소비하지 않도록 `UseResult`를 반환한다. 자동 추천을 보조로 제공하더라도 확인 입력과 명시적 대상 override를 둔다.
- Re-audit Method: 부상자 2명·상태이상 1명·서로 다른 소모품 3개 fixture에서 대상/아이템 선택, 취소, no-op, 행동 소모 여부를 event sequence로 확인한다.
- Confidence: High
- Owner:
  - Architect / Coder
- Notes: 비전투 CharacterInfo 화면은 일부 대상 선택을 제공하지만, 해당 화면이 전투 Item/주문 호출 경로를 대체하지 않는다.

### [A03-F012] 중갑·타워 실드의 STR 제한이 UI·모델 저장 경계에서 강제되지 않는다

- Pass: Debug
- Pattern: `DBG-002`, `SEC-005` (게임 데이터 불변조건 관점)
- Area: 장비 제한·AC 계산·세이브 복구
- Severity: Major
- Status: Confirmed
- Summary: 문서상 Plate STR 15, Tower Shield STR 14 제한이 있지만 `CharacterInfoState`는 클래스 제한만 검사하고 STR를 검사하지 않으며, 공개 `Character::equip()`은 어떤 장비도 장착한다. `fromJson()`도 같은 메서드를 직접 호출한다.
- Evidence:
  - `spec.md:400-402`는 Plate STR 15와 Tower Shield STR 14 제한을 명시한다.
  - `src/controller/CharacterInfoState.cpp:327-354`에는 클래스별 ID 제한만 있고 `strength` 비교가 없다.
  - `src/model/Character.cpp:112-130`의 `equip()`은 null/슬롯만 검사하고 클래스·STR·양손 불변조건을 검사하지 않는다.
  - `src/model/Character.cpp:445-464`의 `fromJson()`은 저장 ID를 검증 없이 `equip()`한다.
- Expected Basis: 아이템 표의 명시적 STR 제한과 장비가 제공하는 AC가 캐릭터 규칙을 넘지 않아야 한다는 불변조건.
- Actual: 낮은 STR 캐릭터가 UI 또는 문서 형식 save를 통해 Plate/Tower를 장착해 AC를 얻을 수 있다. 양손/클래스 규칙도 UI에만 있어 다른 호출 경로에서 우회된다.
- Impact: 세이브 편집·향후 코드 경로·테스트 fixture가 규칙을 우회하고 AC/난이도 곡선을 무너뜨린다.
- Suggested Action: 장비 요구조건을 아이템 데이터에 넣고 `Character::equip()`을 canonical validator로 만든다. 실패 이유를 반환해 UI에 표시하고, deserialization은 invalid load를 거부/마이그레이션한다. STR 14/15 경계와 모든 장착 경로를 테스트한다.
- Re-audit Method: STR 13/14/15 및 각 클래스에서 Plate/Tower/양손/금지 장비를 UI·직접 API·save/load 세 경로로 시도해 동일 결과를 확인한다.
- Confidence: High
- Owner:
  - Architect / Coder
- Notes: TestHarness의 `testAdvancedEquipSwapAndClassLimits()`는 UI 로직을 람다로 모사할 뿐 실제 `CharacterInfoState` 및 STR 경계를 검증하지 않는다.

### [A03-F013] 승리 보상 골드·XP 계산이 동결 공식과 달라 경제/성장 곡선을 재현할 수 없다

- Pass: Debug
- Pattern: `DBG-002`, `IMP-001`
- Area: 보상·경제·레벨업
- Severity: Major
- Status: Confirmed
- Summary: 명세는 몬스터 레벨별 `(1d10 * monster level)` 골드를 정의하지만 Monster 모델에는 레벨이 없고 CombatState는 전체 적 수만큼 `1d5..15`를 곱한다. XP도 몬스터별 분배가 아니라 합산 후 한 번 나눠 정수 버림하여 다수 전투에서 분배 결과가 달라질 수 있다.
- Evidence:
  - `spec.md:352-362`는 레벨업 threshold와 `Monster.xpReward / 살아있는 파티원 수`, 골드 `(1d10 * 몬스터 레벨)` 공식을 정의한다.
  - `include/model/Monster.hpp:33-39,49-56`에는 monster level getter/field가 없다.
  - `src/controller/CombatState.cpp:703-713`은 XP를 합산하고, 골드는 `uniform_int_distribution<>(5, 15) * m_foes.size()`로 계산한다.
  - `src/controller/CombatState.cpp:715-733`은 `totalXp / aliveCount` 한 번만 계산한다. 공식의 몬스터별 정수 분배를 의도했다면 반올림 지점이 다르다.
- Expected Basis: 동결된 보상 공식과 사용자의 경제·성장 곡선 감사 기준.
- Actual: Kobold/Dragon 등 몬스터 종류·레벨에 따라 달라져야 할 골드가 모두 동일한 범위이고, 전투 적 수에만 비례한다. 보상 로그·실제 분배의 계약도 분리되지 않는다.
- Impact: 상점 구매력, 회복 비용, 레벨업까지 필요한 전투 수를 설계·튜닝·재현할 수 없다. 몬스터 위험과 보상이 일관되지 않는다.
- Suggested Action: monster level 또는 명시적 gold reward profile을 데이터 모델에 추가하고 몬스터별 roll을 보상 ledger에 기록한다. XP는 명세의 분배 단위를 고정하고 정수 잔여 처리 규칙을 문서화한다. 골드/XP ledger를 고정 seed 테스트로 잠근다.
- Re-audit Method: 단일/다수 몬스터와 alive 1~4명 fixture에서 기대 gold roll, XP 잔여, 로그·save 값이 공식과 동일한지 확인한다.
- Confidence: High
- Owner:
  - Architect / Coder
- Notes: 실제 장시간 골드 인플레이션 정도는 플레이테스트가 필요하지만, 현재 공식과 구현의 불일치는 실행 없이 확정된다.

### [A03-F014] 깊이·파티 레벨을 반영하는 난이도 곡선과 적 AI 정책이 없다

- Pass: Debug
- Pattern: `DBG-002`, `SPEC-GAP-001`
- Area: 난이도 곡선·몬스터 스폰·전투 AI
- Severity: Major
- Status: Needs Clarification
- Summary: 모든 던전 진입은 새 20x20 맵을 만들고, 인카운터는 깊이·파티 레벨 없이 고정 1~3마리와 전역 확률표를 사용한다. 적 AI는 살아 있는 파티원을 균등 무작위로 고르는 기본 공격과 ID별 특수행동뿐이다. 문서에는 출시 난이도 예산·최종 목표·깊이별 출현 규칙이 없다.
- Evidence:
  - `src/controller/DungeonState.cpp:13-20`은 진입마다 단일 `DungeonMap::generate()`를 호출하고 depth/party level을 전달하지 않는다.
  - `src/controller/CombatState.cpp:166-174`는 1~3마리만 생성한다.
  - `src/model/MonsterFactory.cpp:52-77`의 고정 확률표는 어떤 층/파티 레벨에도 동일하며 Dragon Whelp도 같은 함수에서 3%로 선택된다.
  - `src/controller/CombatState.cpp:561-576`은 살아 있는 멤버 중 하나를 균등 무작위로 선택한다. 보호·위협·상태이상·저체력 우선 등의 정책은 없다.
  - `audit_roadmap.md:75-79`는 다층화를 향후 작업으로 둔다.
- Expected Basis: 사용자 기준의 난이도 곡선·레벨 스케일·전투 AI. 정확한 곡선 값은 승인 문서가 없어 창작하지 않는다.
- Actual: 현재 상태로는 1층 초반에도 고정 확률로 35 HP Dragon Whelp가 등장할 수 있고, 후반으로 갈수록 위협/보상/행동이 증가한다는 보장이 없다.
- Impact: 초반 즉사와 후반 단조로움, 파티 구성의 운 의존이 발생할 수 있다. 장시간 승률·소프트락·회복 경제를 검증할 기준선이 없다.
- Suggested Action: 출시 범위와 depth/party-level encounter budget을 명세하고, 몬스터 tier/출현·특수행동·도망·회복 비용을 같은 곡선으로 설계한다. AI 정책(타겟 우선순위, 상태이상 사용, telegraph)을 데이터/결정적 seed로 고정한다.
- Re-audit Method: depth/party-level별 수백 회 고정 seed 시뮬레이션과 소규모 실제 플레이로 encounter mix, TPK/win rate, 평균 회복비·레벨업 시간을 측정하고 목표 범위와 비교한다.
- Confidence: High for absence; Medium for exact severity because product difficulty intent is unspecified.
- Owner:
  - Architect / Human
- Notes: 레트로 난이도가 높다는 것 자체는 결함이 아니다. 문제는 의도된 난이도를 표현·검증할 곡선과 정책이 없다는 점이다.

### [A03-F015] 세이브가 직접 파일을 truncate하고 손상 시 마지막 진행 대신 기본 상태를 덮어쓴다

- Pass: Debug
- Pattern: `TEST-001`, `DBG-001`
- Area: 저장 데이터 무결성·실패 복구
- Severity: Major
- Status: Confirmed
- Summary: `Party::saveToFile()`은 기존 파일을 직접 `ofstream`으로 열어 쓰며 temp/atomic rename/backup이 없다. 로드 오류는 마지막 유효 세이브를 보존하지 않고 `resetToDefault()`로 덮어쓴 뒤 성공(`true`)을 반환한다.
- Evidence:
  - `src/model/Party.cpp:88-132`는 `std::ofstream file(filePath)`로 직접 파일을 열고 `j.dump(4)`를 쓴다. 임시 파일, fsync, 원자 교체, 백업이 없다.
  - `src/model/Party.cpp:142-187`은 파싱/타입 예외를 잡아 `resetToDefault()`를 호출하고 `true`를 반환한다.
  - `src/model/Party.cpp:191-201`은 reset 시 멤버·퀘스트·인벤토리를 지우고 초기값을 바로 같은 경로에 저장한다.
  - `IMPLEMENTATION_SUMMARY.md:123-124`는 손상 세이브 “안전하게 복구”를 완료로 주장하지만 실제 복구는 마지막 정상 상태가 아닌 새 기본 상태다.
- Expected Basis: 사용자 기준의 저장 데이터 무결성·실패 복구 및 `spec.md:289-300`의 리셋 정책. TPK 하드코어 리셋과 예기치 않은 파일 손상 복구는 구분되어야 한다.
- Actual: 프로세스 중단/디스크 오류로 부분 JSON이 생기면 정상 진행을 복원할 백업이 없고, loader가 손실을 정상 성공으로 보고한다.
- Impact: 유료 게임에서 캐릭터·장비·퀘스트 진행이 비가역적으로 소실될 수 있다. 테스트가 손상 파일을 기본값으로 바꾸는 동작만 확인하여 데이터 보존을 확인하지 않는다.
- Suggested Action: sibling temp 파일에 완전한 JSON을 쓰고 flush/fsync 후 atomic rename하며, 이전 세이브를 rotating backup으로 보존한다. schema 검증 실패는 `LoadResult::Corrupt`로 반환하고 마지막 정상 세이브 또는 복구 선택을 제공한다. TPK만 명시적 hard reset command로 분리한다.
- Re-audit Method: 저장 중 중단/부분 파일, 잘못된 schema, unknown item, 정상 backup을 각각 주입해 원본 보존·backup 복구·정확한 오류 결과·TPK 의도 reset을 구분 확인한다.
- Confidence: High
- Owner:
  - Architect / Coder
- Notes: 이 finding은 보안 공격이 아니라 사용자 진행 데이터 보존과 복구 UX의 문제다.

### [A03-F016] 던전 실행 상태의 저장 계약이 README와 실제 Party 저장 모델 사이에서 불명확하다

- Pass: Debug
- Pattern: `ARCH-002`, `SPEC-GAP-001`
- Area: 던전 좌표·맵 seed·재개/귀환 저장
- Severity: Major
- Status: Needs Clarification
- Summary: README는 파티·골드·퀘스트와 던전 좌표를 영구 저장한다고 설명하지만 `Party::saveToFile()`에는 좌표·맵·seed/depth가 없고 `DungeonState`는 매 진입마다 새 맵을 생성한다. `spec.md`의 save contract에는 해당 필드가 없다.
- Evidence:
  - `README.md:18-20,52-58`은 던전 진행/coordinates를 JSON save에 포함한다고 주장한다.
  - `spec.md:302-340`의 save 예시에는 `gold`, `inventory`, `members`, `active_quests`만 있고 map/position/depth/seed가 없다.
  - `src/model/Party.cpp:88-118`의 저장 필드에도 map/position/depth/seed가 없다.
  - `src/controller/DungeonState.cpp:13-20`은 새 상태 생성 시 항상 새 맵을 생성하며, `src/controller/DungeonState.cpp`에는 `saveToFile()` 호출이 없다.
  - `src/controller/DungeonState.cpp:179-183`은 계단 귀환 때 `changeState(TownState)`로 던전 상태를 파괴한다.
- Expected Basis: 사용자 기준의 저장 계약·진행 무결성. 정확히 “던전 run 재개”를 제품 요구로 할지는 문서 확인이 필요하다.
- Actual: 게임 중단 후 재개 시 파티 HP/퀘스트 등 일부만 저장되고 맵·좌표·FOW는 재개되지 않는다. 이는 README의 설명과 충돌한다.
- Impact: 사용자가 기대하는 체크포인트와 실제 복구가 달라지고, 장거리 탐험·자동이동·던전 목표를 저장할 수 없다. 반대로 run 저장이 비목표라면 README가 과대주장이다.
- Suggested Action: `run persistence` 정책을 명세로 결정한다. 재개를 지원하면 map topology/seed, position, direction, FOW/stepped, depth와 schema migration을 저장하고 중간 이벤트의 atomic checkpoint를 정의한다. 지원하지 않으면 README·성공 기준을 “town hub progress only”로 수정하고 종료/귀환 UX를 명시한다.
- Re-audit Method: 전투 전·전투 후·자동이동 중·계단 귀환 직전 종료 후 재실행하는 시나리오에서 기대한 재개/초기화 정책이 일관되게 관찰되는지 확인한다.
- Confidence: High for documentation/code conflict; Medium for required behavior.
- Owner:
  - Architect / Human
- Notes: Wizardry 비교 기준의 10층을 요구사항으로 승격하지 않았다. 현재 문서 내부의 저장 의미 충돌만 판정한다.

### [A03-F017] TestHarness가 모델 단위만 실행하여 실제 게임 루프의 주요 실패 모드를 잠그지 않는다

- Pass: Debug
- Pattern: `TEST-001`, `DBG-002`
- Area: 통합 테스트·headless 재현·State 호출 경로
- Severity: Major
- Status: Confirmed
- Summary: 테스트 타겟은 Controller/Game을 빌드하지 않고 모델·LocalizationManager만 포함한다. 전투/장비/퀘스트 테스트도 실제 State event flow 대신 모델 메서드와 UI 로직을 수동 모사하므로, 본 감사에서 확인한 Bless 고아 상태, 자동 아이템 소모, STR 장비 우회, 드롭/TPK/보상·재진입 문제를 잡지 못한다.
- Evidence:
  - `CMakeLists.txt:95-110`의 `TEST_SOURCES`는 `test_harness.cpp`와 `LocalizationManager`, 모델 소스만 포함하며 `CombatState.cpp`, `DungeonState.cpp`, `TownState.cpp`, `CharacterInfoState.cpp`, `Game.cpp`가 없다.
  - `src/test_harness.cpp:407-488`의 전투 효과 테스트는 `ConsumableItem::applyEffect()`와 `Character::processTurnEffects()`를 직접 호출하고 CombatState의 `Num2/Num3/nextTurn()`을 호출하지 않는다.
  - `src/test_harness.cpp:494-578`은 양손/클래스 제한을 UI 람다로 모사하며 실제 `CharacterInfoState` event path를 호출하지 않는다.
  - `src/test_harness.cpp:209-270`은 퀘스트에 `addItem()`/`updateQuestCollectProgress()`를 직접 주입하고 실제 상점·전투·보드 flow를 실행하지 않는다.
  - `src/test_harness.cpp:795-847`의 전체 실행 목록에도 State 통합·TPK·드롭·seed replay·전투 타겟 fixture가 없다.
- Expected Basis: 표준 `TEST-001`·`DBG-002` 및 사용자가 요구한 결정적 검증 가능성. broad green smoke만으로 상용 core loop PASS를 선언하지 않는다.
- Actual: `./build/TestHarness --run-all`은 통과했지만 이는 도메인 일부의 pass다. 실제 State 입력, 상태 전이, 보상·저장 timing, controller-only 정책은 실행되지 않는다.
- Impact: 문서의 “플레이어블 검증 완료” 주장이 실제 핵심 루프 증거보다 강하다. 회귀가 발견된 뒤에도 모델 테스트만 녹색으로 남아 출시 판단을 오도할 수 있다.
- Suggested Action: Controller를 직접 테스트하기보다 가능한 핵심 규칙을 headless service로 추출하고, event sequence fixture로 Town→Dungeon→Combat→Town, skill/item target, TPK, reward/drop, save checkpoint를 검증한다. 모든 테스트에 RNG seed와 상태 snapshot/hash를 기록한다.
- Re-audit Method: fresh build에서 headless campaign fixture를 반복 실행하고 상태 전이·전투 로그·inventory/xp/gold/save hash가 기대 baseline과 일치하는지, 각 기존 finding의 실패 fixture가 실제로 red가 되는지 확인한다.
- Confidence: High
- Owner:
  - Architect / Coder
- Notes: `cmake --build` 및 `TestHarness`의 최종 실행 성공은 기록했지만, 실행 범위가 좁다는 이 finding을 상쇄하지 않는다.

## 6. Uncertainties and Clarifications Needed

다음은 현재 증거만으로 결함의 기대 상태를 창작하지 않기 위해 명세 결정을 요청하는 항목이다.

1. **출시 제품 범위:** 단일 20x20 vertical slice인가, 층 진행·최종 목표를 포함한 유료 완결형 던전 RPG인가? `A03-F002`, `A03-F014`, `A03-F016`의 gate가 이 결정에 따라 달라진다.
2. **길드 모집 정책:** 직업·이름·능력치 선택을 제공할 것인지, 무작위 모집을 핵심 규칙으로 유지할 것인지. 무작위라면 역할 중복·재모집·파티 순서와 난이도 예산을 명세해야 한다(`A03-F001`).
3. **반복 퀘스트 정책:** 코볼트/메이스 퀘스트가 일회성인지 반복 가능한지, 반복 가능하면 보상 상한·cooldown·재료 provenance를 어떻게 둘 것인지(`A03-F005`).
4. **던전 run 저장:** 좌표·FOW·맵 seed·depth를 저장해 재개할지, town hub 진행만 저장하고 중간 run은 폐기할지(`A03-F016`).
5. **스켈레톤 감쇄 수치 및 피해 타입:** 문서는 감쇄 존재만 말하고 정확한 비율/피해 타입은 정하지 않는다(`A03-F009`).
6. **난이도 목표선:** depth/party-level별 encounter mix, TPK/win rate, 회복비·레벨업 시간의 허용 범위가 없다(`A03-F014`). 장시간 밸런스 플레이는 수행하지 않았다.

## 7. Perspective Decision

### Verified strengths

- 모델 수준의 20x20 DFS 연결성, visited/walkable BFS 경로 차단, 기본 능력치/AC/레벨업, 상태이상/물약, JSON 파싱 smoke는 통과했다.
- 최대 4인 파티·공용 인벤토리·기본 상점/퀘스트/TPK 상태 전이의 코드 골격은 존재한다.
- 하드코어 TPK 자체는 `spec.md`와 `DESIGN_DECISIONS.md`에 의도된 규칙으로 기록되어 있으므로 레트로 난이도 결함으로 판정하지 않았다. 다만 corrupt save와 TPK reset은 `A03-F015`처럼 분리되어야 한다.

### Gate decision

`HOLD — game-system commercial release candidate not established`.

현재 트리에서 Major 확정 항목은 파티 선택성 부족, 드롭/콘텐츠 단절, 퀘스트 경제 우회, 저장 스키마·복구, 전투 공식/적 다양성, Bless 고아 상태, 지원 행동 자동 소모, 장비 제한 우회, 보상 공식, 통합 테스트 공백이다. 난이도 곡선과 run 저장 정책도 명세가 닫히지 않아 PASS 계열 판정을 내릴 수 없다. 장시간 밸런스와 실제 상호작용이 미검증인 상태에서 모델 단위 테스트의 녹색만으로 상용 출시 후보로 승격할 수 없다.

출시 차단 우선순위:

1. **출시 차단:** `A03-F006`, `A03-F007`, `A03-F010`, `A03-F011`, `A03-F012`, `A03-F015`, `A03-F017` — 저장 계약/복구, 결정성, 전투 핵심 판정, 플레이어 선택, 장비 불변조건, 실제 루프 회귀 검증.
2. **콘텐츠 확장·제품 결정:** `A03-F001`, `A03-F002`, `A03-F003`, `A03-F004`, `A03-F005`, `A03-F013`, `A03-F014`, `A03-F016` — 파티/층/목표/드롭/퀘스트 정책과 경제·난이도 budget을 먼저 확정한 뒤 구현.
3. **재감사 조건:** canonical spec/schema 갱신 후 fixed-seed headless campaign, State event flow, save crash/recovery, loot/quest/economy simulation, 장시간 balance session을 실행하고 각 finding ID에 연결한다.
