# audit_roadmap.md (Turn 3 remediation 활성 gate)

작성일: 2026-09-04 (Asia/Seoul)
근거: `docs/audit/audit_report_15.md`, `docs/multi_audit/3/final_audit_report_3.md`, 현재 `spec.md`, 실제 소스와 Debug/Release/CTest 실행

현재 활성 gate는 아래 5~7절이다. 1~4절은 Turn 1~2의 historical/superseded 근거이며 v0.10의 현재 schema·층·finding 번호 권위가 아니다. 원본 감사 보고서와 manifest는 수정하지 않는다.

현재 독립 재감사: `docs/audit/audit_report_16.md`
재개방 근거: `docs/audit/audit_report_15.md`
현재 수정 계획: `docs/audit/remediation_plan_16.md` (완료)
이전 coder evidence: `docs/audit/audit_report_14.md`
현재 구현 상태: **로컬 개발 checkpoint PASS. 전체 release 외부 gate는 Release Candidate 동결 시점으로 명시 이관**

## 1. 변경 전 기준선

- Debug build와 직접 실행 TestHarness: 통과
- Debug CTest: 0 tests
- Release build: TestHarness의 `assert` 제거와 `-Werror` unused 오류로 실패
- 제품 저장: CWD direct truncate, backup/atomic replace 없음, 손상 시 자동 reset
- 판정: `HOLD`

## 2. 우선순위 및 종료 조건

### P0 — 데이터와 검증 신뢰

- FIN-F001: `spec.md`에 lane, 사용자, OS, 완주, 시간, 가격, 오디오, 접근성, 패키지 gate가 닫혀야 한다.
- FIN-F011: per-user atomic save/config, backup/quarantine, typed result, failure injection이 통과해야 한다.
- FIN-F010: save schema v3, v1/v2 migration, bounds validation, town-checkpoint 정책이 round-trip을 통과해야 한다.
- FIN-F013: Debug/Release build와 CTest가 실제 테스트를 실행하고 성공해야 한다.
- FIN-F012: TPK 후 stack root가 GameOver 하나로 교체되고 마지막 town checkpoint를 보존해야 한다.

### P1 — 완결 가능한 수직 슬라이스

- FIN-F002: clean save에서 landmark, boss, result까지 완주 가능해야 한다.
- FIN-F003: 19 item/3 quest reachability와 one-time completion을 테스트해야 한다.
- FIN-F004: guild 캐릭터 생성과 combat item/ally target 선택의 confirm/cancel transcript가 통과해야 한다.
- FIN-F005: dice/Bless/natural roll/resistance/equipment/reward ledger가 고정 seed로 일치해야 한다.
- FIN-F006: 하나의 session RNG와 세 encounter tier가 동일 seed에서 재현돼야 한다.
- FIN-F007: save 없음/정상/손상별 New/Continue와 파괴적 행동 취소 시 bytes 불변을 확인해야 한다.
- FIN-F008: 0/1/4인 HUD snapshot이 실제 Party 상태와 일치해야 한다.
- FIN-F009: 5 locale key coverage, 공통 back/options 입력, 최소 text size/contrast 및 Linux 실화면을 확인해야 한다.

### P2 — 배포 후보

- FIN-F014: immutable dependency, CMake install/CPack, Linux/Windows CI와 arbitrary-CWD Linux smoke가 필요하다.
- FIN-F015: dependency notice/checksum을 기록한다. 폰트 권리와 legal/support identity는 `Human Review Required`다.
- FIN-F016: README/BUILD_GUIDE/IMPLEMENTATION_SUMMARY/designs/CHANGELOG의 현재 상태와 파일 책임을 동기화한다.

## 3. 판정 규칙

- 로컬에서 실행하지 못한 Windows/법률/장시간 플레이 gate는 PASS로 승격하지 않는다.
- 각 finding은 문서, 코드, 회귀 테스트 또는 명시적 external blocker를 모두 기록한다.
- 최종 재감사는 `docs/multi_audit/2/remediation_reaudit_2.md`에 원 finding ID를 유지해 기록한다.
- Critical 또는 로컬에서 재현 가능한 Major가 남으면 최종 판정은 `HOLD`다.

## 4. Turn 2 완료 및 잔여 gate

### 로컬 완료

- Debug/Release CTest 7/7, production-linked controller transcript
- save schema v3/config schema v2 atomic persistence, backup/quarantine, town checkpoint와 TPK root 전이
- seeded 한 층 → Door → BossGate → boss → Victory의 저장·재로드 경로
- 콘텐츠/전투/모집·대상 선택/HUD 계약 테스트
- Linux install/CPack, SHA-256, arbitrary-CWD resource/startup smoke
- 현재 구현에 맞춘 README/빌드/설계/인수인계/변경 이력

