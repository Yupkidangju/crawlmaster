# Crawlmaster Audit Report 10 수정 독립 재감사 — Re-audit #2

작성일: 2026-09-03 (Asia/Seoul)  
대상: Crawlmaster 0.9.4 현재 작업 트리, Linux Release package, local MinGW Windows package  
입력 감사: `docs/audit/audit_report_10.md`  
구현 계획: `docs/audit/remediation_plan_11.md`  
감사 기준: `AI_AUDIT_DOC_STANDARD.md`, 현재 `spec.md`, `DESIGN_DECISIONS.md`, `designs.md`  
최종 판정: **HOLD — 로컬 코드 finding은 해소, hosted Windows 및 제품 legal/attestation gate 잔존**

## 1. 감사 성격과 계보

- `audit_report_10.md`와 이전 번호 보고서는 수정하지 않았다.
- 보고서 10의 FIN-F005, FIN-F006, FIN-F009를 unit helper가 아니라 production `Skill::execute`, Title Continue, State input/draw 호출 경로에서 다시 재현했다.
- 코드 변경 전 계약과 구현 순서/검증 기준을 `remediation_plan_11.md`, `spec.md`, `DESIGN_DECISIONS.md`, `designs.md`에 기록했다.
- 구현 완료 주장은 fresh Debug/Release, 별도 process, 실제 SFML raster, Linux package, MinGW PE package와 scanner 증거에 독립 대조했다.
- 동일 작업자가 구현과 재감사를 수행했으므로 별도 모델/사람의 독립성은 주장하지 않는다. 코더 주장과 아래 판정은 명령 증거와 현재 파일을 기준으로 분리했다.
- Git repository/remote와 `gh` CLI가 없어 hosted workflow를 실행하거나 remote revision에 artifact를 결속할 수 없었다. 이 범위를 PASS로 승격하지 않았다.

## 2. 범위와 제외

### 포함

- FIN-F005: 일반 공격/공격형 Skill의 Bless, natural 1/20, weapon dice count/type, extra dice, Skeleton mitigation
- FIN-F006: seed, raw engine draw count, schema v2 checkpoint, Title Continue와 별도 process replay
- FIN-F009: 5 locale 콘텐츠/로그, placeholder, Town/Combat input, 75/100/200% font 및 production state/substate raster
- FIN-F010: root legacy save fixture 정리
- FIN-F016: Skill API와 screen-shake 문서 drift
- FIN-F014/F015: Linux/MinGW package, CI SBOM/scanner/attestation 설정, font provenance/license, local OSV/Grype
- 기존 FIN-F001~F004, F007~F008, F011~F013의 회귀 여부

### 제외 또는 미검증

- GitHub hosted Ubuntu/Windows workflow 실제 run ID, artifact URL, remote commit SHA
- Windows MSVC CTest 및 실제 Windows GUI/resource startup
- GitHub/Sigstore provenance와 SBOM attestation 실제 발급·검증
- 제품 저작권자, LICENSE/EULA/privacy/support contact의 사람 승인
- 실제 OS high-DPI 배율과 장시간 동적 화면 전체 조합
- clean save 30~60분 완주와 다중 seed 정량 밸런스
- 물리 disk-full/power-loss/kill-during-write chaos

## 3. Report 10 finding 직접 재현

### FIN-F005 RED

`tests/test_combat_contracts.cpp`에 실제 `SkillFactory`가 만든 Slash, Cleave, Sneak Attack, Shadowstrike를 다음 조합으로 호출했다.

- Bless가 없으면 1 차이로 miss, Bless +2이면 hit가 되는 AC
- Greatsword 2d6의 dice count
- Rapier piercing 및 Greatsword slashing
- `mon_skeleton`의 50% mitigation

수정 전 네 조합 모두 실제 HP delta가 canonical 예상값과 달라 RED였다.

### FIN-F006 RED

`tests/test_controller_contracts.cpp`가 save의 seed/count를 읽고 다른 global seed로 바꾼 뒤 Title Continue를 수행하도록 추가했다. 수정 전에는 다음 API와 consumer가 없어 컴파일 단계에서 RED였다.

- `SessionRng(seed, drawCount)`
- `SessionRng::drawCount()`
- `Party::getSessionRngDrawCount()`
- production Continue restore

