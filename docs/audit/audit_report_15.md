# Crawlmaster Turn 3 remediation 독립 재감사 보고서

## 1. 메타데이터와 판정

- 작성일: 2026-09-04 (Asia/Seoul)
- 원 감사: `docs/multi_audit/3/final_audit_report_3.md`
- 코더 입력: `docs/audit/remediation_plan_14.md`, `docs/audit/audit_report_14.md`
- 대상 tree: `HEAD 927753278f46b92a015197ee229edce4f52e0657` + 현재 미커밋 working tree
- 감사 기준: `AI_AUDIT_DOC_STANDARD.md`, 현재 `spec.md`, 실제 코드·테스트·패키지
- 코드 수정: 없음
- **FIN-F001~F035 독립 판정: Verified 33 / Needs Fix 2 / Unverified 0**
- 새 finding: Major 1, Minor 2
- **로컬 remediation 판정: HOLD**
- **전체 release 판정: HOLD**

`audit_report_14.md`는 구현자 self-review임을 자체 명시하므로 coder claim/evidence로만 사용했다. 이번 보고서는 각 finding을 현재 문서·구현·실행 결과에 다시 대조한 독립 판정이다.

## 2. 범위와 제외 범위

### 포함

- Turn 3 FIN-F001~F035의 현재 구현 및 회귀 테스트
- character 생성·상태·사망·TPK·종료 lifecycle
- 신규/legacy quest, 보상 transaction과 완료 원장
- 3층 world 생성, strict v4, migration, save/load/rollback/RNG
- 문서 authority와 current v0.10 evidence provenance
- Debug/Release build·CTest, locale parity, raster 대표 화면, Linux CPack/resource/checksum

### 제외 또는 미검증

- current v0.10 immutable SHA의 hosted Windows MSVC/package/startup/SLSA/SPDX
- clean Windows 10/11, Windows reparse point, macOS
- 실제 OS high-DPI/IME, 30~60분 장시간·다중 seed 밸런스
- power-loss와 다중 writer

위 범위는 기존 문서대로 `UNVERIFIED` 또는 Human Review gate이며 PASS로 승격하지 않았다.

## 3. 독립 실행 증거

### 빌드와 테스트

```text
cmake --build build/debug --parallel 2
ctest --test-dir build/debug --output-on-failure --no-tests=error
-> build PASS, 16/16 PASS, 141.73초

cmake --build build/release --parallel 2
ctest --test-dir build/release --output-on-failure --no-tests=error
-> build PASS, 16/16 PASS, 43.13초
```

- `git diff --check`: PASS
- 5 locale JSON parse/key parity: `ko/en/ja/zh_tw/zh_cn` 각각 450 key, PASS
- Turn 3 sealed manifest: SHA `396f27c1d0b292b611906886a6bad3152a48308d97574a3764b00fc1cb0af998`, 6/6 report verified, missing 0
- representative raster 직접 판독:
  - KO 200% CharacterInfo: Dead/Poison/Paralysis/STR/DEX/Bless 표시 확인
  - KO 100% ShutdownState: 재시도/명시 종료 안내 확인
  - JA 200% QuestJournal: 선택 quest와 가이드의 겹침 없는 표시 확인

### Linux package

```text
cpack --config build/release/CPackConfig.cmake -C Release
-> Crawlmaster-0.10.0-Linux-x86_64.tar.gz 생성

sha256sum -c Crawlmaster-0.10.0-Linux-x86_64.tar.gz.sha256
-> OK

임시 디렉터리에 새 archive를 풀고 /tmp에서 packaged binary --verify-resources
-> Resource OK
```

- archive 크기: 16,746,871 bytes
- SHA-256: `8ce6363685fbdad3ca1b6fb66e3b12207cc99da6416f011b819818a3378b0de5`
- 첫 checksum 시도는 sidecar의 상대 경로를 repository root에서 해석해 실패했고, sidecar 디렉터리에서 재실행하여 성공했다. 이는 호출 위치 오류이며 package 결함이 아니다.

## 4. FIN-F001~F035 재감사 ledger

