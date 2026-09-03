# Hosted release gate 보완 계획 — Runs 33779295660, 33780571383, 33781740992

작성일: 2026-09-04 (Asia/Seoul)
입력 감사: `audit_report_11.md`
대상 source SHA: `a8096c32066de7cab49f6678b5da3610b1a4a11a`에서 시작하며, 각 보완 commit의 원격 SHA를 hosted run에 결속한다.

## 목표

첫 hosted Ubuntu 실행에서 `RngReplayCheckpointContinue`가 DISPLAY 부재로 abort한 환경 차이를 수정하고, Ubuntu/Windows build·CTest·package·SBOM·scanner·attestation gate를 동일 source revision에서 다시 실행한다.

## 재현 증거

- Hosted run `33779295660`, Ubuntu job `100728721229`
- Ubuntu build 성공 후 CTest 12/13, `RngReplayCheckpointContinue`만 `Failed to open X11 display`
- 로컬 `env -u DISPLAY ctest -R RngReplayCheckpointContinue`: exit 8
- 로컬 `xvfb-run -a ctest -R RngReplayCheckpointContinue`: PASS
- 같은 run의 Windows job `100728721472`은 Ninja가 MSYS2 MinGW GNU 14.2를 선택해 요청한 MSVC gate가 아니었고, bundled FreeType link에서 `_setjmp` unresolved로 실패했다.
- Hosted run `33780571383`, Windows job `100732929651`은 Visual Studio 17 2022 x64/MSVC 구성을 통과했지만 `/WX`에서 다음 경고가 오류로 승격되어 build 단계에서 중단됐다.
  - `Persistence.cpp`: `_wopen`, `getenv`에 대한 C4996
  - `ResourceLocator.cpp`: `getenv`에 대한 C4996
  - `test_harness.cpp`: `size_t`를 `int`로 암시 변환한 C4267 두 건
- Hosted run `33781740992`, Windows job `100736819048`은 위 경고를 모두 통과한 뒤 `CharacterInfoState.cpp`의 inventory 크기 두 곳에서 추가 C4267이 드러나 build 후반에 중단됐다.

## 변경 범위

- Linux/Unix에서만 `RngReplayCheckpointContinue` executable을 `xvfb-run -a`로 실행한다.
- Windows에서는 기존처럼 executable을 직접 실행한다.
- Windows hosted configure는 Visual Studio 17 2022 x64 generator를 사용하고 build·CTest·install·CPack에 Release config를 명시한다.
- production RNG, save schema와 runtime 동작은 변경하지 않는다.
- Windows 파일 열기는 MSVC secure CRT의 `_wsopen_s`로 전환하고 환경 변수 읽기는 `_dupenv_s` 기반 값 복사로 전환한다.
- 테스트의 컨테이너 크기는 `std::size_t`로 유지해 손실 가능 변환을 제거한다.
- CharacterInfoState의 기존 `int` UI index 계약과 비교하는 inventory 크기는 명시적 경계 변환으로 의도를 고정한다.

## 검증 기준

1. DISPLAY 없는 로컬 Linux에서 해당 fixture chain이 PASS한다.
2. 전체 Release CTest 13/13이 PASS한다.
3. 변경 commit을 push한 뒤 Ubuntu/Windows hosted job이 모두 PASS한다.
4. 두 artifact와 build/SBOM attestations를 source SHA에 결속해 검증한다.
5. Windows hosted build가 `/W4 /WX`를 유지한 채 성공하고 CTest·install·CPack·arbitrary-CWD startup까지 진행된다.
