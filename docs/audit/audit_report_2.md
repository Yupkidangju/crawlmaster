# D3D 감사 보고서 — 실행 화면 문자열/문양 손상 집중 감사

- **감사 일자:** 2026-07-13 (Asia/Seoul)
- **프로젝트 경로:** `/mnt/Projects_SSD/cpp/crawlmaster`
- **감사 기준:** `AI_AUDIT_DOC_STANDARD.md`
- **이전 보고서:** `audit_report_1.md`
- **감사 방식:** Implementation Compliance / Debug & Engineering Quality / Security의 독립 3-pass
- **코드 수정 여부:** 없음
- **최종 판정:** **HOLD**

## 0. 감사 요약

사용자가 제보한 “실행 시 화면에 이상한 메시지나 문양이 나오는” 현상을 실제 가상 X11 화면에서 재현했다. 타이틀 화면은 정상 범위였으나 신규 게임으로 마을에 진입하자 제목 뒤부터 화면 전체에 사각형, 임의 영문 조각, 기호가 대량 출력되었다.

직접 원인은 `TownState::updateTuiContent()`의 7개 호출이 `LocalizationManager::get()`을 두 번 호출하여 **서로 다른 임시 `std::string` 객체의 `begin()`과 `end()`를 하나의 UTF-8 범위로 전달하는 것**이다. 이 범위는 유효하지 않으며 SFML 변환기가 문자열 경계를 넘어 메모리를 읽는 미정의 동작을 일으킨다.

추가로 다음 문제가 확인되었다.

- 전투 제목은 UTF-8 변환 없이 ANSI `std::string` 경로로 전달된다.
- 언어 판별에 존재하지 않는 번역 키 `"Language"`를 사용하여 한국어를 포함한 모든 언어에서 일부 메시지가 영문 분기로 고정된다.
- 공용 폰트 `neodgm.ttf`는 한글 범위는 포함하지만 감사한 대표 일본어/중국어 글리프를 포함하지 않아 5개 언어 표시 주장을 충족하지 못한다.
- `config.json`의 문서 계약과 구현 키/기본값이 다르다.
- 기존 `build/` 캐시는 현재 `/mnt/...`가 아니라 과거 `/home/...` 소스와 라이브러리를 가리킨다.
- 테스트 하네스는 12개 테스트가 모두 통과했지만 UI 문자열 조립·UTF-8 변환·실제 렌더링 경로를 포함하지 않아 이번 결함을 검출하지 못했다.

따라서 국소적인 원인 수정 방향은 명확하지만, 현재 실행 산출물은 기능적 신뢰성과 메모리 안전성 gate를 통과하지 못하므로 `PASS` 또는 `PASS WITH KNOWN RISKS`로 판정할 수 없다.

## 1. Audit Scope

### 1.1 프로젝트 및 기술 범위

- C++20 데스크톱 게임
- SFML 2.6.1 그래픽/윈도우/시스템 모듈
- nlohmann/json 3.11.3
- CMake 3.20 이상 빌드
- JSON 기반 5개 언어 리소스와 설정/세이브 파일

### 1.2 확인한 문서

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
- `audit_report_1.md`

### 1.3 확인한 구현/설정/자산

- `CMakeLists.txt`
- `include/`, `src/` 전체 인벤토리: 44개 파일, 약 7,399줄
- `src/core/Game.cpp`
- `src/core/LocalizationManager.cpp`
- `src/controller/TitleState.cpp`
- `src/controller/TownState.cpp`
- `src/controller/SettingsState.cpp`
- `src/controller/CombatState.cpp`
- `src/test_harness.cpp`
- `assets/lang/*.json` 5개
- `assets/fonts/PerfectDOSVGA437.ttf`
- `assets/fonts/neodgm.ttf`
- `build/CMakeCache.txt`, `build/Crawlmaster`, `build/TestHarness`

### 1.4 검사한 케이스와 명령

- `./TestHarness --run-all`: 12개 테스트 모두 통과
- `jq -e . assets/lang/*.json`: 5개 JSON 구문 모두 통과
- 5개 번역 파일의 키 집합 해시 비교: 모두 동일, 각 112개 키
- `rg`를 이용한 전체 `setString`, `fromUtf8`, `LocalizationManager::get` 호출 검색
- `fc-query`, `fc-scan`을 이용한 번들 폰트 대표 글리프 범위 검사
- 가상 X11에서 `build/Crawlmaster` 실행 후 타이틀 화면 캡처
- 가상 X11에서 Enter 입력 후 마을 화면 캡처 및 화면 손상 재현
- 현재 빌드 캐시의 소스/바이너리 경로 및 `ldd` 링크 경로 검사
- `/tmp/crawlmaster-audit-build`에 현재 소스를 사용한 fresh CMake 구성
- SFML 공식 문서와 C++ iterator range 규칙 그라운딩

