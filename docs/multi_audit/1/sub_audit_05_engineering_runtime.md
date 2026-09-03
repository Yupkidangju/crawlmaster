# Sub Audit Report

## 1. Audit Metadata

- Audit Turn: 1
- Perspective: Build / Release, Debugging, Runtime Startup, FSM Lifetime, Randomness, Save/Config Integrity
- User Goal: `$multi-audit` 위저드리 스타일 게임으로서의 인터페이스, 게임요소, 컨텐츠가 상용수준인지 감사하고 부족한 부분의 개선방법을 확인
- Audit Basis: Standard-backed / Goal-driven
- Standard Path: `/mnt/Projects_SSD/cpp/crawlmaster/AI_AUDIT_DOC_STANDARD.md`; `/home/eunho1/.codex/skills/multi-audit/references/report-contract.md`
- Source changes: 없음. 배정 보고서만 생성함.

## 2. Assigned Scope

다음 구현과 출시 후보 검증을 현재 소스, 헤더, 프로젝트 문서, 빌드 산출물 및 실행 결과로 대조했다.

- CMake Debug/Release 경고 및 전체 타깃 빌드
- 의존성 해석 및 실행 바이너리 런타임 링크
- TestHarness, CTest 등록, assert/NDEBUG 의미, 회귀 테스트의 실제 생산 코드 커버리지
- 앱 기동, SFML/X11 의존성, 자산·폰트·번역 파일 경로
- GameStateManager 및 전투/던전 상태 수명, TPK 전환
- `std::random_device`/`std::mt19937` 사용과 seed 재현성
- `save.json`/`config.json` 저장, 로드, 손상 복구, 쓰기 실패, 데이터 보존 경계
- 플랫폼별 사용자 경로 및 재배치 가능한 배포 산출물

상용수준 기준은 가격 미정의 소규모 유료 PC 게임 출시 후보이며, 구현 기능의 존재와 출시 가능한 검증 상태를 분리했다.

## 3. Excluded and Uninspected Scope

- `audit_report_*.md`, `docs/audit/**`, 다른 감사자의 `docs/multi_audit/**` 보고서는 읽지 않았다. `docs/multi_audit/1/audit_run.json`은 배정 파일 경로 확인을 위한 메타데이터로만 읽었다.
- `build/**/_deps/**`의 FetchContent로 내려받은 SFML/json 소스 자체는 사용자 지시에 따라 감사하지 않았다. 다만 루트 CMake의 dependency 선택, 버전 정책과 최종 바이너리 링크 메타데이터는 포함했다.
- 인터페이스 시각 품질, 콘텐츠 깊이, 세부 전투 밸런스는 다른 관점의 배정 범위로 보고, 여기서는 해당 기능의 저장·런타임·테스트 연결만 다뤘다.
- 실제 디스크 고장·권한 거부를 제품 코드에 주입하는 실행은 소스/배정 보고서 외 파일을 만들지 않는 제약 때문에 수행하지 못했다. 관련 호출 경로와 반환값 무시는 정적 증거로 판정했다.

## 4. Evidence Examined

### 4.1 Documents and source inventory

- `spec.md`: FSM (`118-121`), 저장/리셋 및 초기값·세이브 계약 (`289-350`), 자동 이동 및 상태 입력 (`364-383`), 검증 명령 및 Phase claims (`434-470`)
- `README.md`: 사용자 기능과 좌표 저장 주장 (`7-22`, `49-60`), 빌드/실행 (`24-38`, `62-69`)
- `BUILD_GUIDE.md`: prerequisite, FetchContent, 자산 복사, 하네스 격리 주장 (`7-19`, `23-28`, `46-72`), 미완료 실행 전 체크리스트 (`76-81`)
- `IMPLEMENTATION_SUMMARY.md`: 런타임/파일 책임표 (`7-71`), 저장·상태 검증 완료 주장 (`120-135`)
- `DESIGN_DECISIONS.md`: TPK 리셋 및 GameOverState 주장 (`46-54`), FSM/설정 결정 (`71-112`)
- `audit_roadmap.md`: 모든 Phase/위험·테스트가 통과했다는 현재 상태 주장 (`7-79`)
- `CMakeLists.txt`: 전역 경고와 타깃/자산 정의 (`3-16`, `76-113`)
- `src/main.cpp`, `src/core/Game.cpp`, `src/core/GameStateManager.cpp`
- `src/model/Party.cpp`, `include/model/Party.hpp`, `src/model/Character.cpp`, `include/model/Character.hpp`, `src/core/LocalizationManager.cpp`, `include/core/LocalizationManager.hpp`
- `src/controller/{DungeonState,CombatState,CharacterInfoState,TownState,SettingsState}.cpp` 및 대응 헤더
- `src/model/DungeonMap.cpp`, `src/model/MonsterFactory.cpp`, `src/model/Skill.cpp`, `include/model/Monster.hpp`, `include/model/ConcreteItems.hpp`
- `src/test_harness.cpp`, `assets/lang/*.json`, `assets/fonts/*.ttf`, 루트 `save.json`

### 4.2 Commands and observed results

