# Turn 2 수정 및 독립 재감사 보고서

작성일: 2026-09-03 (Asia/Seoul)  
대상 프로젝트: Crawlmaster 0.9.4  
원본 감사: `../1/final_audit_report_1.md`  
수정 계획: `remediation_plan_2.md`  
최종 판정: **HOLD — 로컬 구현 차단 결함은 대부분 해소됐으나 외부 출시 gate가 남음**

## 1. 범위, 계보와 판정 원칙

- Turn 1 최종 보고서와 6개 sealed source report는 수정하지 않았다.
- 원본 16개 canonical finding ID를 유지해 현재 문서, 실제 호출 경로, 테스트와 패키지를 다시 대조했다.
- 제품 계약 변경은 구현 전에 `spec.md`, `DESIGN_DECISIONS.md`, `designs.md`, `audit_roadmap.md`에 먼저 기록했다.
- 로컬에서 실행하지 않은 Windows hosted job, 5-locale 전체 화면 판독, 장시간 플레이와 법률 검토는 PASS로 승격하지 않았다.
- 현재 workspace는 Git 저장소로 인식되지 않으므로 commit, push와 source SHA 계보는 검증하지 않았다.

Turn 1 sealed manifest 재검증 결과:

```text
sub_audit_01_product_contract.md OK
sub_audit_02_interface_ux.md OK
sub_audit_03_game_systems.md OK
sub_audit_04_content_depth.md OK
sub_audit_05_engineering_runtime.md OK
sub_audit_06_commercial_release.md OK
source_report_manifest.json
  508933a51f84c6fbaa773888a40f3b273f1eeca65283a08bfafc878f27e6e1ac
source_report_manifest.sha256.json
  057eb72e8744f5dd549f76dc0e893d50f6764c219d56a2c584c2e5dbd9db93bf
```

## 2. 변경 전 기준선과 계약 선행 변경

### 변경 전 재현

- Debug 제품과 직접 실행 TestHarness는 통과했다.
- CTest에는 등록된 테스트가 없었다.
- Release에서는 `NDEBUG`로 `assert`가 제거된 뒤 미사용 변수 경고가 `-Werror`가 되어 TestHarness가 실패했다.
- 저장과 설정은 CWD에 직접 truncate했고 backup, atomic replace, quarantine, typed error가 없었다.
- 제품 문서와 구현에 boss/ending, 실제 party HUD, 명시적 모집·아이템·치유 대상 선택, install/package/CI가 없었다.

### 구현 전에 닫은 계약

- 제품 lane을 Linux/Windows용 **한 층 상용 데모 후보**로 정하고 가격은 TBD, 오디오는 비목표로 명시했다.
- 저장은 schema v2, per-user 경로, town-only checkpoint, v1 migration, TPK checkpoint 복구로 확정했다.
- 동일 session seed의 단일 RNG stream, encounter tier, boss/result와 일회성 보상을 정의했다.
- 모집과 전투 자원 사용은 preview → target → confirm/cancel이며 무효 효과는 비용을 소비하지 않도록 정의했다.
- Linux relocatable package, Windows CI 구성, checksum, notices와 human legal gate를 분리했다.

주요 계약 근거: `spec.md`, `DESIGN_DECISIONS.md`, `designs.md`, `audit_roadmap.md`.

## 3. Finding별 독립 대조 결과