### 1.5 런타임 재현 증거

- 타이틀 캡처: 1280x900 PNG, SHA-256 `74ddfedad04d316afeecef6c9e69fe5e4332be73cc880bcb793e7491175baf76`
- 마을 캡처: 1280x900 PNG, SHA-256 `7b72e3263cf079ea580077da876ce6a9365a258d68e105373fe9cf374b405cf5`
- 재현 순서:
  1. `build/`에서 `Crawlmaster` 실행
  2. 기본 한국어 타이틀에서 Enter 입력
  3. `TownState` 진입 직후 화면 전체에 임의 글리프·사각형·메모리성 문자열 출력 확인
- stderr에는 폰트 또는 번역 파일 로드 실패가 없었다. 따라서 재현된 대량 문자열 손상은 단순 자산 누락 증상과 구분된다.

## 2. Excluded Scope

- `.git/`: 현재 감사 샌드박스에서는 `.git`이 정상 저장소로 인식되지 않아 diff/commit 계보 검사를 수행하지 못했다.
- 실제 사용자의 Wayland/X11 세션: 사용자 화면을 직접 조작하지 않고 격리된 가상 X11에서 재현했다.
- 오디오: CMake에서 SFML Audio가 비활성화되어 있고 현재 가상 볼륨 기능만 있으므로 제외했다.
- 장시간 플레이를 통한 모든 전투/퀘스트 분기: 초기 마을 진입에서 결정적으로 결함이 재현되어 전체 수동 플레이는 중단했다.
- fresh sanitizer 전체 빌드: `/tmp` fresh CMake 구성은 성공했으나 감사 세션의 make jobserver 환경에서 전체 타깃 빌드가 완료되지 않아 sanitizer 판정에는 사용하지 않았다.
- 외부 취약점 스캐너/CI: 프로젝트에 제품용 CI 및 보안 스캐너 구성이 없으며 이번 감사에서는 네트워크 기반 스캔을 수행하지 않았다.
- `build/_deps/`: vendored dependency 소스 자체의 코드 품질은 감사하지 않았다.

## 3. Pass 1: Implementation Compliance Findings

### [IMP-F001] 5개 언어 실시간 출력 완료 주장과 언어 분기 구현이 불일치

- **Pass:** Implementation
- **Pattern:** IMP-001, IMP-003
- **Area:** i18n 계약, 문서-구현 정합성
- **Severity:** Major
- **Status:** Needs Fix
- **Summary:** 마을 동작 메시지와 일부 메뉴가 현재 언어 enum이 아니라 존재하지 않는 번역 키 `"Language"`로 언어를 판별한다.
- **Evidence:**
  - `TownState.cpp:68,86,92,99,135,137,157,160,171,179,181,199,207,209,221,230,232,336,341,345,356`
  - `LocalizationManager::get()`은 누락 키를 키 문자열 자체로 반환한다 (`LocalizationManager.cpp:46-53`).
  - 5개 언어 JSON의 키 집합은 동일하지만 `Language` 키는 없다.
  - `spec.md:31,442-447`, `audit_roadmap.md:16,26,51-54`, `IMPLEMENTATION_SUMMARY.md:125`는 5개 언어 실시간 UI 출력을 완료로 주장한다.
- **Expected:** 현재 언어가 KO이면 한국어, EN/JA/ZH_TW/ZH_CN이면 각각 해당 번역 리소스를 사용해야 한다.
- **Actual:** `lm.get("Language")`는 항상 문자열 `"Language"`를 반환하므로 `== "ko"`는 항상 거짓이다. 해당 경로는 모든 언어에서 영문 또는 영문 혼합 문구를 출력한다.
- **Impact:** 문서상 핵심 완료 기준인 5개 언어 지원을 위반하며 한국어 화면에도 예상하지 않은 영문 메시지가 나타난다.
- **Suggested Fix:** 언어 분기를 `getLanguage()` enum 비교로 단일화하거나, 모든 사용자 표시 문자열을 언어 JSON 키로 이동하여 조건 분기 자체를 제거한다.
- **Re-audit Method:** 5개 언어 각각에서 길드 생성/해고, 상점 구매/판매, 교회, 두 퀘스트 수락·진행·완료 메시지를 캡처 비교한다.
- **Owner:** Coder
- **Notes:** 단순 키 추가만으로는 일본어/중국어가 한국어/영어 이분법에 갇히므로 5개 언어 계약을 복구할 수 없다.

