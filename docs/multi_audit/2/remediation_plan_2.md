# Turn 2 remediation 계획 및 finding 검증 매트릭스

작성일: 2026-09-03 (Asia/Seoul)  
원본 감사: `../1/final_audit_report_1.md`  
현재 상태: 로컬 구현 및 Turn 2 재감사 완료 — 외부 출시 gate로 HOLD  
결과 보고서: `remediation_reaudit_2.md`

## 1. 범위와 증거 원칙

- 원본 Turn 1 보고서와 sealed source reports는 수정하지 않는다.
- 각 finding은 현재 문서와 실제 호출 경로에서 다시 확인한다.
- 계약 변경은 `spec.md`와 `DESIGN_DECISIONS.md`를 먼저 갱신한다.
- 동작 변경은 Release-safe 회귀 테스트가 먼저 실패하는 것을 확인한 뒤 구현한다.
- Git metadata가 없으므로 commit/push는 수행할 수도, 검증할 수도 없다.

## 2. 현재 확인 결과와 계획

| Finding | 현재 검증 | 우선순위 | 계획된 완료 증거 |
| --- | --- | --- | --- |
| FIN-F001 | Confirmed | P0 | lane/OS/완주/가격/오디오/접근성/패키지 계약 |
| FIN-F011 | Confirmed Critical | P0 | atomic write, backup, quarantine, typed result, failure tests |
| FIN-F010 | Confirmed | P0 | schema v2, v1 migration, validation, town checkpoint tests |
| FIN-F013 | Confirmed | P0 | Release-safe runner, CTest registration, Debug/Release green |
| FIN-F012 | Confirmed | P0 | clear-and-replace root, GameOver transition test |
| FIN-F002 | Confirmed | P1 | landmark/boss/result campaign transcript |
| FIN-F003 | Confirmed | P1 | content registry, loot/quest reachability matrix |
| FIN-F004 | Confirmed | P1 | preview/target/confirm/cancel transcript |
| FIN-F005 | Confirmed | P1 | deterministic combat/equipment/reward ledger |
| FIN-F006 | Confirmed | P1 | one seeded RNG and tier simulation |
| FIN-F007 | Confirmed | P1 | New/Continue and destructive cancel byte tests |
| FIN-F008 | Confirmed | P1 | Party HUD snapshot tests and runtime capture |
| FIN-F009 | Confirmed, raster partial | P1 | 5-locale key/input/text/contrast checks; Linux capture |
| FIN-F014 | Confirmed | P2 | install/CPack/CI/arbitrary-CWD smoke |
| FIN-F015 | Confirmed | P2 | immutable pins/notices/checksums; font/legal remains human gate |
| FIN-F016 | Confirmed | P2 | current file responsibility and completion-state docs |

## 3. 외부 gate

- Windows hosted artifact 및 실기 smoke: `UNVERIFIED` until CI/runtime evidence exists.
- 30~60분 완주 시간과 장기 밸런스: `UNVERIFIED` until measured playtest.
- 번들 폰트의 상업 재배포 권리와 법률/지원 주체: `Human Review Required`.

이 외부 gate는 로컬 구현을 중단시키지 않지만 최종 상용 PASS를 차단한다.

## 4. 완료 요약

- FIN-F001~F008, FIN-F010~F013, FIN-F016: 로컬 구현 및 회귀 증거 Verified
- FIN-F006: 결정적 RNG와 tier 구조 Verified, 정량 밸런스는 `UNVERIFIED`
- FIN-F009: locale key/설정은 구현, 전 화면 literal/raster/고DPI는 Partial
- FIN-F014: Linux package는 Verified, Windows hosted/실기는 Partial
- FIN-F015: immutable direct dependency/notice/checksum은 구현, 폰트·legal은 Human Review Required
- 최종 판정과 명령별 증거는 `remediation_reaudit_2.md`를 따른다.
