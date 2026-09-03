# D3D 독립 재감사 보고서 — v0.9.2 핫픽스 검증

- **감사 일자:** 2026-07-13 (Asia/Seoul)
- **프로젝트 경로:** `/mnt/Projects_SSD/cpp/crawlmaster`
- **감사 기준:** `AI_AUDIT_DOC_STANDARD.md`
- **이전 독립 감사:** `audit_report_3.md`
- **중간 구현측 보고서:** `audit_report_4.md`
- **재감사 회차:** Re-audit #2
- **실행 방식:** Implementation Compliance / Debug & Engineering Quality / Security 3-pass
- **코드 수정 여부:** 없음
- **최종 판정:** **HOLD**

## 0. 감사 요약

v0.9.2 핫픽스는 이전 화면 손상의 직접 원인이던 invalid iterator pair를 올바르게 제거했다. 한국어 마을 화면을 실제 실행한 결과, 이전에 사라지거나 임의 글리프로 오염되던 마을 제목이 `=== 마을 (TOWN CAMP) ===`로 정상 표시됐다. `CombatState`의 ANSI 변환 경로, config 키/기본값, version authority도 코드와 문서에서 정렬됐다.

그러나 `audit_report_4.md`의 전체 PASS는 현재 증거로 유지할 수 없다.

- `getSf()`는 UTF-8 문자열을 안전한 `sf::String`으로 변환할 뿐 폰트에 없는 glyph를 추가하지 않는다.
- 실제 일본어 설정 화면은 제목, 언어명, 메뉴, 가이드 대부분이 사각형 대체 글리프로 출력됐다.
- `neodgm.ttf`는 ASCII와 한글 `AC00-D7A3`만 확인되며 대표 일본어 `U+3042`, CJK `U+4E00`을 포함하지 않는다.
- 신규 `testTownAndCombatUIStatesI18nSafety()`는 이름과 달리 `TownState`/`CombatState` 구현을 링크·생성·렌더링하지 않고 `LocalizationManager::getSf()`의 비어 있지 않은 반환값만 검사한다.
- 일부 새 일본어/번체 문자열에는 한국어 조사 `의`가 혼입되어 있다.
- 새 사용자 메시지를 JSON 번역 리소스가 아닌 `TownState.cpp`의 5개 인자 하드코딩으로 추가하여 `spec.md`의 “모든 UI 텍스트를 LocalizationManager 리소스 키로 획득” 계약과 어긋난다.

따라서 UB와 연결된 메모리 안전 finding은 해소됐지만, 5개 언어 실제 출력과 회귀 검증이라는 Major gate가 남아 전체 판정은 `HOLD`다.

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
- `audit_report_1.md` ~ `audit_report_4.md`

### 1.2 확인한 구현·자산·산출물

- `CMakeLists.txt`
- `include/core/LocalizationManager.hpp`
- `src/core/LocalizationManager.cpp`
- `src/core/Game.cpp`
- `src/controller/TownState.cpp`
- `src/controller/CombatState.cpp`
- `src/controller/SettingsState.cpp`
- `src/test_harness.cpp`
- `assets/lang/*.json`
- `assets/fonts/PerfectDOSVGA437.ttf`
- `assets/fonts/neodgm.ttf`
- `build/CMakeCache.txt`
- `build/Crawlmaster`
- `build/TestHarness`

### 1.3 실행한 검사와 결과

- invalid `fromUtf8` temporary iterator pair 검색: 0건
- 누락 `Language` 번역 키 분기 검색: 0건
- 비ASCII Combat 제목 직접 ANSI 전달 검색: 0건
- `cmake --build build --target Crawlmaster TestHarness -j2`: 통과
- `./TestHarness --run-all`: 13개 테스트 통과
- 5개 JSON 구문·키 수·키 집합 비교: 각 112키, 동일 집합
- `fc-query` 번들 폰트 대표 codepoint coverage 검사
- build cache 경로와 실제 inode/realpath 검사
- 가상 X11 한국어 TownState 실행 및 캡처
- 가상 X11 일본어 SettingsState 실행 및 캡처
- SFML 2.6.1 `sf::String`, `sf::Font::hasGlyph` 공식 문서 재확인

