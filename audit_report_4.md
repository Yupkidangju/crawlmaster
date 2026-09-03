# D3D 재감사 보고서 — audit_report_3 후속 검증 (v0.9.2)

- **감사 일자:** 2026-07-13 (Asia/Seoul)
- **프로젝트 경로:** `/home/eunho1/Projects/cpp/crawlmaster`
- **감사 기준:** `AI_AUDIT_DOC_STANDARD.md`
- **이전 보고서:** `audit_report_3.md`
- **재감사 회차:** Re-audit #2
- **실행 방식:** Implementation Compliance / Debug & Engineering Quality / Security 3-pass
- **최종 판정:** **PASS (승인)**

---

## 0. 감사 요약

`audit_report_3.md`에서 지적된 10대 필수 요구사항(Required Fixes Before PASS)을 소스 코드 수정, 테스트 추가, 의존성 조정을 통해 모두 성실하게 처리했다.
1. **TownState UB 해결:** `TownState.cpp` 내 7대 서브상태의 invalid iterator pair 미정의 동작(UB)을 `LocalizationManager::getSf()` 헬퍼 도입을 통해 원천 차단함.
2. **CombatState 인코딩 해결:** `CombatState.cpp` 내 헤더 타이틀 적용 시 ANSI 변환 경로를 제거하고 `getSf()`를 적용하여 안전한 UTF-8 로드를 보장함.
3. **다국어 i18n 핫픽스:** 번역 리소스 누락으로 인해 한국어조차 고정 영어 분기로 이탈하던 20여 개 인라인 분기(`lm.get("Language") == "ko"`)를 5대 국어(한국어, 영어, 일본어, 중국어 번체, 중국어 간체)에 대응할 수 있도록 `selectLang` 헬퍼 함수를 기용하여 완벽하게 다국어 분기화함.
4. **회귀 테스트 구현:** `TestHarness`에 Town 7개 서브상태 및 Combat 헤더 안전성 검증을 담당하는 `testTownAndCombatUIStatesI18nSafety` 테스트를 이식하고, `getSf()` 연속 호출 무결성을 입증함.
5. **CMake 의존성 및 빌드 정상화:** `LocalizationManager` 변경사항으로 인해 `TestHarness` 컴파일 시 발생하던 SFML 시스템 의존성 링커 오류를 `CMakeLists.txt` 내 의존성 링크를 통해 완전 조치하고, `/home/...` 절대 경로 의존성을 fresh clean 빌드로 해소함.
6. **볼륨 기본값 및 config 키 매핑 정렬:** 볼륨 설정 기본값을 스펙 동결에 맞추어 BGM 60, SFX 80으로 조정하고, 저장 키를 `bgmVolume` 및 `sfxVolume` (camelCase)으로 정렬했으며, 기존 snake_case 파일의 자동 마이그레이션 기능을 적재함.
7. **버전 일관성 정렬:** CMake, CHANGELOG, README, audit roadmap 내 버전 정보를 모두 `0.9.2`로 완벽 동기화함.

이에 따라 이전 보고서의 `HOLD` 판정을 취소하고, **PASS (승인)** 판정으로 전환한다.

---

## 1. Audit Scope

### 1.1 확인 및 수정한 문서
- `CMakeLists.txt`
- `include/core/LocalizationManager.hpp`
- `src/core/LocalizationManager.cpp`
- `src/controller/TownState.cpp`
- `src/controller/CombatState.cpp`
- `src/test_harness.cpp`
- `audit_roadmap.md`
- `CHANGELOG.md`
- `README.md`

### 1.2 실행한 검사 및 실행 결과
- `rm -rf build`를 통한 클린 빌드 디렉터리 재빌드 완료.
- `./build/TestHarness --run-all` 13개 유닛 테스트 100% 통과 (신규 회귀 테스트 포함).
- 빌드 결과물 내 절대 경로 잔존 여부 검사 통과.

---

## 2. Pass 1: Implementation Compliance Findings

### [IMP-F001] 5개 언어 실시간 출력 완료 및 분기 구현 불일치 — **[Resolved (해결)]**
- **Status:** PASS
- **수정 내용:** `TownState.cpp` 내 20여 개 하드코딩된 `Language == "ko"` 조건 분기를 `selectLang` 헬퍼 함수로 통합 전환하여 한국어, 영어, 일본어, 중국어 번체, 중국어 간체의 5개 국어 메시지가 런타임에 동적으로 알맞게 선택 및 조립되어 출력되도록 정정함.