| Finding | 독립 상태 | 현재 증거와 판정 |
| --- | --- | --- |
| FIN-F001 | Verified | seedless v1/v2는 canonical JSON+salt FNV-1a seed를 사용하며 두 process fixture 비교가 통과한다. |
| FIN-F002 | Verified | 성공 load와 operation rollback은 Party 및 global RNG seed/draw count를 함께 복원한다. |
| FIN-F003 | Verified | Town/Combat/Dungeon/CharacterInfo/CharacterCreation의 pre-commit 실패는 deep `PartyCheckpoint`로 memory/RNG를 복구한다. |
| FIN-F004 | Verified | 현재 계약과 구현은 latest successful full-session checkpoint로 일치한다. |
| FIN-F005 | Verified | v4 root/Character/Quest required-field matrix와 strict parser가 존재한다. |
| FIN-F006 | Verified | retrieve key↔ready quest↔resolved object 및 completed↔no-key를 load/save에서 양방향 검증한다. |
| FIN-F007 | Verified | common validator가 Door 1개, entry U/FOW, terminal farthest, connectivity와 object 위치를 load/save에 적용한다. |
| FIN-F008 | Verified | `DISCOVERED => visited`를 world validator와 renderer 모두 강제한다. |
| FIN-F009 | Verified | `CharacterIdentityRules`가 draft와 v3/v4 deserializer에서 UTF-8/code-point/control/format/noncanonical boundary를 공유한다. |
| FIN-F010 | Verified | class×level max spell-slot matrix를 parser와 Agency test가 검증한다. |
| FIN-F011 | Verified | turn effect 뒤 entity death를 재확인하며 party/foe poison death regression이 통과한다. |
| FIN-F012 | Verified | victory와 successful escape가 공통 combat-buff cleanup을 사용한다. |
| FIN-F013 | Verified | CharacterInfo normal/large 상태 행, semantic assertion과 raster가 확인됐다. |
| FIN-F014 | Verified | save 유무와 무관하게 New Game 첫 Enter는 confirm만 열고 cancel은 state/save를 보존한다. |
| FIN-F015 | Verified within original scope | 일반 save failure/CommittedDurabilityUnknown의 ShutdownState 재시도·명시 종료는 동작한다. 단, failed recovery와 결합된 새 Major는 RA15-F001이다. |
| FIN-F016 | Verified | spec/designs가 boss/BossGate entry-trigger로 정렬되고 auto-path regression이 해당 정책을 검증한다. |
| FIN-F017 | **Needs Fix** | `audit_roadmap.md:8-11`이 여전히 report 10/plan 12/report 13과 “Turn 3 진행 중”을 현재 값으로 표시해 `:89`의 완료 결과와 충돌한다. spec controller roster와 implementation summary도 새 `ShutdownState` 책임을 열거하지 않는다. |
| FIN-F018 | Verified | current v0.10 hosted Windows/SLSA 주장은 `UNVERIFIED`로 낮아졌고 0.9.4 evidence와 분리됐다. |
| FIN-F019 | Verified (local automated scope) | 4인 creation/death/TPK, 세 quest report/failure/repeat/reload, auto objective, TPK latest checkpoint, repeated Continue/New Game 회귀가 추가됐다. 실제 장시간 실기는 별도 미검증이다. |
| FIN-F020 | Verified | negative progress no-op, overflow saturating clamp가 구현·테스트됐다. |
| FIN-F021 | Verified | Party ledger와 retained Quest completion flag를 보고 시 함께 갱신한다. |
| FIN-F022 | Verified | null member를 거부한다. |
| FIN-F023 | Verified | set-derived ID를 정렬하고 save→load→save exact bytes를 검증한다. |
| FIN-F024 | Verified | CharacterInfo도 common no-effect consumable guard를 사용하며 inventory/RNG 불변을 검증한다. |
| FIN-F025 | Verified within original scope | failed load가 기존 memory/pointer/active flag/global RNG를 바꾸지 않는다. 이 보존 상태의 shutdown 정책은 RA15-F001로 별도 판정한다. |
| FIN-F026 | Verified | 목적형 report는 canonical world object가 RESOLVED일 때만 완료한다. |
| FIN-F027 | Verified | completed ID의 canonical/duplicate validation이 존재한다. |
| FIN-F028 | Verified | Town product path와 `Party::acceptQuest`는 canonical definition만 수용한다. |
| FIN-F029 | Verified | orphan legacy locale key 9개가 제거되고 5 locale parity가 통과한다. |
| FIN-F030 | Verified | save 직전 Party/World invariant validation이 invalid mutable state의 commit을 막는다. |
| FIN-F031 | **Needs Fix** | primary와 backup이 모두 손상된 경로는 수정됐지만, 원 finding의 `primary absent + corrupt .bak` 경로는 `Party.cpp:447-450`에서 조기 반환해 backup을 격리하지 않는다. 독립 probe 결과 `load_status=1`, `backup_after=present`. |
| FIN-F032 | Verified | global RNG를 소비하던 no-arg map generation API와 호출자가 제거됐다. |
| FIN-F033 | Verified (Linux) | primary/backup leaf symlink read/write 거부와 target bytes 불변을 확인했다. Windows reparse-point는 미검증 유지. |
| FIN-F034 | Verified | 문서와 14px 일반 본문/16px 핵심 상태 정책이 일치한다. |
| FIN-F035 | Verified | shop purchase 8종이 factory의 단일 registry를 사용하고 Town input/render가 같은 값을 소비한다. |

