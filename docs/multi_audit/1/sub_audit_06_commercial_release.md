# Sub Audit Report

## 1. Audit Metadata

- Audit Turn: 1
- Perspective: Commercial release readiness (packaging, platform, asset/licence provenance, supply chain, QA, operations)
- User Goal: `$multi-audit 위저드리 스타일 게임으로서의 인터페이스, 게임요소, 컨텐츠가 상용수준인지 감사. 부족한 부분은 개선방법을 확인.`
- Audit Basis: Standard-backed / Goal-driven
- Standard Path: `/mnt/Projects_SSD/cpp/crawlmaster/AI_AUDIT_DOC_STANDARD.md`
- Report Contract: `/home/eunho1/.codex/skills/multi-audit/references/report-contract.md`
- Decision Basis: 가격이 정해지지 않은 소규모 유료 PC 게임의 신뢰 가능한 출시 후보. Steamworks 자료는 실제 제출 결과가 아닌 출시 불변조건 비교 자료로만 사용했다.

## 2. Assigned Scope

다음 출시 표면을 현재 트리의 증거와 대조했다.

- Release 구성, 설치·패키지 규칙, 런타임 자산 복사 및 Linux/Windows 경로
- 실행 파일, 폰트·번역 자산, 아이콘·스크린샷·오디오의 존재와 제품 영향
- SFML/nlohmann_json 의존성 고정, 빌드 재현성, 링크 런타임, 라이선스 고지와 provenance
- 테스트 하네스의 Release 동작, GUI/패키지 smoke 범위, CI/CD와 운영 release gate
- 버전·크레딧·EULA/개인정보/지원 정보의 출시 준비도
- `save.json`, `config.json`, TPK reset, 손상 세이브 및 게임 진행 데이터의 손실 경계
- 상용 제품 관점의 인터페이스·게임 요소·컨텐츠 계약 drift (보상/초기 자산/HUD/다국어 포함)

## 3. Excluded and Uninspected Scope

- 다른 `audit_report_*.md`, `docs/audit/**`, `docs/multi_audit/**` 산출물은 읽지 않았다. `docs/multi_audit/1/audit_run.json`도 읽지 않았다. 이 보고서 파일만 새로 작성했다.
- `build/**/_deps`의 벤더 구현 소스 감사는 제외했다. 다만 빌드 선언, 생성된 링크/설치 메타데이터 및 의존성 라이선스 파일은 출시·고지 판단에 필요한 범위에서만 확인했다.
- Steam 계정, App ID, 실제 depot 업로드·심사·가격 설정은 접근하거나 수행하지 않았다.
- Windows/MSVC 실기기, Linux/X11 실기기와 다중 모니터·고 DPI 시각 검증은 실행 환경 부재로 미검증이다. Xvfb 기동 smoke는 수행했지만 시각적 PASS로 승격하지 않았다.
- 폰트 권리, EULA 및 개인정보 문구에 대한 법률 판단은 하지 않는다. 증거가 없는 권리·정책은 `Human Review Required`로 남긴다.

## 4. Evidence Examined

### Project documents and source

- `spec.md`, `designs.md`, `README.md`, `BUILD_GUIDE.md`, `CHANGELOG.md`, `DESIGN_DECISIONS.md`, `IMPLEMENTATION_SUMMARY.md`, `LESSONS_LEARNED.md`, `audit_roadmap.md`
- `CMakeLists.txt`
- `src/core/Game.cpp`, `src/core/LocalizationManager.cpp`, `src/core/GameStateManager.cpp`
- `src/model/Party.cpp`, `src/model/Character.cpp`, `src/model/ItemFactory.cpp`
- `src/controller/CharacterInfoState.cpp`, `TownState.cpp`, `DungeonState.cpp`, `CombatState.cpp`, `SettingsState.cpp`, `TitleState.cpp`
- `src/view/DungeonRenderer.cpp`, `src/test_harness.cpp`, 관련 헤더와 현재 `assets/`

### Asset and build inventory

- 프로젝트 자산은 `assets/fonts/`의 `DroidSansFallbackFull.ttf` (4,033,420 bytes), `PerfectDOSVGA437.ttf` (172,232 bytes), `neodgm.ttf` (651,044 bytes)와 `assets/lang/`의 5개 JSON(각 141 keys)뿐이다.
- 프로젝트 루트에는 아이콘, 스크린샷, 오디오, `LICENSE`, `NOTICE`, EULA, privacy 또는 지원 문서가 없다. `build/**/_deps`의 예제 이미지는 제품 자산으로 세지 않았다.
- 현재 Release 구성 `build/audit-commercial`은 `CMAKE_BUILD_TYPE=Release`, `BUILD_SHARED_LIBS=TRUE`이며 GCC 15.2.0/Linux에서 구성됐다.
- 실행 명령과 결과:
  - `cmake -S . -B build/audit-commercial -DCMAKE_BUILD_TYPE=Release` → exit 0
  - `cmake --build build/audit-commercial --target Crawlmaster --config Release --parallel 2` → exit 0
  - `cmake --build build/audit-commercial --config Release --parallel 2` → exit 2 (`TestHarness`의 `-DNDEBUG`+`-Werror` compile failure)
  - `cmake --install build/audit-commercial --prefix <temporary-dir>` → exit 1; generated dependency install script가 `/usr/local/lib/pkgconfig` 생성 권한에서 실패
  - 기존 Debug 타깃 `cmake --build build --target TestHarness --config Debug --parallel 2` 및 `./TestHarness --run-all` → exit 0
  - `xvfb-run -a timeout 2s ./Crawlmaster` (Release build dir) → timeout exit 124, 창 루프까지 기동 로그 확인. `env -u DISPLAY ...`는 X11 display 부재로 exit 134; 이는 호스트 capability failure로 분류했다.
  - 자산 없는 별도 working directory에서 같은 바이너리를 Xvfb로 기동하면 폰트·번역 누락 경고 후 계속 실행하며, `./save.json`을 생성했다.
  - `strace -e trace=openat`는 실행 중 `./save.json`, `/usr/share/fonts/opentype/noto/NotoSansCJK-Medium.ttc`, `assets/fonts/neodgm.ttf`, `assets/lang/ko.json`, `./config.json` 접근을 확인했다.
- Release 바이너리 `readelf -d build/audit-commercial/Crawlmaster` 결과는 `libsfml-{graphics,window,system}.so.2.6` 의존성과 `/home/eunho1/Projects/cpp/crawlmaster/build/audit-commercial/_deps/sfml-build/lib` 절대 `RUNPATH`를 가진다.

### External comparison references

