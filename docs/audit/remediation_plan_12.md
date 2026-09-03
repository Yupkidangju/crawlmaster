# Hosted release gate 보완 계획 — Runs 33779295660, 33780571383, 33781740992, 33782865463, 33783852397

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
- Hosted run `33782865463`, Windows job `100740514064`은 위 경고를 통과한 뒤 `CombatState.cpp`의 대상 순환 연산에서 `size_t` 결과를 `int` index에 대입한 C4267 한 건으로 중단됐다. 동일한 index 계약을 쓰는 좌·우 대상 순환과 turn 순환을 함께 정렬한다.
- Hosted run `33783852397`, Windows job `100743759955`은 MSVC `/W4 /WX` 전체 build를 성공했다. CTest는 multi-config의 `Release/` 실행 파일이 `build/ci/assets`를 찾지 못했고, hosted software OpenGL이 NPOT 1600×900 canvas를 2048×1024 texture로 올리며 1024px 한도를 초과해 5개가 실패했다. 저장 회귀는 읽기 전용 descriptor에 `_commit`을 호출한 Windows sync 구현에서 백업 회전이 실패했다.

## 변경 범위

- Linux/Unix에서만 `RngReplayCheckpointContinue` executable을 `xvfb-run -a`로 실행한다.
- Windows에서는 기존처럼 executable을 직접 실행한다.
- Windows hosted configure는 Visual Studio 17 2022 x64 generator를 사용하고 build·CTest·install·CPack에 Release config를 명시한다.
- production RNG, save schema와 runtime 동작은 변경하지 않는다.
- Windows 파일 열기는 MSVC secure CRT의 `_wsopen_s`로 전환하고 환경 변수 읽기는 `_dupenv_s` 기반 값 복사로 전환한다.
- 테스트의 컨테이너 크기는 `std::size_t`로 유지해 손실 가능 변환을 제거한다.
- CharacterInfoState의 기존 `int` UI index 계약과 비교하는 inventory 크기는 명시적 경계 변환으로 의도를 고정한다.
- CombatState의 기존 `int` UI/turn index 계약에 맞춰 vector 크기를 연산 전에 명시적으로 변환한다.
- ResourceLocator는 multi-config 실행 디렉터리의 형제 `assets`를 build-tree 후보로 허용하고, Windows backup sync descriptor는 실제 flush가 가능한 write access로 연다.
- Font raster canvas는 제품 기준 해상도이자 software OpenGL 한도 내인 1024×768로 사용하고 Windows에서도 production state raster CTest를 등록한다.
- package artifact 업로드를 attestation 앞에 두어, 계정 plan의 attestation 가용성과 무관하게 성공한 test/package/startup/SBOM 증거를 보존한다.

## 검증 기준

1. DISPLAY 없는 로컬 Linux에서 해당 fixture chain이 PASS한다.
2. 전체 Release CTest 13/13이 PASS한다.
3. 변경 commit을 push한 뒤 Ubuntu/Windows hosted job이 모두 PASS한다.
4. 두 artifact와 build/SBOM attestations를 source SHA에 결속해 검증한다.
5. Windows hosted build가 `/W4 /WX`를 유지한 채 성공하고 CTest·install·CPack·arbitrary-CWD startup까지 진행된다.
