# Sub Audit Report

## 1. Audit Metadata

- Audit Turn: 1
- Perspective: UI/UX, accessibility, usability, input discoverability
- User Goal: 유료 데스크톱 Wizardry 스타일 던전 RPG의 출시 후보로서 인터페이스·게임 시스템·콘텐츠가 신뢰 가능한 상용 수준인지 판정하고 개선 방향을 우선순위화한다.
- Audit Basis: Standard-backed / Goal-driven
- Standard Path: /mnt/Projects_SSD/cpp/crawlmaster/AI_AUDIT_DOC_STANDARD.md; /home/eunho1/.codex/skills/multi-audit/references/report-contract.md
- Project: Crawlmaster, C++20/SFML 2.6.x desktop game
- Audit Date: 2026-09-03 (Asia/Seoul)
- Decision framing: 레트로 와이어프레임 미학은 결함으로 보지 않았다. Xbox Accessibility Guidelines(XAG 101/102/107/112/113/115)는 법적 합격선이 아닌 비교용 best-practice로 사용했다.

## 2. Assigned Scope

- Title, Town 허브 및 모든 Town 서브상태(Guild, Shop, Shop Buy/Sell, Temple, Castle)
- Dungeon 화면의 3D 뷰, 오토맵, HUD, 로그, 자동 이동과 키보드/마우스 입력
- Combat 화면, 타겟·행동·스킬 팝업, 아이템 사용과 전투 상태 피드백
- CharacterInfo 화면의 파티 선택, 포커스, 장비/인벤토리 조작 및 저장 피드백
- Settings 화면의 언어/볼륨/복귀 조작, 실시간 재바인딩
- 5개 언어 리소스 및 선택 폰트 경로, 텍스트 크기·대비·레이아웃·창 동작
- 튜토리얼·도움말·오류/경고·취소/확인·TPK 안내·QoL 및 접근성
- 상용 소규모 유료 PC 게임 출시 후보라는 품질 기준. AAA 수준의 그래픽이나 오디오를 요구하지 않았다.

## 3. Excluded and Uninspected Scope

