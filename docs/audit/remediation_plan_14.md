# Turn 3 감사 remediation 계획

작성일: 2026-09-04 (Asia/Seoul)
원본 권위: `docs/multi_audit/3/final_audit_report_3.md`
대상: FIN-F001~FIN-F035

## 1. 현재 트리 대조 판정

- **확인됨:** FIN-F001~F015, F017~F027, F030~F035. 보고서의 코드 경로와 현재 구현이 같은 실패 가능성을 가진다.
- **계약 선행 후 수정:** FIN-F004, F016, F023, F024, F028, F035.
- **환경 gate:** FIN-F018의 v0.10 hosted Windows/SLSA는 로컬에서 생성할 수 없으므로 기존 완료 주장을 철회하고 `UNVERIFIED`로 유지한다.
- **coverage finding:** FIN-F019는 개별 수정과 함께 production-linked E2E matrix를 추가해 닫는다.

## 2. 동결한 계약

1. TPK와 rollback은 별도 town 파일이 아니라 **가장 최근 성공한 전체 세션 checkpoint**의 Party·Quest·World·RNG를 함께 복원한다.
2. 자동 이동도 보스/최종 관문 칸에 실제 진입하면 전투를 시작한다. 아이템·NPC·계단은 `E` 없이는 활성화하지 않는다.
3. 동일 domain/RNG 상태의 save는 정렬된 set과 canonical JSON을 사용해 exact byte가 동일해야 한다.
4. 효과가 없는 소모품은 전투 안팎 모두 소비하거나 RNG를 진행시키지 않는다.
5. 제품 퀘스트는 canonical registry 전용이며 custom Quest는 수락하지 않는다.
6. `ItemFactory::getShopCatalog()`는 실제 구매 가능한 기본 8종의 단일 진실원이다.
7. 14px은 보조문이 아니라 현재 고정 1024x768 TUI의 일반 본문 최소값이다. 제목/중요 상태는 더 큰 role을 사용한다.

## 3. 우선순위

- **P0 데이터 무결성:** FIN-F001~F008, F020~F023, F025~F027, F030~F033
- **P1 캐릭터/전투/종료:** FIN-F009~F015, F024
- **P2 문서/API/UI/E2E:** FIN-F016~F019, F028~F029, F034~F035

## 4. 완료 기준

- 각 finding에 실패 전 재현 또는 negative fixture가 있고 수정 후 통과한다.
- Debug/Release 전체 CTest, 5 locale JSON parity와 영향 UI raster가 통과한다.
- `docs/audit/audit_report_14.md`가 FIN-F001~F035의 독립 재감사 상태와 로컬/외부 gate를 기록한다.
- 원본 Turn 3 보고서와 manifest는 수정하지 않는다.
