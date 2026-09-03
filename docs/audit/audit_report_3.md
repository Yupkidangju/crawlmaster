# D3D 재감사 보고서 — audit_report_2 후속 검증

- **감사 일자:** 2026-07-13 (Asia/Seoul)
- **프로젝트 경로:** `/mnt/Projects_SSD/cpp/crawlmaster`
- **감사 기준:** `AI_AUDIT_DOC_STANDARD.md`
- **이전 보고서:** `audit_report_2.md`
- **재감사 회차:** Re-audit #1
- **실행 방식:** Implementation Compliance / Debug & Engineering Quality / Security 3-pass
- **코드 수정 여부:** 없음
- **최종 판정:** **HOLD 유지**

## 0. 감사 요약

`audit_report_2.md`의 11개 pass finding, 2개 cross-pass conflict, 2개 명세 확인 항목을 현재 트리에서 다시 검사했다. 관련 소스·문서·빌드 캐시는 이전 감사와 동일하며, 기존 Major finding을 해소하는 변경 또는 새 회귀 테스트는 확인되지 않았다.

핵심 화면 결함도 다시 확인되었다.

- 이전 감사 실행: 마을 진입 직후 제목 뒤부터 화면 전체에 임의 글리프와 메모리성 문자열 출력.
- 이번 재감사 실행: 동일 경로에서 메뉴와 파티 정보는 출력되지만 마을 제목이 완전히 사라짐.
- 현재 소스: `TownState.cpp`의 7개 invalid iterator pair가 그대로 존재.

실행마다 표현이 달라지는 것은 미정의 동작의 비결정적 특성과 일치한다. 이번에 대량 글리프가 나타나지 않았다는 사실은 수정 증거가 아니다. 기대된 `TOWN_TITLE`이 사라졌고 invalid range 자체가 남아 있으므로 `DBG-F001`과 연결된 보안 finding은 미해결이다.

`./TestHarness --run-all`은 다시 12/12 통과했지만 UI State와 SFML Graphics 경로를 포함하지 않는 검증 공백도 그대로다. 따라서 이전 `HOLD`를 유지한다.

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
- `audit_report_1.md`
- `audit_report_2.md`

### 1.2 확인한 구현·자산·산출물

- `CMakeLists.txt`
- `src/controller/TownState.cpp`
- `src/controller/CombatState.cpp`
- `src/core/LocalizationManager.cpp`
- `src/core/Game.cpp`
- `src/test_harness.cpp`
- `assets/lang/*.json`
- `assets/fonts/*.ttf`
- `build/CMakeCache.txt`
- `build/Crawlmaster`
- `build/TestHarness`

### 1.3 재실행한 검사

- invalid `fromUtf8(begin, end)` 호출 전체 검색
- 비ASCII `setString(std::string)` 경로 검색
- 존재하지 않는 `Language` 번역 키 사용처 검색
- 설정 키/기본값과 `spec.md` 계약 대조
- CMake/CHANGELOG/README/audit roadmap 버전 대조
- 5개 번역 JSON 구문, 키 수, 키 집합 해시 비교
- 번들 폰트 대표 한글/일본어/중국어 codepoint coverage 검사
- 기존 빌드 캐시 source root와 `ldd` 동적 라이브러리 경로 검사
- `./TestHarness --run-all`
- 가상 X11에서 타이틀 → 마을 진입 runtime smoke 및 화면 캡처
- SFML 2.6.1 공식 문자열 문서와 C++ iterator range 규칙 재확인

### 1.4 이번 런타임 증거

- 캡처 파일: 1280x900 PNG
- SHA-256: `831d2e0be9b3fa14e28c07b2406bbb5020dfd6ea94dcc43c0cc3920d51c5e4a2`
- stdout:
  - 세이브 로드 성공
  - 기본 설정 사용
  - `TitleState`에서 `TownState`로 전환 성공
- stderr:
  - 폰트 또는 번역 JSON 로드 실패 없음
  - 가상 X11의 TextEntered 입력 context 및 vertical sync 경고만 존재
- 화면 결과:
  - `Plaza Menu`, 한국어 메뉴, 파티/가방 정보는 표시
  - 화면 상단에 있어야 할 `TOWN_TITLE`은 표시되지 않음

## 2. Excluded Scope

- `.git`: 감사 샌드박스에서 정상 Git 저장소로 인식되지 않아 commit/diff 기반 변경 계보는 제외했다.
- 실제 사용자 데스크톱 세션: 격리된 가상 X11에서만 실행했다.
- 모든 전투·퀘스트 장시간 수동 플레이: 기존 Major 결함이 초기 마을 진입에서 미해결로 확인되어 중단했다.
- dependency CVE scanner와 CI: 제품용 scanner/CI 구성이 없고 외부 scanner는 실행하지 않았다.
- `build/_deps` 내부 dependency 소스 리뷰: 제품 구현 범위 밖으로 제외했다.

