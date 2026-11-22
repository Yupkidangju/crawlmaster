# BUILD_GUIDE.md (빌드·테스트·패키지·릴리스 가이드)

작성 기준: 2026-11-22, Crawlmaster v1.0.0 AAA Release

## 1. 요구사항

- CMake 3.28 이상
- C++20: GCC 11+, Clang 13+, MSVC 2022+
- Make 또는 Ninja
- 네트워크: 최초 FetchContent 구성 시 필요
- Linux: X11, Xrandr, Xcursor, Xi, udev, OpenGL, FreeType 개발 패키지 (오디오는 헤더 단독 `miniaudio`를 사용하므로 별도 ALSA/PulseAudio `-dev` 헤더가 필요 없다)

Debian/Ubuntu 예시:

```bash
sudo apt-get update
sudo apt-get install --yes ninja-build libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev libgl1-mesa-dev libfreetype6-dev xvfb
```

## 2. 재현 가능한 의존성 (Immutable Pins)

CMake는 시스템 SFML을 우선 사용하지 않고 다음 세 의존성을 immutable commit으로 가져온다.

- SFML 2.6.1: `69ea0cd863aed1d4092b970b676924a716ff718b` (정적 링크, Zlib)
- nlohmann/json 3.11.3: `9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03` (헤더 단독, MIT)
- miniaudio 0.11.26: `877ea4a7c2601c7b0f2a44e83dfac42e6470fea6` (헤더 단독, MIT-0 / Unlicense)

출처와 라이선스 원문은 `THIRD_PARTY_NOTICES.md`, `FONT_PROVENANCE.md`, `ASSET_PROVENANCE.md`, `licenses/`를 참고한다. `DEPENDENCY_MANIFEST.spdx.json`은 고정 source component 목록이며 `scripts/generate_release_sbom.py`가 설치 트리 전체 파일과 platform runtime을 결합한 artifact SPDX 2.3 SBOM을 생성한다.

## 3. Debug/Release 빌드 및 CTest 검증

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --parallel 4
ctest --test-dir build/debug --output-on-failure --no-tests=error

cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel 4
ctest --test-dir build/release --output-on-failure --no-tests=error
```

CTest는 총 15개 타겟을 실행한다(헤드리스 환경에서는 `CRAWLMASTER_HEADLESS=1`로 `miniaudio` Null 백엔드를 자동 활성화).

1. `ResourceVerification`: 5개국어 JSON, 3종 폰트, 텍스처/몬스터, 3종 GLSL 셰이더, 34종 비무음 WAV 검증
2. `TestHarness`: 5층 영속 월드·전투·장비/대장간·도감·Save v5 원자 저장·손상 격리·실패 주입 회귀
3. `HudContractTests`: Party HUD 스냅샷 계약
4. `CombatContractTests`: 전열/후열·속성·스태거/브레이크·인텐트·대장간 강화/재연마·2페이즈 보스 계약
5. `ContentContractTests`: 43종 아이템·24종 스킬·24종 몬스터·8종 퀘스트·5층 도달성 계약
6. `AgencyContractTests`: 무효 소모품 비소비·확정형 캐릭터 모집·잠긴 문/비밀 벽 개방 계약
7. `LocalizationContractTests`: 464키 × 5개국어 키·플레이스홀더 완전 일치 계약
8. `AudioContractTests`: `miniaudio` 초기화·볼륨/음소거·BGM/환경음/34 WAV 로드 계약
9. `FontRasterTests`: 5개국어 폰트 래스터 증거 생성
10. `ControllerContractTests`: 프로덕션 Controller/FSM 전이 계약
11. `UiStateRasterTests`: 15종 1280×720 프로덕션 UI·5바이옴 3D·보스 전투 PNG 스크린샷 증거 생성
12~14. `RngReplay*`: 독립 프로세스 v5 체크포인트 write → Continue RNG 시드/드로우 리플레이
15. `LegacySaveQuarantineTest`: 구버전 `v1~v4` 세이브 자동 안전 격리 및 `UnsupportedVersion` 반환 검증

## 4. 설치, 패키징 및 임의 CWD 스모크

```bash
cmake --install build/release --prefix build/install
cpack --config build/release/CPackConfig.cmake
python3 scripts/generate_release_sbom.py \
  --install-root build/install --platform Linux \
  --output build/release/package/Crawlmaster-Linux-artifact.spdx.json
```

설치 레이아웃:

```text
bin/Crawlmaster(.exe)
share/crawlmaster/assets/{audio,fonts,lang,shaders,textures}
share/doc/crawlmaster/{README.md,THIRD_PARTY_NOTICES.md,FONT_PROVENANCE.md,ASSET_PROVENANCE.md,DEPENDENCY_MANIFEST.spdx.json,licenses/}
```

Linux에서 Windows x86_64 교차 컴파일 검증:

```bash
cmake -S . -B build/mingw-release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/mingw-release --parallel 4
```

## 5. CI/CD 자동 릴리스 (`.github/workflows/ci.yml`)

- `push`, `pull_request`, `workflow_dispatch` 시 `ubuntu-24.04`(Ninja)와 `windows-2022`(Visual Studio 17 2022 x64 MSVC `/W4 /WX`)에서 빌드·CTest·CPack·임의 CWD 스모크·SPDX SBOM·Syft·Grype(`--fail-on high`)·Build/SBOM Attestation을 수행한다.
- `v*` 태그(`git tag -a v1.0.0 ... && git push origin v1.0.0`) 푸시 시 `publish-release` 잡이 양 플랫폼의 검증된 아카이브(`.tar.gz`, `.zip`), `.sha256`, SPDX/Syft/Grype 산출물을 GitHub Release로 자동 배포한다.