### FIN-F009 RED

- Town 6개 비-HUB 경로와 Combat item/skill overlay에서 `O`를 눌러 SettingsState를 기대했으나 8개 전이가 실패했다.
- Item/Monster/Skill/Quest 이름은 언어를 바꿔도 한국어 literal로 유지됐다.
- 최초 200% CJK raster에서 `DroidSansFallbackFull.ttf`가 advance 검사에는 통과했지만 혼합 ASCII/CJK를 tofu 사각형으로 그렸다.
- 최초 200% production raster에서 Town/Combat/CharacterInfo/Settings의 dense panel overlap/clip이 확인됐다.

## 4. 수정 결과

### FIN-F005 — Verified

- `CombatRules::resolveAttack(..., situationalAttackBonus)`가 모든 attack roll의 proficiency, class ability, Bless, natural 1/20을 판정한다.
- `CombatRules::rollAttackDamage()`가 base dice count/sides, flat bonus, extra dice와 critical 배수를 한 곳에서 계산한다.
- 일반 공격과 Slash/Shield Bash/Cleave/Sneak Attack/Poison Dart/Shadowstrike가 같은 규칙을 사용한다.
- 공격형 Skill은 장착 무기의 count/sides/type을 보존하고 `mitigateDamage()` 후 HP를 변경한다.
- `CombatContractTests`의 실제 Skill 4조합과 기존 일반 공격/장비/reward ledger가 Debug/Release에서 통과했다.

판정: Report 10이 확인한 production consumer 단절은 해소됐다.

### FIN-F006 — Verified

- `SessionRng`는 seed와 원시 `mt19937` draw count를 소유한다.
- 범위 난수는 명시적 rejection sampling으로 생성하여 실제 raw draw 소비량을 기록한다.
- schema v2는 `lastSessionSeed`와 `sessionRngDrawCount`를 저장한다. untrusted count는 10,000,000으로 제한한다.
- New Game은 0이 아닌 새 entropy seed로 session을 시작한다.
- Title Continue는 seed가 있으면 `SessionRng(seed, drawCount)`로 복원하고 저장 메타데이터를 덮어쓰지 않는다. legacy seed 0만 새 canonical checkpoint로 승격한다.
- `RngReplayCheckpointWrite`와 `RngReplayCheckpointContinue`는 CTest의 서로 다른 process에서 write 후 다음 16개 난수 sequence를 재현했다.

판정: seed를 기록만 하고 소비하지 않던 production lifecycle 결함은 해소됐다. town-only 저장 계약상 mid-combat resume는 범위 밖이다.

### FIN-F009 — Verified within Linux demo contract

- 5 locale는 각각 394개 key이며 중복 key가 없고 key/placeholder set이 동일하다.
- 19 item, 8 monster, 12 skill, 3 quest 이름·설명을 locale catalog에서 동적으로 읽는다.
- Skill/아이템/상태/전투/던전 로그와 CharacterInfo/Town/Combat label을 key+placeholder 경로로 이동했다.
- source literal guard가 Report 10의 대표 raw literal 재도입을 차단한다.
- Town 모든 substate와 확인 overlay, Combat의 player item/skill overlay에서 `O`가 SettingsState를 push한다. 적 턴은 입력으로 상태를 변경하지 않는다.
- 150~200%에서 Town/Combat compact label, CharacterInfo 상세/가방 단일 panel, compact Dungeon HUD와 Settings guide를 사용한다.
- `FontRasterTests`: 5 locale × 3 scale 15개 raster, 모든 sample glyph가 missing-glyph texture와 다른지 검사한다.
- `UiStateRasterTests`: 5 locale × 3 scale × 25 production view = **375 PNG**를 생성한다.
  - Title/overwrite confirm
  - Town HUB/Guild/preview/dismiss confirm/Shop/buy/sell/sale confirm/Temple/Castle
  - Settings, GameOver, Victory
  - CharacterInfo bag/details
  - Dungeon
  - Combat base/skill list/ally target/confirm/item list/target/confirm
- 최초 tofu와 200% overlap을 실제 PNG로 확인하고 수정한 뒤 대표 KO/EN/JA/ZH-TW/ZH-CN 화면을 다시 판독했다.

