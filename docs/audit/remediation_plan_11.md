# Audit Report 10 remediation 계획

작성일: 2026-09-03 (Asia/Seoul)  
입력 감사: `audit_report_10.md`  
예정 재감사: `audit_report_11.md`  
상태: 로컬 구현 및 Re-audit #2 완료 — 외부 gate로 HOLD  
결과 보고서: `audit_report_11.md`

## 1. 범위와 판정 원칙

- `audit_report_10.md`와 이전 번호 보고서는 수정하지 않는다.
- FIN-F005, FIN-F006, FIN-F009를 production 호출 경로에서 먼저 재현하고 회귀 테스트를 RED로 확인한다.
- FIN-F010과 FIN-F016의 fixture/API/visual 문서 drift를 같은 작업에서 정리한다.
- hosted Windows, 폰트 권리, legal owner처럼 현재 환경에서 생성할 수 없는 증거는 구성 파일이나 자기 주장으로 PASS 처리하지 않는다.
- 소스 revision과 remote가 없는 상태에서는 hosted CI를 실행할 수 없으므로 workflow의 fail-closed 증거 수집 계약만 구현하고 실제 run은 `UNVERIFIED`로 남긴다.

## 2. 확정 계약

### FIN-F005

- 일반 공격과 명중 굴림을 사용하는 모든 공격형 Skill은 `CombatRules`의 명중, 자연 1/20, Bless, 피해 주사위, 피해 타입, resistance 계산을 사용한다.
- 무기 기반 Skill은 장착 무기의 dice count/sides/type을 보존한다.
- Skill 고유 attack bonus, flat damage bonus, extra dice만 parameter로 추가한다.

### FIN-F006

- `SessionRng`는 seed와 원시 `mt19937` draw count를 소유한다.
- schema v2 checkpoint는 `lastSessionSeed`와 `sessionRngDrawCount`를 함께 저장한다.
- Continue는 저장된 seed/count를 복원한 뒤 다음 production 난수부터 이어간다.
- New Game은 새 entropy seed로 global session을 초기화한 뒤 최초 checkpoint를 쓴다.
- v1 또는 seed가 0인 저장은 Continue 시 새 session seed를 만들고 즉시 canonical v2로 저장한다.

### FIN-F009

- 사용자에게 보이는 UI chrome, 콘텐츠 이름/설명, 상태/전투/아이템 로그는 locale key와 placeholder formatter를 거친다.
- `O`는 모든 Town substate와 Combat의 플레이어 overlay에서 Settings를 열며, 적 턴에는 상태 mutation을 막기 위해 거부한다.
- `Esc`는 현재 overlay를 먼저 취소하고, overlay가 없을 때만 상위 상태로 복귀한다.
- 5 locale key parity, 필수 content/log key, 75/100/200% text sizing과 input transcript를 자동 테스트한다.
- 실제 CJK raster와 고DPI 판독은 캡처가 생성·검토된 경우에만 Verified로 판정한다.

### FIN-F010 / FIN-F016

- root legacy `save.json`은 제품 입력이 아니므로 `tests/fixtures/save_v1.json`으로 이동하고 migration 목적을 문서화한다.
- `Skill::execute` 문서 시그니처는 실제 `bool` 계약과 일치시킨다.
- 미구현 screen shake는 구현 완료가 아니라 Deferred visual polish로 표시한다.

## 3. 구현 순서와 검증

1. 전투 Skill 조합 테스트 RED → 공통 규칙 적용 → focused GREEN.
2. seed/count 저장·Continue replay 테스트 RED → RNG lifecycle 구현 → focused GREEN.
3. localization/content/input 계약 테스트 RED → formatter와 production UI 경로 정리 → focused GREEN.
4. fixture/document drift와 공급망 evidence 생성.
5. Debug/Release 전체 CTest, Linux package smoke, SBOM/scanner, 가능하면 hosted run 조회.
6. 독립 5축 재검토 후 `audit_report_11.md` 작성.

## 4. 종료 기준

- FIN-F005/006은 unit helper가 아니라 `Skill::execute`, Title Continue, Dungeon/Combat production consumer를 통과해야 한다.
- FIN-F009는 key 수만 같다는 증거로 완료하지 않는다. 실제 호출 문자열과 input state transcript를 검사한다.
- hosted Windows와 human legal approval가 없으면 최종 판정은 HOLD다.

## 5. 완료 결과

- FIN-F005, FIN-F006, FIN-F009, FIN-F010, FIN-F016: 현재 Linux demo 계약에서 Verified
- Debug/Release: 각각 CTest 13/13 PASS
- Local MinGW: 전체 target compile/link, Windows ZIP/checksum/PE payload Verified
- Font: 3개 번들 파일의 byte identity/upstream/license/raster Verified
- Supply chain: artifact SPDX, OSV/Grype 0 matches, 고정 action SHA와 attestation workflow 추가
- FIN-F014: hosted Windows/runtime `UNVERIFIED`
- FIN-F015: hosted attestation와 product legal/support Human Review Required
- 최종 판정: `audit_report_11.md`의 HOLD