### 1.4 런타임 증거

#### 한국어 마을 화면

- 캡처 SHA-256: `1baa732ac4a7f5340113c6cf2353654d2b5e04b15e721ee4495c330b682bb4a7`
- 결과: 마을 제목, 메뉴, 파티, 가방 정보 정상 표시
- 판정: 기존 UB/제목 소실 회귀는 이 경로에서 해소됨

#### 일본어 설정 화면

- 캡처 SHA-256: `a1539a021ff3641ed3cbba8d7fc6bbc1cee15b70bc53d15fe117debf954b0c`
- 결과: 레이아웃과 ASCII 숫자/영문은 표시되나 일본어 제목·메뉴·가이드가 다수 대체 사각형으로 출력
- 판정: 5개 언어 glyph 출력 완료 주장은 실패

## 2. Excluded Scope

- 실제 사용자 데스크톱: 격리된 Xvfb에서 검증했다.
- 전체 전투/퀘스트 장시간 플레이: 5개 언어 Major gate 실패가 설정 화면에서 확인되어 전체 수동 시나리오는 중단했다.
- dependency CVE scanner/CI: 제품용 scanner와 CI가 없어 실행하지 않았다.
- `build/_deps` dependency 내부 코드: 제품 구현 범위에서 제외했다.
- ASan/UBSan: 이번 수정의 소스 범위와 한국어 runtime은 확인했지만 별도 sanitizer build는 실행하지 않았다.

## 3. Pass 1: Implementation Compliance Findings

### [IMP-F001] Re-audit #2 — 언어 분기 결함은 제거됐지만 번역 리소스 계약과 일부 번역 품질이 불일치

- **Pass:** Implementation
- **Pattern:** IMP-001, ARCH-002
- **Area:** i18n source of truth, 5개 언어 메시지
- **Severity:** Major
- **Status:** Needs Fix — 부분 해소
- **Summary:** 존재하지 않는 `Language` 키를 사용하던 분기는 제거됐으나 새 메시지가 JSON 밖에 하드코딩됐고 일부 언어 문자열에 한국어가 혼입됐다.
- **Evidence:**
  - `TownState.cpp:19-30`: 파일 내부 `selectLang(ko,en,ja,tw,cn)` helper.
  - `TownState.cpp:82-246`: 사용자 메시지를 5개 문자열 인자로 직접 반복 전달.
  - `TownState.cpp:185`: 번체 문자열 `隊伍中沒有需要治療의 勇士`에 한국어 조사 `의` 혼입.
  - `TownState.cpp:244`: 일본어 문자열 `150XP의報酬`에 한국어 조사 `의` 혼입.
  - `spec.md:444`: 모든 UI 텍스트 출력부는 `LocalizationManager`에서 다국어 리소스 키를 획득하도록 규정.
- **Expected:** 사용자-facing 텍스트는 5개 JSON의 동일 key로 관리되고 각 언어 문자열이 해당 언어만 포함한다.
- **Actual:** 새 메시지는 controller source에 하드코딩되며 두 문자열은 언어가 혼합됐다.
- **Impact:** 번역 검수·누락 검사가 JSON key parity로 포착되지 않고 특정 이벤트에서 잘못된 텍스트가 노출된다.
- **Suggested Fix:** 모든 `selectLang` 메시지를 5개 JSON의 명명된 key로 이동하고 동일 key 집합 및 언어별 script 검사를 추가한다.
- **Re-audit Method:** source의 사용자 문자열 하드코딩 검색, JSON key parity, 길드/상점/교회/퀘스트 5개 언어 이벤트 캡처.
- **Owner:** Coder / Localization Owner
- **Notes:** 원래의 항상-영문 분기 결함은 해결됐으나 finding 전체 gate는 아직 충족하지 못했다.

### [IMP-F002] Re-audit #2 — 일본어·중국어 폰트 coverage 미해결

