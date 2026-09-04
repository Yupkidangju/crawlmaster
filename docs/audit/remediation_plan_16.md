# RA15 remediation 계획

작성일: 2026-09-04 (Asia/Seoul)
권위: `docs/audit/audit_report_15.md`

## 대조 판정

- RA15-F001: Confirmed. TPK load 실패 뒤 active Party가 shutdown save 대상이다.
- RA15-F002 / FIN-F031: Confirmed. primary absent 분기가 corrupt backup을 격리하지 않고 조기 반환한다.
- RA15-F003 / FIN-F017: Confirmed. active roadmap 계보와 ShutdownState 책임표가 stale하다.

## 동결 계약

- failed TPK/Title recovery는 in-memory state를 보존하면서 `recoveryPending`으로 전환한다.
- `recoveryPending`에서는 일반 save와 shutdown auto-save를 금지한다.
- 성공 load 또는 확인된 New Game만 write eligibility를 복구한다.
- recovery-pending ShutdownState의 Enter는 load 재시도, Esc는 저장 없이 종료다.
- primary가 없고 backup만 손상됐으면 backup을 quarantine하고 `Corrupt`를 반환한다.

## 완료 조건

- corrupt/no-backup, missing-primary+corrupt-backup, transient I/O의 TPK→shutdown matrix가 dead Party overwrite를 만들지 않는다.
- Debug/Release CTest 16/16, Linux package/resource/checksum과 Turn 3 manifest 검증을 다시 통과한다.
- 후속 재감사 보고서는 RA15-F001~F003와 FIN-F017/FIN-F031을 원 ID로 연결한다.