### [IMP-F002] 번들 공용 폰트가 일본어와 중국어 표시 계약을 충족하지 않음

- **Pass:** Implementation
- **Pattern:** IMP-001, BUILD-001
- **Area:** 다국어 폰트 자산, runtime packaging
- **Severity:** Major
- **Status:** Needs Fix
- **Summary:** 게임은 모든 언어에서 단일 `neodgm.ttf`를 사용하지만 이 폰트는 감사한 대표 일본어/중국어 코드포인트를 포함하지 않는다.
- **Evidence:**
  - `Game.cpp:89-103`은 언어와 무관하게 `neodgm.ttf` 하나를 우선 로드한다.
  - `fc-query` 결과 `neodgm.ttf`는 한글 음절 범위 `AC00-D7A3`를 포함하지만 대표 히라가나 `U+3042`와 CJK 통합 한자 `U+4E00`은 확인되지 않았다.
  - `PerfectDOSVGA437.ttf`에서도 해당 대표 글리프가 확인되지 않았다.
  - 번역 자산은 `ja.json`, `zh_tw.json`, `zh_cn.json`을 포함하고 문서는 5개 언어 정상 표시를 주장한다.
- **Expected:** 번들 자산만으로 한국어, 영어, 일본어, 중국어 번체/간체의 실제 문자열을 모두 표시해야 한다.
- **Actual:** 일본어·중국어 전환 시 다수 글리프가 대체 사각형 또는 누락 글리프로 표시될 가능성이 결정적이다.
- **Impact:** 3개 번역 리소스가 존재해도 사용자 화면은 읽을 수 없으며 5개 언어 완료 기준을 충족하지 못한다.
- **Suggested Fix:** 지원 문자 전체를 포함하는 재배포 가능한 폰트를 번들하거나 언어별 폰트/명시적 fallback 전략을 설계한다. 시작 시 각 번역 리소스의 고유 코드포인트에 대해 `sf::Font::hasGlyph` 검증을 수행하는 방안도 포함한다.
- **Re-audit Method:** 5개 JSON의 모든 문자열 코드포인트를 선택된 폰트의 glyph set과 대조하고, 5개 언어별 대표 화면을 캡처한다.
- **Owner:** Architect / Coder
- **Notes:** 폰트 라이선스와 배포 크기 정책은 문서에 명시되어야 한다.

### [IMP-F003] `config.json` 동결 계약과 구현 키/기본값이 불일치

- **Pass:** Implementation
- **Pattern:** IMP-001, ARCH-002
- **Area:** 설정 계약, runtime defaults
- **Severity:** Major
- **Status:** Needs Fix
- **Summary:** `spec.md`의 동결 계약은 camelCase와 기본값 60/80을 사용하지만 구현은 snake_case와 50/50을 사용한다.
- **Evidence:**
  - `spec.md:294-297,341-347`: `bgmVolume`, `sfxVolume`, 기본 60/80.
  - `LocalizationManager.cpp:11-14`: 기본 50/50.
  - `LocalizationManager.cpp:96-100,115-118`: `bgm_volume`, `sfx_volume` 읽기/쓰기.
- **Expected:** `spec.md`를 마스터플랜으로 하여 문서화된 키와 기본값을 정확히 읽고 쓴다.
- **Actual:** 문서 예제대로 만든 설정 파일은 언어만 적용되고 볼륨 값은 무시된다. 설정 파일이 없을 때도 문서와 다른 기본값이 사용된다.
- **Impact:** 외부 설정 호환성과 초기 사용자 경험이 문서와 다르다.
- **Suggested Fix:** `spec.md` 계약을 기준으로 구현을 정렬하고, 이미 생성된 snake_case 파일이 배포된 적이 있다면 명시적 1회 마이그레이션 정책을 추가한다.
- **Re-audit Method:** 문서 예제 파일 로드, 무설정 초기값, 저장 후 재로드, 구형 키 마이그레이션을 각각 검증한다.
- **Owner:** Architect / Coder
- **Notes:** 키 이름 변경 여부는 `SPEC_IS_LAW`에 따라 문서를 먼저 확정해야 한다.

### [IMP-F004] 버전 권위가 0.1.0, 0.9.0, 0.9.1로 분산됨

