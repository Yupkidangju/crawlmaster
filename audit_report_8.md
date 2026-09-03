# D3D 독립 재감사 보고서 — v0.9.3 다국어 렌더링 및 테스트 자산 동기화 검증

- **감사 일자:** 2026-07-13 (Asia/Seoul)
- **프로젝트 경로:** `/mnt/Projects_SSD/cpp/crawlmaster`
- **감사 기준:** `AI_AUDIT_DOC_STANDARD.md`
- **이전 독립 감사:** `audit_report_5.md`
- **후속 구현측 보고서:** `audit_report_6.md`, `audit_report_7.md`
- **재감사 회차:** Re-audit #3
- **실행 방식:** Implementation Compliance / Debug & Engineering Quality / Security 3-pass
- **코드 수정 여부:** 없음
- **최종 판정:** **REWORK REQUIRED**

## 0. 감사 요약

v0.9.3의 공용 `Assets` 타겟은 의도대로 동작한다. `Crawlmaster`와 `TestHarness` 빌드 시 소스 `assets/`가 `build/assets/`로 복사됐고, 5개 번역 JSON과 3개 폰트의 SHA-256이 양쪽에서 일치했다. Debug 빌드와 격리된 두 실행 위치의 `TestHarness --run-all`도 각각 13개 시나리오를 통과했다.

그러나 `audit_report_6.md`의 전체 PASS와 `audit_report_7.md`의 후속 검증 완료 주장은 실제 사용자 화면에서 유지되지 않는다.

- 일본어 설정으로 재시작한 Town 화면에서 ASCII 문자 대부분이 사각형으로 표시됐다.
- 한국어 Town에서 일본어로 실시간 전환한 뒤 복귀하면 지속 `sf::Text` 객체가 기존 한글 폰트를 유지하여 일본어 문자가 사각형으로 표시됐다.
- `DroidSansFallbackFull.ttf`를 FFmpeg/FreeType 경로로 직접 렌더링한 결과도 `=== TOWN CAMP ===` ASCII 부분이 사각형이었다.
- `hasGlyph` 테스트는 ASCII를 전부 제외하며, 실제 State/renderer를 링크하거나 화면을 비교하지 않는다. 따라서 위 실패 상태에서도 13/13을 통과한다.
- Town 허브가 조회하는 번역 키 9개가 5개 JSON 모두에 없어서 `TOWN_CAMP_OPTION_1` 같은 키 이름이 사용자 화면에 그대로 노출된다.
- Town/Combat에는 여전히 다국어 리소스 밖의 사용자-facing 영문 및 한국어 문자열이 남아 있다.
- 문서에는 CJK 렌더링과 키 매핑이 완료됐다고 기록됐지만, 실제 화면과 정적 key coverage가 반증한다.
- 테스트 하네스는 실행 디렉터리의 기본 `save.json`을 변경한다. `BUILD_GUIDE.md`가 게임과 같은 `build/`에서 테스트 실행을 안내하므로 실제 사용자 세이브를 덮어쓸 수 있다.

동일한 “이상한 문자/사각형” 결함이 폰트 교체와 테스트 보강 후에도 다른 형태로 반복됐고, 현재 자동 테스트가 실패 모드를 감지하지 못한다. 폰트 선택 수명주기, 번역 key completeness, 실제 렌더 검증, 테스트 저장소 격리를 함께 재설계해야 하므로 단순 `HOLD`보다 강한 **REWORK REQUIRED**로 판정한다.

## 1. Audit Scope

### 1.1 확인한 문서

- `AGENTS.md`
- `AI_AUDIT_DOC_STANDARD.md`
- `spec.md`
- `designs.md`
- `README.md`
- `CHANGELOG.md`
- `BUILD_GUIDE.md`
- `IMPLEMENTATION_SUMMARY.md`
- `LESSONS_LEARNED.md`
- `DESIGN_DECISIONS.md`
- `audit_roadmap.md`
- `audit_report_1.md` ~ `audit_report_7.md`

### 1.2 확인한 구현·자산·산출물

- `CMakeLists.txt`
- `include/core/Game.hpp`
- `include/core/LocalizationManager.hpp`
- `src/core/Game.cpp`
- `src/core/LocalizationManager.cpp`
- `src/controller/TitleState.cpp`
- `src/controller/TownState.cpp`
- `src/controller/SettingsState.cpp`
- `src/controller/CombatState.cpp`
- `src/view/DungeonRenderer.cpp`
- `src/test_harness.cpp`
- `assets/lang/*.json`
- `assets/fonts/*.ttf`
- `build/CMakeCache.txt`
- `build/Crawlmaster`
- `build/TestHarness`
- `build/assets/**`

### 1.3 실행한 검사와 결과

- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`: 통과
- `cmake --build build --target Crawlmaster TestHarness -j2`: 통과
- 소스 자산을 복사한 격리 디렉터리에서 `TestHarness --run-all`: 13/13 통과
- 빌드 자산을 복사한 격리 디렉터리에서 `TestHarness --run-all`: 13/13 통과
- 소스/빌드 번역 JSON 및 폰트 SHA-256 대조: 전부 일치
- 5개 JSON 구문 검사: 통과
- 5개 JSON key 수·집합 대조: 각 132개, 언어 간 key 집합 동일
- source의 literal `get()`/`getSf()` key와 JSON key 대조
- 비한국어 JSON의 한글 범위 검색
- `selectLang` 및 invalid temporary iterator pattern 검색
- 폰트 메타데이터 및 charset 확인
- `DroidSansFallbackFull.ttf` 직접 텍스트 렌더링
- Xvfb에서 한국어 Town → 일본어 설정 전환 → Town 복귀 화면 캡처
- Xvfb에서 저장된 일본어 설정으로 재시작 → Town 화면 캡처

### 1.4 런타임 증거

#### 한국어에서 일본어로 실시간 전환 후 Town

- 캡처: `/tmp/crawlmaster-audit8-ko-to-ja.png`
- SHA-256: `3c15c8a6776cf59d0d817a63b2295e78a66bb54441a7a04d8c5639b368fd2744`
- 결과:
  - Town 제목의 일본어 문자가 사각형으로 표시됨.
  - 파티 상태의 일본어 문자가 다수 사각형으로 표시됨.
  - 누락 번역 키 `TOWN_CAMP_WELCOME`, `TOWN_CAMP_OPTION_*`가 번역되지 않고 그대로 표시됨.
- 판정: 실시간 언어 전환과 번역 key completeness 모두 실패.

#### 일본어 설정 저장 후 재시작한 Town

- 캡처: `/tmp/crawlmaster-audit8-ja-restart.png`
- SHA-256: `2f2920a89acfc9c5b337074cfc664ae55ca8906c21acaae3f7aabe942b2a8153`
- 결과:
  - 일본어 일부는 표시되지만 ASCII 영문·숫자·구두점 대부분이 사각형으로 표시됨.
  - Town의 혼합 CJK/ASCII UI를 정상적으로 읽을 수 없음.
- 판정: CJK 폰트로 시작해도 실제 혼합 UI 렌더링 실패.

#### CJK 폰트 직접 렌더링

- 입력: `=== TOWN CAMP === 日本語 パーティステータス`
- 캡처: `/tmp/crawlmaster-audit8-font-render.png`
- SHA-256: `4d78cbd427254768ebc294740f5fdc26b8603f79659e8f38ed9e1d1f5c905f8e`
- 결과: 일본어는 표시되지만 ASCII 부분은 사각형으로 렌더링됨.
- 판정: 앱 레이아웃과 무관하게 현재 번들 파일 자체가 제품의 혼합 문자열 요구를 충족하지 못함.

## 2. Excluded Scope

- 실제 사용자 데스크톱 세션: 격리된 Xvfb에서 재현했다.
- 전체 전투·던전 장시간 플레이: Town 진입 직후 Major 렌더링 실패가 재현되어 중단했다.
- Windows/macOS 패키징: Linux Debug 빌드만 검증했다.
- ASan/UBSan: 이전 invalid iterator 수정은 정적 대조로 유지 확인했으며 별도 sanitizer build는 실행하지 않았다.
- dependency CVE scanner/CI: 제품용 scanner와 CI 설정이 없어 실행하지 않았다.
- `build/_deps`: 외부 dependency 내부 구현은 제품 코드 감사에서 제외했다.
- 폰트 법률 자문: 저장소 내 provenance/license 증거 유무만 확인했으며 사용 허가의 법률적 유효성은 판단하지 않았다.

## 3. Pass 1: Implementation Compliance Findings

### [IMP-F001] Re-audit #3 — 번역 리소스 일원화가 불완전하고 Town 허브 키 9개가 누락됨

- **Pass:** Implementation
- **Pattern:** IMP-001, ARCH-002
- **Area:** i18n source of truth, key completeness
- **Severity:** Major
- **Status:** Needs Fix — 재발/부분 해소
- **Summary:** 기존 `selectLang` helper는 제거됐으나 새 Town 허브 호출 키가 JSON에 없고 여러 사용자-facing 문자열이 controller에 남아 있다.
- **Evidence:**
  - `TownState.cpp:321-330`: `TOWN_CAMP_WELCOME`, `TOWN_CAMP_OPTION_1`~`5`, `TOWN_CAMP_OPTION_C`, `TOWN_CAMP_OPTION_O`, `TOWN_CAMP_OPTION_ESC` 조회.
  - 5개 `assets/lang/*.json`: 위 9개 키 전부 없음.
  - `LocalizationManager.cpp:46-52`: 키가 없으면 키 문자열 자체를 반환.
  - 실시간 전환 캡처에서 `TOWN_CAMP_OPTION_1` 등의 raw key 노출.
  - `TownState.cpp:335,343,351-359,385`: `Guild Desk`, `Shop Menu`, 상품명, `Temple Sanctuary` 등 영문 하드코딩.
  - `TownState.cpp:219,241`: 퀘스트 이름·설명이 한국어로 하드코딩.
  - `CombatState.cpp:835-847`: `Turn`, `Slots`, `No Cost` 하드코딩.
  - `spec.md:444-449`: 모든 UI 텍스트는 `LocalizationManager`의 다국어 리소스 키를 통해 획득하도록 규정.
- **Expected:** 모든 사용자-facing 텍스트가 5개 JSON의 동일 key 집합에 존재하고, 누락 key가 빌드/테스트에서 실패한다.
- **Actual:** 9개 호출 key가 모든 JSON에서 누락됐고 controller에 영문·한국어 UI 문자열이 남아 있다.
- **Impact:** 언어와 무관하게 key 이름이 그대로 표시되고 일본어·중국어 화면에 다른 언어가 혼입된다.
- **Suggested Fix:** source의 모든 literal localization 호출을 추출해 5개 JSON과 양방향 대조하고, 누락 9개 및 남은 하드코딩 문구를 JSON key로 이관한다. 런타임 fallback은 개발 경고를 남기되 배포/테스트에서는 누락 key를 실패 처리한다.
- **Re-audit Method:** source literal key ↔ 5개 JSON 완전성 검사, 하드코딩 사용자 문자열 검색, 5개 언어 Town 7개 substate 캡처.
- **Owner:** Coder / Localization Owner
- **Notes:** `NON_EXISTING_KEY_XYZ`는 fallback 동작을 검증하는 의도된 test sentinel이므로 누락 9개 집계에서 제외했다.

### [IMP-F002] Re-audit #3 — CJK 번들 폰트가 혼합 문자열을 실제로 렌더링하지 못함

- **Pass:** Implementation
- **Pattern:** IMP-001, BUILD-001
- **Area:** font asset, shipped UI
- **Severity:** Major
- **Status:** Needs Fix — 미해결/구현측 PASS 기각
- **Summary:** 새 폰트는 일본어·중국어 일부 글리프를 제공하지만 제품 UI에 필수인 ASCII를 사각형으로 렌더링한다.
- **Evidence:**
  - `Game.cpp:39-45`: JA/ZH_TW/ZH_CN이면 `m_cjkFont`를 반환.
  - `Game.cpp:94-102`: `DroidSansFallbackFull.ttf`를 CJK 폰트로 로드.
  - 일본어 재시작 Town 캡처에서 영문·숫자·구두점 대부분이 사각형.
  - 같은 폰트의 독립 렌더 입력 `=== TOWN CAMP === 日本語 パーティステータス`에서도 ASCII가 사각형.
  - `designs.md:21`, `README.md:41`, `CHANGELOG.md:19`는 CJK/일어 글리프의 완전한 지원을 주장.
  - SFML의 `hasGlyph`는 문자에 대응하는 glyph 존재 여부를 질의하지만 실제 glyph 윤곽이 읽을 수 있는 문자 모양인지 보장하는 시각적 oracle은 아니다: [SFML 2.6.1 sf::Font](https://www.sfml-dev.org/documentation/2.6.1/classsf_1_1Font.php).
- **Expected:** CJK 언어의 실제 UI에 포함되는 CJK와 ASCII 혼합 문자열이 모두 읽을 수 있게 표시된다.
- **Actual:** CJK는 일부 표시되지만 ASCII가 대체 사각형 모양으로 렌더링되어 화면을 읽을 수 없다.
- **Impact:** 5개 언어 지원의 핵심 제품 요구를 충족하지 못하며 새 폰트 적용만으로 문제를 해결했다는 문서 판단이 잘못된다.
- **Suggested Fix:** 출처와 라이선스가 확인된 정상 폰트 파일로 교체하거나, CJK/ASCII를 font fallback run으로 분리 렌더링한다. 대표 혼합 문자열의 screenshot/golden image 검증을 추가한다.
- **Re-audit Method:** 폰트 단독 렌더 smoke와 JA/ZH_TW/ZH_CN의 Title/Town/Settings/Combat 캡처를 사람이 판독하고 baseline과 비교한다.
- **Owner:** Architect / Coder / UI QA
- **Notes:** cmap/`hasGlyph` 통과와 실제 가독성은 분리 판정해야 한다.

### [IMP-F003] Re-audit #3 — 실시간 언어 전환 시 기존 State의 폰트가 갱신되지 않음

- **Pass:** Implementation
- **Pattern:** IMP-001, DBG-001
- **Area:** runtime font lifecycle, state UI
- **Severity:** Major
- **Status:** Needs Fix — 신규 finding
- **Summary:** 문자열은 draw 시점에 갱신되지만 지속 `sf::Text` 객체는 State 생성 시 선택한 폰트 포인터를 계속 사용한다.
- **Evidence:**
  - `TownState.cpp:283-309`: `initTexts()`에서 한 번만 `m_game.getFont()`를 `m_titleText`, `m_menuText`, `m_partyText`, `m_statusText`에 설정.
  - `TownState.cpp:269-274,312-503`: 매 draw에서 문자열만 갱신하고 `setFont()`는 다시 호출하지 않음.
  - `CombatState.cpp:756-787`: persistent text의 폰트를 초기화 시 한 번 설정; `updateTuiContent()`는 문자열만 갱신.
  - `TitleState.cpp:101-130`: instruction/credit text의 폰트를 초기화 시 한 번 설정.
  - `SettingsState.cpp:169-185`: 설정 화면 자체는 매 draw 임시 text에 현재 폰트를 적용하므로 이 화면만 동적 전환됨.
  - 한국어 Town → 일본어 설정 → Town 복귀 캡처에서 일본어가 사각형으로 재현.
  - SFML `sf::Text`는 설정한 `sf::Font`를 참조하며 font 객체가 text 수명 동안 유지돼야 한다: [SFML 2.6.1 sf::Text](https://www.sfml-dev.org/documentation/2.6.1/classsf_1_1Text.php).
- **Expected:** 설정에서 언어가 변경되면 문자열과 선택 폰트가 같은 프레임 또는 복귀 시점에 함께 갱신된다.
- **Actual:** 문자열만 일본어/중국어로 바뀌고 기존 한글 폰트가 남는다.
- **Impact:** 재시작 없이 언어를 바꾸는 spec의 핵심 경로가 항상 깨진다.
- **Suggested Fix:** 언어 변경 revision/event를 도입해 모든 persistent text의 font를 재바인딩하거나, 각 State의 refresh 경로에서 현재 `Game::getFont()`를 함께 적용한다. State별 임시 수정 대신 공통 text/font 정책을 두는 것이 안전하다.
- **Re-audit Method:** KO↔JA, EN↔ZH_TW, JA↔KO 양방향 전환을 Title/Town/Dungeon/Combat에서 수행하고 재시작 전후 캡처를 비교한다.
- **Owner:** Architect / Coder
- **Notes:** `getFont()` 자체의 동적 반환만으로 이미 생성된 `sf::Text`의 font reference가 자동 교체되지는 않는다.

### [IMP-F004] Re-audit #3 — v0.9.2/0.9.3 완료 문서가 현재 실행 증거를 과대주장함

- **Pass:** Implementation
- **Pattern:** IMP-003, IMP-004, DOC-BACKFILL-001
- **Area:** release claims, document-code sync
- **Severity:** Major
- **Status:** Needs Documentation Recovery
- **Summary:** 감사 로드맵과 변경 문서는 key 매핑 및 CJK 렌더링을 완료/통과로 기록하지만 현재 runtime은 실패한다.
- **Evidence:**
  - `audit_roadmap.md:11-26`: 다국어 key 누락 없이 실시간 출력하고 State에서 실시간 재바인딩한다는 통과 주장.
  - `audit_roadmap.md:69-71`: CJK 폰트 및 전체 codepoint 100% PASS 주장.
  - `CHANGELOG.md:19`: CJK/일어 글리프를 완벽히 지원한다고 기록.
  - `LESSONS_LEARNED.md:79-82`: font switching과 `hasGlyph`로 완전성을 보증했다고 기록.
  - `audit_report_6.md`: 전체 PASS를 발부했지만 실제 renderer 검증과 키 완전성 검사가 없음.
  - `audit_report_7.md:39-41`: 공유 `save.json` 변경 문제를 후속 과제로 남겼으나 accepted risk owner/조건이 없음.
  - `IMPLEMENTATION_SUMMARY.md`는 새 이중 폰트 lifecycle과 v0.9.3 자산 타겟을 현재 파일 책임에 반영하지 않음.
  - `DESIGN_DECISIONS.md`는 언어별 폰트 전환 전략, 대안, trade-off, 자산 provenance를 기록하지 않음.
- **Expected:** 완료 문서는 실제 renderer evidence, key coverage, test isolation과 같은 gate를 통과한 범위만 완료로 선언한다.
- **Actual:** helper/asset-level 검증을 UI 완료로 확대 해석했고 반증된 위험을 계속 통과로 표시한다.
- **Impact:** 후속 구현자와 릴리즈 담당자가 깨진 UI 및 데이터 변경 위험을 정상 상태로 오인한다.
- **Suggested Fix:** 코드 수정 후 spec/design/implementation summary/design decision/audit roadmap/changelog의 주장 범위를 실제 검증 레이어에 맞춰 동기화하고, 과거 PASS 보고서는 superseded 상태를 명시한다.
- **Re-audit Method:** 각 완료 주장에 대응하는 source owner, 자동 test, runtime 캡처를 1:1로 대조한다.
- **Owner:** Architect / Documentation Owner / Auditor
- **Notes:** 이 finding은 문서를 현재 실패 상태로만 낮추라는 의미가 아니라, 재검증 후 사실에 맞는 상태로 갱신하라는 요구다.

### [IMP-F005] Re-audit #3 — v0.9.3 공용 자산 동기화는 해결됨

- **Pass:** Implementation
- **Pattern:** BUILD-001
- **Area:** build assets
- **Severity:** Info
- **Status:** Verified — 해결
- **Summary:** `Crawlmaster`와 `TestHarness`가 공용 `Assets` 타겟을 통해 최신 자산을 받는다.
- **Evidence:** `CMakeLists.txt:86-93,112-113`; build 성공; 소스/빌드의 모든 번역 JSON과 폰트 SHA-256 일치.
- **Expected:** 두 실행 파일이 같은 최신 `assets/`를 사용한다.
- **Actual:** 충족.
- **Impact:** `audit_report_7.md`가 지적한 stale build asset 문제는 해소됨.
- **Suggested Fix:** 복사 자체는 유지하되 실제 renderer/test isolation finding과 분리해 판단한다.
- **Re-audit Method:** 자산 변경 후 두 target 개별 빌드, hash 비교, 각 cwd 테스트.
- **Owner:** Auditor
- **Notes:** 자산이 최신이라는 사실은 자산 내용이 올바르다는 의미가 아니다.

### [IMP-F006] Re-audit #3 — v0.9.3 version authority 정렬

- **Pass:** Implementation
- **Pattern:** IMP-002
- **Area:** version files
- **Severity:** Info
- **Status:** Verified
- **Summary:** CMake, README, CHANGELOG, BUILD_GUIDE, audit roadmap이 v0.9.3 변경을 일관되게 추적한다.
- **Evidence:** `CMakeLists.txt:2-4`, `README.md:22,60,91,122,153`, `CHANGELOG.md:7-11`, `audit_roadmap.md:1-3,35-36`.
- **Expected:** patch version과 변경 범위의 일치.
- **Actual:** 일치.
- **Impact:** version drift 없음.
- **Suggested Fix:** 없음.
- **Re-audit Method:** 다음 version bump 시 전체 검색.
- **Owner:** Auditor
- **Notes:** 버전 정렬은 기능 PASS와 별도다.

## 4. Pass 2: Debug / Engineering Quality Findings

### [DBG-F001] Re-audit #3 — invalid iterator 및 Combat ANSI 경로 수정 유지

- **Pass:** Debug
- **Pattern:** DBG-001
- **Area:** UTF-8 conversion, object lifetime
- **Severity:** Info
- **Status:** Verified — 유지
- **Summary:** 생존 로컬 문자열의 begin/end를 사용하는 `getSf()` 경로가 유지되고 invalid temporary pair는 재발하지 않았다.
- **Evidence:** `LocalizationManager.cpp:55-58`; `CombatState.cpp:790-793`; invalid temporary iterator pattern 검색 결과 0건.
- **Expected:** 동일한 생존 문자열 객체의 valid iterator range 사용.
- **Actual:** 충족.
- **Impact:** 기존 OOB read 원인은 제거된 상태 유지.
- **Suggested Fix:** sanitizer 회귀 gate를 추가하면 증거가 강화된다.
- **Re-audit Method:** ASan/UBSan 반복 상태 전환.
- **Owner:** Auditor
- **Notes:** 현재 이상 문양의 원인은 이 과거 UB가 아니라 font/key lifecycle이다.

### [DBG-F003] Re-audit #3 — `hasGlyph` 테스트가 실제 실패 모드를 검출하지 못함

- **Pass:** Debug
- **Pattern:** TEST-001, DBG-002
- **Area:** regression test authority
- **Severity:** Major
- **Status:** Needs Fix — 미해결
- **Summary:** 테스트 명칭과 성공 로그는 Town/Combat UI와 100% glyph coverage를 주장하지만 실제 State, font switching, glyph 모양을 검증하지 않는다.
- **Evidence:**
  - `test_harness.cpp:662-746`: title key 비어있지 않음, 반복 `getSf`, JSON codepoint `hasGlyph`만 검사.
  - `test_harness.cpp:728-731`: ASCII `<=127` 전체를 font 검증에서 제외.
  - `CMakeLists.txt:95-107`: TestHarness source에 `TownState.cpp`, `CombatState.cpp`, `Game.cpp`, renderer가 없음.
  - 현재 CJK 폰트의 ASCII가 실제로 사각형이지만 양쪽 격리 실행에서 13/13 통과.
  - 누락된 Town 허브 9개 key는 테스트 대상 key 목록에 없음.
  - 성공 로그 `5대 언어 100% Glyph Coverage 확보`는 실제 렌더 결과와 충돌.
- **Expected:** 과거 사각형/이상 문양 실패가 재발하면 자동 test가 실패하고 State/font lifecycle 및 key completeness를 직접 검증한다.
- **Actual:** cmap 수준의 helper test만 통과하며 실제 화면은 실패한다.
- **Impact:** 배포 gate가 반복적으로 false PASS를 생성한다.
- **Suggested Fix:** 테스트를 (1) source literal key completeness, (2) font asset smoke, (3) State font rebinding integration, (4) Xvfb screenshot/golden 또는 pixel/OCR 기반 renderer test로 분리한다. 테스트 명칭과 로그도 실제 범위로 제한한다.
- **Re-audit Method:** 고의로 key 하나 삭제, ASCII-box 폰트 적용, KO→JA font rebinding 제거 시 각각 대응 test가 실패하는 mutation 검증을 수행한다.
- **Owner:** Coder / Test Owner / Auditor
- **Notes:** `hasGlyph` 검사는 보조 gate로 유지할 수 있으나 유일한 renderer oracle로 사용할 수 없다.

### [DBG-F004] Re-audit #3 — build 및 양쪽 자산 실행 경로 재현

- **Pass:** Debug
- **Pattern:** BUILD-001
- **Area:** build reproducibility
- **Severity:** Info
- **Status:** Verified
- **Summary:** 현재 source와 build asset을 각각 격리해 같은 TestHarness를 실행했으며 양쪽 모두 정상 종료했다.
- **Evidence:** Debug configure/build 통과; 두 격리 cwd에서 13개 시나리오 통과; asset hash 일치.
- **Expected:** BUILD_GUIDE의 build target과 산출물 생성 성공.
- **Actual:** 충족.
- **Impact:** 컴파일 및 stale asset 차단은 현재 gate를 통과.
- **Suggested Fix:** fresh clean build 및 Release test는 후속 릴리즈 감사에서 추가한다.
- **Re-audit Method:** clean Debug/Release build, isolated test cwd, artifact tree 비교.
- **Owner:** Auditor
- **Notes:** 실행 중 `Failed to create stream fd: Operation not permitted`가 출력됐지만 오디오가 비활성인 테스트 하네스는 exit 0이었고 검증 assertions는 통과했다. 격리 환경 메시지로 기록한다.

### [DBG-F005] Re-audit #3 — TestHarness가 게임 기본 `save.json`을 변경함

- **Pass:** Debug
- **Pattern:** TEST-001, SEC-004
- **Area:** test isolation, persistent data
- **Severity:** Major
- **Status:** Needs Fix — 기존 known issue 승격
- **Summary:** 테스트가 전용 임시 경로만 사용하지 않고 실행 cwd의 기본 세이브를 반복 로드·저장한다.
- **Evidence:**
  - `test_harness.cpp:617-659`: `Party` 기본 경로를 사용하는 상점 테스트에서 `saveToFile()`/`loadFromFile()` 호출.
  - 전체 테스트 로그: `./save.json`을 여러 차례 생성·저장·손상 복구·정산.
  - `BUILD_GUIDE.md:63-70`: 게임과 테스트를 같은 build cwd에서 실행하도록 안내.
  - `audit_report_7.md:39-41`: 공유 `save.json` 변경을 기존 문제로 명시.
  - 이번 감사는 프로젝트 데이터를 보존하기 위해 `/tmp/crawlmaster-audit8-*` 격리 cwd에서 실행함.
- **Expected:** 테스트는 임시 디렉터리/주입된 파일 경로만 사용하며 게임용 `save.json`과 `config.json`을 읽거나 쓰지 않는다.
- **Actual:** 실행 위치의 기본 게임 세이브를 테스트 fixture로 사용하고 최종 상태를 원복한다고 보장하지 않는다.
- **Impact:** 문서대로 테스트하면 개발자 또는 사용자의 진행 데이터가 변경·초기화될 수 있다.
- **Suggested Fix:** 모든 persistence API에 테스트 경로를 명시하고, 테스트별 unique temp directory와 RAII cleanup을 적용한다. 기본 경로 접근이 발생하면 test를 실패시키는 guard를 추가한다.
- **Re-audit Method:** sentinel `save.json`/`config.json` hash를 준비하고 전체 테스트 전후 동일성 확인, temp artifact 잔존 여부 검사.
- **Owner:** Coder / Test Owner
- **Notes:** 단순 문서 경고로 수용할 위험이 아니라 데이터 격리 실패이므로 PASS 전에 수정해야 한다.

## 5. Pass 3: Security Findings

### [SEC-F001] Re-audit #3 — invalid range 기반 memory read 위험 제거 유지

- **Pass:** Security
- **Pattern:** SEC-004
- **Area:** memory safety
- **Severity:** Info
- **Status:** Verified
- **Summary:** 이전 invalid iterator pair의 직접 원인은 재발하지 않았다.
- **Evidence:** DBG-F001과 동일.
- **Expected:** 소유권과 길이가 확정된 string range만 사용.
- **Actual:** 충족.
- **Impact:** 이전 OOB read 위험 해소 유지.
- **Suggested Fix:** sanitizer 자동화 권장.
- **Re-audit Method:** ASan/UBSan UI 상태 반복 전환.
- **Owner:** Auditor
- **Notes:** 현재 렌더 결함과 분리한다.

### [SEC-F002] Re-audit #3 — 네트워크·셸 실행 표면 없음

- **Pass:** Security
- **Pattern:** SEC-003, SEC-004, SEC-007
- **Area:** network/shell boundary
- **Severity:** Info
- **Status:** Verified — 유지
- **Summary:** 제품 코드에 원격 bind, 인증 표면, 임의 shell 실행 경로가 없다.
- **Evidence:** source/CMake 검색 및 로컬 SFML desktop architecture.
- **Expected:** 로컬 전용 제품 경계.
- **Actual:** 충족.
- **Impact:** 원격 공격 표면 낮음.
- **Suggested Fix:** 외부 integration 추가 시 재감사.
- **Re-audit Method:** socket/process/shell API 및 새 dependency 재검색.
- **Owner:** Auditor
- **Notes:** font 및 파일 저장 경계는 별도 finding이다.

### [SEC-F003] 신규 번들 폰트의 provenance와 재배포 근거가 없음

- **Pass:** Security
- **Pattern:** SEC-006, DOC-BACKFILL-001
- **Area:** shipped binary asset, supply-chain provenance
- **Severity:** Major
- **Status:** Needs Spec Clarification / Human Review Required
- **Summary:** 4,033,420-byte 외부 TTF가 배포 자산으로 추가됐지만 출처 URL, upstream version/hash, 라이선스 또는 NOTICE가 저장소에 없다.
- **Evidence:**
  - `assets/fonts/DroidSansFallbackFull.ttf`: 번들 및 build 복사 대상.
  - 제품 파일 범위에서 `LICENSE`, `COPYING`, `NOTICE` 파일 없음.
  - 문서 검색에서 폰트 출처·라이선스·재배포 조건 없음.
  - `spec.md:61`은 기본 폰트를 “프리웨어 DPF 폰트”로 기술하지만 새 Droid 자산의 authority를 설명하지 않음.
  - font SHA-256: `acb6440a713d880a13a21b468ba7cd43f5a2b2934972e51be791c880730777b8`.
- **Expected:** 배포되는 제3자 바이너리 자산은 canonical source, version/hash, license, notice, 재배포 범위가 추적 가능하다.
- **Actual:** 파일명과 내부 family metadata 외 provenance 증거가 없다.
- **Impact:** 자산 위변조 여부와 재배포 권한을 검증할 수 없으며 릴리즈 법적/공급망 gate를 닫을 수 없다.
- **Suggested Fix:** 공식 upstream에서 자산을 재획득하고 checksum을 고정하며 라이선스/NOTICE 및 attribution을 저장소와 배포물에 포함한다. 사용 가능 여부는 담당자가 검토한다.
- **Re-audit Method:** source URL, upstream hash/version, license text, packaged NOTICE, reproducible asset hash 대조.
- **Owner:** Human / Release Owner / Architect
- **Notes:** 감사자는 법률 자문을 제공하지 않으므로 최종 허가 판단은 Human Review Required다.

## 6. Cross-Pass Conflicts

### [XPF-F001] 구현측 PASS 및 audit roadmap의 통과 주장과 실제 UI 실패가 충돌

- **Related Findings:** IMP-F001, IMP-F002, IMP-F003, IMP-F004, DBG-F003
- **Conflict:** `audit_report_6.md`와 `audit_roadmap.md`는 CJK glyph와 실시간 다국어를 전체 PASS로 기록하지만, 두 runtime 경로에서 각각 다른 사각형 실패가 재현됐다.
- **Resolution:** helper/cmap 결과보다 shipped runtime 화면 증거를 우선한다. 해당 PASS를 `Rejected as False Positive`로 분류한다.
- **Gate Impact:** REWORK REQUIRED.
- **Required Fix Before PASS:** font asset, runtime font rebinding, key completeness를 고친 뒤 5개 언어 실제 화면과 자동 회귀 test를 함께 통과해야 한다.

### [XPF-F002] 13/13 테스트 및 “100% Glyph Coverage”와 실제 폰트 가독성 실패가 충돌

- **Related Findings:** IMP-F002, IMP-F003, DBG-F003
- **Conflict:** 자동 테스트는 전부 통과하지만 ASCII를 검사하지 않고 State/renderer도 포함하지 않아 실제 사용자 실패를 관찰하지 않는다.
- **Resolution:** 현재 테스트는 JSON codepoint-to-cmap 보조 검사와 `getSf()` unit test로만 인정한다.
- **Gate Impact:** Major regression gate 미충족.
- **Required Fix Before PASS:** visual/failure-mode test와 State integration test 추가.

### [XPF-F003] 빌드 가이드의 테스트 명령과 데이터 보존 요구가 충돌

- **Related Findings:** DBG-F005
- **Conflict:** `BUILD_GUIDE.md`는 game data와 같은 cwd에서 TestHarness 실행을 안내하지만 테스트는 기본 `save.json`을 변경한다.
- **Resolution:** 데이터 보존을 우선해 테스트를 temp storage에 격리해야 한다.
- **Gate Impact:** Major.
- **Required Fix Before PASS:** sentinel save/config 불변성 검증 및 모든 test path 주입.

### [XPF-F004] 자산 복사 성공과 자산 적합성 실패를 분리해야 함

- **Related Findings:** IMP-F002, IMP-F005, SEC-F003
- **Conflict:** v0.9.3은 최신 폰트를 정확히 복사하지만 그 폰트의 실제 렌더 적합성과 provenance는 통과하지 못한다.
- **Resolution:** build transport는 Verified로 유지하고 asset content/release eligibility는 별도 Major finding으로 유지한다.
- **Gate Impact:** 자산 복사 finding은 해소됐지만 전체 PASS에는 영향 없음.
- **Required Fix Before PASS:** 검증된 올바른 자산과 라이선스 evidence로 동일 복사 경로 재검증.

## 7. Repeated Failure Diagnosis

- **반복 유형:** 동일한 CJK 사각형/이상 문양 finding이 `getSf()` 도입, 새 폰트 추가, `hasGlyph` 테스트 추가 뒤에도 계속 재발.
- **원인 분류:** 구현 구조와 테스트 부족.
- **구조 원인:** 언어 상태와 `sf::Text` font binding 수명주기가 분리되어 있으며, 선택된 font asset의 실제 혼합 문자열 품질을 renderer contract로 검증하지 않음.
- **테스트 원인:** State/renderer를 제외한 helper test가 UI 완료 명칭과 PASS authority를 가짐. 누락 key와 ASCII visual failure도 gate 밖임.
- **결정:** `Refactor`.
- **필요 범위:** 공통 font/text refresh 정책, localization key completeness gate, renderer-level visual smoke, persistence test isolation.
- **Rewrite Module 여부:** 현재 evidence만으로 LocalizationManager 전체 재작성은 필요하지 않다.
- **Human Review Required:** 제3자 폰트 출처와 재배포 허가.

## 8. Required Fixes Before PASS

1. 혼합 CJK/ASCII를 정상 렌더링하는 검증된 폰트 또는 font-run fallback 구현.
2. 언어 변경 시 모든 persistent `sf::Text`의 font를 재바인딩하는 공통 lifecycle 구현.
3. Town 허브 누락 key 9개를 5개 JSON에 추가하고 source ↔ JSON 양방향 completeness test 추가.
4. Town/Combat/Quest의 남은 사용자-facing 영문·한국어 하드코딩을 JSON key로 이관.
5. `hasGlyph` 테스트를 실제 범위에 맞게 정정하고 ASCII 및 key coverage를 포함.
6. Town/Combat/Game을 포함하는 integration test와 5개 언어 Xvfb visual regression 추가.
7. TestHarness의 모든 save/config I/O를 unique temp directory로 격리.
8. 폰트 canonical source, checksum, license, NOTICE 및 packaged attribution 확정.
9. spec/design/IMPLEMENTATION_SUMMARY/DESIGN_DECISIONS/README/CHANGELOG/audit roadmap을 실제 검증 상태와 동기화.
10. 과거 `audit_report_6.md` PASS 및 현재 audit roadmap 통과 상태를 본 보고서로 supersede 처리.

## 9. Accepted Risks

- 없음.
- 실제 사용자 화면의 읽기 불가, 번역 key 노출, 테스트의 세이브 변경, 제3자 자산 provenance 미확정은 무기한 known risk로 수용할 수 없다.

## 10. Needs Spec Clarification

### [NSC-001] CJK font fallback 및 visual acceptance authority

- 단일 범용 폰트, 언어별 폰트, 문자 run별 composite fallback 중 canonical 전략을 명시해야 한다.
- `hasGlyph`만으로 통과할지, representative string raster baseline까지 필수로 할지 결정해야 한다.
- 언어 전환 시 기존 State의 모든 text가 즉시 바뀌어야 하는 시점을 명시해야 한다. 현재 spec의 “실시간 전환”을 기준으로 본 감사는 복귀 첫 프레임을 요구했다.

### [NSC-002] 외부 폰트 자산 provenance 및 재배포 정책

- 허용 라이선스, attribution 위치, upstream pin/hash, 자산 교체 승인 owner를 정해야 한다.
- 시스템 폰트 fallback을 배포 요구로 허용할지, 번들 자산 실패 시 앱을 시작 실패시킬지 경계를 정해야 한다.

## 11. Re-audit Checklist

- [x] Debug configure/build 통과
- [x] 소스/빌드 자산 hash 동일
- [x] 격리된 소스 자산 TestHarness 13/13 통과
- [x] 격리된 빌드 자산 TestHarness 13/13 통과
- [x] invalid temporary iterator pair 0건
- [x] 비한국어 JSON의 한글 범위 혼입 0건
- [ ] source literal localization key 누락 0건
- [ ] 사용자-facing controller 하드코딩 0건
- [ ] JA/ZH_TW/ZH_CN 혼합 CJK/ASCII 폰트 단독 렌더 판독 가능
- [ ] KO↔JA, EN↔ZH_TW, JA↔KO 실시간 전환 후 기존 State 글리프 정상
- [ ] 5개 언어 Title/Town/Settings/Dungeon/Combat visual smoke 통과
- [ ] 실제 State/renderer가 회귀 test target에 포함됨
- [ ] 고의 font/key/font-rebinding regression에서 자동 test 실패
- [ ] TestHarness 전후 sentinel save/config hash 불변
- [ ] 폰트 upstream/license/NOTICE/package attribution 확인
- [ ] 완료 문서와 runtime evidence 동기화

## 12. Final Decision

**REWORK REQUIRED**

판단 근거:

- v0.9.3의 자산 복사와 build reproducibility는 해결됐다.
- 이전 UB와 ANSI 변환 경로도 해결 상태를 유지한다.
- 그러나 실제 일본어 runtime은 시작 방식에 따라 CJK 또는 ASCII 중 하나가 사각형으로 표시된다.
- Town 허브의 번역 key 9개가 모든 언어에서 누락되어 raw key가 노출된다.
- 현재 테스트는 이 두 실패를 모두 놓치면서 100% coverage를 주장한다.
- 테스트가 기본 세이브를 변경해 데이터 보존 경계도 충족하지 못한다.
- 외부 폰트의 provenance/license evidence가 없어 릴리즈 자산 gate도 닫히지 않는다.
- 동일 유형의 오류가 여러 재감사에서 false PASS 뒤 재발했으므로 font/text lifecycle과 regression test authority를 함께 리팩토링해야 한다.

Required Fixes를 반영하고 실제 5개 언어 화면, 실패모드 자동 테스트, 데이터 불변성, 자산 provenance를 확보한 뒤 Re-audit #4를 수행해야 한다.
