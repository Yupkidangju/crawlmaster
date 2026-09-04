# audit_roadmap.md (Turn 2 재감사 후 출시 gate)

작성일: 2026-09-04 (Asia/Seoul)
근거: `docs/multi_audit/1/final_audit_report_1.md`, 현재 `spec.md`, 실제 소스와 변경 전 Debug/Release/CTest 실행

이 문서는 과거 Phase 완료 목록이 아니라 현재 데모 후보가 다음 gate를 통과할 수 있는지 판정한다. Turn 1의 원본 감사 보고서는 수정하지 않는다.

현재 독립 결과: `docs/audit/audit_report_10.md`  
현재 수정 계획: `docs/audit/remediation_plan_12.md`
최신 재감사: `docs/audit/audit_report_13.md`
현재 구현 상태: **로컬 Needs Fix 0, FIN-F014와 FIN-F015 supply-chain gate 해소, legal/high-DPI/장시간 사람 evidence 잔존**

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
- FIN-F010: schema v2, v1 migration, bounds validation, town-checkpoint 정책이 round-trip을 통과해야 한다.
- FIN-F013: Debug/Release build와 CTest가 실제 테스트를 실행하고 성공해야 한다.
- FIN-F012: TPK 후 stack root가 GameOver 하나로 교체되고 마지막 town checkpoint를 보존해야 한다.

### P1 — 완결 가능한 수직 슬라이스

- FIN-F002: clean save에서 landmark, boss, result까지 완주 가능해야 한다.
- FIN-F003: 19 item/3 quest reachability와 one-time completion을 테스트해야 한다.
- FIN-F004: guild preview와 combat item/ally target 선택의 confirm/cancel transcript가 통과해야 한다.
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
- schema v2 atomic save/config, backup/quarantine, town checkpoint와 TPK root 전이
- seeded 한 층 → Door → BossGate → boss → Victory의 저장·재로드 경로
- 콘텐츠/전투/모집·대상 선택/HUD 계약 테스트
- Linux install/CPack, SHA-256, arbitrary-CWD resource/startup smoke
- 현재 구현에 맞춘 README/빌드/설계/인수인계/변경 이력

### PASS 전 필수

- FIN-F009: Linux 5-locale × 3-scale production raster는 확보. 실제 OS high-DPI와 모든 장시간 동적 overlay 판독
- FIN-F014: hosted Ubuntu/Windows artifact와 Windows MSVC CTest/package/5초 startup은 완료. clean Windows 10/11 VC++ runtime 조건은 잔여 위험
- FIN-F015: 폰트 provenance/SBOM/scanner와 hosted SLSA/SPDX attestation은 완료. legal/support human sign-off 잔존
- 실제 30~60분 완주와 여러 seed의 정량 밸런스 측정
