# Hosted release gate 보완 계획 — Run 33779295660

작성일: 2026-09-04 (Asia/Seoul)
입력 감사: `audit_report_11.md`
대상 source SHA: `a8096c32066de7cab49f6678b5da3610b1a4a11a`

## 목표

첫 hosted Ubuntu 실행에서 `RngReplayCheckpointContinue`가 DISPLAY 부재로 abort한 환경 차이를 수정하고, Ubuntu/Windows build·CTest·package·SBOM·scanner·attestation gate를 동일 source revision에서 다시 실행한다.

## 재현 증거

- Hosted run `33779295660`, Ubuntu job `100728721229`
- Ubuntu build 성공 후 CTest 12/13, `RngReplayCheckpointContinue`만 `Failed to open X11 display`
- 로컬 `env -u DISPLAY ctest -R RngReplayCheckpointContinue`: exit 8
- 로컬 `xvfb-run -a ctest -R RngReplayCheckpointContinue`: PASS

## 변경 범위

- Linux/Unix에서만 `RngReplayCheckpointContinue` executable을 `xvfb-run -a`로 실행한다.
- Windows에서는 기존처럼 executable을 직접 실행한다.
- production RNG, save schema와 runtime 동작은 변경하지 않는다.

## 검증 기준

1. DISPLAY 없는 로컬 Linux에서 해당 fixture chain이 PASS한다.
2. 전체 Release CTest 13/13이 PASS한다.
3. 변경 commit을 push한 뒤 Ubuntu/Windows hosted job이 모두 PASS한다.
4. 두 artifact와 build/SBOM attestations를 source SHA에 결속해 검증한다.

