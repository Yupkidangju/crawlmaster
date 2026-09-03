# Crawlmaster Turn 2 수정 독립 재감사 보고서 — Re-audit #1

작성일: 2026-09-03 (Asia/Seoul)  
대상: Crawlmaster 0.9.4 현재 작업 트리 및 Linux 최종 패키지  
원본 감사: `docs/multi_audit/1/final_audit_report_1.md`  
코더 제출 증거: `docs/multi_audit/2/remediation_reaudit_2.md`  
감사 기준: `AI_AUDIT_DOC_STANDARD.md`, 현재 `spec.md`, `DESIGN_DECISIONS.md`, `audit_roadmap.md`  
독립 최종 판정: **HOLD**

## 1. 감사 성격과 판정 원칙

`remediation_reaudit_2.md`의 finding별 표와 자체 `APPROVE`는 구현자가 제출한 완료 주장 및 증거 인덱스로 취급했다. 본 보고서는 그 판정을 승계하지 않고 현재 문서, 실제 소스, 테스트, fresh build와 최종 패키지를 독립적으로 대조한다.

- Turn 1의 `FIN-F001~FIN-F016` ID를 유지한다.
- 이전 sealed 보고서와 manifest는 수정하지 않는다.
- 로컬에서 확인하지 못한 Windows, 법률, 전체 locale 실화면, 장시간 플레이를 PASS로 올리지 않는다.
- 변경된 제품 계약이 이전 요구를 합리적으로 좁힌 경우 새 계약을 기준으로 판정하되, 문서와 구현이 실제로 일치해야 한다.
- 코드와 제품 문서는 수정하지 않고 본 감사 보고서만 추가한다.

## 2. 감사 범위와 제외 범위

### 확인 범위

- 제품 계약: `spec.md`, `DESIGN_DECISIONS.md`, `designs.md`, `README.md`, `BUILD_GUIDE.md`, `IMPLEMENTATION_SUMMARY.md`, `CHANGELOG.md`, `audit_roadmap.md`
- 구현: 전체 `include/`, `src/`, `tests/`, `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/ci.yml`
- 데이터/자산: 5개 locale JSON, 3개 font, root `save.json`, dependency manifest와 notices
- 실행: fresh Debug/Release build, CTest, install/CPack, archive checksum, arbitrary-CWD resource/GUI smoke, ELF linkage, 패키지 payload 비교
- 보존: Turn 1 source-report manifest 재검증

### 미검증

- Git commit/diff/source revision: 현재 workspace가 Git 저장소가 아님
- GitHub hosted Ubuntu/Windows workflow 실제 실행과 artifact
- Windows 실기 기동 및 UI
- 5 locale 전체 State/substate의 raster, wrap, focus, 고DPI 판독
- 실제 30~60분 완주, 다중 seed 승률/TPK율/경제 측정
- 폰트 재배포 권리와 제품 법률/지원 주체의 사람 승인
- 물리 disk-full, 전원 차단과 프로세스 kill 중 save chaos

## 3. 독립 명령 증거

### 3.1 Fresh Debug/Release

```text
cmake -S . -B build/independent-reaudit-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/independent-reaudit-debug --parallel 2
ctest --test-dir build/independent-reaudit-debug --output-on-failure --no-tests=error
-> 7/7 PASS, 0.30 sec

cmake -S . -B build/independent-reaudit-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/independent-reaudit-release --parallel 2
ctest --test-dir build/independent-reaudit-release --output-on-failure --no-tests=error
-> 7/7 PASS, 0.21 sec
```

실제 등록 테스트:

1. ResourceVerification
2. TestHarness
3. HudContractTests
4. CombatContractTests
5. ContentContractTests
6. AgencyContractTests
7. ControllerContractTests

### 3.2 최종 패키지와 fresh package

```text
sha256sum build/final-release/package/Crawlmaster-0.9.4-Linux-x86_64.tar.gz
-> 184131f0391e9f490f6f66e244aae86422d0ca1765291db4a1840e750039e321

(cd build/final-release/package &&
 sha256sum -c Crawlmaster-0.9.4-Linux-x86_64.tar.gz.sha256)
-> OK

cmake --install build/independent-reaudit-release
cpack --config build/independent-reaudit-release/CPackConfig.cmake
-> fresh TGZ와 SHA-256 sidecar 생성
```