| Finding | 재감사 상태 | 문서·코드·실행 증거 | 판정 |
| --- | --- | --- | --- |
| FIN-F001 | **Verified** | `spec.md`에 lane, 대상, OS, 30~60분 목표, 가격 TBD, 오디오 비목표, 접근성·패키지 gate를 기록하고 README/roadmap을 동기화했다. | 제품 계약 부재 해소. 실제 플레이 시간은 별도 외부 gate. |
| FIN-F002 | **Verified (local)** | seeded DungeonMap이 Door와 최원거리 BossGate를 만들고 boss 전투, Victory, campaign-complete 저장·재로드·재진입 차단을 production-linked controller test로 검증했다. | 완결 경로 부재 해소. 30~60분 수동 완주는 미측정. |
| FIN-F003 | **Verified** | 19 item registry, 9개 고급 아이템 drop source, 3개 canonical quest, 일회성 완료·보상과 boss 보장 drop을 content/controller test로 검증했다. | 선언 콘텐츠의 정상 경로 연결 완료. |
| FIN-F004 | **Verified** | `RecruitmentDraft`, `CombatActionRules`, item/ally target preview·confirm·cancel을 추가했다. full-HP/무효 대상은 슬롯과 턴을 소비하지 않는다. | 플레이어 선택 없는 자동 소비 해소. |
| FIN-F005 | **Verified** | 2d6, natural 1/20, Bless +2, Skeleton 저항, 장비 class/STR/양손 제약, `1d10 × tier` 보상을 고정 seed ledger로 검증했다. deserialize와 UI swap도 같은 장비 불변조건을 사용한다. | 문서와 계산 분리 해소. |
| FIN-F006 | **Verified (mechanism)** | `SessionRng` 하나로 생성·조우·전투·보상을 재현하고 early/middle/late pool과 tier를 고정했다. `random_device`는 최초 seed 생성 한 곳에만 남았다. | 재현 surface와 곡선 구조 해소. 실제 승률·TPK율 정량 tuning은 미검증. |
| FIN-F007 | **Verified** | Title에 New/Continue를 분리하고 overwrite confirm, 손상/seed/내구성 경고를 추가했다. 모집·판매·해고·아이템/스킬에 확인·취소 경계를 적용했다. | 사용자 의도 및 파괴적 동작 계약 해소. |
| FIN-F008 | **Verified** | immutable `PartyHudSnapshot`이 실제 0~4인 이름·직업·HP·dead·poison·paralysis·bless를 renderer에 전달한다. 0/1/4인과 snapshot 불변성을 테스트했다. | 고정 가짜 HUD 제거. |
| FIN-F009 | **Partial — Needs Fix** | 5 locale JSON은 모두 유효하고 189개 동일 keyset이며 Settings에 75~200% text scale/high contrast, 공통 최소 14px를 추가했다. 그러나 일부 item/monster/skill 설명과 combat log literal이 catalog 밖에 남고 5개 언어 전 화면 raster·wrap·focus·고DPI 판독은 실행하지 않았다. | **Major 출시 gate 유지.** |
| FIN-F010 | **Verified** | schema v2 canonical camelCase/nested Character, v1 migration, 인원/값/quest 중복·교차 검증, 명시적 New Game, town-only checkpoint와 campaign/seed 저장을 round-trip했다. | 스키마·초기값·진행 계약 충돌 해소. |
| FIN-F011 | **Verified (local fault gates)** | per-user path, size cap, symlink 거부, same-directory temp → file fsync → backup rotate → atomic replace → directory sync, corrupt quarantine/backup recovery, typed status를 구현했다. pre-commit I/O 실패와 post-commit durability-unknown을 주입해 rollback/경고를 검증했다. | 기존 Critical 데이터 손실 경계는 해소. 실제 프로세스 kill·물리 disk-full chaos는 미실행. |
| FIN-F012 | **Verified** | `GameStateManager::replaceAll`, `GameOverState`, 다음 update에서의 TPK 전이를 사용한다. production-linked test에서 update 전 비전이, 이후 root 1개와 town checkpoint 복구를 확인했다. | stale DungeonState 제거. |
| FIN-F013 | **Verified** | Release-safe CHECK, CTest 7개, 앱과 controller test가 공유하는 `CrawlmasterRuntime`을 구성했다. 최종 Debug/Release 모두 7/7 PASS다. | Release/test gate 복구. |
| FIN-F014 | **Partial** | executable-relative assets, install/CPack TGZ, SHA-256 sidecar, arbitrary-CWD Linux package smoke, Ubuntu/Windows CI 정의와 immutable action SHA를 추가했다. 최종 Linux package에 developer RPATH와 shared SFML 의존성이 없다. Windows hosted artifact와 실기 실행은 미검증이다. | Linux 로컬 경로는 해소, cross-platform 출시 gate 유지. |
| FIN-F015 | **Human Review Required** | SFML/json을 commit SHA로 고정하고 direct-dependency SPDX manifest, notices, license 원문과 package checksum을 추가했다. 번들 폰트 3개의 원출처·상업 재배포 권리, 완전한 transitive SBOM/scanner/signature, legal/support identity 승인은 확보되지 않았다. | **Major human/legal gate 유지.** |
| FIN-F016 | **Verified** | README, BUILD_GUIDE, IMPLEMENTATION_SUMMARY, designs, CHANGELOG, LESSONS_LEARNED가 현재 파일 책임과 완료/UNVERIFIED 경계를 설명한다. 존재하지 않는 renderer와 100% 완료 주장을 제거했다. | 현재 문서 과대 설명 해소. |

