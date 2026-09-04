# Crawlmaster RA15 remediation 후속 재감사 보고서

## 1. 메타데이터와 판정

- 작성일: 2026-09-04 (Asia/Seoul)
- 재개방 근거: `docs/audit/audit_report_15.md`
- 수정 계약: `docs/audit/remediation_plan_16.md`, 현재 `spec.md`, `designs.md`, `DESIGN_DECISIONS.md`
- 대상 tree: `HEAD 927753278f46b92a015197ee229edce4f52e0657` + 현재 미커밋 working tree
- 환경: GCC 15.2.0, CMake 4.2.3, Linux x86_64
- 감사 방식: 구현 완료 뒤 변경을 멈추고 문서→호출 경로→failure-specific assertion→빌드·패키지 순서로 분리 재검사했다. 별도 인간 감사는 아니다.
- **대상 finding 판정: Verified 5 / Needs Fix 0**
- **로컬 remediation 판정: PASS**
- **전체 release 판정: HOLD**

전체 release HOLD는 이번 finding 때문이 아니라 current v0.10 immutable SHA의 hosted Windows MSVC/package/SLSA/SPDX, clean Windows 10/11, 실제 OS high-DPI/IME, 장시간 밸런스와 human/legal gate가 남아 있기 때문이다.

## 2. 범위

### 포함

- RA15-F001~F003
- 재개방 FIN-F017, FIN-F031
- TPK/Title 복구 실패, shutdown save/load 분기, write eligibility
- primary absent + malformed/oversized backup quarantine
- Debug/Release CTest 16개, 5 locale UI raster, Linux CPack/resource/startup/checksum
- Turn 3 sealed source-report manifest 무결성

### 제외

- report 15에서 제외한 hosted Windows/SLSA, clean Windows, macOS, power-loss, 다중 writer와 장시간 실기
- 기존 dirty working tree의 RA15 범위 밖 변경에 대한 새 전체 감사

## 3. 계약 대조

- `spec.md:351-367`은 failed TPK/Continue가 `recoveryPending`을 세우고, 성공 load 또는 확인된 New Game만 해제하며, 일반 저장과 종료 자동 저장을 금지한다고 정의한다.
- `designs.md:352-358`은 pending 종료에서 Enter를 load retry, Esc를 무저장 종료로 정의한다.
- `DESIGN_DECISIONS.md` 17번은 failed-load 비파괴성과 write eligibility를 분리한 이유를 기록한다.
- `IMPLEMENTATION_SUMMARY.md:21,59,71`은 `ShutdownState`와 recovery failure 책임을 현재 state/파일 책임표에 포함한다.

문서 계약과 구현 사이에 새 drift를 찾지 못했다.

## 4. Finding 재감사 ledger

| Finding | 상태 | 현재 증거와 판정 |
| --- | --- | --- |
| RA15-F001 | **Verified** | `Party::saveToFile()`이 pending을 `RecoveryPending`으로 거부하고(`Party.cpp:130-134`), 성공 load만 flag를 해제한다(`:413-426`). TPK와 Title 실패가 pending을 세우며(`CombatState.cpp:362-368`, `TitleState.cpp:57-99`), `Game::requestShutdown()`은 save 전에 pending을 분기한다(`Game.cpp:61-77`). `ShutdownState` Enter는 load만 재시도하고 Esc는 무저장 종료한다(`ShutdownState.cpp:21-43`). |
| RA15-F002 | **Verified** | primary `NotFound` + backup 존재 분기가 corrupt backup을 candidate helper로 quarantine하고 typed `Corrupt`/`IoError`를 반환한다(`Party.cpp:452-470`). |
| RA15-F003 | **Verified** | `audit_roadmap.md:8-12`가 report 14 → report 15 → plan 16 → report 16 계보와 현재 local PASS/전체 HOLD를 표시한다. spec controller roster와 implementation 책임표에 `ShutdownState`가 있다. |
| FIN-F017 | **Verified** | active roadmap의 오래된 report 10/plan 12/report 13 현재 표시는 제거됐고, 완료 gate와 후속 RA15 결과가 분리됐다. |
| FIN-F031 | **Verified** | primary+backup 동시 손상뿐 아니라 primary absent + malformed/oversized backup의 격리 path, 원 bytes, `.bak` 제거, 두 번째 `NotFound`가 직접 assertion으로 고정됐다(`test_harness.cpp:620-649`). |

## 5. Failure-specific 회귀 증거

### corrupt primary / no backup + TPK

`testRecoveryPendingBlocksTpkAndTitleShutdownOverwrite`는 저장된 파티원을 사망시키고 primary를 손상한 뒤 backup을 제거한다. TPK 복구 실패 후 다음을 검증한다(`test_controller_contracts.cpp:830-865`).

- GameOver 전이와 dead in-memory Party 보존
- `recoveryPending == true`
- 직접 save가 `RecoveryPending`
- shutdown 요청과 Enter load retry가 primary를 만들지 않음
- Esc만 명시 종료 승인

같은 테스트의 Title Continue 경로도 손상 save 격리 후 pending과 shutdown 무저장 경계를 검증한다(`:868-890`). `resetToDefault()`는 pending을 해제하지 않고 확인된 `startNewGame()`만 해제한다(`:892-900`).

별도 성공 경로는 유효 checkpoint가 있는 pending 종료에서 Enter가 load에 성공하면 flag를 해제하고 종료를 승인하는지 검증한다.