최종 archive와 fresh archive의 외부 TGZ hash는 tar metadata 때문에 달랐지만, 14개 package member의 이름과 payload SHA-256은 모두 동일했다. 양쪽 `bin/Crawlmaster`의 SHA-256은 다음과 같았다.

```text
d7ccab0766787387957bd839fed02e161da9deac76de9e3394fa8e73d64776a1
```

ELF 직접 확인:

- RPATH/RUNPATH 없음
- shared SFML NEEDED 없음
- FreeType, X11, Xrandr, Xcursor, udev, C++ runtime 등 문서화된 OS runtime 의존성 존재
- binary string에서 개발자 home/build path 유출 없음

arbitrary-CWD package 실행:

```text
package/bin/Crawlmaster --verify-resources
-> exit 0

asset 없는 bin-only copy --verify-resources
-> exit 2

xvfb-run -a package/bin/Crawlmaster
-> 3초 동안 창 유지, timeout exit 124
```

### 3.3 무결성 및 정적 증거

- Turn 1 `report_integrity.py verify`: 6/6 source report OK
- source manifest SHA-256: `508933a51f84c6fbaa773888a40f3b273f1eeca65283a08bfafc878f27e6e1ac`
- 5개 locale: 각각 189 keys
- credential/secret pattern scan: 프로젝트 표준 문서의 일반 용어 외 제품 secret 발견 없음
- Git: `fatal: not a git repository`

## 4. Finding별 독립 재판정

| Finding | 코더 주장 | 독립 판정 | Gate |
| --- | --- | --- | --- |
| FIN-F001 | Verified | **Verified** | 해소 |
| FIN-F002 | Verified local | **Verified within current demo contract** | 장시간 완주는 별도 UNVERIFIED |
| FIN-F003 | Verified | **Verified** | 해소 |
| FIN-F004 | Verified | **Verified** | 해소 |
| FIN-F005 | Verified | **Needs Fix — Major** | HOLD |
| FIN-F006 | Verified mechanism | **Needs Fix — Major** | HOLD |
| FIN-F007 | Verified | **Verified** | 해소 |
| FIN-F008 | Verified | **Verified** | 해소 |
| FIN-F009 | Partial | **Needs Fix — Major** | HOLD |
| FIN-F010 | Verified | **Needs Fix — Minor residual** | 후속 수정 |
| FIN-F011 | Verified local | **Verified locally** | Critical 해소 |
| FIN-F012 | Verified | **Verified** | 해소 |
| FIN-F013 | Verified | **Verified** | 해소 |
| FIN-F014 | Partial | **Partially Verified — Major external gate** | HOLD |
| FIN-F015 | Human Review Required | **Human Review Required — Major** | HOLD |
| FIN-F016 | Verified | **Needs Fix — Minor residual** | 후속 수정 |

요약:

- Verified 또는 현재 demo 계약 내 Verified: 9개
- 기존 Critical 해소: 1개
- Major Needs Fix: 3개
- Major Partial/external: 1개
- Major Human Review: 1개
- Minor residual: 2개

## 5. Pass 1 — Implementation Compliance

### [FIN-F001] Re-audit #1 — 상용 제품 범위와 출시 acceptance

- Severity: Major
- Status: **Verified**
- Evidence:
  - `spec.md:13-17`이 제품을 Linux 한 층 pre-release demo candidate로 한정한다.
  - 가격 TBD, Ubuntu 기준선, Windows UNVERIFIED, macOS 제외, 오디오 비목표가 명시됐다.
  - `README.md:5-25`, `audit_roadmap.md`, `IMPLEMENTATION_SUMMARY.md:4`가 상용 PASS가 아님을 유지한다.
- Decision: 이전의 release lane/완주 범위 부재는 해소됐다.
- Remaining Evidence: 목표 30~60분은 실제 플레이 전까지 UNVERIFIED로 올바르게 표시됐다.

### [FIN-F002] Re-audit #1 — 완결 가능한 한 층 진행 경로

- Severity: Major
- Status: **Verified within current demo contract**
- Evidence:
  - `DungeonMap::placeLandmarks()`가 Door와 최원거리 BossGate를 생성한다.
  - `DungeonState::checkCurrentTileLog()`가 미완료 BossGate에서 boss CombatState를 push한다.
  - boss 승리 시 `campaignCompleted` 저장 후 `VictoryState`로 root 전환한다.
  - ContentContractTests가 landmark 도달성을, ControllerContractTests가 boss reward/Victory/save/re-entry 차단을 검사한다.