1. `cmake -S . -B build/audit-commercial -DCMAKE_BUILD_TYPE=Release && cmake --build build/audit-commercial -j2`
   - CMake 구성은 성공했고 `Crawlmaster`는 링크됐지만 `TestHarness` 컴파일에서 종료 코드 `2`.
   - `build/audit-commercial/CMakeFiles/TestHarness.dir/flags.make`는 `-O3 -DNDEBUG -std=c++20 -Wall -Wextra -Werror -pedantic`를 확인한다.
   - `src/test_harness.cpp`의 assert 안에서만 사용되는 변수들이 `unused-variable`/`unused-but-set-variable`로 다수 실패했다.
2. `cmake --build build/audit-commercial --target Crawlmaster -j2`: 종료 코드 `0`.
3. `cmake -S . -B build/audit-debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build/audit-debug -j2`: 종료 코드 `0`.
4. `./build/audit-debug/TestHarness --run-all`: 종료 코드 `0`, 하네스가 출력한 모든 테스트 성공.
5. `ctest --test-dir build/audit-debug --output-on-failure`: 종료 코드 `0`이지만 `No tests were found!!!`.
6. `timeout 5s ./Crawlmaster` (표시 가능한 세션, 빌드 출력 디렉터리): 창을 생성하고 저장/설정 로드를 시작했으며 입력 대기 중 timeout 종료 코드 `124`.
7. `env DISPLAY= timeout 5s ./build/audit-commercial/Crawlmaster` (프로젝트 루트): `Failed to open X11 display` 후 종료 코드 `134`(abort).
8. `readelf -d build/audit-commercial/Crawlmaster`/`ldd`: SFML shared library가 필요하고 RUNPATH가 `/home/eunho1/Projects/cpp/crawlmaster/build/audit-commercial/_deps/sfml-build/lib`라는 개발자 빌드 절대 경로로 기록됨.

## 5. Findings

### [A05-F001] Release 전체 빌드가 TestHarness의 NDEBUG/assert 제거로 실패함

- Area: Release build / warning policy / release gate
- Pass: Debug / Engineering Quality
- Pattern: BUILD-001
- Severity: Major
- Status: Confirmed
- Summary: 지정된 상용 후보 Release 전체 빌드는 게임 타깃만 성공하고 기본 `all` 빌드에 포함된 `TestHarness`가 실패한다. Release에서 `assert`가 제거되므로 검증 코드가 사라지는 동시에 `-Werror`가 남은 로컬 변수들을 오류로 승격한다.
- Evidence:
  - `CMakeLists.txt:11-16`에서 모든 타깃에 `-Wall -Wextra -Werror -pedantic`를 전역 적용한다.
  - `CMakeLists.txt:95-113`에서 `TestHarness`를 기본 빌드 타깃으로 정의하고 `Assets`에 의존시킨다.
  - Release flags가 `-DNDEBUG`를 포함한다.
  - 실제 Release 출력: `src/test_harness.cpp:32` `getModifier` unused, `145`, `149-150`, `178`, `183`, `197` 등 다수 `[-Werror=unused-variable]`; 최종 exit `2`.
  - 같은 산출물에서 `cmake --build ... --target Crawlmaster -j2`는 exit `0`이다.
- Expected Basis: 사용자 지정 Release 빌드 명령, `BUILD-001`, `TEST-001`; 릴리즈 게이트는 제품 타깃과 검증 타깃의 결과를 숨기지 않아야 한다.
- Actual: 전체 Release 빌드가 PASS가 아니며, 경고를 끄면 하네스의 assert 검증 자체가 제거된다.
- Impact: 배포 직전 검증이 실행 불능이고, 테스트가 우연히 빌드되더라도 항상 성공하는 거짓 녹색 상태가 될 수 있다.
- Suggested Action: assert 대신 Release에서도 동작하는 `CHECK/EXPECT` 또는 검증 라이브러리를 도입하고, 테스트를 별도 실행 가능한 타깃으로 유지한다. 제품 전체 Release 빌드와 Release 하네스 실행을 CI gate로 묶는다. 테스트에만 경고를 무조건 끄는 방식은 검증력 저하를 감추므로 사용하지 않는다.
- Re-audit Method: 새 clean Release 디렉터리에서 `cmake --build ... -j2`가 모든 타깃을 성공하는지 확인하고, `TestHarness`를 Release로 실행했을 때 실패 입력이 non-zero가 되는지 확인한다.
- Confidence: High
- Notes: Debug 전체 빌드와 Debug 하네스는 성공했으나 그것은 Release gate를 대체하지 않는다.

### [A05-F002] CTest 미등록 및 하네스가 생산 런타임·실패 모드를 검증하지 않음

- Area: Test registration / regression quality / production-path coverage
- Pass: Debug / Engineering Quality
- Pattern: TEST-001
- Severity: Major
- Status: Confirmed
- Summary: 테스트 실행 파일은 존재하지만 CTest에 등록되지 않았고, 하네스의 상당수 단언은 실제 구현을 호출하지 않거나 실제 저장 스키마·FSM을 검증하지 않는다.
- Evidence:
  - `CMakeLists.txt:95-113`에 `enable_testing()`, `include(CTest)`, `add_test()`가 없다.
  - `ctest --test-dir build/audit-debug --output-on-failure`가 `No tests were found!!!`를 출력했다.
  - `src/test_harness.cpp:800-804`는 `--run-all`을 선택적으로 해석할 뿐 항상 모든 함수를 직접 호출한다.
  - `CMakeLists.txt:96-106`의 `TEST_SOURCES`는 모델/Localization만 포함하고 `Game`, `GameStateManager`, controller, renderer를 포함하지 않는다. 따라서 startup chain, Settings push/pop, Dungeon→Combat→TPK FSM은 테스트하지 않는다.
  - `src/test_harness.cpp:57-69`의 JSON 테스트는 제품의 `gold`가 아닌 `party_gold`, 존재하지 않는 `depth`를 만든 모의 JSON만 파싱한다.
  - `src/test_harness.cpp:375-387`, `493-574`의 장비 제한/스왑은 생산 UI/모델 경로가 아니라 테스트 안의 lambda와 수동 시뮬레이션이다.
  - `src/test_harness.cpp:190-203`의 손상 복구 테스트는 메모리 기본값만 확인하고, 손상된 지정 경로가 복구·교체되었는지 확인하지 않는다.
