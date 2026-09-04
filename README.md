# Crawlmaster (크롤마스터)

[한국어](#한국어) | [English](#english) | [日本語](#日本語) | [繁體中文](#繁體中文) | [简体中文](#简体中文)

> Pre-release vertical-slice demo candidate. This repository is not yet an Early Access or 1.0 commercial release.

## 한국어

Crawlmaster는 C++20과 SFML 2.6.1로 만든 1인칭 20×20 와이어프레임 던전 RPG 데모 후보입니다.

현재 구현:

- New Game/Continue 분리, 모집 후보 preview/reroll/confirm
- 마을의 길드·상점·교회·3개 퀘스트
- seed 기반 DFS 미로, Door 랜드마크, 최종 BossGate와 Dragon Whelp, Victory/Game Over 화면
- 실제 파티 HP·사망·독·마비·Bless를 표시하는 HUD
- 명시적 전투 아이템/아군 대상 선택, d20 자연 1/20, Bless, 2d6, 장비 제한, tier 보상
- 19개 아이템 registry와 상점·시작 장비·드롭·퀘스트 보상 획득원
- OS별 사용자 데이터 경로의 schema v2 JSON, 원자 교체, 백업, 손상 파일 격리
- 한국어/영어/일본어/중국어 번체/간체 UI 리소스, text scale과 high contrast 설정
- Linux/hosted Windows Release-safe CTest 13개, 독립 process RNG replay와 5 locale×3 scale production raster 증거
- Linux TGZ 및 hosted MSVC Windows ZIP, artifact SPDX SBOM/Grype·OSV, native SLSA/SPDX attestation과 SHA-256 sidecar

저장은 Linux에서 `$XDG_DATA_HOME/crawlmaster` 또는 `$HOME/.local/share/crawlmaster`, Windows에서 `%APPDATA%/Crawlmaster`를 사용합니다. 활성 던전 좌표는 저장하지 않으며 마지막 마을 checkpoint와 campaign 완료 상태를 저장합니다. TPK는 정상 저장을 삭제하지 않습니다.

오디오는 현재 비목표입니다. Ubuntu 24.04 x86_64를 Linux 기준선으로 하며 X11/Xrandr/Xcursor/udev/FreeType/OpenGL 런타임이 필요합니다. Windows Server 2022 hosted MSVC build/test/package/5초 startup과 native SLSA/SPDX attestation은 [audit report 13](docs/audit/audit_report_13.md)에서 검증했지만, clean Windows 10/11의 VC++ runtime 전제와 실제 high-DPI/장시간 실기는 아직 `UNVERIFIED`입니다. macOS는 범위 밖이며 제품 legal/support 주체 승인은 Human Review gate입니다.

### 빌드와 검증

CMake 3.28 이상, C++20 컴파일러와 Linux의 X11/OpenGL 개발 패키지가 필요합니다.

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --parallel 2
ctest --test-dir build/debug --output-on-failure --no-tests=error

cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel 2
ctest --test-dir build/release --output-on-failure --no-tests=error
cpack --config build/release/CPackConfig.cmake
```

개발 빌드 실행: `./build/debug/Crawlmaster`

## English

Crawlmaster is a pre-release first-person 20×20 wireframe dungeon RPG demo candidate built with C++20 and SFML 2.6.1.

Implemented surfaces include explicit New/Continue flow; recruitment preview and confirmation; town services and three quests; a seeded maze with a landmark, boss gate, final Dragon Whelp encounter, Victory and Game Over states; a live party/status HUD; explicit combat item and ally targeting; deterministic combat rules shared by basic attacks and skills; checkpointed RNG replay; 19 reachable item definitions; schema-v2 atomic save/config; localized content/logs; production raster evidence; Linux/hosted Windows Release-safe CTest; and Linux TGZ/hosted MSVC Windows ZIP packages with SBOM, checksums, and native SLSA/SPDX attestations.

Saves use the platform user-data directory. Active dungeon coordinates are intentionally not resumed; the last town checkpoint, campaign completion, RNG seed and raw draw count are saved. TPK never deletes the valid save. Audio is out of scope. Ubuntu 24.04 x86_64 is the Linux baseline. Hosted Windows Server 2022 MSVC build/test/package/startup is verified, while clean Windows 10/11 runtime and real high-DPI/long-play evidence remain unverified; macOS is out of scope. Font provenance is recorded in [FONT_PROVENANCE.md](FONT_PROVENANCE.md); product legal/support approval remains a Human Review gate.

Use the commands in the Korean section or [BUILD_GUIDE.md](BUILD_GUIDE.md) to build, test, install, and package.

## 日本語

Crawlmaster は C++20 と SFML 2.6.1 で構築した、20×20 の一人称ワイヤーフレーム・ダンジョン RPG のプレリリース体験版候補です。

New Game/Continue、募集候補、街と3クエスト、seed 固定迷路、最終ボス、実パーティ HUD、共通戦闘規則、保存可能なRNG checkpoint、19アイテム、schema v2の原子的保存、5言語のコンテンツ/ログ、3段階の実レンダリング、Linuxおよびhosted MSVC Windows CTest/packageを実装しています。

ダンジョン座標の途中再開は行わず、最後の街 checkpoint と campaign 完了、RNG状態を保存します。Windows Server 2022 hosted検証とSLSA/SPDX attestationは完了しましたが、clean Windows 10/11、高DPI・長時間プレイと製品legal/support承認は未完了です。フォント技術証拠は [FONT_PROVENANCE.md](FONT_PROVENANCE.md) を参照してください。

## 繁體中文

Crawlmaster 是以 C++20 與 SFML 2.6.1 製作的 20×20 第一人稱線框地下城 RPG 預發行試玩候選版本。

目前包含共用戰鬥規則、可保存的RNG checkpoint、五語系內容/日誌與三種文字比例實際渲染、Linux及hosted MSVC Windows CTest/package，並保留既有完整單層遊戲路徑。

Windows Server 2022 hosted驗證與SLSA/SPDX attestation已完成；clean Windows 10/11、高DPI、長時間遊玩與產品legal/support核准仍為 `UNVERIFIED`。字型技術證據見 [FONT_PROVENANCE.md](FONT_PROVENANCE.md)。

## 简体中文

Crawlmaster 是使用 C++20 与 SFML 2.6.1 制作的 20×20 第一人称线框地下城 RPG 预发布试玩候选版本。

当前包含共用战斗规则、可保存的RNG checkpoint、五语言内容/日志与三种文字比例实际渲染、Linux及hosted MSVC Windows CTest/package，并保留既有完整单层游戏路径。

Windows Server 2022 hosted验证与SLSA/SPDX attestation已完成；clean Windows 10/11、高DPI、长时间游玩与产品legal/support批准仍为 `UNVERIFIED`。字体技术证据见 [FONT_PROVENANCE.md](FONT_PROVENANCE.md)。