- **Pass:** Implementation
- **Pattern:** IMP-001, BUILD-001
- **Area:** multilingual font assets
- **Severity:** Major
- **Status:** Needs Fix — 미해결
- **Summary:** `getSf()` 도입은 encoding safety를 해결했지만 glyph coverage는 해결하지 않았다.
- **Evidence:**
  - `Game.cpp:89-103`: 모든 언어에서 `neodgm.ttf` 단일 폰트를 우선 사용.
  - `fc-query`: `neodgm.ttf` coverage는 `20-7E`, `AC00-D7A3`; 대표 `U+3042`, `U+4E00` 없음.
  - 일본어 설정 runtime에서 대체 사각형 다수 재현.
  - SFML 2.6.1 공식 문서는 codepoint에 glyph가 없으면 font-specific default가 반환되며, 특정 언어 적합성은 `hasGlyph`로 확인할 수 있다고 설명한다: [SFML 2.6.1 sf::Font](https://www.sfml-dev.org/documentation/2.6.1/classsf_1_1Font.php).
- **Expected:** 한국어, 영어, 일본어, 번체, 간체의 실제 번역 문자열이 모두 읽을 수 있게 표시된다.
- **Actual:** 일본어는 실제 화면에서 읽을 수 없으며 중국어도 같은 font coverage 근거로 실패한다.
- **Impact:** 제품 핵심 완료 기준인 5개 언어 지원 불충족.
- **Suggested Fix:** 전체 번역 codepoint를 포함하는 배포 가능 폰트 또는 언어별 font/fallback 전략을 구현하고 `hasGlyph` 기반 검증을 추가한다.
- **Re-audit Method:** 5개 JSON 전체 codepoint-to-glyph 검사와 각 언어 Title/Town/Settings/Combat 캡처.
- **Owner:** Architect / Coder
- **Notes:** `audit_report_4.md`의 “getSf로 glyph 리스크 해소” 주장은 encoding과 glyph를 혼동한 false positive다.

### [IMP-F003] Re-audit #2 — config 계약 정렬

- **Pass:** Implementation
- **Pattern:** IMP-001, ARCH-002
- **Area:** config keys and defaults
- **Severity:** Info
- **Status:** Verified — 해결
- **Summary:** camelCase 계약, 기본 60/80, 구형 snake_case fallback이 구현됐다.
- **Evidence:** `LocalizationManager.cpp:12,101-109,124-127`; 일본어 Settings runtime에서도 60/80 표시.
- **Expected:** `spec.md`의 `bgmVolume`, `sfxVolume`, 60/80과 일치.
- **Actual:** 일치하며 구형 key fallback도 존재.
- **Impact:** 이전 config drift 해소.
- **Suggested Fix:** 구형 key migration을 직접 잠그는 테스트를 추가하면 더 강한 증거가 된다.
- **Re-audit Method:** camelCase 및 snake_case fixture load/save.
- **Owner:** Auditor
- **Notes:** PASS gate 차단 없음.

### [IMP-F004] Re-audit #2 — version authority 정렬

- **Pass:** Implementation
- **Pattern:** IMP-002
- **Area:** version files
- **Severity:** Info
- **Status:** Verified — 해결
- **Summary:** CMake, CHANGELOG, README, audit roadmap이 0.9.2로 정렬됐다.
- **Evidence:** `CMakeLists.txt:4`, `CHANGELOG.md:7`, README v0.9.2 항목, `audit_roadmap.md:1-3,59-64`.
- **Expected:** 현재 SemVer의 일치.
- **Actual:** 0.9.2로 일치.
- **Impact:** 이전 version drift 해소.
- **Suggested Fix:** 없음.
- **Re-audit Method:** 다음 version bump 시 전체 검색.
- **Owner:** Auditor
- **Notes:** 해결.

### [IMP-F005] Re-audit #2 — 폰트 문서와 runtime 정책 불일치

- **Pass:** Implementation
- **Pattern:** DOC-BACKFILL-001
- **Area:** spec/design/build documentation
- **Severity:** Minor
- **Status:** Needs Documentation Recovery — 미해결
- **Summary:** 디자인·빌드 문서는 계속 PerfectDOS를 기본/필수로 규정하지만 runtime은 neodgm을 우선 사용한다.
- **Evidence:** `designs.md:19`, `BUILD_GUIDE.md:47,56,73`, `spec.md:61,77` 대 `Game.cpp:89-103`, README font troubleshooting.
- **Expected:** 필수 font, 우선순위, 언어별 fallback, packaging checklist의 일치.
- **Actual:** 문서별 font authority가 분리됐다.
- **Impact:** 배포 자산 검증과 IMP-F002 해결 작업의 기준이 불명확하다.
- **Suggested Fix:** 5개 언어 font 전략 확정 후 spec/design/build/README를 동기화한다.
- **Re-audit Method:** 문서와 runtime asset load 순서 대조.
- **Owner:** Architect
- **Notes:** audit_report_4에서 누락된 기존 finding이다.

## 4. Pass 2: Debug / Engineering Quality Findings

### [DBG-F001] Re-audit #2 — invalid iterator pair 제거

- **Pass:** Debug
- **Pattern:** DBG-001, DBG-002
- **Area:** UTF-8 conversion, object lifetime
- **Severity:** Info
- **Status:** Verified — 해결
- **Summary:** `getSf()`가 생존 로컬 문자열 하나의 iterator pair로 안전하게 변환한다.
- **Evidence:** `LocalizationManager.cpp:55-58`; invalid temporary iterator 검색 0건; 한국어 Town 제목 runtime 정상.
- **Expected:** 동일 문자열 객체의 valid range 사용.
- **Actual:** 충족.
- **Impact:** 기존 비결정적 제목 손상과 OOB read 원인 제거.
- **Suggested Fix:** 없음.
- **Re-audit Method:** sanitizer를 추가하면 최종 memory-safety 증거를 강화할 수 있다.
- **Owner:** Auditor
- **Notes:** SFML `fromUtf8`는 UTF-8 sequence begin/end를 받는다: [SFML 2.6.1 sf::String](https://www.sfml-dev.org/documentation/2.6.1/classsf_1_1String.php).

### [DBG-F002] Re-audit #2 — Combat 제목 ANSI 경로 제거

- **Pass:** Debug
- **Pattern:** DBG-001
- **Area:** encoding consistency
- **Severity:** Info
- **Status:** Verified — 해결
- **Summary:** Combat 제목도 `getSf()` 경로를 사용한다.
- **Evidence:** `CombatState.cpp:790-793`.
- **Expected:** JSON UTF-8의 일관된 conversion.
- **Actual:** 충족.
- **Impact:** 기존 locale-dependent 제목 손상 경로 제거.
- **Suggested Fix:** 없음.
- **Re-audit Method:** 5개 언어 Combat runtime은 IMP-F002 해결 후 수행.
- **Owner:** Auditor
- **Notes:** 해결.

### [DBG-F003] Re-audit #2 — 신규 UI 회귀 테스트가 실제 State/renderer를 검증하지 않음

- **Pass:** Debug
- **Pattern:** TEST-001, DBG-002
- **Area:** regression test authority
- **Severity:** Major
- **Status:** Needs Fix — 부분 해소
- **Summary:** helper 안정성 테스트는 추가됐지만 테스트 이름·문서가 주장하는 Town/Combat UI 검증은 수행하지 않는다.
- **Evidence:**
  - `test_harness.cpp:661-704`: 5개 언어에서 `getSf()` 반환이 비어 있지 않고 반복 호출 결과가 같은지만 검사.
  - `CMakeLists.txt:95-110`: TestHarness target에 `TownState.cpp`, `CombatState.cpp`, `Game.cpp`, 실제 renderer/state wiring이 없음.
  - 일본어 runtime은 읽을 수 없지만 테스트 13/13 통과.
- **Expected:** Town 7개 substate와 Combat header를 실제로 구성하거나 Xvfb에서 렌더링하여 title, glyph, language content를 검증한다.
- **Actual:** `LocalizationManager` helper 단위 테스트만 존재하며 font와 renderer는 검사하지 않는다.
- **Impact:** 5개 언어 UI가 실패해도 자동 gate가 PASS한다.
- **Suggested Fix:** 테스트 명칭/문서 주장을 helper 범위로 축소하고, 별도 Xvfb UI smoke 또는 렌더 가능한 State integration test와 `hasGlyph` 검사를 추가한다.
- **Re-audit Method:** 일본어/중국어 glyph 실패 시 새 테스트가 실패하고 올바른 폰트 적용 후 통과하는지 확인한다.
- **Owner:** Coder / Auditor
- **Notes:** 기존 테스트 공백을 일부만 해소했다.

### [DBG-F004] Re-audit #2 — build source root 경로 동일성 확인

- **Pass:** Debug
- **Pattern:** BUILD-001
- **Area:** build cache, path identity
- **Severity:** Info
- **Status:** Verified — 해결
- **Summary:** CMakeCache의 `/home/...` 표기는 현재 `/mnt/...` root와 동일한 실제 디렉터리를 가리키는 alias다.
- **Evidence:** 두 경로의 `realpath`가 모두 `/mnt/Projects_SSD/cpp/crawlmaster`; root와 changed source의 device/inode가 동일; source hash 동일; build 성공.
- **Expected:** 현재 source와 build input의 동일성.
- **Actual:** 동일 inode로 확인.
- **Impact:** 이전에 의심한 별도 stale source 문제는 반증됐다.
- **Suggested Fix:** BUILD_GUIDE에서 canonical path를 사용하면 혼선을 줄일 수 있다.
- **Re-audit Method:** realpath/inode와 build source root 재확인.
- **Owner:** Auditor
- **Notes:** 절대 build RPATH는 dev artifact 특성이며 이번 stale-source finding과 구분했다.

## 5. Pass 3: Security Findings

### [SEC-F001] Re-audit #2 — invalid range 기반 memory read 위험 제거

- **Pass:** Security
- **Pattern:** SEC-004
- **Area:** memory safety
- **Severity:** Info
- **Status:** Verified — 해결
- **Summary:** invalid iterator pair의 직접 원인이 제거됐다.
- **Evidence:** DBG-F001 증거와 동일; invalid pattern 검색 0건; 한국어 runtime 정상.
- **Expected:** 소유권과 길이가 확정된 string range만 읽기.
- **Actual:** `getSf()`가 생존 로컬 문자열을 사용한다.
- **Impact:** 기존 OOB read/정보 노출 경로 제거.
- **Suggested Fix:** sanitizer 회귀를 자동 gate로 추가하는 것을 권장한다.
- **Re-audit Method:** ASan/UBSan State 반복 전환.
- **Owner:** Auditor
- **Notes:** 현재 증거로 원 finding은 해결 판정한다.

### [SEC-F002] Re-audit #2 — 네트워크·셸 실행 표면 없음

- **Pass:** Security
- **Pattern:** SEC-003, SEC-004, SEC-007
- **Area:** network/shell boundary
- **Severity:** Info
- **Status:** Verified — 유지
- **Summary:** 제품 코드에 원격 bind 또는 임의 shell 실행 표면이 없다.
- **Evidence:** source 검색과 desktop-only architecture.
- **Expected:** 로컬 전용 제품 경계.
- **Actual:** 충족.
- **Impact:** 네트워크 공격 표면 낮음.
- **Suggested Fix:** 없음.
- **Re-audit Method:** 새 외부 integration 추가 시 재검색.
- **Owner:** Auditor
- **Notes:** dependency scanner는 제외 범위다.

## 6. Cross-Pass Conflicts

### [XPF-F001] audit_report_4 PASS와 실제 일본어 runtime 실패가 충돌

- **Related Findings:** IMP-F001, IMP-F002, IMP-F005, DBG-F003
- **Conflict:** `audit_report_4.md`는 getSf 적용만으로 5개 언어 glyph 문제가 해결됐고 모든 Major/Minor finding이 해소됐다고 주장한다. 실제 일본어 화면은 대체 사각형으로 읽을 수 없다.
- **Resolution:** encoding conversion과 font glyph coverage를 분리 판정하고 runtime evidence를 우선한다. `audit_report_4.md`의 IMP-F002 PASS 및 전체 PASS를 `Rejected as False Positive`로 분류한다.
- **Gate Impact:** HOLD.
- **Required Fix Before PASS:** 5개 언어 font/fallback 구현과 실제 화면 검증.

### [XPF-F002] 테스트 이름·문서 주장과 실제 test target 범위가 충돌

- **Related Findings:** DBG-F003
- **Conflict:** 테스트와 문서는 Town 7개 substate/Combat header UI 안전성을 검증한다고 주장하지만 target은 해당 State 구현을 포함하지 않는다.
- **Resolution:** 현재 테스트는 `LocalizationManager::getSf()` helper unit test로만 인정한다.
- **Gate Impact:** Major regression gate 미충족.
- **Required Fix Before PASS:** State/renderer integration 또는 Xvfb failure-mode test 추가.

## 7. Required Fixes Before PASS

1. 일본어·번체·간체 전체 glyph를 실제 표시할 수 있는 번들 font/fallback 전략 구현.
2. 5개 JSON 전체 codepoint에 대해 선택 font의 `hasGlyph` 검증 추가.
3. 일본어·중국어 Title/Town/Settings/Combat runtime 캡처 통과.
4. `selectLang` 하드코딩 메시지를 5개 JSON 리소스 key로 이동.
5. `TownState.cpp:185,244`의 혼입된 한국어 조사와 다른 번역 문구를 언어별 검수.
6. 신규 테스트를 실제 State/renderer 또는 Xvfb UI failure-mode 검증으로 확장.
7. spec/design/BUILD_GUIDE/README의 font authority와 packaging checklist 동기화.
8. 수정 후 관련 CHANGELOG, IMPLEMENTATION_SUMMARY, LESSONS_LEARNED, audit roadmap 갱신.

## 8. Accepted Risks

- 없음.
- 5개 언어 지원을 제품 완료 기준으로 명시했으므로 일본어·중국어 대체 사각형은 수용 가능한 known risk가 아니다.

## 9. Needs Spec Clarification

### [NSC-001] 5개 언어 font/fallback authority — 미해결

- 단일 CJK 범용 폰트, 언어별 폰트, composite fallback 중 어떤 전략을 채택할지 정해야 한다.
- 필수 glyph 범위, 라이선스, 번들 크기, 자산 누락 시 시작 실패/경고 정책을 명시해야 한다.

## 10. Re-audit Checklist

- [x] invalid temporary iterator pair 0건
- [x] Combat 비ASCII ANSI 직접 경로 0건
- [x] 한국어 Town 제목 반복 표시
- [x] config camelCase와 60/80 기본값 정렬
- [x] v0.9.2 version authority 정렬
- [x] current source/build path identity 확인
- [ ] 일본어 전체 대표 화면에서 대체 사각형 0건
- [ ] 번체·간체 전체 대표 화면에서 대체 사각형 0건
- [ ] 5개 JSON 전체 codepoint glyph coverage 통과
- [ ] 사용자-facing 메시지의 controller hardcoding 0건
- [ ] 5개 언어 문자열 script 혼입 0건
- [ ] 실제 State/renderer 회귀 테스트 통과
- [ ] font 관련 문서 authority 동기화

## 11. Final Decision

**HOLD**

판단 근거:

- 직접 화면 손상의 UB와 memory-safety 결함은 올바르게 해소됐다.
- config, version, build source identity finding도 해결됐다.
- 그러나 일본어 runtime이 실제로 읽을 수 없으며 중국어도 동일 glyph coverage 결함을 가진다.
- 테스트 13/13 통과는 helper만 검증하므로 실제 UI 실패를 반증하지 못한다.
- 5개 언어 메시지 source-of-truth와 일부 번역 내용도 spec에 맞지 않는다.
- `audit_report_4.md`의 전체 PASS는 위 runtime/coverage 증거와 충돌한다.

남은 작업은 font/i18n/test/documentation 경계로 국소화되어 있으므로 `REWORK REQUIRED`까지 확대하지 않는다. Required Fixes를 반영하고 5개 언어 runtime 증거를 확보한 뒤 Re-audit #3를 수행해야 한다.