요약: **13개 finding의 로컬 구현 결함을 검증 완료**, FIN-F009와 FIN-F014는 Partial, FIN-F015는 Human Review Required다. FIN-F002의 목표 시간과 FIN-F006의 정량 밸런스는 finding의 구조적 결함은 해소됐지만 제품 검증 증거가 별도로 남는다.

## 4. 핵심 구현 증거

### 데이터 무결성과 FSM

- `include/core/Persistence.hpp`, `src/core/Persistence.cpp`: typed result, per-user path, size/symlink guard, atomic/backup/quarantine 경계
- `src/model/Party.cpp`: schema v2, migration, validation, explicit New Game, checkpoint/campaign metadata
- `src/core/LocalizationManager.cpp`: 설정 schema v2와 동일 atomic recovery
- `src/core/GameStateManager.cpp`: `replaceAll()` root 전이
- `src/controller/GameOverState.cpp`, `src/controller/VictoryState.cpp`: 지속 결과 화면
- `tests/test_controller_contracts.cpp`: TPK, 계단 저장, 저장 실패, boss single commit, reload/re-entry 차단과 durability warning

### 전투·콘텐츠·사용자 선택

- `src/core/SessionRng.cpp`, `src/model/CombatRules.cpp`, `src/model/MonsterFactory.cpp`: seed와 canonical 계산·tier
- `src/model/DungeonMap.cpp`: Door/BossGate와 도달성
- `src/model/ItemFactory.cpp`, `src/model/Quest.cpp`: canonical registry와 보상 경로
- `src/model/RecruitmentDraft.cpp`, `src/controller/CombatStateActions.cpp`: preview/target/confirm/cancel
- `tests/test_combat_contracts.cpp`, `tests/test_content_contracts.cpp`, `tests/test_agency_contracts.cpp`: 독립 계약 ledger

### UI·배포

- `src/view/PartyHudSnapshot.cpp`, `src/view/DungeonRenderer.cpp`: domain-derived HUD
- `src/controller/SettingsState.cpp`, `src/core/LocalizationManager.cpp`: text scale/high contrast
- `src/main.cpp`: `--verify-resources` 양성/부정 검증 진입점
- `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/ci.yml`: CTest/install/CPack/CI
- `THIRD_PARTY_NOTICES.md`, `DEPENDENCY_MANIFEST.spdx.json`, `licenses/`: 직접 의존성 고지

## 5. 최종 실행 증거

### Debug와 Release

```text
cmake -S . -B build/final-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/final-debug --parallel 2
ctest --test-dir build/final-debug --output-on-failure --no-tests=error
-> 100% tests passed, 0 failed out of 7

cmake -S . -B build/final-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/final-release --parallel 2
ctest --test-dir build/final-release --output-on-failure --no-tests=error
-> 100% tests passed, 0 failed out of 7
```

7개 테스트:

1. ResourceVerification
2. TestHarness
3. HudContractTests
4. CombatContractTests
5. ContentContractTests
6. AgencyContractTests
7. ControllerContractTests

### 설치, 패키지와 런타임

