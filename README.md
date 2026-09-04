# Crawlmaster (크롤마스터)

[한국어](#한국어) | [English](#english) | [日本語](#日本語) | [繁體中文](#繁體中文) | [简体中文](#简体中文)

> Pre-release vertical-slice demo candidate. This repository is not yet an Early Access or 1.0 commercial release.

## 한국어

Crawlmaster는 C++20과 SFML 2.6.1로 만든 1인칭 3층×20×20 와이어프레임 던전 RPG 데모 후보입니다.

현재 구현:

- New Game/Continue 분리, 이름·나이·성별·직업 선택과 무제한 능력치 리롤·가중 10포인트 배분
- 마을의 길드·상점·교회와 회수·고정 보스·NPC 탐색 퀘스트 및 `Q` 퀘스트 일지
- 새 게임에 귀속되는 3층 seed 기반 DFS 미로, 층간 계단, 발견 목표, 최종 BossGate와 Dragon Whelp
- 실제 파티 HP·사망·독·마비·Bless를 표시하는 HUD
- 명시적 전투 아이템/아군 대상 선택, d20 자연 1/20, Bless, 2d6, 장비 제한, tier 보상
- 19개 아이템 registry와 상점·시작 장비·드롭·퀘스트 보상 획득원
- OS별 사용자 데이터 경로의 save schema v4/config schema v2 JSON, 영속 월드 snapshot, 원자 교체, 백업, 손상 파일 격리
- 한국어/영어/일본어/중국어 번체/간체 UI 리소스, text scale과 high contrast 설정
- Linux Release-safe CTest, 독립 process RNG replay와 5 locale×3 scale production raster 증거
- Linux TGZ와 SHA-256 sidecar. current v0.10 hosted MSVC Windows ZIP 및 SLSA/SPDX attestation은 `UNVERIFIED`

저장은 Linux에서 `$XDG_DATA_HOME/crawlmaster` 또는 `$HOME/.local/share/crawlmaster`, Windows에서 `%APPDATA%/Crawlmaster`를 사용합니다. New Game이 만든 3층 지형·안개·목표 상태는 다음 New Game까지 유지됩니다. 활성 던전 좌표는 저장하지 않고 Continue는 1층 입구에서 시작합니다. TPK는 정상 저장을 삭제하지 않습니다.

오디오는 현재 비목표입니다. Ubuntu 24.04 x86_64를 Linux 기준선으로 합니다. [audit report 13](docs/audit/audit_report_13.md)의 hosted Windows/SLSA 증거는 0.9.4 source에만 해당하며 current v0.10 hosted MSVC·attestation, clean Windows 10/11과 실제 high-DPI/장시간 실기는 `UNVERIFIED`입니다. macOS는 범위 밖이며 제품 legal/support 주체 승인은 Human Review gate입니다.

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

Crawlmaster is a pre-release first-person three-floor 20×20 wireframe dungeon RPG demo candidate built with C++20 and SFML 2.6.1.

Implemented surfaces include explicit New/Continue flow; identity/class selection; town services; retrieval, fixed-boss and missing-NPC quests; a save-bound three-floor seeded world; deterministic combat rules; schema-v4 atomic saves; five localized catalogs; and Linux packaging evidence. Current v0.10 hosted Windows and attestation evidence is unverified.

Saves use the platform user-data directory. Generated floors, fog and objective state persist until New Game; active dungeon coordinates are intentionally not resumed and Continue starts at the first-floor entrance. TPK restores the latest successful full-session checkpoint. Audio is out of scope. Ubuntu 24.04 x86_64 is the Linux baseline. Current v0.10 hosted Windows, attestation, clean Windows 10/11, and real high-DPI/long-play evidence remain unverified; macOS is out of scope.

Use the commands in the Korean section or [BUILD_GUIDE.md](BUILD_GUIDE.md) to build, test, install, and package.

## 日本語

Crawlmaster は C++20 と SFML 2.6.1 で構築した、20×20 の一人称ワイヤーフレーム・ダンジョン RPG のプレリリース体験版候補です。

New Game/Continue、キャラクター作成、3階層の永続seed迷路、回収・固定ボス・NPC捜索クエスト、最終ボス、schema v4の原子的保存、5言語UIを実装しています。

ダンジョン座標の途中再開は行わず、最新の完全なsession checkpointを復元します。current v0.10のhosted Windows、SLSA/SPDX、高DPI・長時間プレイは `UNVERIFIED` です。

## 繁體中文

Crawlmaster 是以 C++20 與 SFML 2.6.1 製作的 20×20 第一人稱線框地下城 RPG 預發行試玩候選版本。

目前包含角色建立、三層永久seed迷宮、回收／固定首領／NPC搜尋任務、最終首領、schema v4原子儲存與五語系UI。

current v0.10 的 hosted Windows、SLSA/SPDX、clean Windows 10/11、高DPI與長時間遊玩仍為 `UNVERIFIED`。

## 简体中文

Crawlmaster 是使用 C++20 与 SFML 2.6.1 制作的 20×20 第一人称线框地下城 RPG 预发布试玩候选版本。

当前包含角色创建、三层持久seed迷宫、回收／固定首领／NPC搜寻任务、最终首领、schema v4原子保存与五语言UI。

current v0.10 的 hosted Windows、SLSA/SPDX、clean Windows 10/11、高DPI与长时间游玩仍为 `UNVERIFIED`。