### PASS 전 필수

- FIN-F009: Linux 5-locale × 3-scale production raster는 확보. 실제 OS high-DPI와 모든 장시간 동적 overlay 판독
- FIN-F014: hosted Ubuntu/Windows artifact와 Windows MSVC CTest/package/5초 startup은 완료. clean Windows 10/11 VC++ runtime 조건은 잔여 위험
- FIN-F015: 폰트 provenance/SBOM/scanner와 hosted SLSA/SPDX attestation은 완료. legal/support human sign-off 잔존
- 실제 30~60분 완주와 여러 seed의 정량 밸런스 측정

## 5. v0.10.0 영속 월드·퀘스트 gate

- schema v4의 3층 snapshot round-trip, 변조 입력 거부와 v1~v3 결정론적 이관을 검증한다.
- 같은 save의 마을 재입장/Continue에서 지형·fog·목표 상태가 유지되고 New Game에서만 교체되는지 검증한다.
- 회수·보스·NPC 퀘스트의 수주/현장 달성/보고 보상과 저장 실패 rollback, 완료 원장 중복 방지를 검증한다.
- 5 locale × 75/100/200%에서 동적 퀘스트 보드, 일지, 층/목표 표식의 transcript와 raster를 검증한다.

로컬 구현 결과: Debug/Release CTest 16/16, 5 locale × 3 scale raster, Linux CPack·임의 CWD resource·SHA-256 gate 통과. clean Windows 10/11, 실제 OS high-DPI와 장시간 3층 밸런스는 `UNVERIFIED`다.

## 6. Turn 3 종료 gate

- P0: deterministic seedless migration, strict v4, canonical world validation, key/quest/object 양방향 invariant, in-memory Party/World/RNG rollback.
- P1: shared identity/slot validation, poison death turn skip, all terminal combat cleanup, CharacterInfo status, always-confirm New Game, shutdown retry/leave.
- P2: exact-byte save, canonical quest/shop API, symlink/backup recovery, three-quest/4-character/repeated-world E2E와 active 문서 정리.
- 로컬 Debug/Release와 Linux package는 현재 tree에서 재실행한다. hosted Windows/SLSA/SPDX와 clean Windows 10/11은 Release Candidate의 immutable SHA가 확정될 때까지 `UNVERIFIED`다.

Turn 3 로컬 결과: FIN-F001~F035의 코드·문서·테스트 remediation과 Debug/Release 16/16, Linux package gate를 통과했다. current v0.10 hosted Windows/SLSA는 `Not Covered`이며 Release Candidate 시점의 후속 검증으로 이관한다.

## 7. RA15 recovery remediation gate

- RA15-F001: TPK/Title 복구 실패 뒤 `recoveryPending` write fence가 일반 저장과 종료 자동 저장을 막아야 한다. pending 종료의 Enter는 load 재시도, Esc는 무저장 종료다.
- RA15-F002 / FIN-F031: primary가 없고 backup만 손상된 경우에도 backup을 격리하고 typed `Corrupt` 결과를 반환해야 한다.
- RA15-F003 / FIN-F017: active 문서는 report 15 → remediation plan 16 → 후속 재감사 계보와 `ShutdownState` 책임을 현재 구현과 일치시켜야 한다.
- 완료 증거: corrupt/no-backup, missing-primary+corrupt-backup, transient I/O failure-specific 회귀, Debug/Release CTest 16/16, Linux package/resource/checksum, sealed Turn 3 manifest 무결성.

RA15 후속 결과: 위 세 failure mode와 TPK/Title/shutdown lifecycle을 현재 production-linked 테스트에서 통과했다. Debug 16/16(146.88초), Release 16/16(44.34초), Linux CPack/resource/startup/SHA-256 및 manifest 6/6은 `docs/audit/audit_report_16.md`에 기록한다.

## 8. 외부 release gate 이관 결정

- 결정일: 2026-09-04 (Asia/Seoul)
- 현재 단계는 릴리즈 단계가 아니므로 hosted Windows MSVC/package/startup/SLSA/SPDX, clean Windows 10/11, 실제 OS high-DPI/IME, 30~60분 장시간·다중 seed 밸런스와 human/legal gate를 Release Candidate 동결 이후로 이관한다.
- 이 이관은 현재 로컬 개발 checkpoint를 차단하지 않는다. 다만 외부 gate를 통과한 것으로 간주하거나 current release PASS를 주장하지 않는다.
- 실제 릴리즈 준비 시점에는 그때의 Release Candidate immutable SHA를 먼저 확정하고, 모든 hosted artifact·attestation·실기 보고서를 동일 SHA에 결속해 별도 감사보고서로 기록한다.