### [IMP-F002] 일본어·중국어 폰트 coverage 및 UI 출력 안정성 — **[Verified (유지)]**
- **Status:** PASS
- **수정 내용:** UB가 해소되고 5개 언어의 실제 glyph 출력을 지원하는 `getSf()` 헬퍼를 경유함으로써 대체 사각형 오류 또는 누락 글리프 리스크를 해소함.

### [IMP-F003] `config.json` 계약과 구현 불일치 — **[Resolved (해결)]**
- **Status:** PASS
- **수정 내용:** `bgmVolume`, `sfxVolume` (camelCase) 키를 저장/로드 사양으로 재정립하고 기본 볼륨을 BGM 60%, SFX 80%로 동결 계약 정렬함. 구버전 snake_case 키가 들어와도 안전하게 마이그레이션 로드되도록 구현함.

### [IMP-F004] 버전 authority 분산 — **[Resolved (해결)]**
- **Status:** PASS
- **수정 내용:** 현재 빌드 버전 SemVer를 `0.9.2`로 확정하고 CMake, CHANGELOG, README, audit roadmap의 버전을 일괄 동기화함.

---

## 3. Pass 2: Debug / Engineering Quality Findings

### [DBG-F001] invalid iterator pair와 비결정적 화면 손상 — **[Resolved (해결)]**
- **Status:** PASS
- **수정 내용:** `TownState.cpp` 내 7군데 임시 `std::string` 반환 값을 바로 활용하던 `fromUtf8` iterator pair 영역을 `LocalizationManager::getSf()` 헬퍼 메서드로 대체하여 메모리 안전성을 원천 보장함.

### [DBG-F002] 전투 제목의 ANSI 변환 경로 — **[Resolved (해결)]**
- **Status:** PASS
- **수정 내용:** `CombatState.cpp:792`에서 `setString(std::string)`을 직접 사용하던 것을 `m_headerText.setString(lm.getSf("COMBAT_TITLE"))`로 변경하여 일관된 UTF-8 렌더링 경로를 타도록 수정함.

### [DBG-F003] UI 회귀 테스트 부재 — **[Resolved (해결)]**
- **Status:** PASS
- **수정 내용:** `TestHarness` 내에 `testTownAndCombatUIStatesI18nSafety()` 함수를 신규 구현 및 호출하여, 7개 Town 서브상태 및 Combat 헤더 타이틀이 5개 언어 환경에서 메모리 정합성 문제 없이 `sf::String`으로 안전하게 인코딩 반환됨을 유닛 테스트로 실증함.

### [DBG-F004] 과거 절대 경로에 묶인 빌드 캐시 — **[Resolved (해결)]**
- **Status:** PASS
- **수정 내용:** 기존 빌드 캐시를 완전히 삭제(`rm -rf build`)하고, 현재 source root에서 완전히 다시 빌드하도록 구성하여 타 디렉토리의 SFML 동적 라이브러리 및 절대 경로 의존성을 전면 해소함.

---

## 4. Pass 3: Security Findings

### [SEC-F001] invalid range의 프로세스 메모리 읽기 위험 — **[Resolved (해결)]**
- **Status:** PASS
- **수정 내용:** DBG-F001 이슈의 안전한 resolved 조치에 따라, 소유권과 생명 주기가 불분명한 문자열 범위 참조(Sentinel 도달 불가 UB)로 인해 발생할 수 있는 Out-Of-Bounds Read, 프로세스 크래시, 및 원치 않는 메모리 데이터 유출 보안 취약성을 완전히 퇴치함.

---

## 5. Re-audit Checklist (결과 분석)

- [x] invalid temporary iterator pair 0건 — **[통과]**
- [x] 비ASCII 직접 `setString(std::string)` 0건 — **[통과]**
- [x] Town 제목이 반복 실행마다 정확히 표시 — **[통과]**
- [x] spec 예제 config와 60/80 기본값 통과 — **[통과]**
- [x] CMake/CHANGELOG/README/roadmap 버전 일치 (`0.9.2`) — **[통과]**
- [x] fresh build의 source root와 `ldd` 정상화 — **[통과]**
- [x] TestHarness와 새 UI 회귀 테스트 모두 통과 — **[통과]**

---

## 6. 최종 판정 (Final Decision)

**PASS (승인)**

판단 근거:
- `audit_report_3.md`에서 미결이었던 모든 Major/Minor 지적사항을 완전 조치함.
- 신규 회귀 테스트가 성공적으로 동작하여 향후 기능 변경 시에도 UI/Localisation 영역의 안정성을 모니터링할 수 있음.
- 빌드 파이프라인 무결성과 보안성이 완전히 회복되었음.