- Expected Basis: `TEST-001`, `DBG-001`, `DBG-002`; 문서의 “전체 회귀 테스트” 주장은 CTest/생산 경로와 실패 모드를 재현해야 한다.
- Actual: CI/개발자가 `ctest`를 실행해도 아무 테스트가 실행되지 않으며, Debug 하네스의 exit `0`은 FSM·앱 기동·쓰기 실패·TPK 복구를 보증하지 않는다.
- Impact: 기능 변경이나 저장 회귀가 테스트를 통과한 것으로 오인되고, `audit_roadmap.md`의 100%/통과 주장을 독립적으로 재현할 수 없다.
- Suggested Action: production core를 라이브러리/오브젝트 타깃으로 분리해 하네스와 앱이 같은 코드를 링크하고, 각 테스트를 `add_test()`로 등록한다. 실제 `Party` round-trip/schema fixture, corruption repair, read-only/write-failure, state-stack/TPK, resource bootstrap을 failure-specific 테스트로 추가한다.
- Re-audit Method: `ctest --test-dir <clean-build> -N`에 명시적인 테스트 수가 나타나는지 확인하고, 실패 fixture마다 실제 생산 API를 호출해 오류 시 non-zero와 보존/복구 결과를 확인한다.
- Confidence: High

### [A05-F003] 배포 산출물이 재배치 불가능하고 작업 디렉터리에 의존함

- Area: Packaging / runtime assets / platform paths / dynamic dependencies
- Pass: Debug / Engineering Quality
- Pattern: BUILD-001
- Severity: Major
- Status: Confirmed
- Summary: 현재 CMake는 자산을 build tree에 복사할 뿐 install/package 계약을 만들지 않는다. 바이너리에는 개발자 build tree의 절대 SFML RUNPATH가 들어가고, 자산·저장·설정은 현재 작업 디렉터리 또는 호스트 전용 폰트 경로를 사용한다.
- Evidence:
  - `CMakeLists.txt:86-93`은 `${CMAKE_CURRENT_BINARY_DIR}/assets`로 `copy_directory`만 수행한다. first-party `install(TARGETS ...)`, 자산 install, 패키지 실행 파일 배치 규칙이 없다.
  - `src/core/Game.cpp:99-111`은 `/usr/share/fonts/...`를 먼저 시도한 뒤 `assets/fonts/...` 상대 경로를 사용한다.
  - `src/core/LocalizationManager.cpp:60-73`은 `assets/lang/...` 상대 경로만 탐색한다.
  - `include/model/Party.hpp:65`, `include/core/LocalizationManager.hpp:38-39`의 기본 저장/설정 경로는 각각 `./save.json`, `./config.json`이다.
  - `readelf -d build/audit-commercial/Crawlmaster`의 `RUNPATH`는 `/home/eunho1/Projects/cpp/crawlmaster/build/audit-commercial/_deps/sfml-build/lib`이며 SFML shared libraries가 `NEEDED`로 남아 있다.
  - Build tree에서 자산이 복사된 상태에서는 실행이 시작되지만 이는 이동된/설치된 산출물의 증거가 아니다.
- Expected Basis: `BUILD-001`, 사용자 목표의 앱 기동·자산 경로·플랫폼 경로; 상용 실행 파일은 설치 위치와 실행 cwd에 독립적인 자산 및 라이브러리 해석을 가져야 한다.
- Actual: 실행 파일만 배포하거나 다른 PC/다른 위치로 옮기면 절대 RUNPATH의 SFML을 찾지 못할 수 있고, cwd에 따라 서로 다른 세이브를 읽거나 자산·번역을 못 읽을 수 있다. Windows에는 SFML DLL을 exe 옆에 배치하는 규칙도 없다.
- Impact: 데스크톱 런처에서 작업 디렉터리가 달라질 때 기동 실패, 빈 폰트/번역, 저장 파일 분산 또는 권한 거부가 발생해 출시 산출물로 신뢰할 수 없다.
- Suggested Action: CMake install/package를 정의하고 SFML runtime 및 자산을 함께 배치한다. Linux `$ORIGIN`, macOS `@loader_path`, Windows DLL 배치를 플랫폼별로 검증한다. 리소스는 실행 파일/설치 루트 기준 resolver로 찾고, 세이브/설정은 XDG/`%APPDATA%` 등 사용자 데이터 디렉터리로 분리한다.
- Re-audit Method: 임시 install prefix에 설치한 뒤 임의 cwd에서 실행하고 `ldd`, 자산 로드, 사용자 데이터 생성 위치를 확인한다. 설치 경로 밖의 개발자 절대 경로가 ELF/Mach-O/Windows 런타임 메타데이터에 남지 않아야 한다.
- Confidence: High