### missing primary + corrupt backup

`testMissingPrimaryCorruptBackupIsQuarantined`는 malformed JSON과 1 MiB 초과 backup을 각각 실행해 다음을 검증한다(`test_harness.cpp:620-649`).

- 결과 `Corrupt`
- backup이 고유 quarantine path로 이동
- quarantine bytes가 입력과 동일
- 다음 load는 동일 candidate를 반복하지 않고 `NotFound`

### transient read `IoError` independent process

`RngReplayCheckpointContinue`의 별도 프로세스 probe는 Linux leaf symlink read 거부로 `IoError`를 재현한다(`test_rng_process_replay.cpp:75-106`). Title failure 뒤 pending이 유지되고 shutdown Enter가 load만 재시도하며, 실제 target save bytes는 Enter/Esc 전후 모두 동일하다.

## 6. 빌드·테스트·UI 증거

최종 소스 기준:

```text
cmake --build build/debug --parallel 2
ctest --test-dir build/debug --output-on-failure --no-tests=error
-> build PASS, 16/16 PASS, 146.88초

cmake --build build/release --parallel 2
ctest --test-dir build/release --output-on-failure --no-tests=error
-> build PASS, 16/16 PASS, 44.34초
```

- `TestHarness`: missing-primary malformed/oversized backup quarantine PASS
- `ControllerContractTests`: TPK/Title/shutdown write fence와 New Game 해제 PASS
- `RngReplayCheckpointContinue`: 별도 process transient `IoError` bytes 불변 PASS
- `LocalizationContractTests`: 5 locale parity PASS, 각 452 key
- `UiStateRasterTests`: 5 locale × 75/100/200%, recovery-pending 화면 포함 PASS
- KO 200% `shutdown-recovery-pending.png` 직접 판독: 제목/오류/Enter load retry/Esc no-save 안내 겹침 없음
- `git diff --check`: PASS

TDD red 증거로 production 구현 전 `ControllerContractTests`는 `Party::isRecoveryPending` 부재 컴파일 오류를 냈고, 구현 후 위 full gate에서 green이 됐다. 최초 잘못 지정한 `CrawlmasterTests` target 이름은 즉시 `TestHarness`로 교정했으며 제품 결함으로 분류하지 않는다.

## 7. Linux package와 manifest

```text
cpack --config build/release/CPackConfig.cmake -C Release
-> Crawlmaster-0.10.0-Linux-x86_64.tar.gz 생성

sha256sum -c Crawlmaster-0.10.0-Linux-x86_64.tar.gz.sha256
-> OK

임시 디렉터리에 archive 해제 후 임의 CWD에서 --verify-resources
-> Resource OK

timeout 5s xvfb-run -a <packaged binary>
-> 5초 동안 정상 기동 후 timeout 124를 기대값으로 확인
```

- archive 크기: 16,749,704 bytes
- SHA-256: `9600199e8d31bbab6e54e5c3522dd2907c65e41004c798632252b7037c06db31`
- 첫 package 해제 시도는 `cmake -E tar`에 대상 디렉터리를 member 인수로 잘못 넘겨 실패했다. 같은 archive를 정확한 작업 디렉터리에서 해제해 위 검증을 통과했으므로 package 결함이 아니다.
- Turn 3 manifest SHA-256: `396f27c1d0b292b611906886a6bad3152a48308d97574a3764b00fc1cb0af998`
- source report: 6/6 verified, missing 0. 원본 report/manifest는 수정하지 않았다.

## 8. 3-pass 판단

### Pass 1 — Implementation Compliance

RA15-F003/FIN-F017의 active authority drift가 해소됐고 recovery contract, controller roster, 구현 책임표와 실제 호출 경로가 일치한다. 새 finding 없음.

### Pass 2 — Debug / Engineering Quality

세 failure mode가 broad smoke가 아니라 이름 붙은 assertion으로 고정됐다. quarantine `IoError`는 무시하지 않고 전달하며, checkpoint rollback은 recovery flag도 보존한다. Debug/Release 16/16과 package gate가 통과했다. 새 finding 없음.

### Pass 3 — Security / Data Integrity

검증되지 않은 in-memory Party가 save 권한을 얻지 못하고, symlink read `IoError`에서도 target bytes가 불변이다. 손상 backup 입력은 크기/JSON 검증 후 격리되고 반복 소비되지 않는다. 새 finding 없음.

### Cross-pass conflict

없음. failed-load 메모리 보존과 shutdown 내구성 정책의 기존 충돌은 `recoveryPending` write fence로 해소됐다.

## 9. 최종 결정

### RA15 로컬 remediation

**PASS — RA15-F001~F003, FIN-F017, FIN-F031 모두 Verified.**

### 전체 release

**HOLD.** 이번 로컬 finding은 닫혔지만 report 15의 외부/실기 제외 gate는 새 immutable source SHA와 해당 환경 증거가 생기기 전까지 그대로 유지한다.

## 10. Coder handoff

추가 로컬 수정 finding은 없다. 향후 release gate를 진행할 때는 current immutable commit을 만든 뒤 hosted Windows MSVC/package/startup/SLSA/SPDX를 그 SHA에 결속하고, clean Windows 및 실제 high-DPI/IME·장시간 실기 결과를 별도 보고서에 기록한다.