- Adjudication:
  - 코더 보고서가 “한 층 → BossGate → boss → Victory를 하나의 controller transcript로 검증”했다고 표현한 것은 과장이다. 현재 controller test는 boss CombatState를 직접 push하고, gate 진입은 별도 source/content test로 확인한다.
  - 실제 호출 경로는 소스상 연결되어 있으므로 finding의 구조적 결함은 해소로 판정한다.
- Remaining Evidence: clean save 실제 완주와 30~60분 측정은 아직 없다.

### [FIN-F003] Re-audit #1 — 콘텐츠 reachability

- Severity: Major
- Status: **Verified**
- Evidence:
  - canonical item 19개, quest 3개가 registry에 있다.
  - 비상점·비시작 9개 item은 monster drop 또는 quest reward에 연결된다.
  - `qst_hunt_spiders`가 Castle 입력/표시/진척/완료 경로에 연결된다.
  - 완료 quest ID를 저장해 같은 reward 재수령을 차단한다.
  - boss는 plate/tower shield를 보장 지급한다.
- Tests: `ContentContractTests`, `ControllerContractTests` PASS.
- Note: 메이스 수집 quest는 상점 메이스 구매로도 완료할 수 있지만 현재 spec이 그 경로를 허용하므로 별도 결함으로 유지하지 않는다.

### [FIN-F004] Re-audit #1 — 모집 및 전투 선택 agency

- Severity: Major
- Status: **Verified**
- Evidence:
  - `RecruitmentDraft`는 preview/reroll/confirm 전까지 Party를 변경하지 않는다.
  - Combat Item은 list → ally target → confirm/cancel을 거친다.
  - Cure Wounds는 명시 target을 사용하며 full/invalid target에서 slot/turn을 소비하지 않는다.
  - production controller transcript가 confirm/cancel 전후 HP, inventory, spell slots를 검사한다.
- Tests: AgencyContractTests와 ControllerContractTests PASS.

### [FIN-F005] Re-audit #1 — 전투 규칙 단일 진실원

- Severity: **Major**
- Status: **Needs Fix**
- Summary: 일반 공격은 `CombatRules`를 사용하도록 수정됐지만 공격형 Skill은 여전히 별도 계산을 하며 Bless, Skeleton resistance, weapon dice count 계약을 우회한다.
- Evidence:
  - `CombatState::performPlayerAttack()`만 `resolveAttack`, `rollWeaponDamage`, `mitigateDamage`를 호출한다.
  - `src/model/Skill.cpp`에는 `getBlessTurns`, `CombatRules::resolveAttack`, `CombatRules::mitigateDamage`, `getDamageDiceCount` 호출이 없다.
  - Slash/Cleave/Sneak Attack/Shadowstrike는 weapon의 dice sides만 읽고 항상 한 번 굴린다. Greatsword 사용 시에도 2d6가 아니라 1d6 기반 skill damage가 된다.
  - 공격형 Skill은 `target->takeDamage`를 직접 호출하므로 Skeleton의 slashing/piercing 50% 감소가 적용되지 않는다.
  - `spec.md:160`은 Bless를 모든 Attack Roll의 +2로 정의한다.
  - CombatContractTests는 일반 `CombatRules`만 검사하고 실제 Skill 클래스와 위 세 경계를 조합하지 않는다.
- Impact:
  - 동일 캐릭터/무기/버프/적이 일반 공격과 Skill에서 서로 다른 규칙을 사용한다.
  - Turn 1 FIN-F005의 근본 원인인 rule data와 실제 consumer 단절이 남아 있다.
- Required Fix:
  - 공격형 Skill이 공통 typed attack/damage resolution을 사용하게 한다.
  - Skill별 추가 보너스와 추가 dice만 parameter로 전달하고 Bless, natural roll, weapon dice count/type, mitigation은 공통 경로에서 처리한다.
  - Greatsword+Slash/Cleave, Blessed Skill, Skeleton 대상 slashing/piercing/bludgeoning 조합 회귀 테스트를 추가한다.
- Re-audit Method: 같은 seed에서 일반 공격과 모든 공격형 Skill의 attack/damage ledger를 비교한다.