판정: Report 10의 local literal, O-key, CJK raster와 200% 고정 panel 결함은 현재 Linux 계약에서 해소됐다. 실제 Windows/OS high-DPI와 모든 장시간 동적 조합은 별도 external evidence로 `UNVERIFIED`다.

### FIN-F010 — Verified

- root `save.json`을 제거했다.
- 동일 bytes의 legacy 입력은 `tests/fixtures/save_v1.json`으로 이동했다.
- `tests/fixtures/README.md`가 v1 migration 전용이며 제품 기본 저장/package 자산이 아님을 기록한다.
- canonical schema 예시에는 `sessionRngDrawCount`가 추가됐다.

### FIN-F016 — Verified

- `spec.md`의 `Skill::execute` 예시를 실제 `bool` 계약으로 수정했다.
- 미구현 0.1초 screen shake는 `designs.md`에서 Deferred visual polish로 분류했다.
- README/BUILD_GUIDE/IMPLEMENTATION_SUMMARY/CHANGELOG/audit_roadmap을 실제 13-test, RNG, raster, package/SBOM 범위와 동기화했다.

## 5. 전체 finding 재판정

| Finding | Report 10 | Re-audit #2 | Gate |
| --- | --- | --- | --- |
| FIN-F001 | Verified | **Verified** | 해소 |
| FIN-F002 | Demo contract Verified | **Verified within demo contract** | 장시간 완주 별도 |
| FIN-F003 | Verified | **Verified** | 해소 |
| FIN-F004 | Verified | **Verified** | 해소 |
| FIN-F005 | Needs Fix Major | **Verified** | 해소 |
| FIN-F006 | Needs Fix Major | **Verified** | 해소 |
| FIN-F007 | Verified | **Verified** | 해소 |
| FIN-F008 | Verified | **Verified** | 해소 |
| FIN-F009 | Needs Fix Major | **Verified within Linux demo contract** | OS high-DPI/Windows는 외부 증거 |
| FIN-F010 | Minor residual | **Verified** | 해소 |
| FIN-F011 | Verified locally | **Verified locally** | 기존 Critical 해소 유지 |
| FIN-F012 | Verified | **Verified** | 해소 |
| FIN-F013 | Verified | **Verified** | 해소 |
| FIN-F014 | Partial Major | **Partially Verified** | hosted Windows/runtime 미완료 |
| FIN-F015 | Human Review Major | **Partially Verified / Human Review Required** | hosted attestation와 product legal 미완료 |
| FIN-F016 | Minor residual | **Verified** | 해소 |

결과: local implementation `Needs Fix`는 0개다. 출시를 막는 외부 Major gate는 FIN-F014와 FIN-F015다.

## 6. Build, test와 runtime 증거

### Linux Debug/Release

```text
cmake --build build/reaudit11-debug --parallel 2
ctest --test-dir build/reaudit11-debug --output-on-failure --no-tests=error
-> 13/13 PASS, 123.67 sec

cmake --build build/reaudit11-release --parallel 2
ctest --test-dir build/reaudit11-release --output-on-failure --no-tests=error
-> 13/13 PASS, 38.46 sec
```

13개 gate는 ResourceVerification, TestHarness, HUD/Combat/Content/Agency/Localization, FontRaster, 3개 process RNG fixture, UiStateRaster, ControllerContractTests다.

### Linux package

- Package: `build/reaudit11-release/package/Crawlmaster-0.9.4-Linux-x86_64.tar.gz`
- SHA-256: `efd1560635b549990a57ecbc761dc1ad81a92e9f827e46c930da3484e4d29a6a`
- sidecar verification: PASS
- arbitrary-CWD `--verify-resources`: exit 0
- arbitrary-CWD Xvfb startup: timeout exit 124
- developer RPATH/shared SFML: 없음

### Local Windows cross-build

```text
cmake ... -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
cmake --build build/reaudit11-mingw --parallel 2
-> 전체 product/test target compile/link PASS
```

- Package: `build/reaudit11-mingw/package/Crawlmaster-0.9.4-Windows-x86_64.zip`
- SHA-256: `acbe494fb55167cad61e8bcba66668a8579af7829c042b859887f8f952a80be7`
- ZIP sidecar verification, font/locale/license payload: PASS
- PE import: ADVAPI32, GDI32, KERNEL32, msvcrt, OPENGL32, USER32, WINMM
- MinGW libstdc++/libgcc DLL 의존성 없음