### [A05-F004] headless·결정적 런타임 smoke 경로가 없고 디스플레이 부재 시 abort함

- Area: Startup chain / headless testability / runtime failure handling
- Pass: Debug / Engineering Quality
- Pattern: DBG-001
- Severity: Major
- Status: Confirmed
- Summary: `main`은 SFML 창을 즉시 만들고 이벤트 루프를 돌리는 경로뿐이며, production state/startup을 UI 없이 검사하는 명령이나 테스트가 없다. 표시 장치가 없으면 사용자 친화적 오류가 아니라 abort한다.
- Evidence:
  - `src/main.cpp:7-12`는 `Game` 생성 후 `run()`만 호출하고 headless/smoke 인자를 제공하지 않는다.
  - `src/core/Game.cpp:11-37`은 `sf::RenderWindow`와 무한 이벤트 루프를 기동한다.
  - TestHarness는 `Game`/controller를 링크하지 않아 이 부팅 체인을 실행하지 않는다.
  - `env DISPLAY= timeout 5s ./build/audit-commercial/Crawlmaster`는 `Failed to open X11 display` 후 exit `134`였다.
  - 표시 가능한 세션에서 `timeout 5s ./Crawlmaster`는 창 대기 후 exit `124`로 끝났으며, 자동 종료 가능한 smoke 결과를 만들지 못했다.
  - `BUILD_GUIDE.md:80`도 Linux sandbox/CLI headless를 체크리스트 미완료 항목으로 남긴다.
- Expected Basis: `DBG-001`, `DBG-002`; 사용자 감사 계약의 headless/결정적 테스트 요구. 데스크톱은 표시 장치가 필요하더라도 CI용 core smoke와 명시적인 no-display 오류 경로가 필요하다.
- Actual: CI에서 startup path, font/config bootstrap, FSM wiring을 자동 판정할 수 없고, X11 없는 환경에서는 프로세스가 비정상 종료한다.
- Impact: 패키지 누락·자산 경로·상태 전이 회귀가 실제 사용자 실행 전까지 드러나지 않으며, 지원 환경에서의 실패 진단도 어렵다.
- Suggested Action: SFML 의존 core/model을 headless simulation으로 분리하고, `--headless --smoke` 또는 테스트 전용 bootstrap으로 상태 전이·저장·리소스 계약을 검증한다. GUI 실행 전 `RenderWindow` 실패를 잡아 명확한 오류 코드/메시지를 반환하고, Xvfb 기반 GUI smoke도 별도 등록한다.
- Re-audit Method: 표시 장치 없이 headless smoke가 non-zero/expected output으로 종료하고, 표시 장치가 있을 때 packaged GUI smoke가 자동 종료하며 Title→Town→Dungeon→Combat의 최소 경로를 검증하는지 확인한다.
- Confidence: High
- Notes: PC 게임이 GUI를 요구한다는 사실은 인정하지만, 그것은 headless 회귀 검증 부재를 해소하지 않는다.

### [A05-F005] Save/Config 저장이 원자적이지 않고 손상 복구가 지정 경로·쓰기 성공을 보장하지 않음

- Area: Save/config durability / corruption recovery / write failure
- Pass: Debug / Engineering Quality
- Pattern: TEST-001
- Severity: Major
- Status: Confirmed
- Summary: `save.json`과 `config.json` 모두 기존 파일을 직접 truncate한 뒤 쓰며 flush/rename/fsync/백업 또는 실제 write status 확인이 없다. 손상 세이브를 지정 경로로 로드할 때 복구는 인자로 받은 경로가 아니라 static 기본 경로에 기록된다.
- Evidence:
  - `src/model/Party.cpp:120-127`은 `std::ofstream file(filePath)` 후 `file << j.dump(4)`하고 stream 상태를 확인하지 않은 채 `true`를 반환한다.
  - `src/core/LocalizationManager.cpp:129-135`도 같은 direct-truncate 패턴으로 config를 기록한다.
  - `src/model/Party.cpp:183-201`의 load catch는 `resetToDefault()`를 호출하고, `resetToDefault()`는 `saveToFile()`에 `filePath`를 전달하지 않는다. 따라서 `loadFromFile(customPath)` 실패 시 customPath가 아닌 `getDefaultSavePath()`가 대상이 된다.
  - Debug 하네스 출력에서도 custom `party-save.json` 손상 후 `[Save] .../save.json`가 기록되며, 테스트는 `party-save.json` 교체/보존을 확인하지 않는다.
  - `src/controller/CombatState.cpp:229-236`, `src/model/Party.cpp:200`, `src/controller/TownState.cpp`의 여러 저장 호출 및 `src/controller/SettingsState.cpp:22-25,60-64`는 bool 반환을 검사하지 않는다.
  - `src/core/LocalizationManager.cpp:95-119`는 config 값을 범위/enum 검증 없이 멤버에 먼저 대입하며, 파싱 중 예외가 나도 transactional rollback이 없다.