## 3. Pass 1: Implementation Compliance Findings

### [IMP-F001] Re-audit #1 — 5개 언어 실시간 출력 완료 주장과 언어 분기 구현 불일치

- **Pass:** Implementation
- **Pattern:** IMP-001, IMP-003
- **Area:** i18n 계약
- **Severity:** Major
- **Status:** Needs Fix — 미해결
- **Evidence:** `TownState.cpp`의 21개 경로가 여전히 `lm.get("Language") == "ko"`를 사용한다. 5개 JSON에는 `Language` 키가 없고 누락 키는 키 자체를 반환한다.
- **Expected:** 현재 Language enum 또는 5개 언어별 번역 키로 모든 사용자 메시지를 선택한다.
- **Actual:** 조건은 항상 거짓이므로 한국어에서도 영문 분기로 고정되고 일본어·중국어는 독립 분기가 없다.
- **Impact:** 5개 언어 지원 완료 기준 위반, 화면 언어 혼합.
- **Suggested Fix:** 사용자 문자열을 5개 JSON 키로 일원화하거나 `getLanguage()` enum을 사용한다.
- **Re-audit Method:** 5개 언어에서 길드·상점·교회·퀘스트 동작 메시지를 모두 캡처한다.
- **Owner:** Coder
- **Notes:** `audit_report_2.md`의 IMP-F001과 동일한 source signature다.

### [IMP-F002] Re-audit #1 — 일본어·중국어 폰트 coverage 불충족

- **Pass:** Implementation
- **Pattern:** IMP-001, BUILD-001
- **Area:** 다국어 폰트 자산
- **Severity:** Major
- **Status:** Needs Fix — 미해결
- **Evidence:** `Game.cpp`는 계속 `neodgm.ttf` 단일 공용 폰트를 우선 사용한다. `fc-query`에서 `AC00-D7A3` 한글 범위는 확인되지만 대표 일본어 `U+3042`, CJK `U+4E00`은 두 번들 폰트 모두에서 확인되지 않았다.
- **Expected:** 5개 번역 JSON의 실제 codepoint를 번들 자산만으로 표시한다.
- **Actual:** 일본어·중국어 glyph fallback 정책이 없다.
- **Impact:** JA/ZH_TW/ZH_CN UI의 대체 사각형 또는 누락 글리프 위험.
- **Suggested Fix:** 5개 언어 전체 glyph를 포함하는 폰트 또는 언어별 font/fallback 체계를 설계한다.
- **Re-audit Method:** 번역 전체 codepoint-to-glyph 검사와 언어별 화면 캡처를 수행한다.
- **Owner:** Architect / Coder
- **Notes:** 폰트 파일과 로드 순서는 이전 감사와 동일하다.

### [IMP-F003] Re-audit #1 — `config.json` 계약과 구현 불일치

- **Pass:** Implementation
- **Pattern:** IMP-001, ARCH-002
- **Area:** 설정 계약
- **Severity:** Major
- **Status:** Needs Fix — 미해결
- **Evidence:** `spec.md`는 `bgmVolume`/`sfxVolume` 및 60/80을 규정한다. 구현은 `bgm_volume`/`sfx_volume` 및 50/50을 계속 사용한다.
- **Expected:** `spec.md` 동결 계약과 구현의 키·기본값 일치.
- **Actual:** 문서 예제 볼륨 값은 로드되지 않고 무설정 기본값도 다르다.
- **Impact:** 설정 호환성과 초기값 신뢰성 저하.
- **Suggested Fix:** spec 계약에 맞게 구현을 정렬하고 기존 snake_case 파일이 있다면 마이그레이션한다.
- **Re-audit Method:** 문서 예제 로드, 기본값, save/reload, 구형 키 마이그레이션을 검사한다.
- **Owner:** Architect / Coder
- **Notes:** 관련 파일 hash가 이전 감사와 동일하다.

### [IMP-F004] Re-audit #1 — 버전 authority 분산