- [SFML 공식 라이선스](https://www.sfml-dev.org/license/): SFML libraries/source가 zlib/png license이며 상업 사용·재배포를 허용하되 origin/notice 관련 제한을 둔다.
- [nlohmann/json v3.11.3 LICENSE.MIT](https://github.com/nlohmann/json/blob/v3.11.3/LICENSE.MIT): MIT notice가 copies/substantial portions에 포함되어야 한다.
- [Steamworks Store Page](https://partner.steamgames.com/doc/store/page), [Graphical Assets](https://partner.steamgames.com/doc/store/assets), [Release Process](https://partner.steamgames.com/doc/store/releasing): store presence와 game build checklist, graphical assets 및 store/build 일치 여부의 비교 기준으로만 사용했다.

## 5. Findings

### [A06-F001] 제품 설치·패키지·업데이트 산출물이 정의되지 않았고 빌드 디렉터리에 종속됨

- Area: Release packaging, install, Linux/Windows runtime
- Severity: Major
- Status: Confirmed
- Summary: CMake가 실행 파일과 `assets` 복사만 만들고 제품 설치·패키지 규칙을 만들지 않는다. 현재 Release 실행 파일은 개발자 빌드 경로를 `RUNPATH`로 가리킨다.
- Evidence:
  - `CMakeLists.txt:21-50`은 FetchContent/find_package만, `CMakeLists.txt:77-113`은 `Crawlmaster`, `Assets`, `TestHarness`만 정의한다. `install()`, 제품용 `CPack`, runtime output directory, DLL/SO 수집 규칙이 없다.
  - `build/audit-commercial/cmake_install.cmake:45-53`에는 nlohmann_json/SFML 하위 설치 스크립트 include만 있고 Crawlmaster/테스트/자산 install entry가 없다. `cmake --install ... --prefix <temporary-dir>`도 SFML 하위 스크립트가 `/usr/local/lib/pkgconfig`를 만들려다 exit 1이다.
  - `build/audit-commercial/CPackConfig.cmake:52-65`는 파일명·버전이 `SFML-2.6.1-GNU-15.2.0-Release`, vendor가 `SFML Team`, license/readme가 SFML source다. 제품 CPack 설정으로 사용할 수 없다.
  - `readelf -d`에서 `/home/eunho1/Projects/cpp/crawlmaster/build/audit-commercial/_deps/sfml-build/lib` 절대 RUNPATH와 SFML shared-library NEEDED가 확인됐다.
  - Windows 다중 구성에 대한 `RUNTIME_OUTPUT_DIRECTORY`/config별 자산 복사 규칙도 없다. 현재 `Assets`는 `${CMAKE_CURRENT_BINARY_DIR}/assets` 하나만 복사한다.
- Expected Basis: 사용자 계약의 Release 빌드·install/package 규칙, 일반적인 relocatable PC game 불변조건, `BUILD-001`. Steam을 택한다면 Steam game build checklist와 실제 depot 파일이 일치해야 한다.
- Actual: 소스 트리에서 재현 가능한 설치·압축·uninstall·update 산출물이 없고, 개발자 빌드 캐시가 있어야만 현재 Linux 실행 파일의 SFML 링크가 해결된다. 실제 패키지는 생성되지 않았다.
- Impact: 고객 PC에서 실행할 골드 마스터를 만들 수 없으며, 빌드 디렉터리 삭제·경로 변경·다른 사용자에서 실행 시 동적 라이브러리/자산이 사라진다. Windows의 Debug/Release 출력 경로와 자산 경로도 검증되지 않는다.
- Suggested Action: `install(TARGETS)`, `install(DIRECTORY assets)`, 제품 문서·라이선스 install, config-aware runtime output을 정의하고 CPack/플랫폼별 depot 스크립트를 추가한다. Linux는 정적 링크 또는 `$ORIGIN` 기반 bundled `.so`/AppImage/Steam Runtime 중 하나를 선택하고, Windows는 Release DLL·VC runtime·assets를 같은 패키지에 포함한다. 설치 후 빈 작업 디렉터리에서 실행하는 smoke gate를 고정한다.
- Re-audit Method: 새 build directory에서 Linux Release와 Windows MSVC Release를 구성·빌드하고, `cmake --install --prefix <empty-dir>` 또는 실제 패키지 생성 후 파일 목록·SHA-256·uninstall을 확인한다. 패키지 디렉터리에서 의존성 개발 경로 없이 `Crawlmaster`를 실행하고 모든 자산을 읽는지 확인한다.
- Confidence: High
- Notes: Release `Crawlmaster` 타깃 자체의 컴파일 성공은 제품 패키지 PASS를 의미하지 않는다.

### [A06-F002] Release 품질 게이트가 컴파일 실패하며, assertion 기반 테스트는 Release에서 비활성화됨

- Area: Release build, test gate
- Severity: Major
- Status: Confirmed
- Summary: 현재 Release 전체 빌드는 `TestHarness`에서 실패한다. 원인은 테스트 assertion이 `-DNDEBUG`로 제거되어 변수가 unused가 되고, 전역 `-Werror`가 이를 오류로 올리는 구조다.
- Evidence:
  - `cmake --build build/audit-commercial --config Release --parallel 2` exit 2. 대표 오류는 `src/test_harness.cpp:32`의 `getModifier` unused-but-set-variable, `:145`, `:150`, `:178`, `:608` 등의 assertion-only 변수 unused이며 “all warnings being treated as errors”로 종료됐다.
  - `build/audit-commercial/CMakeFiles/TestHarness.dir/flags.make`는 `-O3 -DNDEBUG -std=c++20 -Wall -Wextra -Werror -pedantic`를 기록한다.
  - 같은 build directory에 남아 있던 `TestHarness` 바이너리 실행은 현재 실패한 rebuild를 대체하지 않는다. Debug 하네스만 현재 source를 assertion 활성 상태로 exit 0 실행했다.
- Expected Basis: Release candidate는 게임과 테스트 gate가 같은 현재 트리에서 성공해야 하며, 회귀 검증이 최적화 구성에서 사라지면 안 된다. `BUILD-001`, `TEST-001`.
- Actual: 게임 타깃만 Release로 생성되고 테스트 타깃은 clean rebuild 불가다. 만약 경고를 억지로 무시해 빌드하면 `assert(...)`가 no-op이 되어 성공 로그만 남을 수 있다.
- Impact: 출시 빌드에서 검증 명령이 재현되지 않고, “모든 단위 테스트 검증 완료” 로그를 제품 품질 증거로 사용할 수 없다. Release 전용 UB/경계 회귀가 숨겨질 수 있다.
- Suggested Action: assertion 대신 Release에서도 동작하는 명시적 test check/반환 코드 또는 별도 test assertion macro를 사용하고, 테스트 타깃의 `NDEBUG` 정책을 의도적으로 결정한다. `enable_testing()`/`add_test()`로 CTest에 등록하며 Release/Debug 모두 failure injection이 실제로 실패하는지 고정한다. 품질을 낮추기 위해 전역 `-Werror`를 제거하는 식의 우회는 사용하지 않는다.
- Re-audit Method: 별도 빈 build directory에서 Release configure/build를 수행해 두 executable이 성공하는지, `TestHarness --run-all`이 고의적인 잘못된 fixture에서 non-zero로 실패하는지 확인한다. Debug와 Release의 test count/assertion 실행 증거를 기록한다.
- Confidence: High

### [A06-F003] 테스트가 모델 단위에 머물러 실제 게임 루프·GUI·패키지 경로를 검증하지 않음

- Area: QA, end-to-end, platform smoke
- Severity: Major
- Status: Confirmed
- Summary: 하네스는 `LocalizationManager`와 model 소스만 링크하며 Title/Town/Dungeon/Combat/Game 상태를 실제 이벤트로 구동하지 않는다. GUI 전이, input, assets, package runtime, save-on-exit는 테스트되지 않았다.
- Evidence:
  - `CMakeLists.txt:96-110`의 `TEST_SOURCES`에는 `src/test_harness.cpp`, `LocalizationManager.cpp`, model 파일만 있고 `Game.cpp`, `TitleState.cpp`, `TownState.cpp`, `DungeonState.cpp`, `CombatState.cpp`, `CharacterInfoState.cpp`, `DungeonRenderer.cpp`가 없다. `enable_testing()`/`add_test()`도 없다.
  - `src/test_harness.cpp:346-404`, `:497-573`의 장비 테스트는 일부 UI 정책을 lambda/주석으로 시뮬레이션하며 실제 `CharacterInfoState` 입력 경로를 호출하지 않는다.
  - `BUILD_GUIDE.md:76-81`은 X11, 실제 화면 캡처, CJK 가독성을 체크리스트로 남겼지만 완료 증거가 없다. Xvfb 기동은 창 루프 진입만 확인했으며 인터랙션·시각 판독은 수행하지 않았다.
- Expected Basis: `spec.md:23-31`의 Title→Town→Dungeon→Combat 성공 기준, `TEST-001`, `DBG-001`, `DBG-002`, 상용 후보의 플랫폼 smoke 불변조건.
- Actual: Debug model 하네스 exit 0 외에 전체 루프·실제 패키지·Linux/Windows UI의 결정적 증거가 없다. Release 하네스는 F002로 빌드되지 않는다.
- Impact: 문서의 “플레이어블 검증 완료” 주장이 실제 출시 경로를 보장하지 않으며, HUD placeholder, untranslated strings, save loss, missing asset이 모두 회귀 테스트를 우회한다.
- Suggested Action: CTest/CI에 headless domain tests와 GUI smoke를 분리 등록하고, Xvfb/Windows VM에서 입력 transcript로 Title→Town→Guild/Shop/Temple/Castle→Dungeon→Combat→return/quit를 검증한다. 패키지 디렉터리에서 asset load, save/config path, missing dependency, screenshot baseline을 확인한다.
- Re-audit Method: 실제 Release 패키지를 clean Linux/Windows 환경에서 설치·기동하고 최소 한 번의 캐릭터 생성, 저장, 전투, 장비/소모품, 언어 전환, 정상 종료를 자동 입력으로 수행한다. 화면 캡처와 저장 파일 diff를 artifact로 남긴다.
- Confidence: High

### [A06-F004] 의존성·툴체인 provenance와 재현성이 잠기지 않음

- Area: Supply chain, dependency parity, reproducible build
- Severity: Major
- Status: Confirmed
- Summary: nlohmann_json과 SFML의 tag는 보이지만 commit/hash lock과 SBOM/scanner가 없고, SFML은 pinned FetchContent보다 시스템 패키지를 먼저 선택한다.
- Evidence:
  - `CMakeLists.txt:25-29`는 nlohmann/json `GIT_TAG v3.11.3`, `:33-50`은 `find_package(SFML 2.6 QUIET)` 후 실패할 때만 SFML `GIT_TAG 2.6.1`을 FetchContent한다.
  - 따라서 SFML 2.6 이상인 임의 시스템 패키지가 있으면 선언된 2.6.1과 다른 build input이 된다. 두 tag 모두 immutable commit/hash 또는 archive hash로 검증하지 않는다.
  - 현재 로컬 캐시가 실제로 resolve한 commit은 json `9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03`, SFML `69ea0cd863aed1d4092b970b676924a716ff718b`지만 이 값은 소스 manifest/릴리즈 manifest에 기록되지 않는다.
  - `BUILD_SHARED_LIBS=TRUE`와 절대 build-cache RUNPATH는 F001의 runtime drift를 재확인한다. 프로젝트 루트에는 lockfile, SBOM, dependency scan report, CI workflow가 없다.
- Expected Basis: `spec.md:48-61`의 의존성·빌드 재현성 주장, `DEP-001`, `SEC-006`, 상용 공급망의 source/toolchain provenance 불변조건.
- Actual: 개발자 머신의 SFML 설치 유무와 버전에 따라 다른 binary가 만들어질 수 있고, 새 configure에는 네트워크 fetch가 필요하다. 보안 advisory/라이선스 triage가 shipped scope 및 정확한 commit과 연결되지 않는다.
- Impact: 고객에게 전달한 binary를 정확히 재생성·감사·회수하기 어렵고, 호스트별 ABI/security drift가 생긴다.
- Suggested Action: canonical provider를 하나로 정해 시스템 SFML 우회를 명시적 opt-in으로 격리하거나 제거하고, dependency commit SHA/archive SHA, compiler/container/toolchain digest를 lock/manifest에 기록한다. SPDX/CycloneDX SBOM, OSV/CVE scanner, license allowlist와 artifact signature/checksum을 CI gate로 추가한다.
- Re-audit Method: 네트워크·시스템 SFML이 없는 clean environment와 있는 environment에서 dependency resolution이 동일한 SHA인지 확인하고, manifest와 binary linkage/SBOM을 대조한다. advisory scan 결과와 재현 build hash를 보관한다.
- Confidence: High
- Notes: vendor source 내부 취약점은 이 finding의 조사 범위가 아니다.

### [A06-F005] 번들 폰트와 third-party 고지·재배포 provenance가 출시 전 미확정

- Area: Asset rights, third-party notices, font runtime
- Severity: Major
- Status: Confirmed
- Summary: SFML/json의 공식 라이선스 텍스트는 build cache에만 있고, 실제 번들 폰트 세 개의 출처·버전·재배포 허가·고지 파일이 제품 트리에 없다.
- Evidence:
  - `assets/`에는 세 TTF와 다섯 번역 JSON만 있으며 root `LICENSE`/`NOTICE`/font license 파일이 없다. `fc-query`의 font family/foundry는 `Droid Sans Fallback/1ASC`, `Ubuntu Mono/DAMA`, `NeoDunggeunmo/5757`을 보여주지만 세 파일 모두 `license=` 메타데이터가 비어 있다.
  - `file`은 `neodgm.ttf`에 public-domain origin을 암시하는 내부 name string을 보이지만, 변환·추가 작업 및 현재 파일의 재배포 조건을 독립 문서로 확정하지 않는다. `PerfectDOSVGA437.ttf`의 내부 family가 Ubuntu Mono로 표시되어 filename과 provenance 대조도 필요하다.
  - `designs.md:18-22`는 `DroidSansFallbackFull.ttf`의 “재배포 근거는 감사 미해결 게이트”를 명시하고, `CHANGELOG.md:7-17`, `:196-202`도 `hasGlyph`가 가독성 보증이 아니며 bundled font provenance/permission을 Human Review Required로 남긴다.
  - build cache의 `sfml-src/license.md:3-21`은 SFML zlib/png 및 external library 목록을, `json-src/LICENSE.MIT:1-20`은 nlohmann_json MIT notice를 제공하지만 현재 제품 package에는 복사 규칙이 없다.
- Expected Basis: 사용자 지시의 SFML 공식 zlib/png 및 nlohmann_json license 확인, `spec.md:58-61`, `SEC-006`, commercial redistribution의 notice/provenance 불변조건. 법률 결론은 하지 않는다.
- Actual: SFML의 상업 사용 허용 문구와 json MIT 원문은 확인되지만, 제품이 번들하는 폰트 권리와 고지의 chain of title이 없다. 공식 license/third-party notices를 설치 산출물에 넣는 규칙도 없다.
- Impact: 유료 배포에 필요한 자산 사용권과 notice 의무를 증명할 수 없고, 현재 상태로는 법률·권리 출시 gate를 통과시킬 수 없다. 폰트가 환경마다 달라지면 CJK/ASCII 화면도 달라진다.
- Suggested Action: 각 폰트의 원출처 URL, 버전/commit, checksum, 원 라이선스와 변환 라이선스를 확보하고 권리자가 불명확하면 명확한 상업 재배포 라이선스의 대체 폰트로 교체한다. `THIRD_PARTY_NOTICES`/license bundle을 제품 패키지에 포함하고 SPDX/SBOM에 asset을 등록한다. CJK 혼합 문자열은 선택된 실제 runtime font로 시각 검수한다.
- Re-audit Method: Human Review이 서명한 권리표와 source artifact를 대조하고, package 안의 notice 파일·checksum·attribution을 확인한다. Linux(시스템 Noto 존재/부재)와 Windows에서 동일한 bundled font 선택 및 실제 캡처를 검증한다.
- Confidence: High
- Notes: SFML 공식 라이선스가 상업 사용을 허용한다는 사실만으로 개별 번들 폰트의 권리를 대체하지 않는다.

### [A06-F006] 배포 채널/플랫폼 결정과 store-facing 그래픽 자산이 없음

- Area: Platform release plan, store presence, product assets
- Severity: Major
- Status: Needs Clarification
- Summary: “PC 상용 출시”의 canonical 채널과 Windows/Linux 지원 matrix가 문서에 결정되지 않았고, 어느 채널에도 제출할 수 있는 아이콘·스크린샷 자산이 현재 트리에 없다.
- Evidence:
  - 문서는 `spec.md:9-13`과 README에서 desktop platforms만 말하며 Steam App ID/depot/branch, 다른 store manifest, minimum OS/GPU/runtime matrix를 정의하지 않는다.
  - source asset inventory에는 `.ico/.png/.jpg/.jpeg` 제품 파일, screenshot, trailer, store capsule이 없고 아이콘 설정 코드도 없다. `Game.cpp:11-15`의 창 생성에는 `setIcon`이 없다.
  - Steam을 선택할 경우 공식 [Graphical Assets](https://partner.steamgames.com/doc/store/assets)는 여러 capsule/icon/screenshot을 요구하는 비교 기준이며, [Release Process](https://partner.steamgames.com/doc/store/releasing)는 store presence와 game build checklist를 별도로 둔다. 현재 산출물은 그 어느 항목의 증거도 아니다.
- Expected Basis: 사용자 지시의 플랫폼·패키징·아이콘/스크린샷 포함 범위, `IMP-001`, 일반적인 store/build parity 불변조건. 플랫폼이 불명확하므로 요구사항 자체는 확정하지 않는다.
- Actual: Linux build tree의 실행 파일과 폰트/번역만 확인된다. 가격·지원 OS·배포 채널·store asset deliverable의 권위가 없다.
- Impact: 출시 계획, QA matrix, 고객에게 보이는 제품 identity와 feature/store description 일치를 판정할 수 없다. Steam이라면 listing gate 전에 그래픽 작업이 남고, 다른 채널이라도 동등한 deliverable이 없다.
- Suggested Action: target channel(예: Steam 또는 standalone)과 지원 OS/build matrix를 먼저 `spec.md`/release manifest에서 결정한다. 채널별 required icon/capsule/screenshot/trailer checklist를 추가하고 실제 게임 화면을 캡처해 현재 기능과 일치시킨다. 제품 binary에 OS 아이콘/앱 metadata를 설정한다.
- Re-audit Method: 승인된 channel matrix와 price/support scope를 기준으로 store/build checklist를 채우고, package manifest와 store feature list를 line-by-line 대조한다. 아이콘·최소 1920x1080 screenshots 등 channel 요구 형식과 실화면을 검증한다.
- Confidence: High

### [A06-F007] 오디오가 비목표인데 BGM/SFX 설정은 사용자에게 기능처럼 노출됨

- Area: Product content, settings contract, audio assets
- Severity: Major
- Status: Needs Clarification
- Summary: `spec.md`는 사운드/BGM을 비목표로 명시하지만 Settings 화면과 README는 BGM/SFX volume controls를 제공한다. 현재는 오디오 파일·Audio module·재생 경로가 없어 설정값만 저장된다.
- Evidence:
  - `spec.md:33-38`은 SFML Audio를 배제하고 향후 확장으로 격리한다. `CMakeLists.txt:40-43`은 FetchContent SFML에서 `SFML_BUILD_AUDIO OFF`를 강제한다.
  - `src/controller/SettingsState.cpp:104-135`는 BGM/SFX 막대와 수치를 렌더링하고 `:21-65`는 변경값을 `saveConfig()`한다. `rg` 기준 `src/`와 `include/`에는 `sf::Sound`, `sf::Music`, `sfml-audio` 사용이나 제품 오디오 asset이 없다.
  - `README.md:20`, `designs.md:264-267`은 이를 실제 설정 기능으로 설명하지만 “virtual”이라는 말만 있고 조절 가능한 재생 대상은 없다.
- Expected Basis: `spec.md:31`의 사용자-facing 설정 계약과 비목표의 동시 정합성, `IMP-001`. 오디오가 가격/제품 방향상 필수인지 불명확하므로 요구를 창작하지 않는다.
- Actual: 슬라이더는 영속화되지만 어떤 소리도 바꾸지 않는다. 오디오 없는 레트로 제품으로 의도된 것인지, 출시 전에 Audio를 구현할 것인지 문서 authority가 충돌한다.
- Impact: 고객이 효과가 있다고 기대할 수 있는 inert control이며, 가격 대비 content bar와 store description을 왜곡한다. 오디오를 지연해도 그 사실을 release candidate에 명시할 근거가 없다.
- Suggested Action: 제품 결정을 먼저 확정한다. 오디오 없는 SKU면 슬라이더/README 문구를 제거하거나 “미지원”으로 비활성화하고 store copy에 명시한다. 오디오가 출시 목표면 licensed files, SFML Audio/decoder runtime, volume routing, device failure handling, mute test를 추가한다.
- Re-audit Method: 결정된 spec에 맞춰 설정 UI/README/실제 재생 경로를 대조하고, 유효한 sound fixture에서 BGM/SFX volume 0/50/100의 audible/mute 동작 또는 명시적 disabled state를 테스트한다.
- Confidence: High

### [A06-F008] CWD 기반 save/config와 비원자 쓰기가 설치 환경에서 진행 손실을 유발할 수 있음

- Area: Save/config path, data integrity, error handling
- Severity: Critical
- Status: Confirmed
- Summary: 진행·설정 파일이 OS 사용자 데이터 경로가 아닌 process CWD의 `./save.json`/`./config.json`에 기록되고, `ofstream` 직접 truncate 쓰기와 실패 결과 무시가 사용된다.
- Evidence:
  - `spec.md:289-300`과 `include/model/Party.hpp:65`는 기본 경로를 `./save.json`으로 둔다. `include/core/LocalizationManager.hpp:38-39`는 `./config.json`을 기본값으로 둔다.
  - `src/model/Party.cpp:88-132`는 `std::ofstream file(filePath)`로 기존 파일을 바로 truncate하고 `file << j.dump(4)` 후 stream 상태/flush/atomic rename을 검사하지 않는다. `src/core/LocalizationManager.cpp:122-140`도 동일한 직접 write다.
  - `src/controller/TownState.cpp`의 여러 `party.saveToFile()` 호출과 `src/controller/SettingsState.cpp:21-65`의 `lm.saveConfig()` 호출은 반환값을 확인하지 않는다.
  - Xvfb/`strace` 실행에서 실제 접근이 `./save.json`, `./config.json`임을 확인했다. 별도 no-assets CWD에서도 앱은 해당 CWD에 save를 만들었다.
- Expected Basis: 사용자 지시의 local save/config loss boundary, `spec.md` 저장 계약, 일반적인 desktop writable-user-data 및 crash-safe save 불변조건. 단순한 “상대 경로” 구현은 설치 경로 보장을 대체하지 않는다.
- Actual: Steam/Program Files/읽기 전용 working directory, launcher가 지정한 다른 CWD, disk-full 또는 process interruption에서 save/config가 실패·절단될 수 있고 호출자는 성공 UI/진행을 계속할 수 있다.
- Impact: 플레이어 진행과 설정이 조용히 사라지거나 다음 실행에서 복구 불가 상태가 된다. 같은 바이너리를 여러 위치에서 실행하면 서로 다른 save slot이 생긴다.
- Suggested Action: Windows `%LOCALAPPDATA%`/known folder와 Linux `$XDG_DATA_HOME`/`$XDG_CONFIG_HOME` 등 per-user canonical path를 한 config service에서 resolve하고, migration을 제공한다. 임시 파일→flush/fsync→atomic rename, 마지막 정상 백업/slot/checksum, write 실패 UI와 exit-time flush를 구현한다. save/config 호출자는 실패를 사용자에게 노출한다.
- Re-audit Method: read-only install dir와 임의 CWD에서 실행하고 path를 확인한다. disk-full/permission/interrupted-write fixture를 주입해 원본 보존, 백업 복원, non-zero/error UI를 검증한다. 새 경로 migration과 다중 slot isolation을 확인한다.
- Confidence: High

### [A06-F009] 손상 세이브와 TPK가 동일한 파괴적 reset 경로를 사용함

- Area: Save corruption recovery, intentional permadeath boundary
- Severity: Critical
- Status: Confirmed
- Summary: JSON type/parse 오류가 발생하면 `loadFromFile()`이 `resetToDefault()`를 호출해 기본 save를 다시 쓰며, 의도된 TPK permadeath도 같은 reset 함수로 파티·골드를 즉시 지운다. 손상 원본 격리나 복구 백업이 없다.
- Evidence:
  - `src/model/Party.cpp:135-188`에서 `j["gold"]`/각 필드를 읽다 예외가 나면 catch가 `resetToDefault()`를 호출하고 `true`를 반환한다. `:191-202`는 멤버·인벤토리·퀘스트를 clear하고 100 gold/치유물약 2개로 만든 뒤 save를 덮어쓴다.
  - `src/test_harness.cpp:187-203`은 손상 파일을 넣고 이 reset 결과를 성공으로 단언하지만, 원본 quarantine/backup/복원 가능성은 검사하지 않는다.
  - `src/controller/CombatState.cpp:229-236`은 모든 파티 사망 시 같은 `resetToDefault()`를 호출하고 TitleState로 교체한다. `spec.md:289-300`은 TPK reset 자체를 의도된 규칙으로 기재한다.
- Expected Basis: TPK는 승인된 하드코어 게임 규칙으로 별도 명시·경고되어야 하지만, parse/partial-write/disk corruption은 데이터 손실 없이 quarantine/recovery되어야 한다. 사용자가 요구한 TPK 파괴 경계와 `SEC-005`/일반 데이터 무결성 불변조건.
- Actual: 정상 TPK와 손상 세이브가 코드상 동일한 destructive reset으로 수렴하고, 손상 파일은 보존되지 않는다. `loadFromFile`은 reset 성공을 반환하므로 호출자가 복구와 신규게임을 구분하기 어렵다.
- Impact: 일시적 파일 손상이나 중단된 저장만으로 수 시간의 진행이 영구 삭제될 수 있다. 하드코어 loss를 원하지 않는 사용자가 되돌릴 수 없고, support/QA가 원인을 재현하기 어렵다.
- Suggested Action: parse/schema failure는 `.bad` quarantine와 마지막 정상 backup/slot으로 복원하고, 신규 save 생성과 결과를 구분해 반환한다. TPK는 명시적 GameOver 경로·확인/튜토리얼 경고·문서/플랫폼 cloud-save 정책을 별도로 둔다. 의도된 TPK만 reset metadata와 함께 기록하고 일반 corruption은 절대 reset으로 위장하지 않는다.
- Re-audit Method: truncated JSON, wrong type, unknown item, interrupted atomic rename, 정상 TPK fixture를 각각 실행해 원본/backup 보존과 반환 상태를 확인한다. TPK만 규칙에 따라 삭제되고 corruption은 복원되는지, 사용자 경고와 support log가 분리되는지 검증한다.
- Confidence: High
- Notes: TPK 규칙의 존재 자체를 false positive로 보지 않았다. 문제는 의도된 게임 규칙과 예외 복구가 같은 destructive boundary인 점이다.

### [A06-F010] 캐릭터 장비·소모품 변경은 세이브 없이 메모리에서만 변경됨

- Area: Progress persistence, CharacterInfoState
- Severity: Major
- Status: Confirmed
- Summary: CharacterInfoState가 장비 장착/해제와 소모품 사용으로 Party를 변경하지만 성공 후 `party.saveToFile()`을 호출하지 않는다. 해당 화면을 닫고 게임을 종료하면 변경이 사라진다.
- Evidence:
  - `src/controller/CharacterInfoState.cpp:21-94`의 Enter 입력은 `unequipSelectedSlot()`, `equipSelectedItem()`, `useSelectedConsumable()`만 호출하고 save 호출이 없다.
  - `:285-309`는 소모품을 적용하고 inventory에서 제거하며, `:312-397`은 equip/swap, `:399-425`는 unequip을 수행하지만 모두 save 호출이 없다.
  - `:27-33`의 Escape/I/C는 단순 `popState()`다. `TownState.cpp:64-66`은 Town에서 Title로 교체하고 `TitleState.cpp:34-37`은 종료 시 state를 pop할 뿐 마지막 Party save를 수행하지 않는다.
  - 반대로 Town shop/quest와 Combat reward에는 `party.saveToFile()` 호출이 있으므로 기능별 persistence 정책이 일관되지 않다.
- Expected Basis: `README.md:18-20`, `spec.md:23-31`의 캐릭터 상태·아이템·세이브 계약, 사용자 지시의 save loss boundary.
- Actual: UI에서 물약을 사용하거나 장비를 바꾼 직후 화면에는 반영되지만 process 종료/재실행 후 이전 JSON이 재로드된다.
- Impact: 핵심 RPG 진행이 조용히 유실되고, 플레이어는 저장됐다고 합리적으로 기대하는 UI와 실제 상태가 달라진다.
- Suggested Action: 모든 성공 mutation 후 공통 persistence service를 호출하고 실패를 UI에 표시한다. 상태 화면 exit와 정상 종료에도 flush를 보조로 수행하되 atomic save를 source of truth로 삼는다. 실제 CharacterInfoState 입력/재실행 회귀를 추가한다.
- Re-audit Method: fixture party에서 equip, unequip, potion, swap 각각 수행 후 state pop/process exit/reload를 확인하고 JSON diff가 의도대로 유지되는지 확인한다. 권한 거부 시 UI가 실패를 숨기지 않는지 검증한다.
- Confidence: High

### [A06-F011] 초기 save content가 마스터 스펙과 구현에서 다름

- Area: Game content, new-game economy
- Severity: Major
- Status: Confirmed
- Summary: 마스터 스펙은 신규 save에 `pot_heal` 2개와 `pot_mana` 1개를 명시하지만 구현과 현재 root save에는 치유물약 2개만 있다.
- Evidence:
  - `spec.md:291-295`는 기본 inventory에 치유물약 2개와 마나 물약 1개를 요구한다.
  - `src/model/Party.cpp:191-198`의 `resetToDefault()`는 `pot_heal`만 두 번 push한다. `src/test_harness.cpp:213`, `:351`, `:498`, `:668`의 주석/fixture도 기본 두 물약만 전제한다.
  - 현재 `save.json`은 `inventory: ["pot_heal", "pot_heal"]`이다.
- Expected Basis: `spec.md`가 primary authority인 D3D 문서-구현 정합성 (`IMP-001`), 사용자 목표의 게임 요소/컨텐츠 상용 품질.
- Actual: 신규 게임/손상 reset의 초기 경제와 스펙 예시가 다르며, 마법사/성직자에게 기대된 초기 회복 자원이 지급되지 않는다.
- Impact: 시작 난이도·경제·튜토리얼 기대가 빌드와 문서에 따라 달라진다. QA fixture가 실제 출시 seed를 검증하지 않는다.
- Suggested Action: 제품 결정 후 스펙 또는 구현을 하나의 권위로 정렬하고 초기 save golden fixture를 추가한다. 새 게임, corruption recovery, TPK reset 각각의 inventory를 명시해 regression test로 잠근다.
- Re-audit Method: clean user-data directory에서 신규 시작과 TPK/corrupt recovery를 실행해 exact JSON schema, item counts, UI 표시를 대조한다.
- Confidence: High

### [A06-F012] 던전 좌표/진행 저장 주장이 spec과 구현에서 판정 불가능함

- Area: Save contract, roguelike progression semantics
- Severity: Major
- Status: Needs Clarification
- Summary: README는 `save.json`이 좌표·던전 진행을 저장한다고 설명하지만 `Party::toJson()`에는 map/coordinate 필드가 없고 DungeonState 생성 시마다 새 맵을 만든다. 마스터 save contract에는 좌표가 없다.
- Evidence:
  - `README.md:19`, English `:57`은 party status/gold/quests와 coordinates/dungeon progress 영속화를 주장한다.
  - `src/model/Party.cpp:88-118`은 save JSON에 gold, inventory, members, active_quests만 쓴다. `Character::toJson()`도 캐릭터 필드만 쓴다.
  - `src/controller/DungeonState.cpp:13-19`은 상태 생성 때 `m_map.generate()`를 실행한다. `spec.md:302-340`의 Save File Contract에도 map/position/depth 필드가 없다.
- Expected Basis: `spec.md` primary authority와 `IMP-001`/`IMP-002`; 게임이 매번 새 던전을 시작하는 의도인지, 중단 지점 resume이 필요한지 요구가 불명확하므로 기대를 창작하지 않는다.
- Actual: 코드 기준으로 던전 맵·플레이어 위치는 저장되지 않는다. README만 보면 resume을 기대할 수 있고, spec만 보면 roguelike 신규 맵도 가능하다.
- Impact: 고객의 continue/resume 기대, cloud-save 계획, 진행 loss QA를 결정할 수 없다. store description과 실제 동작이 divergence할 위험이 있다.
- Suggested Action: `spec.md`에서 `run resume` 또는 `매 진입 새 던전` 중 하나를 명시하고 save schema/version/migration을 결정한다. resume이면 map seed/position/FOW/stepped/encounter state를 저장하고, 새 run이면 README에서 좌표·진행 저장 주장을 제거한다.
- Re-audit Method: 결정된 contract의 golden save를 생성·재실행해 같은 map/좌표/FOW 또는 의도된 새 seed가 나오는지 확인하고, quit/relaunch/TPK 경계를 문서와 대조한다.
- Confidence: High

### [A06-F013] 전투 골드 보상 구현이 동결된 경제 공식과 다름

- Area: Game content, reward economy
- Severity: Major
- Status: Confirmed
- Summary: 스펙은 몬스터별 `(1d10 * 몬스터 레벨)` 골드를 규정하지만 구현은 전투 전체에 5..15 한 번을 굴려 foes 수를 곱한다.
- Evidence:
  - `spec.md:360-362`는 몬스터 레벨별 1d10 보상을 명시한다.
  - `src/controller/CombatState.cpp:699-714`는 `std::uniform_int_distribution<>(5, 15)` 한 번으로 `goldDist(gen) * m_foes.size()`를 계산한다. 몬스터 level을 읽지 않는다.
  - `src/test_harness.cpp`는 CombatState 보상 경로를 구동하지 않고 Quest 보상/모델만 단위 테스트한다.
- Expected Basis: 마스터 스펙 경제 공식과 `TEST-001`의 failure-specific content regression 기준.
- Actual: 단일/다수/고레벨 몬스터가 동일 전투 단위와 5..15 범위를 사용하며 1d10 및 monster-level scaling이 적용되지 않는다.
- Impact: 상점·퀘스트 경제와 난이도 progression이 문서와 다르고, random encounter composition에 따라 보상이 예측 불가능하게 왜곡된다.
- Suggested Action: 몬스터별 level과 seedable RNG를 사용해 공식대로 합산하고, encounter fixture별 expected reward/XP를 테스트한다. 상용 밸런스 수치를 바꾸려면 먼저 spec/changelog를 갱신한다.
- Re-audit Method: 고정 seed로 각 몬스터 단독·복수 encounter를 실행해 1d10×level 합계, 저장 gold, UI log가 일치하는지 100회 범위 테스트한다.
- Confidence: High

### [A06-F014] 던전 HUD가 실제 파티가 아닌 고정 샘플과 영문 조작을 표시함

- Area: Interface, runtime data consumption
- Severity: Major
- Status: Confirmed
- Summary: DungeonRenderer의 PARTY STATUS/CONTROLS는 실제 Party·LocalizationManager를 받지 않고 고정된 WARRIOR/CLERIC/ROGUE/MAGE 샘플을 그린다.
- Evidence:
  - `include/view/DungeonRenderer.hpp:22-26`의 render 인자는 window, map, logQueue뿐이며 Party가 없다.
  - `src/view/DungeonRenderer.cpp:241-267`은 `"[PARTY STATUS]"`, HP `18/18` 등 고정 네 줄과 영문 `[CONTROLS]`를 직접 `setString`한다.
  - `spec.md:23-31`, `designs.md:31-66`은 실제 파티/HP/조작 HUD를 성공 기준으로 둔다. TownState만 실제 Party를 동적으로 표시한다.
- Expected Basis: 문서의 party status UI, `IMP-001`, 구현된 상태/데이터는 실제 consumer와 후속 화면 변화까지 검증해야 한다는 감사 표준.
- Actual: 한 명/다른 직업/손상 HP/사망 상태의 파티도 동일한 샘플 HUD를 보며, 일부 control text가 localization을 거치지 않는다.
- Impact: 전투 준비·사망·HP 정보를 잘못 안내하고, 플레이어의 의사결정을 오도한다. 상용 UI 신뢰성과 다국어 listing claim이 깨진다.
- Suggested Action: renderer에 Party/view-model을 주입해 실제 멤버·HP·상태를 렌더링하고, control/status 문자열을 localization key로 이동한다. 빈 파티/4인/사망/긴 이름을 캡처 기반으로 검증한다.
- Re-audit Method: 서로 다른 1/2/4인 fixture와 HP/dead 상태로 실제 DungeonState를 기동해 HUD text/geometry가 Party와 일치하는지 5개 언어로 확인한다.
- Confidence: High

### [A06-F015] 5개 언어 지원 주장이 사용자-facing 문자열 전체와 정렬되지 않음

- Area: Localization, interface content
- Severity: Major
- Status: Confirmed
- Summary: 다섯 JSON의 key coverage는 확인되지만 Dungeon/Town/Combat/Character/Settings에 많은 고정 영문 또는 고정 한국어 로그·레이블이 남아 있다. key 존재 테스트만으로는 실제 화면의 번역 완결성을 보증하지 않는다.
- Evidence:
  - `README.md:20`, `spec.md:31`, `spec.md:444-449`는 5개 언어와 모든 UI text localization을 주장한다.
  - `src/controller/DungeonState.cpp:87-174`, `:244-265`의 Auto-navigation/Moved/Wall/Turned 로그, `src/controller/CombatState.cpp:31-32`, `:350-375`, `:721-728`의 combat 로그, `src/controller/TownState.cpp:341-366`, `:391-393`의 Guild/Shop/Temple/catalog, `src/controller/CharacterInfoState.cpp:131-141`의 Name/Class, `src/controller/SettingsState.cpp:157`의 footer가 직접 문자열을 만든다.
  - `src/test_harness.cpp:629-657`, `:716-792`는 Town key 존재와 JSON codepoint/`hasGlyph`를 검사할 뿐 실제 상태를 각 언어로 렌더링하지 않는다. `CHANGELOG.md:17`도 hasGlyph가 가독성 보증이 아님을 기록한다.
- Expected Basis: `spec.md:446`의 모든 UI 출력부 localization 계약, `TEST-001`, 상용 다국어 UX.
- Actual: JSON key parity는 5×141로 좋지만 실제 게임 진행 로그·catalog·HUD에 언어가 섞인다. CJK font 선택은 runtime system path와 테스트 bundled font도 다르다.
- Impact: 지원 언어를 선택한 고객이 raw/foreign strings를 보고 기능 이해와 store language claim을 신뢰하지 못한다. 폭이 긴 번역의 clipping도 실화면에서 미검증이다.
- Suggested Action: 사용자에게 보이는 모든 문자열을 key/placeholder catalog로 이동하고, dynamic data formatting을 locale-aware view model로 분리한다. 언어별 실제 state flow screenshot/overflow/placeholder tests를 추가한다.
- Re-audit Method: KO/EN/JA/ZH-TW/ZH-CN 각각에서 Title→Town→Shop→Dungeon→Combat→CharacterInfo→Settings를 입력 transcript로 실행해 화면 텍스트가 target language인지, clipping/raw key/placeholder가 없는지 캡처·검사한다.
- Confidence: High

### [A06-F016] 상용 release identity·법적/지원 문서가 제품 산출물에 없음

- Area: Versioning, credits, EULA/privacy/support
- Severity: Major
- Status: Confirmed
- Summary: CMake 프로젝트 버전 `0.9.4`는 존재하지만 executable/version command·build ID·release owner/support contact가 없고, root에 LICENSE/NOTICE/EULA/privacy/support 문서가 없다.
- Evidence:
  - `CMakeLists.txt:4`만 `project(Crawlmaster VERSION 0.9.4)`를 선언한다. `src/main.cpp:7-12`는 인자/version 출력 없이 Game만 실행하고, title credit은 `src/controller/TitleState.cpp:134-142`의 고정 문자열 `(C) 2026 DEEPMIND ADVANCED CODING AGENT. ALL RIGHTS RESERVED.`뿐이다.
  - root inventory에 `LICENSE`, `NOTICE`, `EULA`, `privacy`, `support`, `version` 파일이 없다. `README`/`BUILD_GUIDE`에도 minimum OS, support URL, release manifest, data policy가 없다.
  - 네트워크/telemetry 코드는 관찰되지 않아 수집 개인정보가 있다고 단정하지 않는다. 다만 local save/config의 위치·보존·삭제 정책과 publisher identity를 고객에게 알리는 문서가 없다.
- Expected Basis: 사용자 지시의 버전/크레딧/EULA/개인정보(해당 시), `DEP-001`/release identity 일반 불변조건. 법적 문구의 적정성은 Human Review Required.
- Actual: 내부 문서의 SemVer만 있고 고객·플랫폼·지원·제3자 고지에 연결된 release identity가 없다.
- Impact: 고객 문의·재현·rollback·지원 버전을 식별할 수 없고, third-party notice/terms/privacy 의무를 검토할 수 없다.
- Suggested Action: legal owner가 승인한 LICENSE/EULA/privacy(수집 없음도 명시)/support URL와 `THIRD_PARTY_NOTICES`를 추가하고 package에 포함한다. 앱/manifest에 SemVer+commit/build ID를 노출하고 title credit을 실제 권리자·외부 자산 attribution과 정렬한다.
- Re-audit Method: Human Review된 legal/support 문서와 package contents/version output을 대조하고, 설치된 앱에서 version/build ID와 save-data disclosure를 확인한다.
- Confidence: High
- Notes: 이는 법률 자문이 아니며, 권리·약관·개인정보 문구는 사람의 승인 없이는 PASS로 처리하지 않는다.

### [A06-F017] CI/CD와 cross-platform release automation이 없음

- Area: CI/CD, release operations, scanner gates
- Severity: Major
- Status: Confirmed
- Summary: 저장소에 CI workflow, CMake preset, CTest registration, Windows artifact job, package upload/checksum/signing 또는 dependency scanner 정책이 없다.
- Evidence:
  - 현재 root inventory에 `.github/workflows`, GitLab/Azure/AppVeyor config, `CMakePresets.json`, release script가 없다. `CMakeLists.txt`에도 `enable_testing()`/`add_test()`가 없다.
  - `find`로 확인한 프로젝트에는 SBOM/SPDX/CycloneDX/dependency scan artifact가 없다. 현재 실행 가능한 compiler는 GCC 15.2.0/Linux이며 Windows/MSVC artifact는 없다.
  - root에서 `git status`는 “not a git repository”로 실패해 이 workspace snapshot 안에는 commit lineage도 없다. 이것이 upstream 저장소의 부재를 단정하지는 않지만, 현재 release evidence에는 lineage가 없다.
- Expected Basis: 사용자 목표의 CI/CD, Windows/Linux release, 공급망·QA 운영, `BUILD-001`, `DEP-001`, `TEST-001`.
- Actual: 수동 문서 명령(Debug 중심)과 개인 build cache만 있고, merge-to-release를 재현하는 자동 gate가 없다.
- Impact: 한 플랫폼에서만 통과한 binary가 다른 플랫폼/패키지에서 실패할 수 있으며, asset/license/dependency drift를 release 전에 발견하지 못한다.
- Suggested Action: CI에서 Linux Release, Windows MSVC Release, package install/smoke, CTest, asset/checksum, SPDX/SBOM, vulnerability/license scan을 병렬로 실행한다. artifact에 source commit, toolchain, dependency SHAs를 기록하고 서명·보존·rollback 정책을 추가한다.
- Re-audit Method: clean CI run 링크/로그와 artifact manifest를 열어 두 OS의 build/test/package/smoke/signature가 같은 source commit을 가리키는지 확인한다. 의도적 gate failure가 release를 차단하는지 검증한다.
- Confidence: High

### [A06-F018] 구현 요약 문서가 존재하지 않는 View 파일을 현재 책임처럼 기재함

- Area: Documentation recovery, ownership map
- Severity: Minor
- Status: Confirmed
- Summary: `IMPLEMENTATION_SUMMARY.md`가 `UIRenderer.hpp/.cpp`를 View 책임으로 링크하지만 현재 `include/view`/`src/view`에는 `DungeonRenderer`만 있다. spec의 View 목록과 실제 파일 책임도 다시 닫아야 한다.
- Evidence:
  - `IMPLEMENTATION_SUMMARY.md:55-59`는 `UIRenderer.hpp/.cpp`를 기재한다.
  - 현재 파일 inventory에는 `include/view/DungeonRenderer.hpp`, `src/view/DungeonRenderer.cpp`만 있고 UIRenderer/TownRenderer/CombatRenderer 파일이 없다. 실제 Town/Combat UI는 controller에서 직접 draw한다.
- Expected Basis: `IMP-003`, `IMP-004`, `DOC-BACKFILL-001`; release ownership/maintenance 문서는 현재 source와 일치해야 한다.
- Actual: 다음 코더/QA가 존재하지 않는 파일을 소유 경로로 믿을 수 있다.
- Impact: packaging/resource ownership, UI regression test 대상, 책임자 handoff가 흐려진다. 기능이 누락된 것인지 문서가 오래된 것인지도 판정할 수 없다.
- Suggested Action: 현재 구조가 의도라면 summary/spec의 View 책임과 링크를 controller/DungeonRenderer 기준으로 갱신하고, 분리된 renderer가 출시 목표라면 구현·테스트·문서를 함께 추가한다.
- Re-audit Method: 문서의 모든 file link를 `rg --files`와 대조하고, 각 UI surface의 소유 파일·테스트·package asset 책임표가 실제 tree에 존재하는지 확인한다.
- Confidence: High

## 6. Uncertainties and Clarifications Needed

- 플랫폼/배포 채널(Steam, standalone, 다른 store), 지원 OS/최소 사양, 가격·오디오·컨트롤러 지원의 승인 범위가 없다. A06-F006/F007은 이 결정 없이는 제품 요구 자체를 확정하지 않는다.
- `ItemFactory::getShopCatalog()` (`src/model/ItemFactory.cpp:88-109`)는 18개 item을 반환하지만 spec/design/Town UI는 기본 구매 8종을 말한다. 현재 Town 입력 경로는 8종을 직접 하드코딩하므로 이 factory가 제품 구매 API인지 dormant helper인지 결정이 필요하다.
- Linux CJK runtime은 시스템 Noto를 먼저 선택하고 TestHarness는 bundled `DroidSansFallbackFull.ttf`를 직접 검사한다. 시스템 폰트를 허용할지, 모든 고객이 bundled font를 사용해야 할지 결정해야 한다.
- 실제 X11/Windows GUI, 다중 해상도·고 DPI, input method, controller/accessibility, Steam overlay/cloud save는 이 환경에서 미검증이다. 관련 요구가 없다고 가정해 PASS하지 않는다.
- 폰트·publisher/third-party rights, EULA/privacy 문구와 legal owner는 사람의 승인 없이는 확정할 수 없다.
- Accepted Risk: 없음. TPK 자체는 `spec.md`에 적힌 의도적 규칙이지만, 손상 세이브 reset·권리·패키지·진행 손실은 수용된 위험으로 처리하지 않았다.

## 7. Perspective Decision

**Decision: HOLD — 상용 출시 후보로 PASS할 수 없음.**

현재 가장 강한 차단 근거는 다음과 같다.

1. 제품용 install/package 산출물이 없고 Release binary가 개발자 build-cache 절대 경로에 의존한다 (A06-F001).
2. 현재 Release 전체 빌드가 TestHarness에서 실패하며, assertion 기반 검증은 `NDEBUG`에서 사라진다 (A06-F002).
3. save/config가 CWD와 비원자 write에 묶이고, corruption과 TPK가 진행 삭제 reset으로 수렴한다 (A06-F008, A06-F009).
4. 번들 폰트의 재배포 권리·고지와 제품 legal/support identity가 미확정이다 (A06-F005, A06-F016).
5. 실제 게임 루프/패키지/platform QA와 CI/CD 증거가 없고, HUD/content/localization/reward 계약도 현재 문서와 어긋난다 (A06-F003, A06-F011, A06-F013, A06-F014, A06-F015, A06-F017).

### 출시 차단 게이트 (다음 재감사 전 필수)

- **Build/package:** Linux와 Windows의 clean Release, relocatable install/package, bundled runtime libs/assets, version/build manifest, checksum/signature.
- **QA:** Release에서도 활성인 CTest/회귀 gate, 실제 상태 전이와 입력 transcript, 패키지 smoke, 5개 언어 실화면 캡처, Windows/Linux 증거.
- **Data integrity:** per-user save/config path, atomic+backup save, corruption quarantine, explicit TPK-only reset, mutation save/error UI, migration/schema version.
- **Rights/legal:** 각 TTF와 dependency의 source/version/license/notice/permission 증거, `THIRD_PARTY_NOTICES`, EULA/privacy/support/owner 검토.
- **Product contract:** audio/platform/store scope 결정, initial inventory/reward/HUD/dungeon resume/localization 문서-코드 정렬.

### 출시 후 운영 개선 (출시 차단 해소 후)

- opt-in crash diagnostics와 anonymized build/version correlation, support ticket에 save/repro metadata 연결
- Steam Cloud 또는 명시적 local-only backup/slot/export 정책과 patch rollback/compatibility migration
- controller/remapping, high-DPI/window scaling, accessibility 및 input-method QA
- 오디오·비주얼 자산 확장, store screenshot/trailer refresh, localization review와 content/economy telemetry(개인정보 최소화)

## 8. Coder Handoff

`/mnt/Projects_SSD/cpp/crawlmaster/docs/multi_audit/1/sub_audit_06_commercial_release.md`를 먼저 읽고, 각 finding을 현재 프로젝트 문서와 실제 코드에 대조하여 검증한 뒤 우선순위대로 수정하세요. 계약 변경이 필요하면 관련 문서를 먼저 갱신하고, 수정 후 Linux/Windows Release 테스트·패키지 smoke·자산/라이선스·save integrity 재감사 증거를 기록하세요.