### [FIN-F006] Re-audit #1 — session RNG와 저장 seed lifecycle

- Severity: **Major**
- Status: **Needs Fix**
- Summary: 난수 호출은 `SessionRng`로 집중됐지만 저장된 `lastSessionSeed`를 production lifecycle에서 다시 적용하지 않아 재현 기능이 완성되지 않았다.
- Evidence:
  - production `src/`에서 `SessionRng::reseedGlobal(...)` 호출이 없다. 호출은 테스트에만 있다.
  - production에서 `Party::getLastSessionSeed()`를 소비하는 호출자가 없다.
  - `Party::startNewGame()`은 현재 global seed를 저장할 뿐 새 session seed를 명시적으로 생성/reseed하지 않는다.
  - `TitleState` Continue는 저장된 seed로 RNG를 복원하지 않고, 새 프로세스가 만든 현재 global seed로 `lastSessionSeed`를 덮어쓴 뒤 저장한다.
  - map 생성은 global stream에서 정수 하나를 뽑아 별도 local `SessionRng generationRandom`을 생성한다. 결정적 파생은 가능하지만 saved seed/stream state와의 연결이 없다.
  - CombatContractTests는 동일 seed의 두 로컬 RNG가 같은 값을 내는지만 검사한다. process A save → process B Continue → 동일 map/roll 재현을 검사하지 않는다.
- Impact:
  - 고객이 제공한 saved seed로 실제 세션을 재현할 수 없고, Continue가 기존 reproduction metadata를 파괴한다.
  - “시작 seed는 로그와 저장 메타데이터에 남기며 테스트에서 주입” 계약의 저장/복원 절반이 빠져 있다.
- Required Fix:
  - New Game 또는 각 dungeon run에서 canonical seed를 생성하고 global/session RNG를 그 값으로 초기화한 뒤 저장한다.
  - Continue는 저장 seed를 덮어쓰기 전에 복원하거나, 실제 재현 요구에 맞게 engine state/counter 또는 run seed를 별도로 저장한다.
  - RNG owner를 static global이 아니라 Game/session lifetime에 명시적으로 두는 방안을 우선 검토한다.
- Re-audit Method:
  - 독립 프로세스 A에서 seed/save를 생성하고 프로세스 B에서 Continue한 뒤 첫 map hash, encounter, initiative, damage/reward sequence가 동일한지 검증한다.

### [FIN-F007] Re-audit #1 — New/Continue와 파괴적 행동

- Severity: Major
- Status: **Verified**
- Evidence:
  - Title에 New/Continue/Settings/Exit가 분리됐다.
  - 기존 save가 있으면 New Game confirm 전에는 덮어쓰지 않는다.
  - 모집, 파티원 해고, 판매, Combat item/ally skill에 preview/confirm/cancel 경계가 있다.
  - TPK는 더 이상 정상 save를 삭제하지 않고 town checkpoint를 복구한다.
- Tests: controller confirm/cancel, durability warning, Continue-to-Victory 경로 PASS.

### [FIN-F008] Re-audit #1 — 실제 Party HUD

- Severity: Major
- Status: **Verified**
- Evidence:
  - `DungeonRenderer::render`가 Party를 받아 immutable `PartyHudSnapshot`을 생성한다.
  - 0~4인의 이름, class, HP, dead, poison, paralysis, bless가 실제 domain state에서 복사된다.
  - 고정 WARRIOR/CLERIC/ROGUE/MAGE placeholder는 제거됐다.
- Tests: HudContractTests의 0/1/4명 및 snapshot immutability PASS.

### [FIN-F009] Re-audit #1 — i18n, navigation, text accessibility

- Severity: **Major**
- Status: **Needs Fix**
- Confirmed Improvements:
  - 5 locale가 189개 동일 keyset을 가진다.
  - Settings에 75~200% text scale과 high contrast가 있고 config v2에 저장된다.
  - 일반 text helper가 최소 14px를 강제하며 이전 저대비 색 일부가 개선됐다.