- **Pass:** Implementation
- **Pattern:** IMP-002, BUILD-001
- **Area:** 버전 파일, release authority
- **Severity:** Major
- **Status:** Needs Documentation Recovery
- **Summary:** CMake 프로젝트 버전, 감사 로드맵, 변경 이력의 현재 버전이 서로 다르다.
- **Evidence:**
  - `CMakeLists.txt:3`: `VERSION 0.1.0`
  - `audit_roadmap.md:1-3`: 최종 완료 버전 `v0.9.0`
  - `CHANGELOG.md:7`: 최신 버전 `0.9.1`
  - `README.md`와 `IMPLEMENTATION_SUMMARY.md`도 v0.9.1 기능을 완료로 기술한다.
- **Expected:** 모든 version authority가 단일 SemVer를 가리키고 버전 상승 시 감사 로드맵이 재생성되어야 한다.
- **Actual:** 빌드 메타데이터와 감사 기준이 최신 문서보다 뒤처져 있다.
- **Impact:** 산출물 식별, 재현, 변경 범위 추적, 재감사 기준의 신뢰성이 낮아진다.
- **Suggested Fix:** 현재 릴리스 버전을 확정한 뒤 CMake/로드맵/README/변경 이력/코드 버전 주석을 동기화한다.
- **Re-audit Method:** 모든 버전 문자열 검색과 빌드 산출물의 버전 확인을 재수행한다.
- **Owner:** Architect
- **Notes:** 현재 증거상 0.9.1이 가장 최신 주장이나, 인간 owner의 릴리스 확정이 필요하다.

### [IMP-F005] 폰트 문서가 실제 우선 로드 자산과 불일치

- **Pass:** Implementation
- **Pattern:** DOC-BACKFILL-001
- **Area:** 디자인/빌드 문서
- **Severity:** Minor
- **Status:** Needs Documentation Recovery
- **Summary:** 디자인과 빌드 문서는 PerfectDOS를 기본/필수 자산으로 규정하지만 구현과 README는 neodgm을 우선 사용한다.
- **Evidence:** `designs.md:19`, `BUILD_GUIDE.md:47,56,73`, `spec.md:77` 대 `Game.cpp:89-103`, `README.md:39,68`.
- **Expected:** 디자인 토큰, 패키징 체크리스트, 런타임 로드 순서가 같은 폰트 정책을 설명한다.
- **Actual:** 문서에 따라 점검하면 실제 필수 한글 폰트를 누락해도 빌드 체크를 통과할 수 있다.
- **Impact:** 배포 자산 누락 및 다국어 회귀 가능성이 커진다.
- **Suggested Fix:** `spec.md`에서 5개 언어 폰트 전략을 확정하고 `designs.md`, `BUILD_GUIDE.md`, README를 동기화한다.
- **Re-audit Method:** 문서의 필수 자산 목록과 `Game::loadResources`, 빌드 산출물의 폰트 목록을 대조한다.
- **Owner:** Architect
- **Notes:** IMP-F002 해결 정책과 함께 처리해야 한다.

## 4. Pass 2: Debug / Engineering Quality Findings

### [DBG-F001] 서로 다른 임시 문자열의 iterator pair가 화면 전체 문자열 손상을 유발