- **Pass:** Implementation
- **Pattern:** IMP-002, BUILD-001
- **Area:** version files
- **Severity:** Major
- **Status:** Needs Documentation Recovery — 미해결
- **Evidence:** CMake `0.1.0`, audit roadmap `0.9.0`, CHANGELOG/README 최신 기능 `0.9.1` 상태가 유지된다.
- **Expected:** 현재 릴리스 SemVer가 모든 authority에서 동일하다.
- **Actual:** 세 개 버전이 공존한다.
- **Impact:** 산출물 식별과 재감사 기준 신뢰성 저하.
- **Suggested Fix:** 인간 owner가 현재 버전을 확정하고 모든 version file과 roadmap을 동기화한다.
- **Re-audit Method:** 전체 버전 문자열 및 빌드 메타데이터를 재검색한다.
- **Owner:** Architect
- **Notes:** 새 버전 결정 증거가 없다.

### [IMP-F005] Re-audit #1 — 폰트 문서와 실제 로드 정책 불일치

- **Pass:** Implementation
- **Pattern:** DOC-BACKFILL-001
- **Area:** 디자인·빌드 문서
- **Severity:** Minor
- **Status:** Needs Documentation Recovery — 미해결
- **Evidence:** `designs.md`, `BUILD_GUIDE.md`, `spec.md`는 PerfectDOS를 기본/필수로 기술하고 구현·README는 neodgm 우선 로드를 기술한다.
- **Expected:** 동일한 필수 자산과 fallback 순서를 기술한다.
- **Actual:** 문서 authority가 분리되어 있다.
- **Impact:** 배포 시 실제 필수 폰트 누락 가능성.
- **Suggested Fix:** IMP-F002의 폰트 정책을 먼저 확정한 뒤 관련 문서를 동기화한다.
- **Re-audit Method:** 문서 필수 자산, CMake 복사 목록, runtime load 순서를 대조한다.
- **Owner:** Architect
- **Notes:** 미해결.

## 4. Pass 2: Debug / Engineering Quality Findings

### [DBG-F001] Re-audit #1 — invalid iterator pair와 비결정적 화면 손상