```text
cmake --install build/final-release --prefix build/final-release/install
cpack --config build/final-release/CPackConfig.cmake -B build/final-release/package
-> Crawlmaster-0.9.4-Linux-x86_64.tar.gz generated
-> SHA-256 sidecar generated

sha256sum -c Crawlmaster-0.9.4-Linux-x86_64.tar.gz.sha256
-> OK

package/bin/Crawlmaster --verify-resources  # /tmp에서 실행
-> exit 0, packaged locale/font resource 확인

xvfb-run -a timeout 2 package/bin/Crawlmaster  # /tmp에서 실행
-> exit 124, 의도된 timeout까지 창 프로세스 유지

assets 없는 격리 bin/Crawlmaster --verify-resources
-> exit 2, missing locale 검출
```

최종 패키지:

- 경로: `build/final-release/package/Crawlmaster-0.9.4-Linux-x86_64.tar.gz`
- 크기: 3,232,917 bytes
- SHA-256: `184131f0391e9f490f6f66e244aae86422d0ca1765291db4a1840e750039e321`
- ELF: RPATH/RUNPATH 없음, shared SFML NEEDED 없음. OS 제공 FreeType/X11/Xrandr/Xcursor/udev/C++ runtime은 필요하다.

### 정적·계보 검사

```text
5 locale JSON parse + scalar keyset diff
-> KO/EN/JA/ZH-TW/ZH-CN 모두 189 keys, 동일

rg 'random_device|std::rand|rand\(' include src tests
-> src/core/SessionRng.cpp의 최초 entropy seed 한 곳만 검출

Turn 1 source_report_manifest hash 대조
-> 6/6 sealed source report OK
```

CMake configure에는 vendored SFML의 구형 `cmake_minimum_required` deprecation warning이 남지만 build/test/package 결과에는 영향을 주지 않았다.

## 6. 독립 검토

### 구현 검토

- 결론: **APPROVE — Critical 0, Required 0**
- 확인: production-linked controller transcript, `CrawlmasterRuntime` 공유 링크, Debug/Release 7/7
- 추가 차단 finding: 없음

### 릴리스 검토

- 결론: **APPROVE — Critical 0, Required 0**
- 확인: Release 7/7, CPack checksum, package resource 양성/부정 대조군, arbitrary-CWD smoke
- Windows 실기와 폰트/legal은 `UNVERIFIED`/Human Review로 유지

이 APPROVE는 검토에 제출된 로컬 구현과 Linux 패키지 범위의 승인이다. 아래 상용 출시 HOLD gate를 PASS로 바꾸지 않는다.

## 7. 남은 위험과 다음 gate

1. **FIN-F009:** 사용자-facing literal을 모두 locale catalog로 이동하고, 5개 locale의 모든 State/substate에서 wrap, focus, keyboard/back, 대비, CJK raster와 고DPI를 캡처·판독해야 한다.
2. **FIN-F014:** GitHub hosted Ubuntu/Windows job을 실제 실행해 동일 source revision의 archive, checksum, resource verification과 Windows startup 증거를 보존해야 한다.
3. **FIN-F015:** 폰트별 source/version/license/redistribution 증거, full transitive SBOM·scanner·signature, LICENSE/EULA/privacy/support/credits의 human sign-off가 필요하다.
4. clean save에서 실제 30~60분 완주와 여러 seed의 승률/TPK율·경제/회복비를 측정해야 한다.
5. Git metadata가 복구되면 source revision과 hosted artifact의 동일성을 다시 확인해야 한다.

## 8. 최종 판정

**HOLD**

Turn 1에서 확정한 Critical 저장 손실 경계, Release 빌드 실패, CTest 부재, stale TPK FSM, 완결 경로·콘텐츠·HUD·선택 부재와 로컬 Linux 패키지 부재는 현재 구현과 테스트에서 해소됐다. 따라서 현재 트리는 더 이상 동일한 기능성 프로토타입 상태가 아니라 **로컬 Linux에서 검증된 한 층 상용 데모 후보**다.

그러나 FIN-F009의 전면 i18n/접근성 실화면, FIN-F014의 Windows hosted/실기, FIN-F015의 폰트·법률·공급망 human gate가 닫히지 않았다. 장시간 완주와 정량 밸런스도 미측정이다. 이 증거 없이 정식 유료 출시 PASS로 승격하지 않는다.