- **Pass:** Debug
- **Pattern:** DBG-001, DBG-002, TEST-001
- **Area:** UTF-8 변환, 객체 수명, 런타임 렌더링
- **Severity:** Major
- **Status:** Needs Fix
- **Summary:** 사용자 제보 현상의 직접 원인이다. `fromUtf8`에 유효하지 않은 iterator 범위를 전달한다.
- **Evidence:**
  - `TownState.cpp:298,310,318,326,340,360,367`
  - 각 호출은 `lm.get(KEY).begin()`과 별도의 `lm.get(KEY).end()`를 사용한다. `get()`은 `std::string`을 값으로 반환하므로 두 iterator는 서로 다른 임시 객체에 속한다.
  - 마을 진입 재현에서 제목 이후 화면 전체에 임의 글리프와 문자열 조각이 출력되었다.
  - C++ iterator 규칙은 sentinel이 iterator에서 도달 가능할 때만 유효한 범위이며 invalid range에 대한 라이브러리 함수 적용 결과는 undefined라고 규정한다: [C++ draft — iterator requirements](https://eel.is/c%2B%2Bdraft/iterator.requirements).
  - SFML 2.6.1은 `fromUtf8(begin, end)`가 하나의 UTF-8 sequence 시작/끝 forward iterator를 받는다고 명시한다: [SFML 2.6.1 sf::String](https://www.sfml-dev.org/documentation/2.6.1/classsf_1_1String.php).
- **Expected:** 하나의 생존한 UTF-8 문자열 객체에서 얻은 `[begin, end)` 범위를 전달한다.
- **Actual:** 서로 도달 불가능한 iterator pair를 전달해 변환기가 임의 메모리를 계속 읽는다.
- **Impact:** 화면 손상, 예측 불가능한 메모리 읽기, 크래시 가능성, 플랫폼/최적화별 비결정적 동작.
- **Suggested Fix:** 번역 결과를 로컬 `std::string`에 한 번 저장한 뒤 그 동일 객체의 `begin()`/`end()`를 사용한다. 중복 방지를 위해 검증된 UTF-8 변환 helper를 한 곳에 두는 것이 적합하다.
- **Re-audit Method:** 7개 호출을 모두 재검색하고, ASan/UBSan 빌드에서 마을의 HUB/GUILD/SHOP/SHOP_BUY/SHOP_SELL/TEMPLE/CASTLE 전환을 반복 실행하며 화면 캡처와 sanitizer 로그를 확인한다.
- **Owner:** Coder
- **Notes:** 현상은 재현되었으므로 `Needs Spec Clarification`이 아니라 명확한 `Needs Fix`다.

### [DBG-F002] 전투 제목만 UTF-8 경로를 우회하여 ANSI 변환됨

- **Pass:** Debug
- **Pattern:** DBG-001
- **Area:** UTF-8 변환 일관성
- **Severity:** Minor
- **Status:** Needs Fix
- **Summary:** 전투 제목은 다른 전투 텍스트와 달리 `sf::String::fromUtf8`를 거치지 않는다.
- **Evidence:**
  - `CombatState.cpp:792`: `m_headerText.setString(LocalizationManager::getInstance().get("COMBAT_TITLE"));`
  - 같은 함수의 `813,860,870`은 로컬 문자열을 `fromUtf8`로 변환한다.
  - SFML 2.6.1의 `std::string` 생성자는 입력을 ANSI 문자열로 보고 locale에 따라 UTF-32로 변환한다: [SFML 2.6.1 sf::String](https://www.sfml-dev.org/documentation/2.6.1/classsf_1_1String.php).
- **Expected:** JSON에서 읽은 UTF-8 텍스트는 모든 화면에서 동일한 UTF-8 변환 경로를 사용한다.
- **Actual:** 전투 제목 하나만 locale 의존 ANSI 변환을 사용한다.
- **Impact:** 한국어/일본어/중국어 제목이 locale에 따라 깨지거나 손실될 수 있다.
- **Suggested Fix:** DBG-F001과 동일한 중앙 UTF-8 변환 helper를 사용한다.
- **Re-audit Method:** `setString` 전체 검색 후 비ASCII 입력이 직접 `std::string`으로 전달되는 경로가 없는지 확인하고 5개 언어 전투 화면을 캡처한다.
- **Owner:** Coder
- **Notes:** ASCII 전용 로고/몬스터 아트의 직접 `setString`은 이 finding 대상이 아니다.

### [DBG-F003] i18n 테스트가 렌더링 경로를 검증하지 않아 중대 회귀를 통과시킴

- **Pass:** Debug
- **Pattern:** TEST-001, DBG-002
- **Area:** 회귀 테스트, verification authority
- **Severity:** Major
- **Status:** Needs Fix
- **Summary:** `testLocalizationI18n`은 번역 map과 설정 저장만 검사하고 State 문자열 조립과 SFML 변환을 전혀 호출하지 않는다.
- **Evidence:**
  - `src/test_harness.cpp:564-610`은 KO/EN/JA 일부 키 조회, 없는 키 fallback, config 저장/로드만 검사한다.
  - `TestHarness` 타깃은 `TownState.cpp`, `CombatState.cpp`, SFML graphics를 포함하지 않는다 (`CMakeLists.txt`의 `TEST_SOURCES`).
  - `./TestHarness --run-all`의 12개 테스트가 모두 통과했지만 직후 실제 마을 화면은 손상되었다.
  - `audit_roadmap.md:16,26,29`와 `IMPLEMENTATION_SUMMARY.md:125`는 이 테스트를 근거로 실시간 비주얼 출력까지 완료·검증되었다고 과대 주장한다.
- **Expected:** 과거 한글 깨짐과 실시간 언어 전환을 핵심 위험으로 관리한다면 State별 문자열 변환과 실제 glyph 렌더링을 직접 잠그는 회귀 테스트가 있어야 한다.
- **Actual:** broad headless smoke가 view 경로를 제외해 잘못된 PASS authority가 되었다.
- **Impact:** 런타임 UI가 심각하게 깨져도 자동 검증은 100% 통과한다.
- **Suggested Fix:** 문자열 조립/UTF-8 변환을 테스트 가능한 helper로 분리하고, 최소한 Town 7개 substate와 Combat header를 대상으로 유효 UTF-32 출력·예상 길이·replacement glyph 여부를 검증한다. 별도 Xvfb UI smoke도 권장한다.
- **Re-audit Method:** 결함 코드를 되돌렸을 때 새 테스트가 실패하는지 확인하고, 수정 후 headless + Xvfb 테스트를 모두 실행한다.
- **Owner:** Coder / Auditor
- **Notes:** 테스트 수보다 실패 모드 직접성이 우선이다.

### [DBG-F004] 기존 빌드 캐시와 바이너리가 다른 소스 루트를 참조

- **Pass:** Debug
- **Pattern:** BUILD-001
- **Area:** build cache, artifact path, reproducibility
- **Severity:** Major
- **Status:** Needs Fix
- **Summary:** 현재 프로젝트의 `build/`가 과거 `/home/eunho1/Projects/cpp/crawlmaster`에서 생성된 캐시와 라이브러리를 사용한다.
- **Evidence:**
  - 현재 경로: `/mnt/Projects_SSD/cpp/crawlmaster`
  - `build/CMakeCache.txt:333,339,1046`: source/binary/home이 `/home/eunho1/Projects/cpp/crawlmaster`.
  - `ldd build/Crawlmaster`: SFML 3개 라이브러리를 `/home/eunho1/Projects/cpp/crawlmaster/build/_deps/...`에서 로드.
  - `/tmp/crawlmaster-audit-build`에서 현재 `/mnt/...`를 소스로 fresh configure하는 것은 성공했다.
- **Expected:** 현재 작업공간에서 문서화된 명령으로 생성한 빌드는 현재 소스와 자체 산출물만 참조한다.
- **Actual:** 실행 성공 여부가 별도의 과거 디렉터리 존재에 의존한다.
- **Impact:** 소스와 바이너리 불일치, 다른 머신/경로에서 실행 실패, 재현성 상실.
- **Suggested Fix:** 코드를 고친 뒤 현재 경로에서 fresh build 디렉터리를 생성하고 외부 절대 경로가 남지 않는지 검증한다. 기존 문서를 보존하면서 생성 산출물만 교체해야 한다.
- **Re-audit Method:** fresh configure/build, `CMakeCache.txt`, `ldd`, 자산 복사 경로, 현재 소스 변경 반영 여부를 확인한다.
- **Owner:** Coder / Build Maintainer
- **Notes:** 현재 증상 재현에는 기존 바이너리를 사용했으므로, 수정 검증은 반드시 fresh binary에서 해야 한다.

## 5. Pass 3: Security Findings

### [SEC-F001] invalid UTF-8 range가 프로세스 메모리를 화면에 노출할 수 있음

- **Pass:** Security
- **Pattern:** SEC-004
- **Area:** memory safety, unintended disclosure
- **Severity:** Major
- **Status:** Needs Fix
- **Summary:** DBG-F001의 미정의 동작은 단순 시각 오류를 넘어 문자열 경계 밖 프로세스 메모리를 읽고 glyph로 출력할 수 있다.
- **Evidence:** 마을 재현 화면에는 정상 번역과 무관한 임의 영문 조각과 기호가 광범위하게 나타났다. C++ iterator 규칙상 invalid range에 대한 라이브러리 함수 적용 결과는 undefined다.
- **Expected:** UI renderer는 소유하고 길이가 확정된 문자열 범위만 읽어야 한다.
- **Actual:** 종료 iterator에 도달할 보장이 없어 메모리 읽기 범위가 통제되지 않는다.
- **Impact:** 현재는 로컬 데스크톱 앱이지만 파티 이름, 로그, 설정 등 동일 프로세스 메모리 일부가 의도치 않게 화면에 드러날 가능성이 있다. 크래시 가능성도 있다.
- **Suggested Fix:** DBG-F001을 우선 수정하고 ASan/UBSan으로 모든 변환 경로를 검증한다. 사용자/세이브 데이터가 화면에 나타나는 경로이므로 회귀 테스트를 보안 gate에 포함한다.
- **Re-audit Method:** sanitizer 실행, substate 반복 전환, 긴/비ASCII 파티 이름 및 5개 언어 입력으로 OOB read가 없는지 확인한다.
- **Owner:** Coder / Auditor
- **Notes:** 원격 입력 표면이 없다는 사실은 메모리 안전 결함 자체를 면제하지 않는다.

### [SEC-F002] 네트워크·셸 실행 표면은 현재 제품 코드에서 확인되지 않음

- **Pass:** Security
- **Pattern:** SEC-003, SEC-004, SEC-007
- **Area:** network bind, shell execution, filesystem boundary
- **Severity:** Info
- **Status:** Verified
- **Summary:** 제품 소스에서 socket/bind/listen, shell/system/popen/exec 경로를 확인하지 못했다.
- **Evidence:** `src/`, `include/` 전체 검색. 제품 파일 I/O는 기본적으로 `./save.json`, `./config.json`, 번역/폰트 자산에 한정된다.
- **Expected:** 로컬 전용 데스크톱 범위에서는 원격 무인증 표면과 임의 셸 실행 경로가 없어야 한다.
- **Actual:** 해당 표면은 확인되지 않았다.
- **Impact:** 네트워크 기반 공격 표면은 낮다.
- **Suggested Fix:** 없음. 향후 네트워크/플러그인/사용자 지정 경로 기능 추가 시 별도 보안 설계를 수행한다.
- **Re-audit Method:** 새 dependency와 I/O 경로를 재검색하고 release artifact를 검사한다.
- **Owner:** Auditor
- **Notes:** dependency CVE/scanner 검사는 제외 범위이므로 공급망 전체 PASS를 의미하지 않는다.

## 6. Cross-Pass Conflicts

### [XPF-F001] 문서·단위 테스트의 완료 주장과 실제 UI 실행 결과가 충돌

- **Related Findings:** IMP-F001, IMP-F002, DBG-F001, DBG-F003, SEC-F001
- **Conflict:** `audit_roadmap.md`, `IMPLEMENTATION_SUMMARY.md`, `CHANGELOG.md`, `audit_report_1.md`는 i18n/비주얼 갱신과 전체 검증을 완료 또는 PASS로 주장한다. 그러나 실제 TownState 진입은 화면 전체 손상으로 재현되었고 테스트는 해당 경로를 포함하지 않는다.
- **Resolution:** runtime evidence와 실제 호출 경로를 우선한다. 관련 완료/PASS 주장은 수정 및 회귀 증거 확보 전까지 유효하지 않다.
- **Gate Impact:** 전체 판정 `HOLD`.
- **Required Fix Before PASS:** DBG-F001, DBG-F003, SEC-F001을 우선 해소하고 5개 언어 실제 렌더링 검증을 추가한다.

### [XPF-F002] 이전 감사의 범위 제외가 잘못된 PASS를 만들었음

- **Related Findings:** DBG-F001, DBG-F003, DBG-F004
- **Conflict:** `audit_report_1.md:17`은 확인 코드를 `test_harness.cpp`, `save.json`으로 제한했고 `build/`와 `assets/`를 제외했음에도 `audit_report_1.md:154-155`에서 모든 기준 PASS를 선언했다.
- **Resolution:** UI/runtime 결함 감사에서는 controller/view, 실제 폰트/번역 자산, 실행 바이너리를 필수 범위로 포함해야 한다. 이전 PASS는 현재 증거로 기각한다.
- **Gate Impact:** `audit_report_1.md`의 PASS는 현재 상태의 검증 authority로 사용할 수 없다.
- **Required Fix Before PASS:** 이번 보고서의 범위로 재감사하고 이전 finding lineage를 명시한다.

## 7. Required Fixes Before PASS

1. `TownState.cpp`의 7개 invalid iterator pair를 모두 제거하고 동일 생존 문자열의 범위만 사용한다.
2. UTF-8 변환을 단일 helper 또는 동등한 choke point로 통합하고 Combat 제목을 포함한 비ASCII 직접 `setString(std::string)` 경로를 제거한다.
3. Town 7개 substate와 Combat header를 직접 검증하는 실패모드 회귀 테스트를 추가한다.
4. 5개 언어의 모든 번역 글리프를 표시할 폰트/fallback 정책을 확정하고 번들·문서를 동기화한다.
5. 존재하지 않는 `Language` 키 기반 이분법을 제거하고 5개 언어별 메시지를 번역 리소스로 일원화한다.
6. `config.json` 키와 60/80 기본값을 `spec.md` 동결 계약에 맞추고 필요하면 구형 키 마이그레이션을 추가한다.
7. 0.1.0/0.9.0/0.9.1 버전 권위를 하나로 동기화하고 `audit_roadmap.md`를 현재 버전 기준으로 갱신한다.
8. 현재 `/mnt/...` 소스에서 fresh build를 생성하여 외부 `/home/...` 절대 경로 의존을 제거한다.
9. ASan/UBSan, 전체 TestHarness, Xvfb 5개 언어 UI smoke를 fresh binary에서 통과시킨다.
10. 변경 후 `spec.md`, `designs.md`, `README.md`, `CHANGELOG.md`, `BUILD_GUIDE.md`, `IMPLEMENTATION_SUMMARY.md`, `LESSONS_LEARNED.md`, `DESIGN_DECISIONS.md`, `audit_roadmap.md`를 프로젝트 규칙에 맞게 동기화한다.

## 8. Accepted Risks

- **없음.**
- 기존 문서의 SFML 소프트웨어 렌더링 성능 한계는 제품 trade-off로 기술되어 있으나, `AI_AUDIT_DOC_STANDARD.md`가 요구하는 owner, 만료 조건, 재검토 조건이 없어 이번 보고서에서는 정식 `Accepted Risk`로 승격하지 않았다.
- 화면 문자열 손상과 메모리 안전 문제는 기능 의도나 레트로 연출로 수용할 수 있는 위험이 아니다.

## 9. Needs Spec Clarification

### [NSC-001] 5개 언어 폰트 및 fallback 정책

- 단일 범용 폰트를 번들할지, 언어별 폰트를 선택할지, 여러 폰트를 조합할지 명세가 필요하다.
- 필수 글리프 범위, 라이선스, 배포 크기, 폰트 누락 시 시작 실패/경고/fallback 정책을 확정해야 한다.

### [NSC-002] 현재 릴리스 버전 authority

- 증거상 최신 기능 문서는 0.9.1이나 CMake는 0.1.0이고 감사 로드맵은 0.9.0이다.
- 인간 owner가 현재 릴리스 기준을 확정한 뒤 모든 version file을 동기화해야 한다.

## 10. Re-audit Checklist

- [ ] `rg` 검색에서 서로 다른 임시 객체의 `begin()`/`end()` 조합 0건
- [ ] 비ASCII `setString(std::string)` 직접 전달 0건
- [ ] ASan/UBSan으로 HUB/GUILD/SHOP/SHOP_BUY/SHOP_SELL/TEMPLE/CASTLE 반복 전환 통과
- [ ] 한국어/영어/일본어/번체/간체 타이틀·마을·설정·전투 화면 캡처 통과
- [ ] 5개 번역 JSON 전체 코드포인트에 대응 glyph 존재
- [ ] 5개 언어별 길드/상점/교회/퀘스트 동작 메시지 정상 번역
- [ ] 문서 예시 `config.json` 로드 및 60/80 기본값 통과
- [ ] current version이 CMake/CHANGELOG/README/로드맵에 일치
- [ ] fresh `build/`의 CMake source root와 `ldd`가 `/mnt/Projects_SSD/cpp/crawlmaster` 기준으로 자급적
- [ ] `TestHarness --run-all` 및 새 UI 회귀 테스트 통과
- [ ] 이전 `audit_report_1.md` PASS와 이번 finding의 lineage가 후속 보고서에 명시됨

## 11. 판단 근거 및 남은 리스크

- 사용자 증상은 실제 실행으로 재현되었고 source-level 원인이 7개 호출로 국소화되었다.
- 단위 테스트 통과는 view/runtime 경로의 안전성을 증명하지 않는다.
- 번역 키셋 자체는 5개 파일에서 동일하지만, 키셋 일치가 폰트 coverage·문자열 변환·실제 렌더링을 증명하지 않는다.
- 기존 바이너리는 과거 절대 경로에 의존하므로 수정 후 검증에 재사용하면 안 된다.
- ASan/UBSan 전체 결과와 5개 언어 end-to-end 캡처가 아직 없으므로 수정 후에도 재감사가 필수다.

## 12. Final Decision

**HOLD**

사유:

- 미해결 Major 기능 결함: 실제 화면 전체 문자열 손상
- 미해결 Major 메모리 안전 결함: invalid iterator range
- 미해결 Major i18n 계약 불일치: 언어 분기와 폰트 coverage
- 미해결 Major 검증 공백: 테스트 100% 통과와 실제 UI 실패의 충돌
- 미해결 Major 빌드 재현성 문제: 과거 절대 경로 의존

수정 범위는 비교적 명확하므로 현 단계에서 `REWORK REQUIRED`까지 확대할 근거는 없다. Required Fixes를 반영하고 fresh build + sanitizer + 5개 언어 UI 회귀 증거가 확보되면 3-pass 관련 영역을 재감사하여 PASS 여부를 다시 판정한다.