- **Pass:** Debug
- **Pattern:** DBG-001, DBG-002, TEST-001
- **Area:** UTF-8 변환, 객체 수명, 렌더링
- **Severity:** Major
- **Status:** Needs Fix — 미해결
- **Evidence:**
  - `TownState.cpp:298,310,318,326,340,360,367`이 계속 `lm.get(KEY).begin()`과 별도 임시 객체의 `lm.get(KEY).end()`를 하나의 범위로 전달한다.
  - 이전 감사에서는 화면 전체 임의 글리프가 출력됐다.
  - 이번 재감사에서는 마을 제목이 완전히 누락됐다.
  - C++ iterator 규칙은 sentinel이 iterator에서 도달 가능할 때만 valid range이고 invalid range에 library function을 적용한 결과는 undefined라고 규정한다: [C++ draft iterator requirements](https://eel.is/c%2B%2Bdraft/iterator.requirements).
  - SFML 2.6.1은 `fromUtf8(begin, end)`에 하나의 UTF-8 sequence 범위를 요구한다: [SFML 2.6.1 sf::String](https://www.sfml-dev.org/documentation/2.6.1/classsf_1_1String.php).
- **Expected:** 동일한 생존 문자열에서 얻은 `[begin, end)` 범위 전달과 항상 동일한 제목 표시.
- **Actual:** invalid range가 남아 실행별로 대량 깨진 글자 또는 빈 제목을 만든다.
- **Impact:** 화면 손상, OOB read 가능성, 크래시, 플랫폼/최적화별 비결정성.
- **Suggested Fix:** 번역 결과를 한 번 저장한 생존 `std::string`의 iterator를 사용하고 변환을 중앙 helper로 통합한다.
- **Re-audit Method:** ASan/UBSan fresh build에서 7개 substate를 반복 전환하고 screenshot/sanitizer 로그를 검사한다.
- **Owner:** Coder
- **Notes:** 증상 형태가 달라진 것은 해결이 아니라 미정의 동작의 추가 증거다.

### [DBG-F002] Re-audit #1 — 전투 제목의 ANSI 변환 경로

- **Pass:** Debug
- **Pattern:** DBG-001
- **Area:** UTF-8 변환 일관성
- **Severity:** Minor
- **Status:** Needs Fix — 미해결
- **Evidence:** `CombatState.cpp:792`가 여전히 UTF-8 번역을 `setString(std::string)`으로 직접 전달한다. SFML 문서상 이 생성 경로는 ANSI/locale 기반이다.
- **Expected:** JSON UTF-8 문자열의 일관된 UTF-8 변환.
- **Actual:** 전투 제목만 ANSI 경로 사용.
- **Impact:** 비ASCII 제목 손상 가능성.
- **Suggested Fix:** DBG-F001의 중앙 UTF-8 helper를 사용한다.
- **Re-audit Method:** 비ASCII 직접 `setString(std::string)` 검색과 5개 언어 전투 캡처.
- **Owner:** Coder
- **Notes:** 미해결.

### [DBG-F003] Re-audit #1 — UI 회귀 테스트 부재

- **Pass:** Debug
- **Pattern:** TEST-001, DBG-002
- **Area:** regression tests
- **Severity:** Major
- **Status:** Needs Fix — 미해결
- **Evidence:** TestHarness 12/12 재통과. 그러나 `TEST_SOURCES`에는 Town/Combat/SFML Graphics가 없고 `testLocalizationI18n`은 map lookup과 config만 검사한다. 동일 실행에서 마을 제목 누락이 확인됐다.
- **Expected:** 과거 화면 깨짐 실패모드를 직접 검출하는 State/UTF-8/UI 회귀 테스트.
- **Actual:** headless domain test가 비주얼 완료의 authority로 사용된다.
- **Impact:** 런타임 UI 실패를 자동 gate가 통과시킨다.
- **Suggested Fix:** 문자열 변환 helper 단위 테스트와 Xvfb State smoke를 추가한다.
- **Re-audit Method:** 결함을 되돌리면 새 테스트가 실패하고 수정 후에는 통과하는지 검증한다.
- **Owner:** Coder / Auditor
- **Notes:** 테스트 통과 결과는 이번 finding의 반증이 아니다.

### [DBG-F004] Re-audit #1 — 과거 절대 경로에 묶인 빌드 캐시

- **Pass:** Debug
- **Pattern:** BUILD-001
- **Area:** build reproducibility
- **Severity:** Major
- **Status:** Needs Fix — 미해결
- **Evidence:** 현재 경로는 `/mnt/Projects_SSD/cpp/crawlmaster`지만 `build/CMakeCache.txt` source/binary/home과 `ldd build/Crawlmaster`의 SFML 경로는 계속 `/home/eunho1/Projects/cpp/crawlmaster`다.
- **Expected:** 현재 source root에서 생성된 self-contained build.
- **Actual:** 과거 별도 디렉터리 존재에 의존한다.
- **Impact:** 소스/바이너리 불일치와 다른 환경에서의 실행 실패 가능성.
- **Suggested Fix:** 현재 root에서 fresh build를 생성하고 절대 경로 잔존 여부를 검사한다.
- **Re-audit Method:** clean configure/build, CMakeCache, `ldd`, 자산 복사, source 반영을 확인한다.
- **Owner:** Build Maintainer / Coder
- **Notes:** 미해결.

## 5. Pass 3: Security Findings

### [SEC-F001] Re-audit #1 — invalid range의 프로세스 메모리 읽기 위험

- **Pass:** Security
- **Pattern:** SEC-004
- **Area:** memory safety, unintended disclosure
- **Severity:** Major
- **Status:** Needs Fix — 미해결
- **Evidence:** DBG-F001 코드가 그대로이며 이번 실행에서도 기대 제목이 소실됐다. invalid iterator range의 library 적용 결과는 undefined다.
- **Expected:** 소유권과 길이가 확정된 문자열 범위만 renderer가 읽는다.
- **Actual:** end iterator 도달이 보장되지 않는다.
- **Impact:** OOB read, 크래시, 프로세스 내 문자열의 의도치 않은 화면 노출 가능성.
- **Suggested Fix:** DBG-F001 수정 후 ASan/UBSan과 긴 비ASCII 데이터로 검증한다.
- **Re-audit Method:** sanitizer와 7개 substate 반복 전환.
- **Owner:** Coder / Auditor
- **Notes:** 로컬 앱이라는 사실은 memory safety finding을 면제하지 않는다.

### [SEC-F002] Re-audit #1 — 네트워크·셸 실행 표면 없음

- **Pass:** Security
- **Pattern:** SEC-003, SEC-004, SEC-007
- **Area:** network/shell boundary
- **Severity:** Info
- **Status:** Verified — 유지
- **Evidence:** 제품 코드에서 socket/bind/listen, system/popen/exec 표면이 계속 확인되지 않는다.
- **Expected:** 로컬 전용 desktop 범위에서 원격 무인증 및 임의 shell 표면 없음.
- **Actual:** 해당 표면 없음.
- **Impact:** 네트워크 공격 표면은 낮다.
- **Suggested Fix:** 없음.
- **Re-audit Method:** 새 dependency와 I/O 경로 추가 시 재검색한다.
- **Owner:** Auditor
- **Notes:** dependency scanner 제외로 공급망 전체 PASS를 뜻하지 않는다.

## 6. Cross-Pass Conflicts

### [XPF-F001] Re-audit #1 — 완료 문서·테스트 PASS와 실제 UI 실패 충돌

- **Related Findings:** IMP-F001, IMP-F002, DBG-F001, DBG-F003, SEC-F001
- **Conflict:** 문서는 5개 언어 비주얼 갱신 완료와 테스트 100%를 주장하지만 runtime에서 제목이 누락된다.
- **Resolution:** 실제 호출 경로와 runtime evidence를 우선하여 완료 주장을 계속 기각한다.
- **Gate Impact:** HOLD 유지.
- **Required Fix Before PASS:** DBG-F001/DBG-F003/SEC-F001 수정 및 5개 언어 UI 증거.

### [XPF-F002] Re-audit #1 — audit_report_1 PASS의 범위 부족

- **Related Findings:** DBG-F001, DBG-F003, DBG-F004
- **Conflict:** `audit_report_1.md`는 controller/view/assets/build를 제외하고 전체 PASS를 선언했다.
- **Resolution:** 현재 runtime-inclusive 범위를 authoritative audit로 유지한다.
- **Gate Impact:** `audit_report_1.md` PASS는 현재 상태를 증명하지 못한다.
- **Required Fix Before PASS:** 수정 후 동일 3-pass 범위로 재감사한다.

## 7. Required Fixes Before PASS

1. TownState 7개 invalid iterator pair 제거.
2. UTF-8 변환을 단일 choke point로 통합하고 Combat 제목 ANSI 경로 제거.
3. Town 7개 substate와 Combat header 실패모드 회귀 테스트 추가.
4. 5개 언어 전체 번역 glyph를 지원하는 font/fallback 정책 구현 및 문서화.
5. `Language` 누락 키 기반 분기를 제거하고 5개 언어 리소스로 메시지 통합.
6. `config.json` 키와 60/80 기본값을 spec 계약에 정렬하고 필요 시 구형 키 마이그레이션.
7. 프로젝트 버전 authority 동기화 및 현재 버전 기준 audit roadmap 갱신.
8. 현재 `/mnt/...` root에서 fresh build 생성 및 과거 `/home/...` 경로 의존 제거.
9. fresh binary로 ASan/UBSan, TestHarness, 5개 언어 Xvfb UI smoke 통과.
10. 관련 필수 문서 전체 동기화.

## 8. Accepted Risks

- 없음.
- 화면 손상과 invalid range는 수용 가능한 레트로 연출 또는 성능 trade-off가 아니다.

## 9. Needs Spec Clarification

### [NSC-001] 5개 언어 폰트/fallback 정책 — 미해결

- 단일 범용 폰트, 언어별 폰트, composite fallback 중 authority가 아직 없다.
- 필수 glyph 범위, 라이선스, 배포 크기, 자산 누락 시 동작을 확정해야 한다.

### [NSC-002] 현재 릴리스 버전 authority — 미해결

- 0.1.0/0.9.0/0.9.1 중 현재 릴리스 기준을 인간 owner가 확정해야 한다.

## 10. Re-audit Checklist

- [ ] invalid temporary iterator pair 0건
- [ ] 비ASCII 직접 `setString(std::string)` 0건
- [ ] ASan/UBSan 7개 Town substate 반복 전환 통과
- [ ] Town 제목이 반복 실행마다 정확히 표시
- [ ] 5개 언어 전체 번역 codepoint glyph coverage 통과
- [ ] 5개 언어별 동작 메시지와 대표 화면 통과
- [ ] spec 예제 config와 60/80 기본값 통과
- [ ] CMake/CHANGELOG/README/roadmap 버전 일치
- [ ] fresh build의 source root와 `ldd`가 현재 `/mnt/...` 기준
- [ ] TestHarness와 새 UI 회귀 테스트 모두 통과

## 11. Final Decision

**HOLD 유지**

판단 근거:

- 이전 Major finding을 해소하는 소스·문서·테스트·빌드 변경이 확인되지 않았다.
- invalid iterator range 7건이 현재 코드에 그대로 존재한다.
- 원래 증상이 이번에는 “화면 전체 깨짐”이 아닌 “마을 제목 소실”로 나타났으나 둘 다 동일 미정의 동작의 유효한 실패 형태다.
- 12개 테스트는 다시 모두 통과했지만 UI 경로를 검증하지 않는다.
- i18n, config, version, build reproducibility finding도 모두 미해결이다.

수정 방향은 여전히 국소적이고 명확하므로 `REWORK REQUIRED`로 확대하지 않는다. Required Fixes가 실제 반영된 뒤 관련 문서와 fresh runtime evidence를 포함해 Re-audit #2를 수행해야 한다.