- Expected Basis: 사용자 계약의 저장/복구/쓰기 실패 검사, `SEC-001`, `TEST-001`; 기존 세이브를 보존한 채 같은 슬롯을 원자적으로 교체하고 실패하면 명시적으로 실패해야 한다.
- Actual: 프로세스 종료/디스크 부족/권한 실패 중간에 유효한 세이브가 빈 파일 또는 부분 JSON이 될 수 있다. corrupt custom path는 남고 다른 파일이 기본값으로 덮일 수 있으며, TPK와 설정 UI는 실패를 성공처럼 진행한다.
- Impact: 유료 게임 진행·설정의 영구 손실, 손상 파일 재현 불가, TPK가 실패했을 때 죽은 파티가 재시작에서 되살아나는 등 데이터 무결성 위반이 가능하다.
- Suggested Action: 동일 디렉터리 임시 파일에 serialize→flush→fsync 후 atomic rename하고 이전 파일을 제한된 backup으로 보존한다. 로드 전에 schema/range를 검증해 임시 객체로 완전히 구성하고, 손상 시 원래 `filePath`에 명시적 복구를 수행하거나 복구 실패를 반환한다. config는 language enum과 0..100 볼륨을 clamp/거부하고, 모든 호출자가 반환값을 사용자 오류·재시도 경로로 전달하도록 한다.
- Re-audit Method: read-only 디렉터리, 파일 경로가 디렉터리인 경우, truncated JSON, 타입은 맞지만 범위를 벗어난 JSON, custom save path를 각각 fixture로 실행한다. 원본 바이트 보존/backup, 대상 경로 교체, 오류 코드, 재시작 후 상태를 확인한다.
- Confidence: High

### [A05-F006] CharacterInfoState의 장비·소모품 변경이 자동 저장되지 않음

- Area: State lifecycle / progression persistence
- Pass: Debug / Engineering Quality
- Pattern: DBG-001
- Severity: Major
- Status: Confirmed
- Summary: 캐릭터 화면에서 수행하는 장비 장착·해제와 소모품 사용은 파티 모델을 변경하지만 성공 직후 `Party::saveToFile()`을 호출하지 않는다.
- Evidence:
  - `src/controller/CharacterInfoState.cpp:285-309`는 consumable 효과와 `party.removeItem()` 후 저장 호출이 없다.
  - `src/controller/CharacterInfoState.cpp:376-397`는 장비 스왑 및 인벤토리 변경 후 저장 호출이 없다.
  - `src/controller/CharacterInfoState.cpp:421-426`은 장착 해제 후 저장 호출이 없다.
  - 반면 Town 거래/모집 경로는 `src/controller/TownState.cpp:89,112,149,175,201,221,229,243,252`에서 저장을 호출하므로 저장 정책이 상태별로 갈라져 있다.
  - `README.md:17-20`, `spec.md:373-383`은 캐릭터/인벤토리 변경 및 세이브 반영을 사용자 계약으로 설명한다.
- Expected Basis: 사용자 기능의 변경이 앱 종료·재기동 후 보존되어야 한다는 저장 계약과 상태 수명 불변조건.
- Actual: 캐릭터 화면을 닫은 뒤 다른 저장 이벤트 없이 창을 닫으면 메모리 변경이 사라진다. Debug 하네스의 장비 테스트는 모델을 직접 조작하고 저장 후 재로드하는 실제 `CharacterInfoState` 경로를 검증하지 않는다.
- Impact: 플레이어가 장비 교체·치료·해독·아이템 소모를 성공 메시지와 함께 수행했는데 진행이 롤백되어 상용 신뢰를 훼손한다.
- Suggested Action: 성공한 단일 mutation을 파티의 transactional command로 묶고 즉시 저장하거나 명확한 dirty-save lifecycle을 사용한다. 저장 실패 시 변경을 되돌리거나 재시도 UI를 제공한다.
- Re-audit Method: 실제 `CharacterInfoState` 입력을 headless/integration fixture로 실행해 장착·해제·소모 후 프로세스를 종료하고 새 Party가 같은 장비·인벤토리·HP/상태를 복구하는지 확인한다.
- Confidence: High

### [A05-F007] 문서 세이브 스키마와 구현 스키마가 다르고 기본 리셋 인벤토리도 다름

- Area: Save schema / documentation-code contract / default data
- Pass: Implementation Compliance
- Pattern: IMP-001
- Severity: Major
- Status: Confirmed
- Summary: `spec.md`가 동결한 JSON 필드명과 초기 인벤토리 계약이 현재 serializer/loader 및 루트 세이브와 일치하지 않는다.
- Evidence:
  - `spec.md:289-306`은 초기 인벤토리를 `pot_heal` 2개와 `pot_mana` 1개로 정의한다. `src/model/Party.cpp:191-198`의 `resetToDefault()`는 `pot_heal` 2개만 추가하며, 실제 `save.json:1-9`도 두 개만 가진다.
  - `spec.md:313-330`의 캐릭터 예시는 `maxHp`, `spellSlots`, `poisonTurns`, `equipment.weapon` 및 능력치 `strength` 등의 camelCase/nested 구조다.
  - `src/model/Character.cpp:384-413`은 `max_hp`, `spell_slots`, `poison_turns`, `eq_weapon`, 능력치 `str/dex/con/int/wis/cha`를 쓰고, `417-469`에서 같은 구현 전용 키만 읽는다.
  - `src/model/Party.cpp:135-179`는 필수 키·enum·수치 범위를 schema로 검증하지 않는다. members loader는 `Party::addMember`의 4인 제한을 거치지 않고 `161-167`에서 직접 push한다.
  - `src/test_harness.cpp:57-69`는 이 실제 스키마가 아닌 `party_gold`/`depth` 모의 객체만 검증한다.