이는 Windows source/ABI/package 구조의 보조 증거다. Wine과 실제 Windows가 없어 exe 실행/CTest는 수행하지 않았다.

## 7. Font, legal과 supply-chain 증거

### Font technical provenance

- Noto Sans CJK Regular TTC: Ubuntu `fonts-noto-cjk` 파일과 byte-identical, SIL OFL 1.1
- Ubuntu Mono variable TTF: Ubuntu `fonts-ubuntu` 파일과 byte-identical, Ubuntu Font Licence 1.0
- NeoDunggeunmo: 공식 v1.601 release asset과 byte-identical, SIL OFL 1.1
- 파일별 SHA, upstream/package/release와 license 원문은 `FONT_PROVENANCE.md`와 `licenses/`에 기록했다.
- 실제 tofu가 확인된 Droid font는 package/source asset에서 제거하고 build 복구 위치에만 보존했다.

Font 기술 provenance는 Verified다. 제품 소유자와 최종 유료 배포 승인은 기술 증거와 다른 Human gate다.

### Artifact SBOM/scanner

- `scripts/generate_release_sbom.py`는 install tree 모든 18개 파일, 고정 source components, bundled fonts와 platform runtime dependency를 SPDX 2.3으로 기록한다.
- Linux artifact SBOM: 18 files / 15 packages
  - SHA-256 `0e0e208eb746188df24a8c961969caa0cce27ee3d41b4366838d41b8ae6fc16b`
- Windows artifact SBOM: 18 files / 13 packages
  - SHA-256 `d3fca75d2e65a6a58cde1a7f669f1687f02d011e97b1ef89c59040d2e45f0b3c`
- Grype 0.118.0 high gate: Linux 0 matches, Windows 0 matches
  - reports SHA-256 `e68088d203f2f60861bd4773352f0fcd345b6c2b81d3562a7367f869fe54c022`
  - reports SHA-256 `48ca4a7ffe14ebd7c114c81ee65b376a163e3b03f0d77d7a3a66f7f41763b2e6`
- OSV exact commit query: SFML/json 모두 query 시점 result empty. `docs/audit/osv_scan_2026-09-03.json`에 request/response를 기록했다.

scanner의 0 matches는 알려진 데이터베이스와 현재 식별자에 대한 시점 결과이며 취약점 부재의 보증이 아니다.

### Hosted configuration

`.github/workflows/ci.yml`은 다음 고정 SHA를 사용한다.

- actions/checkout v6.0.2: `de0fac2e4500dabe0009e67214ff5f5447ce83dd`
- actions/upload-artifact v7.0.1: `043fb46d1a93c77aae656e7c1c64a875d1fc6a0a`
- anchore/sbom-action v0.24.0: `e22c389904149dbc22b58101806040fa8d37a610`
- anchore/scan-action v7.3.0: `0d444ed77d83ee2ba7f5ced0d90d640a1281d762`
- actions/attest v4.1.0: `59d89421af93a897026c735860bf21b6eb4f7b26`

tag와 SHA는 upstream `git ls-remote`로 일치함을 확인했다. Workflow는 Ubuntu/Windows package, resource/startup smoke, custom+Syft SBOM, Grype high gate, build/SBOM attestation, artifact upload을 정의한다.

그러나 현재 tree에는 `.git`/remote가 없고 `gh`도 설치되지 않았다. 따라서 workflow run/attestation URL은 존재하지 않으며 hosted gate는 `UNVERIFIED`다.

공식 참고:

- GitHub artifact attestations: https://docs.github.com/en/actions/how-tos/secure-your-work/use-artifact-attestations/use-artifact-attestations
- Anchore Syft formats: https://oss.anchore.com/docs/guides/sbom/formats/
- OSV batch query: https://google.github.io/osv.dev/post-v1-querybatch/
- Ubuntu Font Licence: https://canonical.com/legal/font-licence
- NeoDunggeunmo license/release: https://github.com/neodgm/neodgm

## 8. Five-axis 재검토

### Correctness

