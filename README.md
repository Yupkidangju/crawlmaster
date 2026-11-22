# Crawlmaster (크롤마스터)

[한국어](#한국어) | [English](#english) | [日本語](#日本語) | [繁體中文](#繁體中文) | [简体中文](#简体中文)

> **Crawlmaster v1.0.0** — Dark Fantasy 1인칭 3D 전술 던전 크롤러 RPG (C++20 / SFML 2.6.1 / OpenGL GLSL / miniaudio 0.11.26)

## 한국어

Crawlmaster는 C++20과 SFML 2.6.1, OpenGL/GLSL 셰이더, `miniaudio` 오디오 엔진으로 구축한 **다크 판타지 회화풍 1인칭 3D 전술 던전 크롤러 RPG**입니다.

### 주요 특징 (v1.0.0)

- **1인칭 텍스처 3D 던전 렌더러**: 5개 바이옴별 알베도·Sobel 노멀맵 텍스처, 동적 횃불 점멸 조명, 바이옴 안개, 블룸·비네트·필름 톤매핑 포스트프로세싱, 60fps 부드러운 카메라 보간·헤드밥·부유 입자를 지원하며 GPU 셰이더 미지원 환경에서는 자동으로 **GL 1.1 소프트웨어 텍스처 3D 폴백**으로 전환됩니다.
- **5층 영속 캠페인과 5개 바이옴**: 1F 수몰된 납골당(`CATACOMBS`), 2F 익사자의 수로(`CISTERN`), 3F 저주받은 납골회랑(`OSSUARY`), 4F 흑요석 대장간(`FOUNDRY`), 5F 용의 성소(`SANCTUM`)로 이어지는 5×20×20 영속 던전과 보물상자, 도적 탐지/해제 함정, 단축 레버와 잠긴 문, 비밀 벽, 성소 모닥불, 로어 비석, 도달 층 빠른 이동을 제공합니다.
- **깊이 있는 전술 턴제 전투**: 전열/후열 진형과 무기 사거리, 9속성 상성(약점/저항), 스태거/브레이크 게이지와 행동 중단, 적 인텐트 예고(Telegraphing), 방어(Guard), 2페이즈 최종 보스(`태고의 비룡`), 타격 정지·화면 흔들림·파티클 VFX·플로팅 데미지를 구현했습니다.
- **성장·장비·대장간·도감·퀘스트**: Lv 1~10 성장, 4직업 × 2종(총 8종) Lv3+ 전직(Specialization), 4노드 특성 트리, 24종 스킬, 5등급 희귀도·접사·`+0..+5` 대장간 강화 및 접사 재연마가 가능한 43종 아이템, 24종 몬스터 도감(Bestiary Codex), 8종 정식 퀘스트를 수록했습니다.
- **자체 완결 오디오 엔진**: `miniaudio` 0.11.26(헤더 단독, OS 오디오 개발 헤더 불필요) 기반 5곡 BGM, 5종 바이옴 환경음 루프, 24종 SFX와 마스터/BGM/SFX 볼륨·음소거 설정을 제공하며 헤드리스/CI 환경에서는 자동으로 Null 백엔드로 동작합니다.
- **16:9 와이드스크린 하이브리드 UI & 5개국어 접근성**: 1280×720 논리 캔버스(창 크기 조절, 1600×900/1920×1080, `F11` 전체화면), 마우스 클릭·호버·우클릭 복귀 + 키보드(`WASD`/방향키/단축키) 동시 지원, 5개국어(`ko`, `en`, `ja`, `zh_tw`, `zh_cn`), 글자 배율(75~200%), 고대비 모드를 지원합니다.
- **안전한 영속 저장 (Save v5 / Config v3)**: OS별 사용자 데이터 경로(`$XDG_DATA_HOME/crawlmaster` 또는 `$HOME/.local/share/crawlmaster`, Windows `%APPDATA%/Crawlmaster`)에 원자적 쓰기·백업 복구·`recoveryPending` 쓰기 차단·TPK 비파괴 복원·결정론적 RNG 시드/드로우 리플레이를 유지하며, 구버전 `v1~v4` 세이브는 자동 안전 격리(`quarantine`) 후 안내합니다.

### 빌드와 검증

CMake 3.28 이상, C++20 컴파일러, Linux X11/OpenGL/FreeType 개발 패키지가 필요합니다.

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --parallel 4
ctest --test-dir build/debug --output-on-failure --no-tests=error

cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel 4
ctest --test-dir build/release --output-on-failure --no-tests=error
cpack --config build/release/CPackConfig.cmake
```

개발 빌드 실행: `./build/debug/Crawlmaster` (자세한 내용은 [BUILD_GUIDE.md](BUILD_GUIDE.md) 참고)

---

## English

Crawlmaster v1.0.0 is a **dark-fantasy painterly first-person 3D tactical dungeon crawler RPG** built with C++20, SFML 2.6.1, OpenGL/GLSL shaders, and `miniaudio` 0.11.26.

- **Textured 3D First-Person Dungeon Engine**: Per-biome albedo and Sobel normal maps, flickering dynamic torch lighting, biome fog, bloom/vignette/film tone-mapping post-processing, 60fps smooth camera interpolation, and automatic **GL 1.1 software-textured 3D fallback** on GPU-less environments.
- **5-Floor Persistent Campaign & 5 Biomes**: Sunken Catacombs, Drowned Cistern, Cursed Ossuary, Obsidian Foundry, and Wyrm's Sanctum with chests, traps (Rogue auto-spot & disarm), shortcut levers + locked gates, secret walls, sanctuary campfires, lore plaques, and Town fast-travel.
- **Tactical Turn-Based Combat & Deep Progression**: Front/Back rows, 9 elements, Stagger/Break gauge with turn interrupts, Enemy Intent Telegraphing, Guard, 2-phase Elder Wyrm boss, Lv 1–10 leveling, 8 Lv3+ Specializations, 4-node Talent Trees, 24 skills, 43 items with 5 rarities/affixes/`+0..+5` Blacksmith Forge & Reforge, 24-monster Bestiary Codex, and 8 canonical quests.
- **Self-Contained Audio (`miniaudio` 0.11.26)**: 5 BGM tracks, 5 biome ambient loops, 24 SFX, volume/mute controls, and automatic headless Null backend.
- **16:9 Mouse + Keyboard UI & 5 Languages**: Resizable 1280×720 virtual canvas (`F11` fullscreen), full mouse + keyboard controls, `ko`/`en`/`ja`/`zh_tw`/`zh_cn` localization, 75–200% text scale, high-contrast mode, Save Schema v5 (safe quarantine of legacy v1–v4 saves), and Config Schema v3.

See the build commands above or [BUILD_GUIDE.md](BUILD_GUIDE.md) for Linux & Windows packaging, CTest (15 suites), and SPDX SBOM verification.

---

## 日本語

Crawlmaster v1.0.0 は、C++20・SFML 2.6.1・OpenGL/GLSL・`miniaudio` 0.11.26 で構築した **ダークファンタジー絵画調の一人称 3D タクティカル・ダンジョン RPG** です。

- **3Dテクスチャ一人称ダンジョン**: 5つのバイオーム別アルベド/法線マップ、揺らめく松明ライト、フォグ、ブルーム/ビネット後処理、滑らかなカメラ補間、および GL 1.1 ソフトウェア 3D フォールバックを搭載。
- **5階層永続迷宮とタクティカル戦闘**: 前衛/後衛陣形、9属性弱点、スタッガー/ブレイク、敵の行動予告(Intent)、ガード、2段階構成の最終ボス「太古の飛竜」、Lv1〜10・8種の上級職・4ノード特性・鍛冶屋強化(+5)/再研磨(43種装備)・24種モンスター図鑑・8種クエストを実装。
- **オーディオ＆5言語16:9 UI**: 5曲BGM・5種環境音・24種SFX、1280×720 ワイド画面(`F11`全画面)、マウス+キーボード操作、5言語・文字倍率(75〜200%)・ハイコントラスト、Save v5(旧v1〜v4は安全隔離)・Config v3 に対応。

---

## 繁體中文

Crawlmaster v1.0.0 是以 C++20、SFML 2.6.1、OpenGL/GLSL 與 `miniaudio` 0.11.26 打造的 **暗黑奇幻油畫風第一人稱 3D 戰術地城探索 RPG**。

- **3D 材質第一人稱地城引擎**：5 大生態區域專屬貼圖與法線貼圖、動態火把光源、霧氣、泛光/暗角後製、流暢鏡頭補間，並內建 GL 1.1 軟體 3D 材質後備路徑。
- **5 層持久地城與戰術戰鬥**：前後排陣型、9 大屬性弱點、踉蹌/破防量表、敵人意圖預告、防禦、兩階段最終首領「太古飛龍」、Lv 1–10 成長、8 種轉職專精、4 節點天賦、鐵匠鋪 `+0..+5` 強化與重鑄、24 種怪物圖鑑與 8 個正式任務。
- **完整音效與 16:9 雙操作介面**：5 首 BGM、5 種環境音、24 種音效，16:9 寬螢幕（`F11` 全螢幕）、滑鼠+鍵盤混合操作、五語系、文字縮放（75–200%）、高對比與 Save v5（舊版 v1–v4 安全隔離）。

---

## 简体中文

Crawlmaster v1.0.0 是使用 C++20、SFML 2.6.1、OpenGL/GLSL 与 `miniaudio` 0.11.26 打造的 **暗黑奇幻油画风第一人称 3D 战术地牢探索 RPG**。

- **3D 材质第一人称地牢引擎**：5 大生态区域专属贴图与法线贴图、动态火把光源、雾气、泛光/暗角后处理、流畅镜头补间，并内置 GL 1.1 软件 3D 材质后备路径。
- **5 层持久地牢与战术战斗**：前后排阵型、9 大属性弱点、失衡/破防量表、敌人意图预告、防御、两阶段最终首领「太古飞龙」、Lv 1–10 成长、8 种转职专精、4 节点天赋、铁匠铺 `+0..+5` 强化与重铸、24 种怪物图鉴与 8 个正式任务。
- **完整音效与 16:9 双操作界面**：5 首 BGM、5 种环境音、24 种音效，16:9 宽屏（`F11` 全屏）、鼠标+键盘混合操作、五语系、文字缩放（75–200%）、高对比度与 Save v5（旧版 v1–v4 安全隔离）。