- Expected Basis: `spec.md`의 Master Plan 및 Save File Contract, `IMP-001`, `IMP-003`, `TEST-001`; 하나의 canonical versioned schema와 기본 데이터가 코드·문서·fixture에서 일치해야 한다.
- Actual: spec-shaped 파일은 구현 loader에서 예외가 나 기본값으로 전환될 수 있고, 손상/악의적 파일의 음수 HP·범위 밖 enum·5명 초과 members가 유효 저장처럼 받아들여질 수 있다. 초기 플레이어에게 문서상 마나 물약도 지급되지 않는다.
- Impact: 업데이트/지원 도구/향후 migration이 세이브를 잃거나 잘못된 파티를 복구하고, 저장 데이터가 UI/전투 불변조건을 깨뜨릴 수 있다.
- Suggested Action: canonical schema를 결정하고 `schema_version`과 명시적 migration을 추가한다. 필드명·기본 인벤토리·최대 4인·HP/level/slot/enum/quest 범위를 코드와 fixture로 검증한다. 기존 파일을 자동 reset하기 전에 backup과 사용자 경고를 둔다.
- Re-audit Method: spec-shaped fixture, 현재 serializer round-trip, 누락/추가/unknown ID, 5인/음수/범위 밖 숫자를 모두 로드하고 기대되는 migration·거부·보존 결과를 확인한다.
- Confidence: High

### [A05-F008] 던전 진행 상태가 세이브되지 않아 재입장·재시작 시 맵/좌표가 사라짐

- Area: Dungeon state persistence / resume behavior
- Pass: Implementation Compliance
- Pattern: IMP-001
- Severity: Major
- Status: Confirmed
- Summary: 사용자 문서가 던전 좌표와 진행 상황을 `save.json`에 저장한다고 주장하지만 `Party`/세이브 포맷에는 map, player position, direction, visited/stepped가 없고 새 던전 상태는 매번 새 미로를 생성한다.
- Evidence:
  - `README.md:19`, `README.md:57`은 던전 진행 상황/좌표를 JSON에 영속화한다고 설명한다.
  - `spec.md:289-306`의 저장 정책과 예시에는 좌표·맵·방향·FOW가 없다. 이 문서 내부에서도 요구와 파일 계약이 갈린다.
  - `include/model/Party.hpp:61-65`에는 members/inventory/quests/gold만 있고 DungeonMap 필드가 없다.
  - `src/controller/DungeonState.cpp:13-16`은 생성자마다 `m_map.generate()`를 호출하며, 이동 경로 `110-125`, `137-153`, `240-257`에는 Party 저장이 없다.
  - `src/controller/TownState.cpp:61-63`은 마을에서 던전 진입 시 새 `DungeonState`를 `changeState`한다.
- Expected Basis: README의 좌표/진행 저장 주장과 사용자 감사 계약의 저장·복구 품질. 요구가 철회될 경우 문서에서 명시적으로 제거해야 한다.
- Actual: 전투 push/pop 동안만 현재 `DungeonState`가 메모리에 남고, 마을 복귀 후 재입장하거나 앱을 재시작하면 새로운 무작위 맵과 `(1,1)`이 시작점이 된다.
- Impact: 탐험·오토맵 진행을 잃고 사용자가 세이브가 되었다고 믿는 상태와 실제 제품 동작이 다르다.
- Suggested Action: `DungeonState` snapshot(맵 seed 또는 타일, 위치, 방향, visited/stepped, schema version)을 파티 세이브에 넣거나, 진행 저장을 비목표로 명시하고 README/acceptance를 수정한다. 저장 시점을 이동/전투/마을 전환에 맞춰 정의한다.
- Re-audit Method: 던전에서 이동·FOW·전투 복귀 후 저장하고 프로세스를 재기동해 동일 map hash, 좌표, 방향, FOW가 복구되는지 확인한다. 비영속 결정이면 문서와 테스트가 그 사실을 명시하는지 확인한다.
- Confidence: High
- Notes: 좌표 저장 여부는 spec 내부에서 모순되므로 canonical 요구사항 결정 자체는 `Needs Spec Clarification`이다.

### [A05-F009] TPK가 stale DungeonState를 남기고 문서의 GameOverState를 생략함

- Area: FSM stack lifetime / destructive TPK recovery
- Pass: Debug / Engineering Quality
- Pattern: DBG-001
- Severity: Major
- Status: Confirmed
- Summary: 전투가 던전 위에 push된 상태에서 TPK가 `changeState(TitleState)`를 호출한다. `changeState()`는 top 한 개만 pop하므로 숨겨진 DungeonState가 남고, 문서에 정의된 GameOverState도 실제 소스에 없다.
- Evidence:
  - `src/core/GameStateManager.cpp:9-16`의 `changeState()`는 스택 최상단 하나만 제거한다.
  - `src/controller/DungeonState.cpp:124-125`는 기존 DungeonState 위에 CombatState를 push한다.
  - `src/controller/CombatState.cpp:229-237`은 TPK에서 `resetToDefault()` 후 `changeState(std::make_unique<TitleState>)`를 호출한다.
  - 따라서 `[DungeonState, CombatState]`에서 `[DungeonState, TitleState]`가 된다. Title에서 Exit 또는 새 게임을 선택하면 stale DungeonState가 다시 아래에 노출될 수 있다.
  - `spec.md:118-121`은 `CombatState -> GameOverState -> TitleState`를 정의하지만 `rg` 결과 `src/include`에 `GameOverState` 구현 파일/심볼이 없다.
