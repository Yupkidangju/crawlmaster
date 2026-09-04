# 구현 계획: RA15 recovery remediation

## 개요

RA15-F001~F003와 재개방된 FIN-F017/FIN-F031을 recovery write fence, backup candidate quarantine, active 문서 계보 순으로 수정한다.

## 구현 순서

1. `recoveryPending`과 write eligibility 계약을 문서에 확정한다.
2. TPK/Title load failure가 pending을 세우고 일반/shutdown save를 차단하도록 한다.
3. ShutdownState의 pending Enter를 load retry로 분기한다.
4. primary absent+corrupt backup을 candidate 단위로 quarantine한다.
5. failure matrix와 독립 process probe, Debug/Release/package를 재검증한다.

## 위험과 완화

- failed load의 in-memory 보존과 write 권한을 혼동하지 않는다.
- quarantine 실패는 무시하지 않고 typed result로 전달한다.
- 원 audit report 15와 Turn 3 sealed manifest를 변경하지 않는다.