- Remaining Evidence:
  - 실제 user-facing 한국어/영어 literal이 여전히 다수 남는다. 예: Town의 `Guild Desk`, `Shop Menu`, 영문 shop catalog, `Temple Sanctuary`; Dungeon movement logs; Combat 전체 전투 로그; item/monster/skill name/description; CharacterInfo `Name/Class`.
  - `TownState`의 O key는 HUB에서만 처리되어 Guild/Shop/Temple/Castle에서는 여전히 열리지 않는다.
  - `CombatState`는 monster turn에서 key 처리를 먼저 return하고, item/skill overlay 중 O를 거부한다.
  - 200% scale에서 fixed panel의 wrap/clip/scroll 검증이 없고 CharacterInfo의 base 10~12 text는 최소 14로만 보정되어 spec의 body 16px 목표와 구분이 불명확하다.
  - 전체 5-locale raster/focus/contrast/high-DPI evidence가 없다.
- Impact: 지원 언어와 공통 입력 계약을 여전히 store-facing 기능으로 완전 주장할 수 없다.
- Required Fix:
  - 모든 표시 문장과 content name/description/log를 locale key+placeholder로 이동한다.
  - O/ESC/Enter/focus 의미를 모든 State/substate/overlay에서 공통 router로 처리한다.
  - 75/100/200%와 5 locale의 bounds/wrap/scroll/raster/input matrix를 자동화하고 실화면을 판독한다.
- Re-audit Method: 5 locale × 모든 State/substate × 3 scale의 screenshot/bounds/input transcript.

### [FIN-F010] Re-audit #1 — save schema와 진행 계약

- Previous Severity: Major
- Status: **Needs Fix — Minor residual**
- Verified:
  - runtime schema v2, camelCase/nested Character, v1 migration, value/member/quest validation, 기본 heal 2+mana 1이 구현됐다.
  - town-only checkpoint와 campaign completion 정책이 문서·코드·test에서 일치한다.
- Residual:
  - repository root `save.json`은 여전히 schemaVersion 없는 legacy v1 형태이며 heal 2개만 가진다.
  - 앱/package는 이 파일을 사용하지 않지만 canonical sample처럼 오인될 수 있고 현재 schema/default 계약과 충돌한다.
- Required Fix: root 파일을 제거하거나 `tests/fixtures/save_v1.json`처럼 legacy migration fixture로 명시적으로 이동·문서화한다.
- Re-audit Method: shipped package/file inventory와 모든 JSON fixture의 역할/schema를 열거한다.

## 6. Pass 2 — Debug / Engineering Quality

### [FIN-F011] Re-audit #1 — 저장 내구성과 손상 복구

- Previous Severity: Critical
- Status: **Verified locally**
- Evidence:
  - OS별 per-user path, 1 MiB/256 KiB cap, symlink target 거부
  - same-directory temp write, file fsync, backup rotation, atomic replace, directory sync
  - corrupt quarantine, valid backup recovery, typed status와 UI error propagation
  - CharacterInfo/Town/boss mutation 실패 시 checkpoint reload/rollback
  - pre-commit I/O와 post-commit durability-unknown fault injection
- Tests: TestHarness와 ControllerContractTests가 Debug/Release에서 통과했다.
- Decision: Turn 1 Critical은 현재 로컬 증거 범위에서 해소됐다.
- Remaining Risk:
  - 실제 kill-during-write와 physical disk-full은 미실행이다.
  - backup을 메모리에 성공적으로 load한 뒤 primary restore가 실패해도 `RecoveredFromBackup` status를 유지하는 경로는 향후 typed 결과를 더 세분화할 수 있다.

### [FIN-F012] Re-audit #1 — TPK stale state

- Severity: Major
- Status: **Verified**
- Evidence:
  - `GameStateManager::replaceAll`이 모든 이전 state를 제거한다.
  - TPK는 update에서 checkpoint를 load한 후 GameOverState 하나로 root를 교체한다.
  - ControllerContractTests가 transition 전 Combat 유지, update 후 stack size 1, 복구된 생존 member를 검사한다.

### [FIN-F013] Re-audit #1 — Release build와 자동 테스트

- Severity: Major
- Status: **Verified**
- Evidence:
  - Release-safe CHECK가 assert/NDEBUG 문제를 제거했다.
  - fresh Debug와 Release가 전 target build 성공.
  - 두 구성 모두 CTest 7/7 PASS.
  - `CrawlmasterRuntime`을 앱과 production-linked ControllerContractTests가 공유한다.
- Limit: UI raster와 실제 완주는 FIN-F009/제품 gate로 남는다.