- 공격형 Skill과 일반 공격의 결과 계약이 동일해졌다.
- RNG checkpoint가 production Continue와 별도 process에서 소비된다.
- save v2 backward compatibility는 missing draw count를 0으로 읽어 유지한다.
- 5 locale의 dynamic content와 confirm/input 흐름이 실제 State 경로에서 검증됐다.
- 추가 Required correctness finding은 발견하지 못했다.

### Readability and simplicity

- attack/damage helper와 localization formatter가 중복 계산/치환을 줄였다.
- 대형 텍스트는 기존 모든 좌표에 조건을 흩뿌리지 않고 CharacterInfo 전용 layout과 compact key 정책으로 분리했다.
- `CombatState.cpp`는 1,000줄을 넘는 기존 구조적 부담이 남는다. 이번 finding의 공통 규칙은 model로 이동했지만 향후 combat presentation/action state 분리는 별도 리팩터링 후보다.

### Architecture

- 앱/ControllerContractTests는 계속 `CrawlmasterRuntime` production library를 공유한다.
- content 표시가 current locale에서 해석되고 저장은 stable ID만 사용한다.
- CMake 소형 test target의 source list 중복은 한 차례 link omission을 일으켰으며 현재는 모두 closure됐다. 장기적으로 공통 domain test library가 중복을 더 줄일 수 있다.

### Security and supply chain

- untrusted RNG count cap, save size/symlink/atomic 경계가 유지됐다.
- source/action immutable SHA, package checksum, artifact file hashes, font licenses, OSV/Grype와 attestation workflow가 추가됐다.
- secret/private-key pattern과 `.env`/key file은 발견되지 않았다.
- hosted attestation와 product legal owner가 없으므로 출시 PASS는 불가하다.

### Performance

- Debug 375-view raster gate는 약 114초, Release는 약 35초로 CI 10분 예산 이내다.
- runtime hot path에 네트워크/scan 로직을 추가하지 않았다.
- 실제 30~60분 session과 sync fsync UI latency는 계측하지 않았다.

## 9. 남은 PASS 조건

1. Git repository/remote를 확정하고 같은 remote SHA에서 hosted Ubuntu와 Windows workflow를 실행한다.
2. Windows hosted MSVC CTest, ZIP checksum/resource verify와 5초 GUI startup 결과를 보존한다.
3. 발급된 build/SBOM attestation을 `gh attestation verify`로 검증하고 run/job/artifact URL을 다음 보고서에 기록한다.
4. 제품 copyright owner, LICENSE/EULA/privacy/support contact와 최종 배포 승인을 사람 검토로 확정한다.
5. 실제 OS high-DPI와 clean-save 30~60분 완주/다중 seed 밸런스를 측정한다.

## 10. 최종 판정

**HOLD**

- Critical open: 0
- Local code/document Major Needs Fix: 0
- External Major gates: FIN-F014 hosted Windows/runtime, FIN-F015 hosted attestation/product legal
- 별도 제품 evidence: OS high-DPI, 장시간 완주/정량 밸런스

FIN-F005·006·009와 FIN-F010·016의 현재 로컬 finding은 문서, production 호출, 회귀 테스트와 runtime/raster 증거가 같은 결론을 지지한다. 폰트 provenance와 local artifact SBOM/scanner도 추가됐다. 그러나 구성 파일과 local MinGW package는 실제 hosted Windows 및 서명된 provenance가 아니며, 기술 문서가 제품 법률 주체의 승인을 대신할 수 없다. 따라서 다음 번호 PASS 보고서는 외부 증거가 실제로 생성된 뒤에만 가능하다.

## 11. Coder Handoff

```text
`/mnt/Projects_SSD/cpp/crawlmaster/docs/audit/audit_report_11.md`의 남은 FIN-F014·FIN-F015 외부 gate를 확인하세요. Git remote와 승인된 source SHA를 확정한 뒤 hosted Ubuntu/Windows workflow를 실행하고, Windows MSVC test/package/startup artifact와 build/SBOM attestation 검증 URL을 기록하세요. 제품 legal/support owner의 사람 승인과 OS high-DPI/장시간 플레이 증거를 확보한 뒤 다음 번호 보고서에서만 PASS 여부를 재판정하세요.
```
