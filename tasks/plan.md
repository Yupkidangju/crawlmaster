# 구현 계획: Crawlmaster v1.0.0 AAA 전면 리팩토링

## 목표

`Crawlmaster`를 C++20 코어 기반으로 전면 리팩토링하여, OpenGL 3.3+/GLSL 텍스처 3D 던전(+ GL 1.1 소프트웨어 텍스처 폴백), 다크 판타지 회화풍 아트/오디오, 16:9 반응형 마우스+키보드 MVC UI, 5층 5바이옴 장편 캠페인 및 전술 RPG 시스템을 구축하고 Linux/Windows CI/CD 릴리스(`v1.0.0`)까지 완주한다.

## 마일스톤 실행 순서

1. **Phase 0 (문서 기준 확정):** `spec.md`, `designs.md`, `DESIGN_DECISIONS.md`, `tasks/plan.md`, `tasks/todo.md` 갱신.
2. **Phase 1 (완결형 아트·PBR 텍스처·오디오 에셋 제작):**
   - `image-generator`로 다크 판타지 회화풍 5바이옴 벽/바닥/천장 텍스처, 몬스터/보스 일러스트, 4직업 초상화, 타이틀 키아트, 마을 배경 생성.
   - `scripts/build_assets.py`로 심리스 타일링, Sobel 노멀맵/러프니스맵, 몬스터 알파 컷아웃, UI 9-슬라이스 프레임·아이콘·파티클 스프라이트 생성.
   - `scripts/generate_audio.py`로 5개 BGM, 5개 바이옴 환경음, 24개 전투/탐험/UI SFX WAV 음원 합성 및 RMS 무음/클리핑 검증.
   - `ASSET_PROVENANCE.md` 기록.
3. **Phase 2 (코어 엔진 & 빌드/오디오/에셋 인프라):**
   - `CMakeLists.txt` v1.0.0 갱신 (`miniaudio` 0.11.26 immutable FetchContent, GLSL/에셋 복사, 신규 테스트 타겟).
   - `AudioManager`, `AssetManager`, Config Schema v3 (`LocalizationManager`), 반응형 16:9 `Game` + 화면 전환/토스트/마우스 좌표 매핑.
4. **Phase 3 (5층 캠페인 & 전술 RPG 모델 전면 확장):**
   - `Character` (Lv 1~10, `CombatRow`, 8종 전직, 특성 트리, 4슬롯 장비, 9종 상태이상/가드/도발).
   - `Item`/`Equipment`/`ItemFactory` (36+ 아이템, 5단계 희귀도, `+0~+5` 강화, 접사, `WeaponReach`, 9속성).
   - `Skill`/`SkillFactory` (24+ 직업/전직 스킬 및 주문).
   - `Monster`/`MonsterFactory`/`Bestiary` (20종 몬스터, 스태거/약점/저항, `EnemyIntent` AI, 2페이즈 최종 보스).
   - `CombatRules`/`CombatActionRules` (진형·사거리·속성·스태거/브레이크·강화/재연마 공식).
   - `DungeonMap`/`DungeonWorld`/`Quest` (5층 5바이옴, 상자·함정·비밀벽·레버/잠긴 문·성소·로어, 8종 퀘스트).
   - `Party` & `Persistence` (Save Schema v5 클린 브레이크, 구 v1~v4 자동 격리).
5. **Phase 4 (AAA 3D 렌더러, MVC View/UI, 컨트롤러, 5개국어):**
   - `DungeonRenderer` (GLSL 노멀맵·다중 동적 횃불·안개·블룸/비네트 + 자동 GL 1.1 텍스처 3D 폴백, 60fps 카메라 보간·헤드밥·파티클).
   - `UiTheme`, `UiSystem`, `GameViews` (9-슬라이스, 마우스 호버/클릭/툴팁, 애니메이션 바, 턴 타임라인, VFX, 플로팅 데미지).
   - 전 컨트롤러(`Title`, `Town`+대장간/도감, `CharacterCreation`, `Dungeon`, `Combat`, `CharacterInfo`+특성트리, `QuestJournal`, `Bestiary`, `Settings`, `Victory`, `GameOver`, `Shutdown`) 마우스+키보드 하이브리드 리팩토링.
   - 5개국어(`ko`, `en`, `ja`, `zh_tw`, `zh_cn`) 전 키·플레이스홀더 완전 동기화.
6. **Phase 5 (객관적 AAA 검증 게이트):**
   - Debug/Release `-Werror` 빌드, 전 CTest 통과, Xvfb 실스크린샷(5바이옴 3D·전투·마을·UI·5개국어) 시각 검수, MinGW Windows 크로스 컴파일 검사, CPack & SPDX SBOM 검증.
7. **Phase 6 (문서 동기화 & CI/CD `v1.0.0` 릴리스):**
   - `.github/workflows/ci.yml` 자동 GitHub Release 잡 추가, `README.md`/`BUILD_GUIDE.md`/`IMPLEMENTATION_SUMMARY.md`/`CHANGELOG.md`/`THIRD_PARTY_NOTICES.md`/`DEPENDENCY_MANIFEST.spdx.json`/`LESSONS_LEARNED.md` 갱신.
   - Git 커밋, `v1.0.0` 태그 생성 및 원격 푸시, GitHub Actions Linux/Windows 빌드·패키지·릴리스 성공 확인.