### [FIN-F014] Re-audit #1 — 패키지와 cross-platform release

- Severity: **Major**
- Status: **Partially Verified**
- Linux Verified:
  - immutable dependency commit, static SFML, executable-relative assets, install/CPack TGZ
  - final archive checksum OK
  - final package의 14개 payload가 current-source fresh package와 byte-identical
  - no RPATH/RUNPATH, no shared SFML, no developer path
  - arbitrary-CWD positive/negative resource verification과 Xvfb startup 성공
- Remaining:
  - Git 저장소가 없어 final package와 source commit을 결속할 revision provenance가 없다.
  - `.github/workflows/ci.yml`은 Ubuntu/Windows job을 정의하지만 실제 hosted run/artifact가 없다.
  - Windows ZIP, checksum, resource verify와 startup smoke는 실행되지 않았다.
  - 현재 제품 계약상 Windows는 UNVERIFIED이므로 Linux demo package의 로컬 성공과 cross-platform 출시 PASS를 구분해야 한다.
- External Reference Check:
  - pinned actions/checkout SHA는 공식 v6.0.2와, actions/upload-artifact SHA는 공식 v7.0.1과 일치하는 것으로 확인했다.
- Required Evidence: 같은 Git revision의 hosted Ubuntu/Windows build/test/package/smoke artifact, remote SHA와 package manifest.

## 7. Pass 3 — Security / Supply Chain

### [FIN-F015] Re-audit #1 — dependency, font, legal/support provenance

- Severity: **Major**
- Status: **Human Review Required**
- Verified:
  - SFML/json immutable commit pin
  - 직접 의존성 SPDX 문서, license text, notices와 package 포함
  - 폰트 3개 SHA-256과 내부 family, 미확정 상태 문서화
  - title이 허위 권리자 문구 대신 pre-release identity를 표시
- Remaining:
  - 3개 TTF의 원출처 archive, version, license, conversion history와 상업 재배포 승인
  - project LICENSE/EULA/privacy/support/legal owner
  - full transitive SBOM, vulnerability/license scanner evidence, artifact signature/attestation
  - current dependency vulnerability clean verdict
- Decision: direct dependency pin과 고지는 개선됐지만 유료 배포 gate는 계속 차단한다. 공식 release tag 확인은 full scanner를 대체하지 않는다.

## 8. Cross-Pass / Documentation

### [FIN-F016] Re-audit #1 — 완료·구조 문서 동기화

- Previous Severity: Major
- Status: **Needs Fix — Minor residual**
- Verified:
  - README/BUILD_GUIDE/IMPLEMENTATION_SUMMARY/audit_roadmap이 pre-release demo와 UNVERIFIED/Human Review 경계를 명시한다.
  - 존재하지 않는 renderer를 현재 파일 책임으로 주장하던 핵심 drift는 제거됐다.
- Residual Drift:
  1. `spec.md:294`의 Skill 예시가 `virtual void execute`를 선언하지만 실제 `include/model/Skill.hpp:59`는 `virtual bool execute`다.
  2. `designs.md:232`는 0.1초 screen shake가 적용된다고 단정하지만 source/test에 해당 구현이 없다.
  3. 코더 보고서의 checksum 명령은 실행 cwd를 생략해 project root에서 그대로 실행하면 sidecar의 상대 archive 경로를 찾지 못한다. `BUILD_GUIDE.md:81-82`의 명령은 올바르게 package directory로 이동한다.
- Required Fix: 현재 contract/API와 실제 구현을 맞추고 구현되지 않은 visual polish는 Deferred/Proposal로 표시한다.
- Re-audit Method: 문서 API snippet, file links, 완료 문장을 `rg`/call graph/test/package와 다시 대조한다.

## 9. Five-Axis Review Summary

### Correctness

- 큰 폭 개선: save/FSM/package/test/content/HUD/agency 경계는 실제 테스트로 닫혔다.
- 차단: 공격형 Skill의 공통 전투 규칙 우회와 production seed restore 부재.

### Readability and Simplicity

- `Persistence`, `CombatRules`, `CombatActionRules`, `PartyHudSnapshot`, `CrawlmasterRuntime` 분리는 이전보다 책임이 명확하다.
- Skill별 공격 계산이 별도로 남아 공통 규칙을 다시 분산시킨 점은 FIN-F005의 구조 원인이다.