요약: **33 Verified, 2 Needs Fix(FIN-F017, FIN-F031)**. `audit_report_14.md`의 35/35 local PASS 주장은 현재 tree에서 그대로 유지되지 않는다.

## 5. 새 finding

### [RA15-F001] TPK/save 복구 실패 뒤 정상 종료가 사망한 in-memory Party를 새 정상 save로 기록한다

- Pass: Debug / Security / Cross-Pass
- Pattern: `DBG-001`, `TEST-001`
- Area: failed recovery, TPK, shutdown durability, corrupt-save no-overwrite boundary
- Severity: **Major**
- Status: **Needs Fix**
- Summary: `Party::loadFromFile()`은 FIN-F025에 따라 실패 시 기존 memory와 active-session flag를 보존한다. TPK에서 checkpoint load가 실패하면 그 memory는 전멸한 현재 전투 상태다. 이후 `Game::requestShutdown()`은 active session을 정상 save하고 성공 시 종료하므로, 격리된/복구 불능 save 자리에 사망 Party를 canonical save로 새로 기록한다.
- Evidence:
  - `src/controller/CombatState.cpp:362-368`: TPK는 `loadFromFile()` 결과가 실패해도 `GameOverState(false)`로 전이한다.
  - `src/model/Party.cpp:218-475`: 실패 load는 Party와 `m_hasActiveSaveSession`을 보존한다.
  - `src/core/Game.cpp:61-72`: active session이면 현재 Party를 저장하고 `Saved`에서 즉시 종료한다.
  - `spec.md:351`: backup도 없으면 New Game을 명시적으로 선택하기 전까지 디스크를 덮어쓰지 않는다.
  - `spec.md:54,355`: TPK는 성공 checkpoint 복구이며 정상 save를 덮어쓰지 않아야 한다.
  - 현재 `CrawlmasterRuntime`을 링크한 격리 probe:

```text
restore_status=5 active=1 dead=1 primary=0
shutdown_approved=1 reload_status=1 reloaded_dead=1
```

  `restore_status=5`는 `Corrupt`, `reload_status=1`은 이후 새 save가 `Loaded`된 상태다.
- Expected: checkpoint 복구 실패 상태에서는 자동 save가 금지되고, 복구 재시도·명시적 New Game·저장 없이 종료 중 하나를 선택할 때까지 recovery-pending 경계를 유지해야 한다.
- Actual: 복구 실패 뒤에도 active-saveable로 남아 shutdown save가 dead Party를 정상 checkpoint로 만든다.
- Impact: 마지막 복구 가능 진행을 대체하거나, 적어도 TPK 실패 상태를 정상 save로 위장해 Continue가 사망 Party를 불러오게 한다. 손상 복구와 New Game 분리 계약을 우회한다.
- Suggested Fix: `hasActiveSaveSession`과 별개로 `saveAllowed/recoveryPending` 상태를 두고 TPK/Title load 실패 시 shutdown 자동 저장을 차단한다. GameOver/Title/ShutdownState는 load 재시도, 명시적 New Game, 저장 없이 종료만 제공해야 한다. 성공 load 또는 명시적 새 게임만 write eligibility를 복구해야 한다.
- Re-audit Method: corrupt/no-backup, missing-primary+corrupt-backup, transient IoError에서 TPK→GameOver→Title/close를 각각 실행하고 primary/quarantine bytes, active/recovery flag, shutdown state와 Continue 결과를 검사한다.

### [RA15-F002] primary가 없고 손상 backup만 있으면 `.bak`가 여전히 격리되지 않는다

