# BUILD_GUIDE.md (빌드·테스트·패키지 가이드)

작성 기준: 2026-09-03, Crawlmaster 0.9.4 pre-release demo candidate

## 1. 요구사항

- CMake 3.28 이상
- C++20: GCC 11+, Clang 13+, MSVC 2022+
- Make 또는 Ninja
- 네트워크: 최초 FetchContent 구성 시 필요
- Linux: X11, Xrandr, Xcursor, Xi, udev, OpenGL, FreeType 개발 패키지

Debian/Ubuntu 예시:

```bash
sudo apt-get update
sudo apt-get install --yes ninja-build libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev libgl1-mesa-dev libfreetype6-dev xvfb
```

## 2. 재현 가능한 의존성

CMake는 시스템 SFML을 우선 사용하지 않는다.

- SFML 2.6.1: `69ea0cd863aed1d4092b970b676924a716ff718b`
- nlohmann/json 3.11.3: `9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03`

두 의존성은 immutable commit으로 FetchContent되고 SFML은 정적 링크된다. 출처와 라이선스 원문은 `THIRD_PARTY_NOTICES.md`, `FONT_PROVENANCE.md`, `licenses/`를 본다. `DEPENDENCY_MANIFEST.spdx.json`은 고정 source component 목록이며 `scripts/generate_release_sbom.py`가 설치 트리의 모든 파일과 platform runtime을 결합한 artifact SPDX를 만든다.

## 3. Debug/Release 검증

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --parallel 2
ctest --test-dir build/debug --output-on-failure --no-tests=error

cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel 2
ctest --test-dir build/release --output-on-failure --no-tests=error
```

또는 `CMakePresets.json`의 `debug`/`release` preset을 사용한다.

Linux CTest는 다음 13개를 실행한다.

1. `ResourceVerification`: 5 locale JSON과 3 font의 실제 로드
2. `TestHarness`: 기존 모델/i18n/저장 회귀와 failure injection
3. `HudContractTests`: Party HUD snapshot
4. `CombatContractTests`: RNG/전투/장비/보상
5. `ContentContractTests`: item/quest/landmark/boss reachability
6. `AgencyContractTests`: 모집/소모품/Cure Wounds 선택
7. `LocalizationContractTests`: 5 locale content/placeholder/source-literal 계약
8. `FontRasterTests`: 5 locale × 75/100/200% glyph/raster catalog
9~11. `RngReplay*`: 별도 process write → Continue checkpoint replay
12. `ControllerContractTests`: 실제 production controller/FSM event transcript
13. `UiStateRasterTests`: production State/substate/overlay PNG 증거 생성

## 4. 실행, 설치, 패키지

개발 실행:

```bash
./build/debug/Crawlmaster
```

설치 트리와 package:

```bash
cmake --install build/release --prefix build/install
cpack --config build/release/CPackConfig.cmake
```

Linux에서 Windows x86_64 컴파일·ZIP 구조를 보조 검증하려면 MinGW-w64 설치 후 다음을 사용한다. 이 검사는 Windows 실기 실행이나 hosted MSVC gate를 대체하지 않는다.

```bash
cmake -S . -B build/mingw-release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/mingw-release --parallel 2
cpack --config build/mingw-release/CPackConfig.cmake
```

설치 레이아웃:

```text
bin/Crawlmaster
share/crawlmaster/assets/{fonts,lang}
share/doc/crawlmaster/{README.md,THIRD_PARTY_NOTICES.md,FONT_PROVENANCE.md,DEPENDENCY_MANIFEST.spdx.json,licenses/}
```

런타임은 process CWD가 아니라 실행 파일 기준으로 `assets` 또는 `../share/crawlmaster/assets`를 찾는다. 저장/config는 OS별 per-user directory를 사용한다.

Linux package checksum/resource/arbitrary-CWD smoke:

```bash
archive=$(find build/release/package -maxdepth 1 -name 'Crawlmaster-*.tar.gz' -print -quit)
(cd "$(dirname "$archive")" && sha256sum -c "$(basename "$archive").sha256")

mkdir -p build/package-smoke
(cd build/package-smoke && cmake -E tar xzf "$PWD/../../${archive}")
package_root=$(find build/package-smoke -mindepth 1 -maxdepth 1 -type d -name 'Crawlmaster-*' -print -quit)
(cd /tmp && "$OLDPWD/${package_root}/bin/Crawlmaster" --verify-resources)
(cd /tmp && timeout 5s xvfb-run -a "$OLDPWD/${package_root}/bin/Crawlmaster")
# resource verify가 0이고 창이 유지돼 timeout 124이면 startup smoke 성공
```

## 5. Gate와 한계

- Ubuntu 24.04 x86_64를 Linux CI/runtime 기준선으로 고정한다. X11/Xrandr/Xcursor/udev/FreeType/OpenGL shared runtime이 필요하며 다른 배포판/Steam Runtime 호환은 별도 검증 대상이다.
- local artifact SBOM과 scan:

```bash
python3 scripts/generate_release_sbom.py \
  --install-root build/install --platform Linux \
  --output build/release/package/Crawlmaster-Linux-artifact.spdx.json
grype sbom:build/release/package/Crawlmaster-Linux-artifact.spdx.json --fail-on high
```

- Windows Server 2022 hosted MSVC build, CTest 13/13, ZIP/checksum/resource/startup, SBOM와 Grype high gate는 source `4f988483bf5cbcfdce4c79a6aabab4a67a7043f9`의 run `33786241695`에서 통과했다. package와 명령 증거는 `docs/audit/audit_report_12.md`에 기록했다.
- GitHub native provenance/SBOM attestation은 user-owned private repository plan에서 지원되지 않아 발급되지 않았다. public 전환, Enterprise Cloud 또는 승인된 Sigstore 대체 경로 전까지 release gate로 유지한다.
- macOS는 현재 범위 밖이다.
- 폰트의 byte identity/upstream/license/raster는 `FONT_PROVENANCE.md`로 닫았다. 제품 법률/지원 주체는 Human Review 전까지 유료 배포를 차단한다.