- 다른 audit_report_*.md, docs/audit/**, docs/multi_audit/** 보고서는 읽지 않았다. 이 배정 보고서와 report-contract만 예외다.
- 소스, 테스트, 설정, 제품 문서는 수정하지 않았다. 생성 파일은 이 배정 보고서 하나뿐이다.
- 빌드 생성/벤더 검증, 외부 폰트의 법적 provenance·재배포 허가는 사용자 지시의 build/vendor 제외 범위로 판정하지 않았다. 프로젝트 문서가 해당 게이트를 미해결로 기록한다는 사실만 확인했다.
- Windows/macOS 실기, 고DPI별 동작, 장시간 플레이·입력 피로·실제 패키지 실행은 이 환경에서 검증하지 못했다.
- 캡처 파일은 만들지 않았다. Xvfb 창 기동·창 크기·로그와 정적 코드/문서 증거를 사용했다.
- 스킬 수치, 몬스터 AI, 주사위 규칙의 수학적 균형 자체는 주 배정 범위가 아니다. 다만 사용자에게 보이는 상태·콘텐츠 접근성은 포함했다.

## 4. Evidence Examined

### Project authority and design documents

- spec.md:15-31: 완료 정의, 5개 언어, 언제든 O 설정 진입, 세이브 반영 요구.
- spec.md:40-61: 1024x768 고정 창, MVC/View 책임 및 자산 경로.
- spec.md:93-110: Title/Town/Dungeon/Combat/Settings 상태 흐름.
- spec.md:364-383: 오토맵, CharacterInfo 포커스/Enter, Town/Dungeon/Combat 설정 키와 아이템 조작 계약.
- spec.md:385-432: 18종 아이템·8종 몬스터·3종 기본 퀘스트 콘텐츠 기준.
- spec.md:434-458: Phase 5/6/7/10 완료 주장과 모든 UI 텍스트의 번역 키 요구, CJK 혼합 문자열 게이트.
- designs.md:1-22: 네온 토큰과 폰트 정책.
- designs.md:24-225: Dungeon/Combat/Skill/CharacterInfo/Settings/Shop ASCII 레이아웃과 표시 항목.
- designs.md:228-281: 로그 색상·상태 표시·스킬 취소·언어 전환·상점 FSM·키보드 기본 조작·리사이즈 주의사항.
- IMPLEMENTATION_SUMMARY.md:1-135: 상태/렌더러 파일 책임 및 구현 완료 주장.
- DESIGN_DECISIONS.md:96-112: 설정 키 O, 언어 전환 폰트 수명주기, CJK 혼합 문자열 미해결 게이트.
- BUILD_GUIDE.md:52-81: 자산 출력, 실행 및 실제 CJK 혼합 화면 판독이 hasGlyph만으로 끝나지 않는다는 게이트.
- README.md:1-72: 사용자-facing 기능·세이브·5개 언어·폰트 주장.
- CHANGELOG.md:1-40,190-202: v0.9.4의 폰트 재바인딩 및 남은 provenance/화면 검증 주장.
- audit_roadmap.md:1-79: UI/i18n/로드맵 완료 주장과 후속 범위.

### Source and assets

- src/core/Game.cpp:11-24,39-45,56-91,94-113: 1024x768 non-resizable 창, 이벤트 전달/창 닫기, 언어별 폰트 선택과 Linux 절대 경로 우선 탐색.
- src/controller/TitleState.cpp:18-43,54-103,106-143: Title 입력·3개 메뉴·점멸 안내·정적 로고/크레딧.
- src/controller/TownState.cpp:45-73,75-129,131-210,212-264: Town 서브상태별 입력·설정 접근·길드/상점/교회/성 동작.
- src/controller/TownState.cpp:318-438: 서브상태 텍스트 조립, hardcoded English 라벨/상점 카탈로그, 퀘스트 보드.
- src/controller/TownState.cpp:443-510: 파티/인벤토리 HUD와 일부 동적 번역.
- src/controller/DungeonState.cpp:13-27,69-203,205-290: Dungeon 입력, 오토맵 클릭/BFS, 하드코딩 로그, O/I/C 경로.
- src/view/DungeonRenderer.cpp:123-203,205-299: 미니맵, HUD, static party/control placeholder, 로그 색상/표시 수.
- src/controller/CombatState.cpp:21-149,151-164: Combat 초기 로그, 몬스터 턴 입력 제한, 스킬 팝업, 타겟/행동 키.
- src/controller/CombatState.cpp:382-535: 무기/스킬이 아닌 자동 우선순위 기반 아이템 선택·대상 선택·소모.
- src/controller/CombatState.cpp:545-557,705-749: Run/TPK/보상 로그 및 저장 경로.
- src/controller/CombatState.cpp:756-927: Combat 레이아웃, hardcoded turn/cost/ASCII 라벨, 단일 로그 블록.
- src/controller/CharacterInfoState.cpp:16-94,100-238: 포커스/입력, 화면 레이아웃, 빠진 상태 표시, hardcoded Name/Class 및 고정 텍스트.
- src/controller/CharacterInfoState.cpp:240-426: 소모/장비/해제 성공 후 saveToFile() 부재와 즉시 Enter 동작.
- src/controller/SettingsState.cpp:11-71,77-186: 언어/볼륨/복귀만 있는 설정, 고정 글꼴/대비/스케일/리셋 부재, 정적 English footer.
- src/core/LocalizationManager.cpp:22-85,88-140: 키 fallback, 상대 경로 번역 로드, config 저장.
- include/model/ConcreteItems.hpp:1-226 및 src/model/ItemFactory.cpp:1-110: 아이템 이름/설명이 한국어 고정.
- src/model/MonsterFactory.cpp:1-76 및 include/model/ConcreteSkills.hpp:1-230: 몬스터/스킬 이름·설명이 한국어 고정.
- assets/lang/{ko,en,ja,zh_tw,zh_cn}.json: 각 141개 키가 존재하지만 코드가 모든 UI 문자열을 소비하는지는 별도 검증했다.
- rg --files include/view src/view: 실제 View 파일은 DungeonRenderer뿐이며 문서에 기록된 Town/Combat/UI renderer 파일은 없다.

### Tests and safe runtime

- src/test_harness.cpp:578-627: 일부 번역 조회/config round-trip만 검증.
- src/test_harness.cpp:629-661: Town HUB 9개 키 존재만 검증.
- src/test_harness.cpp:709-793: Town/Combat title의 sf::String 및 hasGlyph 검사. 실제 State draw/input, 레이아웃/overflow, 선택 폰트의 화면 출력은 검증하지 않는다. ASCII codepoint는 775-778에서 제외된다.
- Command: ./TestHarness --run-all (cwd build/audit-commercial) → 전체 도메인 테스트와 glyph/key 검사는 통과했지만 UI 상태/입력/레이아웃 회귀 테스트는 포함되지 않았다.
- Command: timeout 6s xvfb-run -a ./Crawlmaster (cwd build/audit-commercial) → 창 기동 로그 확인: 세이브 로드, XIM 입력 컨텍스트 경고, VSync 미지원, config 부재 기본값. 크래시는 관찰되지 않았다.
- Command: xvfb-run -a ... xwininfo -root -tree → Crawlmaster child window 1024x768 확인.
- Source-derived contrast calculation (sRGB relative luminance): #114411 vs #050B05 = 1.76:1; 오토맵 미방문/안개 바닥 #0A320A vs #050B05 = 1.39:1; 비교로 밝은 네온 #33FF33 = 14.66:1.

## 5. Findings

### [A02-F001] Dungeon 파티 HUD가 실제 파티 상태가 아닌 고정 placeholder를 표시한다

- Area: Dungeon HUD, party status, combat preparation
- Severity: Major
- Status: Confirmed
- Summary: 던전 우측 하단 패널은 실제 Party를 받지 않고 WARRIOR/CLERIC/ROGUE/MAGE와 고정 HP를 항상 그린다. 파티원이 0명 또는 1명이어도 4명인 것처럼 보여 핵심 의사결정 정보를 거짓으로 만든다.
- Evidence: DungeonRenderer::render()는 map/log만 받는다 (src/view/DungeonRenderer.cpp:10-16). drawHUDFrame()는 [PARTY STATUS]와 1. WARRIOR HP:18/18 등 고정 문자열을 직접 그린다 (src/view/DungeonRenderer.cpp:241-267). 실제 Dungeon draw 경로도 renderer에 map/log만 전달한다 (src/controller/DungeonState.cpp:269-272). 감사 실행 cwd의 build/audit-commercial/save.json은 members: []인데도 해당 placeholder는 코드상 제거되지 않는다.
- Expected Basis: designs.md:24-60의 실제 파티 상태 패널, spec.md:17-31의 플레이 가능/상태 피드백 목표, XAG 102 Contrast의 HUD 중요 정보 가시성(https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/102).
- Actual: 현재 멤버 수, 이름, HP, 사망/상태이상, 주문 슬롯, 골드가 Dungeon HUD에 반영되지 않는다. 동일 패널에 정적 [CONTROLS]가 섞여 계층도 깨진다.
- Impact: 플레이어가 회복·도망·전투 진입 여부를 잘못 판단한다. 특히 상태이상/사망 파티원을 확인할 수 없고, 출시 후보의 신뢰 가능한 상태 표시 조건을 충족하지 못한다.
- Suggested Action: Party 또는 immutable HUD snapshot을 renderer 계약에 전달하고, 0~4명 동적 행(이름/직업/HP/상태/슬롯)을 표시한다. 빈 슬롯은 숨기고, 컨트롤 안내는 별도 영역으로 분리한다. 실제 상태 fixture로 0/1/4명, 저HP/사망/독/마비를 렌더링하는 회귀 검사를 추가한다.
- Re-audit Method: 저장 데이터와 같은 Party fixture를 Town→Dungeon으로 연결한 뒤 각 파티 수/상태에서 표시 문자열과 경계 박스가 일치하는지 headless draw probe 또는 일회성 시각 검증으로 확인한다.
- Confidence: High
- Owner: Coder / UI architect
- Notes: 레트로 스타일 자체가 문제가 아니라, 표시 데이터가 도메인 상태와 단절된 것이 문제다.

### [A02-F002] 5개 언어 UI 계약이 화면·로그·아이템 콘텐츠에서 깨진다

- Area: i18n, content localization, runtime language switch
- Severity: Major
- Status: Confirmed
- Summary: JSON 키셋은 5개 언어에 존재하지만 실제 모든 사용자-facing 문자열이 키를 통하지 않는다. Town/Combat/Dungeon/CharacterInfo에 한국어 또는 영어가 하드코딩되고 아이템·몬스터·스킬 이름/설명은 모델에서 한국어만 반환한다.
- Evidence: Town 서브상태는 Guild Desk, Shop Menu, Shop Catalog (Buy), 영어 상품명/가격, Temple Sanctuary를 직접 조립한다 (src/controller/TownState.cpp:339-394). CharacterInfo는 Name:/Class:/Lv.를 직접 그린다 (src/controller/CharacterInfoState.cpp:130-147). Combat은 초기/승패/공격 로그와 Turn, Slots, Slot 1, No Cost, All enemies defeated, Monsters engaged를 직접 생성한다 (src/controller/CombatState.cpp:31-32,545-557,841-855,892-925). Dungeon 이동/자동이동 로그도 English literal이다 (src/controller/DungeonState.cpp:87-101,114-185,244-265). ItemFactory.cpp:10-88, MonsterFactory.cpp:9-56, ConcreteSkills.hpp:15-226의 표시 이름/설명은 한국어 고정이다.
- Expected Basis: spec.md:444-449는 모든 UI 텍스트를 LocalizationManager 키로 얻도록 요구한다. designs.md:242-267은 로그/설정/상태 텍스트를 화면에 표시하고 spec.md:30-31은 5개 언어 실시간 전환을 완료 정의로 둔다. XAG 101 Text display는 HUD·instructional cue·inventory text도 접근 가능해야 한다는 비교 기준을 제공한다 (https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/101).
- Actual: 제목 일부와 일부 메뉴 키만 현재 언어로 바뀐다. 언어 전환 후 이미 m_logQueue/m_battleLog에 저장된 이전 언어 로그도 그대로 남는다. assets/lang의 키 존재 테스트는 이 문자열 소비 경로를 검증하지 않는다.
- Impact: 영어/일본어/중국어 사용자가 핵심 규칙·아이템 효과·전투 결과·오류 원인을 이해하지 못한다. 언어별 글자 폭 차이로 fixed layout overflow도 함께 발생한다. README/Phase 완료 주장이 실제 shipped experience보다 강하다.
- Suggested Action: item/monster/skill ID와 모든 상태/로그를 locale key + placeholder/plural 규칙으로 전환한다. 사용자-facing literal 정적 검사와 상태별 5-locale text snapshot을 추가하고, 언어 전환 시 기존 로그의 처리 정책(재번역 또는 세션 언어 고정)을 명세로 결정한다. 고유명사만 예외로 명시한다.
- Re-audit Method: 각 State를 5개 언어로 생성하여 메뉴·HUD·로그·팝업·오류를 모두 순회하고 raw key/한국어 잔존/영어 잔존을 검사한다. CJK 폭·줄바꿈도 함께 확인한다.
- Confidence: High
- Owner: Coder / Localization architect
- Notes: JSON 키 개수 141개 및 기존 hasGlyph PASS는 실제 번역 완전성의 반증이 아니다.

### [A02-F003] Settings O 진입이 “언제든” 동작하지 않는다

- Area: settings access path, pause/navigation
- Severity: Major
- Status: Confirmed
- Summary: Settings는 HUB/Dungeon 및 Combat의 아군 턴에서만 O로 열리고, Town 서브상태·Combat 적 턴에서는 접근할 수 없다. Title에서도 O 직접 입력은 처리되지 않는다.
- Evidence: Town O 처리는 HUB 분기 안에만 있다 (src/controller/TownState.cpp:45-73); Guild/Shop/Shop Buy/Sell/Temple/Castle 분기에는 O가 없다 (src/controller/TownState.cpp:75-264). Dungeon은 O를 처리한다 (src/controller/DungeonState.cpp:189-197). Combat은 m_turnOrder가 비어 있거나 현재 주체가 몬스터이면 먼저 return한다 (src/controller/CombatState.cpp:45-59), 따라서 적 턴에는 O가 도달하지 않는다. Title은 Up/Down/Enter만 처리한다 (src/controller/TitleState.cpp:18-43).
- Expected Basis: spec.md:30-31,373-375, designs.md:264-267의 Title/플레이 중 설정 접근과 O 계약. XAG 112 UI navigation은 초기 진입부터 설정 경로가 완전히 접근 가능하고 화면 간 상호작용 방식이 일관되어야 한다고 제시한다 (https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/112).
- Actual: 플레이어가 Shop Buy, Temple, Castle 또는 적의 공격 직후에는 설정을 조정하려면 먼저 뒤로 나가거나 턴을 기다려야 한다. 설정이 pause state라는 명세와 모든 화면 접근 요구가 일치하지 않는다.
- Impact: 글꼴/언어/볼륨을 조정해야 하는 플레이어가 일부 핵심 상태에서 막힌다. 전투 중 적 턴에서는 입력이 무시되어 “반응 없음”처럼 보인다.
- Suggested Action: 공통 pause/settings command router를 모든 non-terminal state에서 최우선 처리하고, 스킬 선택·적 턴에도 설정을 열 수 있는지 명세로 고정한다. Title에는 현재 메뉴 외에 O 또는 명시된 Settings entry를 제공하고 모든 화면에 back path를 표시한다.
- Re-audit Method: 모든 상태/서브상태와 Combat actor/enemy turn, Skill overlay에서 synthetic KeyPressed(O)를 보내 Settings push 및 복귀를 검증한다. 자동 이동/스킬 선택 중 pause가 안전하게 정지되는지도 확인한다.
- Confidence: High
- Owner: Coder / State-machine architect
- Notes: SettingsState 자체의 언어/볼륨 조절은 동작하나 진입 coverage가 불완전하다.

### [A02-F004] 화면에 표시된 확인·뒤로가기 입력과 실제 핸들러가 불일치한다

- Area: menu focus, input discoverability, cancel/confirm
- Severity: Major
- Status: Confirmed
- Summary: 일부 화면은 Enter/숫자 선택·뒤로가기를 안내하지만 해당 키가 무시된다. Town은 숫자 즉시 실행, Settings/CharacterInfo는 focus navigation, Combat은 숫자/좌우가 섞여 공통 조작 모델이 없다.
- Evidence: Guild 화면은 “ESC. ”를 붙여 GUILD_BACK(번역 값 자체는 3.)을 표시하지만 handler는 Escape만 처리하고 Num3은 무시한다 (src/controller/TownState.cpp:104-119,339-345). Temple도 TEMPLE_BACK을 ESC.와 함께 그리지만 handler는 Escape만 처리한다 (src/controller/TownState.cpp:190-210,389-394). Combat target 안내는 Enter: Confirm, ESC: Cancel인데 handler에는 target confirm/ESC main-menu 처리가 없고 좌우와 Num1~4만 실행한다 (src/controller/CombatState.cpp:61-149,805-819). Skill 안내도 Enter confirm을 말하지만 실제 선택은 Num1~9/ESC뿐이다 (src/controller/CombatState.cpp:61-120,846-858). COMBAT_SELECT_ITEM 키는 선언되어 있으나 실제 선택 화면 경로가 없다.
- Expected Basis: designs.md:24-225,259-262,269-278, spec.md:373-383의 화면별 포커스/Enter/ESC 계약. XAG 112 UI navigation 및 113 UI focus handling은 일관된 navigation과 항상 식별 가능한 focus를 요구하는 비교 기준이다 (https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/112, https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/113).
- Actual: 표시된 키를 눌러도 무반응하거나, 사용자가 보지 못한 숫자/좌우 조합을 추론해야 한다. Town/Combat에는 선택된 행동을 나타내는 focus state가 없고, CharacterInfo의 frame 색상/“> ”만으로 구분된다.
- Impact: 신규 플레이어와 키보드만 사용하는 플레이어가 진행 경로를 잃는다. 특히 Combat에서 Enter를 믿고 눌러도 행동이 확정되지 않아 턴 손실/혼란이 생긴다.
- Suggested Action: 화면 공통 navigation contract(Up/Down focus, Enter confirm, ESC cancel/back)를 정의하고, 숫자 단축키는 보조 shortcut으로만 둔다. 실제 처리하지 않는 키는 안내에서 제거하거나 handler를 구현한다. focus는 고대비 배경/outline/텍스트 변화로 표시한다.
- Re-audit Method: 각 화면의 안내 문자열을 기준으로 key matrix를 작성하고 모든 advertised key가 하나의 상태 변화/명확한 오류 피드백을 만드는지 자동 검사한다. focus가 화면 안에 계속 보이는지 5개 언어에서 확인한다.
- Confidence: High
- Owner: Coder / UX architect
- Notes: 키보드 우선 설계는 유지할 수 있지만 화면 간 의미를 일관되게 닫아야 한다.

### [A02-F005] Combat Item 명령이 플레이어 선택 없이 임의 아이템·대상을 자동 소비한다

- Area: combat QoL, item targeting, error prevention
- Severity: Major
- Status: Confirmed
- Summary: Combat에서 Num3을 누르면 아이템 목록이나 대상 선택 없이 cure→greater heal→heal→mana→buff 우선순위로 첫 항목을 찾아 자동 적용하고 바로 제거한다. COMBAT_SELECT_ITEM/설계상 Item 선택 경로는 소비되지 않는다.
- Evidence: performUseItem()은 scr_cure, pot_greater_heal, pot_heal, pot_mana, buff를 순서대로 탐색하고 HP/슬롯 상태로 대상을 고른다 (src/controller/CombatState.cpp:382-503). 선택 실패 시 첫 소모품을 actor에게 강제 적용하고, 성공 후 party.removeItem(itemIdx)를 호출한다 (src/controller/CombatState.cpp:502-535). Combat handler는 Num3을 곧바로 이 함수에 연결한다 (src/controller/CombatState.cpp:139-147). assets/lang/*의 COMBAT_SELECT_ITEM 키는 있지만 코드에서 사용되지 않는다.
- Expected Basis: designs.md:64-86,259-262의 Party Action Item 및 스킬 선택 overlay 패턴, spec.md:29-31,373-383의 아이템 효과·대상 사용. XAG 115 Error messages and destructive actions은 판매/저장/영구 변경 전에 review·confirm·undo 기회를 제시하는 비교 기준이다 (https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/115).
- Actual: 플레이어가 어느 물약을 누구에게 쓸지 결정할 수 없고, 해독 스크롤은 첫 독 캐릭터에, 치유 물약은 가장 낮은 비율 캐릭터에 자동 사용된다. 아무 효과가 없는 첫 소모품을 소비할 수도 있다.
- Impact: 희귀 아이템 오사용·소모, 잘못된 파티원 치료, 전략성 저하가 발생한다. 턴제 전투의 핵심 선택과 사용자 신뢰를 훼손한다.
- Suggested Action: Item overlay(아이템명/수량/효과/대상)를 만들고, item→ally/foe target→preview→Enter confirm→ESC cancel 순서로 분리한다. 유효 대상/효과가 없으면 소비하지 말고 원인과 대안을 표시한다. 스택 수량과 현재 선택을 명확히 표시한다.
- Re-audit Method: 여러 물약/여러 HP 상태/독·마비 상태/죽은 멤버 fixture에서 Num3 진입 후 선택·취소·확정·무효 입력을 반복하고 inventory와 로그가 정확히 변하는지 확인한다.
- Confidence: High
- Owner: Coder / Combat UX owner
- Notes: 자동화는 선택 가능한 QoL shortcut으로 추가할 수 있으나 기본 동작이 되어서는 안 된다.

### [A02-F006] CharacterInfo와 Combat의 상태 변경이 일관되게 save.json에 기록되지 않는다

- Area: inventory/equipment persistence, save feedback
- Severity: Major
- Status: Confirmed
- Summary: CharacterInfo에서 장비 장착·해제와 소모품 사용으로 메모리 상태를 바꾸지만 성공 경로에 party.saveToFile()이 없다. Combat Item도 소모 후 저장하지 않는다. 따라서 창 닫기/재시작/던전→Town 재생성 시 마지막 변경이 사라질 수 있다.
- Evidence: CharacterInfo useSelectedConsumable()은 효과 적용·remove만 수행한다 (src/controller/CharacterInfoState.cpp:240-310). equipSelectedItem()과 unequipSelectedSlot()도 inventory/equipment만 변경하고 저장 호출이 없다 (src/controller/CharacterInfoState.cpp:312-426). Combat Item은 remove 후 곧바로 nextTurn()한다 (src/controller/CombatState.cpp:518-535). 반면 Town 구매/판매/길드/교회는 명시적으로 save한다 (src/controller/TownState.cpp:87-89,109-114,145-180,197-202). spec.md:29, designs.md:269-274는 아이템/상태 변경의 세이브 반영을 완료 기준으로 둔다.
- Expected Basis: spec.md:21,29-31, designs.md:269-274, BUILD_GUIDE.md:63-72의 영속 데이터 기대. 저장 실패도 사용자에게 알려야 한다.
- Actual: UI는 성공 메시지를 표시하지만 디스크 write 여부를 확인하지 않고, Game::processEvents()의 Closed 이벤트는 즉시 창만 닫는다 (src/core/Game.cpp:56-64). Settings도 saveConfig() 반환값을 확인하지 않는다 (src/controller/SettingsState.cpp:21-26,60-64).
- Impact: 플레이어가 장시간 관리한 장비·치료·소모품을 잃을 수 있다. 성공 toast와 실제 save 상태가 달라 신뢰 가능한 출시 후보 기준을 충족하지 못한다.
- Suggested Action: 도메인 mutation을 성공적으로 commit하는 공용 command/save 경계로 모으고, CharacterInfo/Combat의 모든 성공 변경 후 atomic save와 오류 표시를 수행한다. 창 닫기/비정상 종료에 대한 autosave 또는 dirty-confirm 정책을 명세화한다.
- Re-audit Method: CharacterInfo/Combat에서 mutation 후 파일 bytes를 확인하고, state pop, Town/Dungeon transition, 프로세스 재실행 후 reload한다. save 실패 fixture에서 사용자-visible error와 변경 rollback을 확인한다.
- Confidence: High
- Owner: Coder / Persistence architect
- Notes: 메모리에서 즉시 부모 상태에 보이는 것은 디스크 영속성의 대체 증거가 아니다.

### [A02-F007] 고정 소형 텍스트와 저대비 토큰이 접근성·CJK 레이아웃을 차단한다

- Area: text display, contrast, scaling, responsive/fixed window behavior
- Severity: Major
- Status: Confirmed
- Summary: 고정 1024x768 창에서 10~16 크기 텍스트를 사용하고 text scale/high contrast/대체 sans-serif option을 제공하지 않는다. 비선택 text와 안개 바닥은 배경 대비가 매우 낮으며, 긴 locale 문자열을 wrap/clip/scroll하지 않는다.
- Evidence: 창은 sf::Style::Titlebar | sf::Style::Close로 1024x768 생성되고 resize handler가 없다 (src/core/Game.cpp:11-15,56-71). Settings 항목은 언어·BGM·SFX·Back뿐이며 scale/high contrast/reset이 없다 (src/controller/SettingsState.cpp:77-158). CharacterInfo 10~12, Settings 11~16, Dungeon log 12, Combat log 12~14를 고정한다 (src/controller/CharacterInfoState.cpp:108-237, src/controller/SettingsState.cpp:86-157, src/view/DungeonRenderer.cpp:270-299, src/controller/CombatState.cpp:756-878). #114411은 #050B05에 1.76:1, 안개 바닥 #0A320A는 #050B05에 1.39:1로 계산됐다. CHAR_INFO_GUIDE_BAR는 en 111자, CJK도 68~79자이며 drawText()에는 폭 제한/줄바꿈/clip이 없다 (src/controller/CharacterInfoState.cpp:437-455, src/controller/SettingsState.cpp:169-186).
- Expected Basis: spec.md:40-43의 고정 해상도는 미학/기본 캔버스 결정으로 존중하되, designs.md:14-22,276-281의 텍스트 토큰과 반응형/리사이즈 주의사항을 실제로 닫아야 한다. XAG 101 Text display는 PC 기본 text size·200% scaling·CJK 40 glyph line width를, XAG 102 Contrast는 중요 text 4.5:1·inactive text 3:1을 비교 기준으로 제시한다 (https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/101, https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/102).
- Actual: 현재 비선택 설정/프레임/상세 가격 text는 3:1에도 미달한다. 미니맵의 미방문/안개 바닥은 핵심 탐색 정보지만 1.39:1이다. 창을 키우거나 DPI를 높일 때 UI가 재배치·확대되지 않고, CJK/장문은 패널 밖으로 나갈 수 있다.
- Impact: 저시력·색각 이상 사용자에게 focus/미니맵/비활성 정보를 식별하기 어렵고, 언어별 문자열이 잘리거나 겹친다. fixed retro layout이 곧 fixed unreadability를 의미하지 않도록 별도 접근성 설계가 필요하다.
- Suggested Action: 기본/고대비 팔레트와 비색상 focus(밝은 배경/outline)를 제공하고 모든 중요/inactive text를 측정 가능한 대비로 재토큰화한다. text/UI scale 최대 200%와 한 방향 scroll/tooltip을 제공한다. locale-aware wrap/ellipsis/scroll container를 도입하고, 고정 내부 캔버스를 letterbox scale하는 창 모드 또는 명시적 고정 모드 선택을 명세화한다.
- Re-audit Method: 100/150/200% text scale, 1024x768 및 고DPI/확대 창에서 모든 화면의 content loss/overflow를 검사하고 대비 표를 자동 계산한다. 5개 언어의 가장 긴 문자열을 실제 font geometry로 측정한다.
- Confidence: High
- Owner: UI architect / Accessibility owner
- Notes: XAG 수치는 법적 판정이 아니라 사용자 지시의 best-practice 비교 기준이다. 1024x768 자체는 spec 결정이므로, 해결은 스타일 폐기가 아닌 scale/letterbox/contrast 추가다.

### [A02-F008] 상태이상·버프·경고가 전용 상태 피드백으로 노출되지 않는다

- Area: status visibility, combat feedback, error messages
- Severity: Major
- Status: Confirmed
- Summary: 설계가 요구한 CharacterInfo 상태 줄(OK/POISONED/PARALYZED)과 STR/DEX 버프 표시가 구현되지 않고, Combat도 HP/Slots 외 상태를 표시하지 않는다. 중요 로그는 단일 평문 블록으로만 쌓이며 상태별 시각 채널이 실제 메시지와 연결되지 않는다.
- Evidence: CharacterInfo draw는 Name/Class/HP/XP/AC/Spells/abilities/equipment만 그린다 (src/controller/CharacterInfoState.cpp:127-180); getPoisonTurns, isParalyzed, combat buff를 draw하는 코드가 없다. Combat action panel은 actor HP/Slots만 표시한다 (src/controller/CombatState.cpp:841-844); 독/마비는 로그에만 들어간다 (src/controller/CombatState.cpp:255-296,650-675). designs.md:246-257는 상태/버프를 별도로 요구한다. Dungeon 로그 색상 분기는 > Wall과 > Stairs 영어 prefix만 인식한다 (src/view/DungeonRenderer.cpp:288-295), 실제 계단 메시지는 localized MSG_DUNGEON_ESC로 생성된다 (src/controller/DungeonState.cpp:282-289). Combat 로그 전체는 하나의 m_logText에 동일 amber 색상으로 출력된다 (src/controller/CombatState.cpp:870-878).
- Expected Basis: designs.md:242-257, spec.md:29-31,373-383의 상태/오류/아이템 효과 피드백. XAG 102 Contrast는 HUD·targeting·map 등 중요 시각 정보의 식별성을, XAG 115는 경고를 다른 텍스트와 시각적으로 구별할 것을 비교 기준으로 제시한다 (https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/102, https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/115).
- Actual: 플레이어는 캐릭터 시트나 현재 턴 패널만 보고 행동 불가/독 지속 턴/버프 남은 턴을 알 수 없다. 로그가 빠르게 7~8줄로 교체되고, 색상은 실제 localized/한국어 메시지와 맞지 않는다.
- Impact: 상태 효과의 규칙을 이해하지 못해 잘못된 행동을 선택하고, 위험 경고·회복 결과를 놓친다. 색상만으로 구별하는 체계도 색각/저시력 접근성이 낮다.
- Suggested Action: HP 옆 persistent status chips/text(남은 턴/버프 수치)를 CharacterInfo와 Combat party panel에 표시한다. 로그를 severity/category 구조로 저장하여 damage/status/reward/error를 별도 색·아이콘·접두사로 표시하고, 중요한 TPK/저장 실패/상태 경고는 transient line이 아닌 modal/banner로 유지한다. 색상 외 텍스트·아이콘을 함께 사용한다.
- Re-audit Method: 독/마비/STR·DEX 버프/회복/저장 실패/벽 충돌/계단 도달 fixture를 각 화면에서 실행하여 상태가 HUD·로그·오류 박스에 동시에 일관되게 반영되는지 확인한다.
- Confidence: High
- Owner: Coder / UX owner
- Notes: 레트로 로그는 유지 가능하지만 핵심 상태를 로그에만 의존해서는 안 된다.

### [A02-F009] 판매·해고·소모·설정 이탈과 TPK reset에 review/confirm/undo가 없다

- Area: destructive actions, error recovery, data safety
- Severity: Major
- Status: Confirmed
- Summary: 구매/판매·마지막 파티원 해고·장비 해제·소모품 사용이 Enter/숫자 한 번으로 즉시 실행된다. TPK는 저장을 reset한 뒤 사후 로그만 남기며, Settings dirty state나 창 닫기 확인도 없다.
- Evidence: Guild Num2가 마지막 멤버를 바로 삭제한다 (src/controller/TownState.cpp:104-114). Shop Sell 숫자키가 판매가/삭제를 즉시 수행한다 (src/controller/TownState.cpp:163-185). CharacterInfo Enter는 equip/unequip/use를 즉시 호출한다 (src/controller/CharacterInfoState.cpp:75-87). TPK nextTurn()은 로그 후 resetToDefault()와 Title 교체를 수행한다 (src/controller/CombatState.cpp:225-236). Game Closed 이벤트는 확인/저장 없이 close한다 (src/core/Game.cpp:56-64). Settings는 변경을 dirty로 추적하거나 reset/unsaved prompt를 제공하지 않는다 (src/controller/SettingsState.cpp:17-71,77-158).
- Expected Basis: 사용자 목표의 취소/확인·파괴적 TPK 안내 요구, designs.md:259-274의 Enter/ESC 계층, spec.md:12,29-31의 영속 저장/하드코어 정책. XAG 115 Error messages and destructive actions는 영구 데이터 변경 전 review/confirm/undo와 오류·경고의 명확한 구별을 제시한다 (https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/115).
- Actual: 판매할 희귀 장비인지, 해고할 멤버가 맞는지, 버프/해독/치유 물약을 지금 쓸지 검토할 기회가 없다. TPK의 리셋이 제품 의도라 하더라도, 플레이어가 시작 전에 하드코어 위험을 확인하는 별도 UI가 없다.
- Impact: 오입력으로 진행·아이템을 되돌릴 수 없고, 접근성 입력 장치 사용자의 accidental activation 위험이 커진다. 설정 변경은 창 닫기로 조용히 사라질 수 있다.
- Suggested Action: 판매/해고/소모/세이브 덮어쓰기마다 대상·효과·잔여 수량을 보여주는 confirm/cancel dialog를 추가하고, 가능하면 최근 행동 undo/복구를 제공한다. TPK는 신규 게임/첫 던전 진입 시 하드코어 경고와 설정 확인을 제공하며, Settings는 Apply/Cancel/Reset 및 unsaved-changes prompt를 둔다. 모든 경고는 고대비 modal/banner와 텍스트·아이콘으로 표시한다.
- Re-audit Method: 잘못된 번호/Enter, 창 닫기, ESC, 각 destructive action 직전·취소·확정·undo를 synthetic input으로 검사하고 save bytes/party state의 before-after를 비교한다.
- Confidence: High
- Owner: Product owner / Coder / UX owner
- Notes: TPK reset 자체를 제거하라는 finding이 아니다. 동결된 하드코어 의도와 사용자가 위험을 알고 확인하는 경로를 분리해야 한다.

### [A02-F010] 타이틀 온보딩과 길드 캐릭터 생성이 상용 수준의 선택·저장 흐름을 제공하지 않는다

- Area: onboarding, character creation, title/load UX
- Severity: Major
- Status: Confirmed
- Summary: Title에는 New Game/Settings/Exit 3개만 있고 Load Game 키는 그려지지 않는다. New Game 선택은 TownState 생성 중 기존 save를 자동 로드한다. Guild의 영웅 생성은 이름/직업 선택 없이 고정 이름과 rand()%4 직업을 부여하며, 해고도 마지막 멤버만 삭제한다.
- Evidence: Title 메뉴는 세 인덱스만 순환하고 TITLE_NEW_GAME, TITLE_SETTINGS, TITLE_EXIT만 그린다 (src/controller/TitleState.cpp:18-43,65-91); TITLE_LOAD_GAME/TITLE_SUBTITLE은 사용되지 않는다. TownState constructor는 진입 즉시 loadFromFile()한다 (src/controller/TownState.cpp:28-37). Guild는 rand()%4와 RANDOM_NAMES를 사용한다 (src/controller/TownState.cpp:75-101), disband는 마지막 index만 제거한다 (src/controller/TownState.cpp:104-114).
- Expected Basis: spec.md:23-31,93-100의 신규 게임/세이브 로드·4명 파티 완료 정의, designs.md:3.4의 CharacterInfo 및 Guild UI 흐름, README의 Title→Town 사용자 경험. 신규 이름/직업 선택 여부가 문서에 명확히 고정되어 있지 않으므로 선택 요구 자체는 확인이 필요하다.
- Actual: 기존 저장이 있는데도 New Game이 새 게임인지 이어하기인지 구분되지 않는다. 첫 플레이어는 파티 구성/역할을 이해하거나 계획할 수 없고, Guild 화면에서 특정 멤버를 선택해 해고할 수 없다.
- Impact: 첫 10분의 핵심 온보딩과 세이브 안전성이 불투명해 이탈·오입력 위험이 크다. Wizardry 스타일의 파티 편성 전략과 4명 생성 완료 기준을 UI가 제대로 전달하지 못한다.
- Suggested Action: Title을 New Game(명시적 save reset/confirm), Load Game(save metadata), Settings, Exit로 분리하거나 현재 의도를 문서로 재정의한다. Guild에 이름 입력/직업·능력 미리보기/확인/취소, 멤버 선택·해고 확인을 추가한다. 랜덤 모집을 유지한다면 랜덤임을 명시하고 reroll/preview 규칙을 제공한다.
- Re-audit Method: 빈 save/기존 save/손상 save에서 Title 경로를 각각 실행하고, New/Load가 기대한 state와 bytes를 만드는지 확인한다. Guild에서 0~4명 생성, 특정 멤버 선택/취소/해고를 순회한다.
- Confidence: High for current behavior; Medium for product expectation
- Owner: Product owner / Coder / UX owner
- Notes: 선택권이 반드시 요구되는지는 Needs Spec Clarification 항목으로도 추적한다. 현재 New Game과 자동 load의 의미 충돌은 확정된 사용성 결함이다.

### [A02-F011] 완료된 UI/i18n 주장에 비해 결정적 UI 회귀 검증이 없다

- Area: runtime evidence, UI regression, release gate
- Severity: Major
- Status: Confirmed
- Summary: --run-all은 도메인/파일/key/glyph를 통과하지만 State 생성·draw·input·레이아웃·overflow·focus·실제 locale 화면을 검증하지 않는다. 따라서 현재의 고정 HUD, 키 불일치, untranslated literal, 대비/래핑 문제를 green test가 잡지 못한다.
- Evidence: testLocalizationI18n()은 일부 key와 config round-trip만 검사한다 (src/test_harness.cpp:578-627). HUB coverage는 9개 key 존재만 검사한다 (src/test_harness.cpp:629-661). UI safety test는 title sf::String과 codepoint hasGlyph를 확인하고 ASCII는 의도적으로 제외한다 (src/test_harness.cpp:709-793). State 클래스나 sf::RenderWindow에 synthetic event를 보내는 테스트가 없다. BUILD_GUIDE.md:76-81도 CJK 혼합 화면은 실제 판독이 필요하고 hasGlyph만으로 release 판단하지 말라고 명시한다.
- Expected Basis: AI Audit Standard TEST-001/BUILD-001, spec.md:23-31,444-449, designs.md:276-281의 플레이 가능·i18n·화면 계약. XAG 112와 113의 전체 경로/포커스 비교 기준(https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/112, https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/113).
- Actual: 실행 가능한 증거는 Xvfb에서 1024x768 창이 기동한 것과 startup 로그뿐이다. 실제 Windows/macOS, 5-locale 상태 순회, input matrix, text geometry, CJK 혼합 raster/overflow는 미검증이다.
- Impact: 녹색 집계 테스트를 상용 UI 신뢰성으로 오해하게 만들며, release gate의 evidence gap이다. 이 finding이 해소되지 않으면 UI 관점 PASS를 주장할 수 없다.
- Suggested Action: deterministic UI smoke harness를 추가해 모든 State/substate/turn/overlay를 instantiate하고 synthetic KeyPressed/MouseUp를 순회한다. 각 화면의 text bounds가 panel 안에 있고 focus/confirm/cancel/state output이 계약과 일치하는지 검사한다. 5개 locale의 representative strings를 실제 선택 font로 raster/geometry 검증하고 Linux Xvfb 및 Windows/macOS 실기 gate를 별도 기록한다.
- Re-audit Method: 빈/기존 save fixture와 5개 언어로 Title→Town→Guild/Shop/Temple/Castle→Dungeon→Combat→CharacterInfo→Settings를 모두 순회하고, test output과 실제 창 geometry/시각 검토 결과를 manifest에 연결한다.
- Confidence: High
- Owner: QA / Coder / Release owner
- Notes: 이번 감사는 캡처 파일을 만들지 않았으므로 CJK 실제 판독을 PASS로 올리지 않았다.

### [A02-F012] UI 책임 문서와 실제 View 구조가 드리프트되어 인수인계 경계가 불명확하다

- Area: implementation compliance, UI architecture/documentation
- Severity: Minor
- Status: Confirmed
- Summary: spec.md와 IMPLEMENTATION_SUMMARY.md는 TownRenderer/CombatRenderer/UIRenderer를 View 책임으로 기록하지만 실제 tree에는 DungeonRenderer만 있고 Town/Combat/CharacterInfo/Settings가 각 Controller에서 직접 draw한다.
- Evidence: spec.md:52-55는 DungeonRenderer, TownRenderer, CombatRenderer, UIRenderer를 선언한다. IMPLEMENTATION_SUMMARY.md:48-63은 존재하지 않는 include/view/UIRenderer.hpp/.cpp 파일 링크와 책임을 기록한다. rg --files include/view src/view 결과는 DungeonRenderer.hpp/.cpp뿐이고, TownState.cpp, CombatState.cpp, CharacterInfoState.cpp, SettingsState.cpp가 직접 sf::Text/box를 그린다.
- Expected Basis: AI Audit Standard IMP-004 및 DOC-BACKFILL-001; 문서가 실제 책임·검증 경계를 설명해야 한다.
- Actual: 신규 작업자는 존재하지 않는 renderer를 찾거나 Controller에 퍼진 UI 계약을 놓칠 수 있다. 이는 직접적인 화면 결함은 아니지만, 앞 findings의 i18n/layout/save 책임을 안정적으로 고치기 어렵게 한다.
- Impact: 유지보수/재감사 coverage가 약해지고, 동일한 색상·폰트·입력·wrap 정책이 화면별로 복제되어 drift가 반복된다.
- Suggested Action: 실제 구조를 문서에 복구해 Controller-owned view임을 명시하거나, 공통 UIRenderer/layout/token/input component를 실제로 추출한다. 책임 파일·상태별 smoke test를 문서에 연결한다.
- Re-audit Method: 문서 파일 책임표와 rg --files/CMake source list/실제 call graph를 다시 대조하고, UI 공통 정책의 단일 source of truth가 존재하는지 확인한다.
- Confidence: High
- Owner: Architect / Documentation owner
- Notes: 다른 감사 보고서의 결론을 사용하지 않고 현재 tree만으로 확인했다.

### [A02-F013] 명세에 있는 퀘스트 콘텐츠가 퀘스트 보드 UI에 연결되지 않는다

- Area: content discoverability, quest board, feature completeness
- Severity: Major
- Status: Confirmed
- Summary: 명세의 기본 퀘스트 3종 중 qst_hunt_spiders는 Quest 모델/퀘스트 보드 UI/수락 경로 어디에도 연결되지 않는다. 몬스터 데이터가 존재하더라도 플레이어가 이 콘텐츠를 발견하거나 수락할 수 없다.
- Evidence: spec.md:427-432는 qst_clear_kobolds, qst_collect_maces, qst_hunt_spiders 3종을 기본 목록으로 선언한다. Town 입력은 qst_clear_kobolds와 qst_collect_maces만 처리한다 (src/controller/TownState.cpp:212-264). Castle 화면 조립도 두 상태만 렌더링한다 (src/controller/TownState.cpp:396-433). 현재 소스에서 qst_hunt_spiders를 검색하면 spec.md 외 호출/표시 경로가 없고, mon_giant_spider 자체는 src/model/MonsterFactory.cpp:23-29에만 등록되어 있다.
- Expected Basis: spec.md:385-432의 Content Data 기준과 designs.md의 Castle quest-board layout. 구현된 콘텐츠는 사용자에게 노출·수락·진행·보상까지 연결되거나, 후속 Phase로 명시적으로 이관되어야 한다(표준 IMP-001/IMP-002).
- Actual: 성주실에는 코볼트/메이스 카드만 보이고, 거미 사냥의 목표·보상·진행도·보고 동작이 없다. UI가 제공하는 콘텐츠 목록과 명세 데이터가 다르다.
- Impact: 명세상 3종 중 1종이 플레이 불가하여 콘텐츠 밀도와 재플레이 목표가 줄고, 플레이어는 해당 몬스터/퀘스트가 존재하는지 알 수 없다.
- Suggested Action: qst_hunt_spiders를 Castle 보드의 locale-aware 카드로 추가하고 수락/진행/보고/보상/저장을 연결한다. 아직 범위 밖이면 spec/README/로드맵에 Deferred로 명시하고 UI에 노출하지 않는 기준과 추적 ID를 기록한다.
- Re-audit Method: Castle에서 세 퀘스트를 각각 신규 수락·진행·완료·재진입하고, 5개 언어의 목표/보상/상태가 동일한지 확인한다. 명세 목록과 실제 보드 항목을 자동 비교한다.
- Confidence: High
- Owner: Product owner / Coder / Content UX owner
- Notes: 이번 finding은 몬스터 밸런스가 아니라 플레이어가 명세상 콘텐츠에 도달할 수 있는지에 대한 판정이다.

## 6. Uncertainties and Clarifications Needed

1. spec.md:40-43은 1024x768 고정을 동결하지만 designs.md:279는 리사이즈 이벤트 비율 고정을 요구한다. 출시 정책이 항상 non-resizable 1024x768인지, 내부 1024x768을 letterbox/scale하는 resizable window인지 확정해야 한다. 현재는 fixed window를 의도된 미학으로 존중하되 접근성 scale 부재를 Major로 남겼다.
2. Guild의 이름/직업이 랜덤이라는 결정은 spec.md에 명시적으로 동결되어 있지 않다. 랜덤 모집이 의도라면 UI에 이를 표시하고 reroll/preview 규칙을 문서화하고, 파티 편성 선택을 요구한다면 Character Creation substate를 추가해야 한다.
3. TPK save reset 자체는 DESIGN_DECISIONS.md:71-91과 spec.md에 동결되어 있으나, 첫 실행/던전 진입 전 경고·확인 시점은 정해져 있지 않다. 하드코어 정책을 유지할지보다 플레이어가 위험을 사전에 인지하고 실수 시 되돌릴 수 있는가를 제품 오너가 확정해야 한다.
4. Linux에서는 Game이 /usr/share/fonts/opentype/noto/NotoSansCJK-Medium.ttc를 우선 선택하지만 UI glyph test는 assets/fonts/DroidSansFallbackFull.ttf를 선택한다 (src/core/Game.cpp:94-113, src/test_harness.cpp:715-760). Windows/macOS의 실제 선택 폰트·혼합 CJK raster·고DPI는 이번 환경에서 검증하지 않았다. 실패로 단정하지 않고 release gate UNVERIFIED로 유지한다.
5. 화면 narration, controller/remapping, color-blind palette, reduced-motion 옵션은 현재 코드에 없다. 사용자가 이번 범위에서 XAG를 best-practice 비교 기준으로 제시했으므로 최소 출시 게이트에서 text/focus/contrast/keyboard 경로를 우선하고, narration/remapping은 플랫폼·제품 범위를 명세로 결정해야 한다.

## 7. Perspective Decision

- Decision: HOLD (UI/UX 관점)
- Rationale: Major confirmed findings A02-F001 through A02-F011 and A02-F013 affect core status truth, i18n, navigation, combat choices, persistence, accessibility, destructive-action safety, content discoverability, or release evidence. Green TestHarness --run-all and Linux Xvfb startup do not clear these findings.
- Minimum release gates (must fix before a commercial release candidate):
  1. Dungeon/Combat/Town/CharacterInfo HUD가 실제 Party·상태를 표시하고, 상태이상·버프·저장 결과를 persistent/명확하게 피드백한다.
  2. 다섯 locale에서 모든 사용자-facing 문자열(아이템/몬스터/스킬/로그/오류 포함)을 번역 키로 제공하고, 고정 패널에서 wrap/overflow가 없다.
  3. 모든 화면·Town 서브상태·Combat turn/overlay에서 Settings, Enter/ESC/back/cancel 경로가 안내와 실제 동작에 일치한다.
  4. Combat Item은 item/target preview-confirm-cancel 선택을 제공하며 판매·해고·소모·TPK reset은 review/confirm 또는 명시적 복구 정책을 가진다.
  5. CharacterInfo/Combat mutations와 Settings 변경은 atomic persistence 및 save failure/unsaved feedback을 검증한다.
  6. 명세에 남아 있는 사용자-facing 퀘스트/콘텐츠는 실제 보드·보상·진행 경로에 연결하거나 Deferred로 명시한다.
  7. 대비·focus·font/text scale·CJK geometry를 자동/시각 gate로 잠그고, UI state/input regression harness를 추가한다. Windows/macOS 실기는 별도 UNVERIFIED gate를 PASS로 올리지 않는다.
- Follow-up polish (after minimum gates):
  - 공통 UIRenderer/layout/token/input abstraction, mouse menu parity, remappable keys, high contrast/color-blind palette, narration, reduced-motion, richer combat animation, quest journal/filtering, autosave indicator and undo history.
  - 랜덤 모집을 유지할 경우 reroll/preview, party reorder, portrait/role affordance를 추가한다.

## 8. Coder Handoff

/mnt/Projects_SSD/cpp/crawlmaster/docs/multi_audit/1/sub_audit_02_interface_ux.md를 먼저 읽고, 각 finding을 spec.md·designs.md 등 프로젝트 문서와 실제 코드에 대조하여 검증한 뒤 우선순위대로 수정하세요. 계약 변경이 필요하면 관련 문서를 먼저 갱신하고, 수정 후 상태별 입력 테스트·5개 언어 UI/레이아웃·세이브/재실행·Linux/Windows/macOS 실행 증거를 기록하세요. 이 배정 보고서는 통합 전에 원본 그대로 봉인되어야 하며, 정정은 별도 supplement로 남겨야 합니다.
