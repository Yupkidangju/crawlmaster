# ASSET_PROVENANCE.md (Crawlmaster v1.0.0 아트 · 셰이더 · 오디오 자산 출처)

작성일: 2026-11-22  
범위: Crawlmaster v1.0.0 저장소 내 완결형(self-contained) 그래픽·셰이더·오디오 자산

## 1. 시각 자산 (`assets/textures/`)

* **제작 방식:** 프로젝트 전용 다크 판타지 회화풍 원화(14종: 타이틀 키아트, 마을 광장 배경, 전사 초상화, 납골당/수로/대장간 PBR 석재 텍스처 5종, 몬스터 원화 6종)를 기반으로 결정론적 Python Pillow 파이프라인(`scripts/build_assets.py`)에서 심리스 타일링, Sobel 탄젠트 공간 노멀맵(`*_n.png`), 5개 바이옴 변형, 4종 영웅 초상화, 24종 몬스터/보스 투명 컷아웃, 9종 3D 프롭, 9-슬라이스 UI 패널 및 파티클 스프라이트 총 81개 PNG 자산을 생성·내장했다.
* **외부 유료/제3자 에셋:** 없음. 모든 텍스처·스프라이트는 프로젝트 소유이며 데이터 드리븐 경로(`assets/textures/`)로 언제든 개별 교체가 가능하다.

## 2. GLSL 셰이더 (`assets/shaders/`)

* `dungeon_lit.vert`, `dungeon_lit.frag`, `post_bloom.frag`: 본 프로젝트에서 직접 작성한 OpenGL GLSL 120+ 호환 셰이더(탄젠트 노멀맵 조명, 동적 횃불, 지수 안개, 소프트 블룸, 필름 톤매핑, 비네트, 피격 플래시).

## 3. 오디오 자산 (`assets/audio/`)

* **제작 방식:** 표준 라이브러리 기반 합성 스크립트(`scripts/generate_audio.py`)로 결정론적으로 생성한 16-bit 22.05kHz PCM WAV 34종(BGM 5종, 바이옴 환경음 5종, 전투/탐험/UI 효과음 24종).
* **재생 엔진:** `miniaudio` 0.11.26 (`877ea4a7c2601c7b0f2a44e83dfac42e6470fea6`, Public Domain / MIT-0).

## 4. 번들 폰트 (`assets/fonts/`)

* `NotoSansCJK-Regular.ttc` (SIL OFL 1.1), `neodgm.ttf` (SIL OFL 1.1), `UbuntuMono[wght].ttf` (Ubuntu Font Licence 1.0) — 상세 해시와 업스트림 출처는 `FONT_PROVENANCE.md`를 따른다.