- Expected Basis: `spec.md` FSM, `DESIGN_DECISIONS.md:46-54`, 상태 스택의 수명 불변조건; TPK 이후에는 새 세션 루트만 남아야 한다.
- Actual: TPK 후 타이틀 아래에 이전 맵/자동이동/로그를 가진 DungeonState가 남으며, GameOver 전용 화면·명시적 복귀 단계가 없다.
- Impact: 하드코어 리셋 뒤 잘못된 게임 상태가 재노출되고, Title 종료/새 게임의 상태 수명이 오염된다. `resetToDefault()` 저장 실패 가능성은 A05-F005와 결합하면 이전 세이브가 재시작에서 남을 수 있다.
- Suggested Action: `resetToDefault`/TPK 전용 transition이 전체 stack을 clear하고 새 Title/GameOver root를 만들도록 한다. GameOverState를 실제 구현할지, 직접 Title로 바꿀지 spec과 UX를 동기화하고 TPK 결과를 명시적으로 전달한다.
- Re-audit Method: fake GameState를 이용해 Title→Town→Dungeon→Combat stack을 구성하고 TPK transition 후 stack depth/current type, Title Exit/New Game 결과를 확인한다. 저장 성공·실패 각각에서 재기동 세이브도 확인한다.
- Confidence: High

### [A05-F010] RNG가 분산된 random_device에 고정되어 replay/seed 재현이 불가능함

- Area: Deterministic debugging / dungeon and combat randomness
- Pass: Debug / Engineering Quality
- Pattern: DBG-002
- Severity: Major
- Status: Confirmed
- Summary: 맵 생성, 인카운터, 몬스터 스폰/AI, 전투, 스킬, 능력치, 레벨업, 물약이 각각 새로운 `random_device`/`mt19937`를 생성하며 공용 RNG 또는 seed 입력이 없다. Town 캐릭터 직업은 별도의 seed 없는 C `rand()`를 쓴다.
- Evidence:
  - `src/model/DungeonMap.cpp:216-219,237-264`는 DFS 재귀 및 loop마다 새 RNG를 만든다.
  - `src/controller/DungeonState.cpp:119-122,146-149,251-254`, `src/controller/CombatState.cpp:167-183,321-323,536-538,571-575,710-713`, `src/model/MonsterFactory.cpp:52-55`, `src/model/Skill.cpp:14-29`, `src/model/Character.cpp:55-58,151-153,298-305`, `include/model/Monster.hpp:64-80,99-107`, `include/model/ConcreteItems.hpp:46-66`에 독립 RNG가 분산되어 있다.
  - `src/controller/TownState.cpp:83`은 `rand() % 4`를 사용하며 프로젝트에 `srand`/seed 정책이 없다.
  - `DungeonMap`/Combat/seed를 주입하는 API나 TestHarness의 동일 seed map/roll snapshot 검증이 없다.
- Expected Basis: 사용자 감사 계약의 난수 재현성, `DBG-002`; 복잡한 상태/전투는 동일 seed와 입력으로 재현 가능해야 한다.
- Actual: 고객 신고/QA 실패의 map, encounter, initiative, damage sequence를 재현할 seed/event log가 없고, 서로 다른 RNG 정책이 결과 기록을 분리한다. Town 직업 선택은 프로세스마다 동일한 C rand 초기 상태가 될 가능성이 있다.
- Impact: flaky regression과 디버깅 비용 증가, 밸런스 검증 불능, 저장/재생 또는 support replay 설계 불가.
- Suggested Action: seedable `RngService`를 Game/session에 주입하고 map/combat/action stream을 명시적으로 분리한다. seed와 event log 또는 replay token을 기록하고, deterministic fixture/hash와 확률 분포 테스트를 추가한다. 운영에서는 cryptographic randomness가 필요한 것이 아니므로 `random_device`를 매 action마다 호출하지 않는다.
- Re-audit Method: 고정 seed로 맵 타일/hash, 스폰, initiative, dice log를 두 번 생성하고 byte-for-byte 일치시키며, 저장/복원 후 같은 stream 위치에서 이어지는지 확인한다.
- Confidence: High

### [A05-F011] dependency 버전 선택은 문서의 고정 버전·재현성 계약보다 느슨함

- Area: Dependency parity / reproducible build
- Pass: Debug / Engineering Quality
- Pattern: DEP-001
- Severity: Major
- Status: Confirmed
- Summary: BUILD_GUIDE는 SFML 2.6.1을 동결했다고 쓰지만 CMake는 `find_package(SFML 2.6)`로 임의의 2.6.x 시스템 패키지를 우선 사용하고, FetchContent Git tag에는 immutable commit/hash 검증이 없다.
- Evidence:
  - `BUILD_GUIDE.md:23-27`은 SFML `2.6.1`, json `3.11.3`을 고정한다고 설명한다.
  - `CMakeLists.txt:24-30`은 json Git tag `v3.11.3`, `32-50`은 SFML `find_package(SFML 2.6)` 후 Git tag `2.6.1` fallback을 사용한다.
  - 루트 `CMakeLists.txt`에 lockfile, archive hash, fetched source provenance 검증, system package의 exact patch/version 거부 규칙이 없다.
  - 현재 환경은 system SFML을 찾지 못해 FetchContent 경로를 사용했지만, 다른 호스트에서는 system 2.6.x가 선택될 수 있다.