### Architecture

- state root replace와 production-linked controller tests는 적절하다.
- static global RNG와 무소비 saved seed는 명시적인 session lifecycle과 맞지 않는다.

### Security and Data Integrity

- 기존 Critical save reset/truncate 경계는 로컬 fault tests로 해소됐다.
- font/legal/full supply-chain evidence는 미완료다.
- 제품 secret은 관찰되지 않았다.

### Performance

- 20x20 map과 현재 test/runtime smoke에서 차단 성능 문제는 관찰되지 않았다.
- 실제 30~60분 run, 200% text layout, synchronous fsync의 UI 지연은 계측하지 않았다.

## 10. Required Fixes Before PASS

### 로컬 코드/문서

1. FIN-F005: 모든 공격형 Skill을 공통 attack/damage/mitigation 규칙으로 통합하고 조합 회귀를 추가한다.
2. FIN-F006: saved/run seed를 실제 production session에 restore하고 독립 프로세스 replay test를 추가한다.
3. FIN-F009: 모든 literal/content localization, 공통 O/Esc/Enter/focus, 5-locale/scale raster·bounds 검증을 완료한다.
4. FIN-F010: root legacy `save.json`의 fixture 역할을 명시하거나 제거한다.
5. FIN-F016: Skill API snippet과 screen-shake 문서 drift를 수정한다.

### 외부/출시

1. FIN-F014: Git source revision과 hosted Ubuntu/Windows artifact를 결속하고 Windows package/startup evidence를 확보한다.
2. FIN-F015: font rights, legal/support owner, full SBOM/scanner/signature를 사람 검토로 닫는다.
3. clean save 실제 완주와 다중 seed 정량 밸런스를 기록한다.

## 11. Re-audit Checklist

- [ ] Blessed Slash/Cleave/Sneak/Shadow attack total에 +2가 적용됨
- [ ] Greatsword 기반 공격형 Skill이 승인된 dice-count 계약을 사용함
- [ ] 공격형 Skill의 slashing/piercing가 Skeleton mitigation을 통과함
- [ ] process A save → process B Continue가 saved/run seed를 실제 복원함
- [ ] 동일 seed 입력 transcript가 map/encounter/combat/reward ledger를 재현함
- [ ] 5 locale의 모든 State/substate/overlay에서 raw literal/key가 없음
- [ ] Town 모든 substate와 Combat turn/overlay에서 승인된 O/Esc/Enter 계약이 동작함
- [ ] 75/100/200%에서 wrap/scroll/bounds/focus/CJK raster가 검증됨
- [ ] root save fixture와 문서 API/visual claims가 현재 구현과 일치함
- [ ] hosted Ubuntu/Windows artifact가 같은 source revision을 가리킴
- [ ] font/legal/SBOM/scanner/signature Human Review 완료
- [ ] 실제 완주·밸런스 기록 확보

## 12. Final Decision

**HOLD**

Turn 1의 Critical 데이터 손실 경계는 로컬에서 해소됐고, fresh Debug/Release 7/7, Linux package install/resource/GUI smoke, payload-current-tree 일치도 독립 확인했다. 현재 트리는 이전의 단순 기술 프로토타입보다 분명히 개선된 **로컬 Linux 한 층 pre-release demo candidate**다.

그러나 코더 보고서에서 Verified로 분류한 FIN-F005와 FIN-F006은 실제 production 호출 경로에서 미해소다. 여기에 이미 인정된 FIN-F009, FIN-F014, FIN-F015가 남아 있으므로 PASS 계열 판정은 불가하다.

- Critical: 0 open
- Major open/gated: FIN-F005, FIN-F006, FIN-F009, FIN-F014, FIN-F015
- Minor residual: FIN-F010, FIN-F016

## 13. Coder Handoff

```text
`/mnt/Projects_SSD/cpp/crawlmaster/docs/audit/audit_report_10.md`의 독립 재감사 결과를 확인하고, FIN-F005·FIN-F006·FIN-F009를 실제 production 호출 경로와 회귀 테스트에 대조해 우선 수정하세요. FIN-F010·FIN-F016의 문서/fixture drift를 함께 정리하고, 이후 hosted Windows 및 font/legal/supply-chain gate 증거를 추가한 뒤 다음 번호 보고서로 재감사하세요.
```