- Pass: Debug
- Pattern: `TEST-001`
- Area: backup recovery
- Severity: **Minor**
- Status: **Needs Fix**
- Related Original: FIN-F031
- Evidence: `Party.cpp:447-450`은 primary `NotFound`와 backup 존재 시 `recoverBackup()` 결과를 즉시 반환한다. backup quarantine은 primary가 `Corrupt`인 뒤의 `:468-470`에만 있다. 추가 테스트 `testCorruptBackupIsAlsoQuarantined`는 primary+backup 동시 손상만 다룬다.
- Independent Probe: `load_status=1 backup_after=present` (`RngProcessReplayTests` process exit 1, malformed `.bak` 존속).
- Impact: Continue가 같은 손상 backup을 반복 시도하고 FIN-F031의 원래 recovery dead-end가 남는다.
- Suggested Fix: candidate 단위로 corrupt backup을 격리하고 quarantine 실패 결과도 무시하지 말고 typed result로 전달한다.
- Re-audit Method: primary absent + malformed/oversized backup 조합에서 `.bak`가 고유 quarantine path로 이동하고 다음 Continue가 같은 candidate를 반복하지 않는지 확인한다.

### [RA15-F003] 활성 감사 로드맵의 현재 계보와 상태가 Turn 3 완료 주장과 충돌한다

- Pass: Implementation Compliance
- Pattern: `IMP-004`
- Area: active documentation authority
- Severity: **Minor**
- Status: **Needs Documentation Recovery**
- Related Original: FIN-F017
- Evidence:
  - `audit_roadmap.md:8-11`은 현재 결과/계획/최신 재감사를 report 10, plan 12, report 13으로 두고 Turn 3 remediation 진행 중이라고 한다.
  - 같은 문서 `:80,89`는 Turn 3 FIN-F001~F035와 16/16 완료를 주장한다.
  - `spec.md:65`의 Controller roster와 `IMPLEMENTATION_SUMMARY.md:47-58` 파일 책임에는 새 `ShutdownState`가 빠져 있다.
- Impact: 후속 작업자가 stale 보고서를 최신 권위로 읽거나 shutdown failure surface를 책임표에서 누락할 수 있다.
- Suggested Fix: roadmap 상단 계보를 remediation plan 14/coder report 14/독립 report 15로 갱신하고 상태를 현재 verdict로 맞춘다. `ShutdownState`를 state machine/책임표에 포함한다.
- Re-audit Method: active 문서의 latest report/status와 실제 파일/CMake state roster를 다시 대조한다.

## 6. Cross-pass 판단

- FIN-F015의 일반 종료 저장 재시도 UI와 FIN-F025의 failed-load 비파괴성은 각각 독립적으로는 구현됐다.
- 두 변경을 결합하면 recovery 실패 상태가 여전히 `active-saveable`로 남아 RA15-F001을 만든다. 개별 green test가 교차 상태의 안전을 보장하지 못한 사례다.
- FIN-F031은 새 test 이름만 보면 해소된 것처럼 보이지만 원 finding의 primary-absent branch를 실행하지 않아 부분 수정이다.
- Debug/Release 16/16은 유효한 정상·부분 실패 증거지만 위 두 실패 mode를 포함하지 않으므로 반증이 아니다.

## 7. Required Fixes Before Local PASS

1. RA15-F001: failed recovery와 shutdown write eligibility를 분리하고 TPK/Title close 회귀를 추가한다.
2. FIN-F031/RA15-F002: primary absent + corrupt backup을 격리하고 정확한 status/path를 검증한다.
3. FIN-F017/RA15-F003: active roadmap lineage/status와 ShutdownState 책임 문서를 동기화한다.
4. 수정 후 Debug/Release 16/16, package/resource/checksum, Turn 3 manifest를 다시 검증한다.

## 8. 최종 판정

### 로컬 remediation

**HOLD**

대부분의 Turn 3 finding은 실제 코드와 테스트로 해소됐고 build/test/package 품질도 개선됐다. 그러나 복구 실패 뒤 사망 Party를 정상 save로 기록하는 Major 데이터 경계가 독립 재현됐으며, FIN-F031과 FIN-F017도 완전히 닫히지 않았다. 따라서 `FIN-F001~F035 35/35 PASS`는 승인하지 않는다.

### 전체 release

**HOLD**

로컬 finding과 별개로 current v0.10 hosted Windows/SLSA, clean Windows, 실제 high-DPI/IME, 장시간 밸런스와 human/legal gate가 남아 있다.

## 9. Coder Handoff

`/mnt/Projects_SSD/cpp/crawlmaster/docs/audit/audit_report_15.md`를 먼저 읽고 RA15-F001~F003 및 재개방된 FIN-F017/FIN-F031을 현재 문서와 실제 코드에 대조해 수정하세요. 계약 변경이 필요하면 `spec.md`와 관련 설계 문서를 먼저 갱신하고, 수정 후 Debug/Release 16/16·Linux package·failure-specific 회귀·독립 재감사 증거를 기록하세요.