- Expected Basis: `DEP-001`, `BUILD-001`; 문서 버전과 실제 configure 경로가 같은 canonical dependency policy를 가져야 한다.
- Actual: 같은 소스가 호스트마다 다른 SFML patch를 링크할 수 있고 Git tag/네트워크 상태에 따라 clean build가 달라진다.
- Impact: Release binary, warning behavior, ABI/runtime 결과의 재현성이 떨어지고, 상용 hotfix를 동일하게 재빌드하기 어렵다.
- Suggested Action: exact version을 검증하고 system package fallback을 명시적 opt-in으로 제한하거나, immutable commit/archive hash와 offline cache/lock manifest를 사용한다. CI에서 dependency provenance와 최종 linked version을 기록한다.
- Re-audit Method: system SFML 2.6.x와 FetchContent 환경을 각각 configure해 선택 경로·버전·ABI가 정책과 일치하는지 확인하고, clean offline rebuild를 반복한다.
- Confidence: Medium-High
- Notes: FetchContent로 내려받은 dependency 내부 소스는 제외했으며, 이 finding은 루트의 선택/버전 정책만 판정한다.

## 6. Uncertainties and Clarifications Needed

- `README.md`는 던전 좌표/진행 저장을 약속하지만 `spec.md`의 Save File Contract에는 좌표·맵·FOW가 없다. “던전 진행을 저장”할지 “세션 중에만 유지”할지 canonical 요구사항을 확정해야 한다. (A05-F008)
- `spec.md`/`DESIGN_DECISIONS.md`는 GameOverState를 정의하지만 현재 구현은 Title로 직접 전환한다. GameOver 전용 UX가 출시 범위인지, 문서가 낡은 것인지 결정해야 한다. (A05-F009)
- 데스크톱 제품의 표시 장치 요구 자체는 명확하나, CI/지원 도구에서 headless simulation과 X11 없는 실행의 오류 계약(메시지/exit code)은 정의되어 있지 않다. (A05-F004)
- 외부 폰트 재배포 권리와 system-font 의존성은 코드만으로 확정할 수 없으며 출시 전 Human Review가 필요하다. 이 보고서는 라이선스 판정을 하지 않았다.

## 7. Perspective Decision

**HOLD**

Debug 경로는 빌드와 현재 하네스를 통과했고 build-tree 내 자산 복사도 관찰되었지만, 상용 후보 관점에서 Release 전체 빌드 실패(A05-F001), CTest/생산 경로 검증 공백(A05-F002), 재배치 불가능한 산출물(A05-F003), 저장·TPK·상태 수명 위험(A05-F005~F008), stale FSM(A05-F009), seed 없는 핵심 난수(A05-F010)가 남아 있다. 따라서 이 관점에서 `PASS` 또는 `PASS WITH KNOWN RISKS`로 올릴 수 없다.

우선순위는 다음과 같다.

1. Release 테스트 게이트와 CTest/생산 경로를 복구한다(A05-F001, A05-F002, A05-F004).
2. 원자적 저장·동일 경로 손상 복구·쓰기 실패 처리 및 캐릭터 변경 저장을 고정한다(A05-F005, A05-F006).
3. canonical 세이브 스키마/던전 진행 요구와 TPK stack 전환을 결정·구현한다(A05-F007~F009).
4. 재배치 가능한 패키지와 seedable RNG/replay 표면을 추가한다(A05-F003, A05-F010, A05-F011).

## 8. Re-audit Checklist

- [ ] clean Debug/Release에서 `Crawlmaster`와 `TestHarness` 전체 빌드 성공
- [ ] `ctest -N`에 실제 등록 테스트가 나타나고 Debug/Release에서 실행 성공
- [ ] Release 테스트의 단언이 `NDEBUG`에서도 유지되고 의도된 실패가 non-zero가 됨
- [ ] headless core smoke 및 Xvfb/실제 GUI smoke가 startup/FSM/resource chain을 검증
- [ ] install/package 산출물을 임의 cwd에서 실행하고 absolute developer RUNPATH가 없음
- [ ] 사용자별 플랫폼 데이터 디렉터리와 executable-relative assets 검증
- [ ] save/config atomic rename, flush/fsync, backup, write-failure/error UI 검증
- [ ] corrupt custom path가 같은 슬롯으로 복구되고 원본/backup 보존 검증
- [ ] CharacterInfo 장비/소모품 변경 후 종료·재기동 round-trip 검증
- [ ] canonical schema/version/migration, 4인 제한 및 숫자/enum 범위 검증
- [ ] 던전 진행 저장 여부가 확정되고 map/seed/FOW 또는 문서 비목표가 일치
- [ ] TPK 후 stack depth/current state와 save success/failure, GameOver 결정 검증
- [ ] 고정 seed map/combat/replay 결과가 반복 실행에서 동일
- [ ] exact dependency provenance와 system/fetched 경로 parity 검증

## 9. Coder Handoff

```text
`/mnt/Projects_SSD/cpp/crawlmaster/docs/multi_audit/1/sub_audit_05_engineering_runtime.md`를 먼저 읽고, 각 finding을 관련 프로젝트 문서와 실제 코드에 대조하여 우선순위대로 수정하세요. 계약 변경이 필요하면 spec.md와 관련 문서를 먼저 갱신하고, 수정 후 clean Debug/Release 빌드, CTest, headless/GUI smoke, 저장·TPK·seed 재현성 검증 결과를 기록하세요.
```
