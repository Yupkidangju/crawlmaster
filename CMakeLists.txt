# CMakeLists.txt
# [v0.9.4] TestHarness 저장소 격리 및 Town 허브 번역 키 회귀 검증을 반영한 패치 버전 갱신
cmake_minimum_required(VERSION 3.28)
project(Crawlmaster VERSION 0.9.4 LANGUAGES CXX)

enable_testing()
include(GNUInstallDirs)

# C++20 표준 강제
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 컴파일러 경고 옵션
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

# 헤더 포함 디렉터리
include_directories(include)

# 의존성 검색 및 FetchContent 설정
include(FetchContent)

# 1. nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03 # v3.11.3
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(json)

# 2. SFML 2.6.1을 immutable commit과 정적 링크로 고정한다.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 69ea0cd863aed1d4092b970b676924a716ff718b # 2.6.1
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(SFML)

# production runtime 소스 목록 정의
set(RUNTIME_SOURCES
    src/core/Game.cpp
    src/core/GameStateManager.cpp
    src/core/Persistence.cpp
    src/core/ResourceLocator.cpp
    src/core/SessionRng.cpp
    src/core/LocalizationManager.cpp
    src/controller/TitleState.cpp
    src/controller/GameOverState.cpp
    src/controller/VictoryState.cpp
    src/controller/TownState.cpp
    src/controller/DungeonState.cpp
    src/controller/SettingsState.cpp
    src/controller/CharacterInfoState.cpp
    src/model/DungeonMap.cpp
    src/view/DungeonRenderer.cpp
    src/view/PartyHudSnapshot.cpp
    src/model/Character.cpp
    src/model/CombatActionRules.cpp
    src/model/CombatRules.cpp
    src/model/Party.cpp
    src/model/ItemFactory.cpp
    src/controller/CombatState.cpp
    src/controller/CombatStateActions.cpp
    src/model/MonsterFactory.cpp
    src/model/Quest.cpp
    src/model/RecruitmentDraft.cpp
    src/model/Skill.cpp
    src/model/SkillFactory.cpp
)

# 앱과 controller transcript test가 같은 production 구현을 링크한다.
add_library(CrawlmasterRuntime STATIC ${RUNTIME_SOURCES})
target_link_libraries(CrawlmasterRuntime PUBLIC
    sfml-graphics sfml-window sfml-system nlohmann_json::nlohmann_json
)
add_executable(Crawlmaster src/main.cpp)
target_link_libraries(Crawlmaster PRIVATE CrawlmasterRuntime)

# [v0.9.3] 자산은 소스 변경만 있어도 매 빌드마다 최신 상태로 동기화한다.
add_custom_target(Assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_CURRENT_BINARY_DIR}/assets
    COMMENT "assets 폴더를 빌드 출력 경로로 복사하는 중..."
)
add_dependencies(Crawlmaster Assets)
add_test(NAME ResourceVerification COMMAND Crawlmaster --verify-resources)

# --- 테스트 하네스 타겟 선언 ---
set(TEST_SOURCES
    src/test_harness.cpp
    src/core/GameStateManager.cpp
    src/core/Persistence.cpp
    src/core/ResourceLocator.cpp
    src/core/SessionRng.cpp
    src/core/LocalizationManager.cpp
    src/model/DungeonMap.cpp
    src/model/Character.cpp
    src/model/CombatActionRules.cpp
    src/model/CombatRules.cpp
    src/model/Party.cpp
    src/model/ItemFactory.cpp
    src/model/MonsterFactory.cpp
    src/model/Quest.cpp
    src/model/RecruitmentDraft.cpp
    src/model/Skill.cpp
    src/model/SkillFactory.cpp
)

add_executable(TestHarness ${TEST_SOURCES})
target_link_libraries(TestHarness PRIVATE sfml-graphics sfml-window sfml-system nlohmann_json::nlohmann_json)
add_test(NAME TestHarness COMMAND TestHarness --run-all)

add_executable(HudContractTests
    tests/test_hud_contracts.cpp
    src/view/PartyHudSnapshot.cpp
    src/core/Persistence.cpp
    src/core/ResourceLocator.cpp
    src/core/SessionRng.cpp
    src/core/LocalizationManager.cpp
    src/model/Character.cpp
    src/model/CombatRules.cpp
    src/model/Party.cpp
    src/model/ItemFactory.cpp
    src/model/Quest.cpp
    src/model/Skill.cpp
    src/model/SkillFactory.cpp
)
target_link_libraries(HudContractTests PRIVATE sfml-system nlohmann_json::nlohmann_json)
add_test(NAME HudContractTests COMMAND HudContractTests)

add_executable(CombatContractTests
    tests/test_combat_contracts.cpp
    src/core/Persistence.cpp
    src/core/ResourceLocator.cpp
    src/core/SessionRng.cpp
    src/core/LocalizationManager.cpp
    src/model/Character.cpp
    src/model/CombatRules.cpp
    src/model/ItemFactory.cpp
    src/model/MonsterFactory.cpp
    src/model/Skill.cpp
    src/model/SkillFactory.cpp
)
target_link_libraries(CombatContractTests PRIVATE sfml-system nlohmann_json::nlohmann_json)
add_test(NAME CombatContractTests COMMAND CombatContractTests)

add_executable(ContentContractTests
    tests/test_content_contracts.cpp
    src/core/Persistence.cpp
    src/core/ResourceLocator.cpp
    src/core/SessionRng.cpp
    src/core/LocalizationManager.cpp
    src/model/Character.cpp
    src/model/CombatRules.cpp
    src/model/DungeonMap.cpp
    src/model/ItemFactory.cpp
    src/model/MonsterFactory.cpp
    src/model/Party.cpp
    src/model/Quest.cpp
    src/model/Skill.cpp
    src/model/SkillFactory.cpp
)
target_link_libraries(ContentContractTests PRIVATE sfml-system nlohmann_json::nlohmann_json)
add_test(NAME ContentContractTests COMMAND ContentContractTests)

add_executable(AgencyContractTests
    tests/test_agency_contracts.cpp
    src/core/Persistence.cpp
    src/core/ResourceLocator.cpp
    src/core/SessionRng.cpp
    src/core/LocalizationManager.cpp
    src/model/Character.cpp
    src/model/CombatActionRules.cpp
    src/model/CombatRules.cpp
    src/model/ItemFactory.cpp
    src/model/Party.cpp
    src/model/Quest.cpp
    src/model/RecruitmentDraft.cpp
    src/model/Skill.cpp
    src/model/SkillFactory.cpp
)
target_link_libraries(AgencyContractTests PRIVATE sfml-system nlohmann_json::nlohmann_json)
add_test(NAME AgencyContractTests COMMAND AgencyContractTests)

add_executable(LocalizationContractTests
    tests/test_localization_contracts.cpp
    src/core/Persistence.cpp
    src/core/ResourceLocator.cpp
    src/core/SessionRng.cpp
    src/core/LocalizationManager.cpp
    src/model/Character.cpp
    src/model/CombatRules.cpp
    src/model/ItemFactory.cpp
    src/model/MonsterFactory.cpp
    src/model/Quest.cpp
    src/model/Skill.cpp
    src/model/SkillFactory.cpp
)
target_link_libraries(LocalizationContractTests PRIVATE
    sfml-system nlohmann_json::nlohmann_json
)
target_compile_definitions(LocalizationContractTests PRIVATE
    CRAWLMASTER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
add_dependencies(LocalizationContractTests Assets)
add_test(NAME LocalizationContractTests COMMAND LocalizationContractTests)

add_executable(FontRasterTests
    tests/test_font_raster.cpp
    src/core/Persistence.cpp
    src/core/ResourceLocator.cpp
    src/core/SessionRng.cpp
    src/core/LocalizationManager.cpp
    src/model/Character.cpp
    src/model/CombatRules.cpp
    src/model/ItemFactory.cpp
    src/model/MonsterFactory.cpp
    src/model/Quest.cpp
    src/model/Skill.cpp
    src/model/SkillFactory.cpp
)
target_link_libraries(FontRasterTests PRIVATE
    sfml-graphics sfml-window sfml-system nlohmann_json::nlohmann_json
)
add_dependencies(FontRasterTests Assets)
if(UNIX AND NOT APPLE AND NOT XVFB_RUN_EXECUTABLE)
    find_program(XVFB_RUN_EXECUTABLE xvfb-run)
endif()
if(XVFB_RUN_EXECUTABLE)
    add_test(NAME FontRasterTests COMMAND ${XVFB_RUN_EXECUTABLE} -a
        $<TARGET_FILE:FontRasterTests> "${CMAKE_CURRENT_BINARY_DIR}/font-raster-evidence")
else()
    add_test(NAME FontRasterTests COMMAND FontRasterTests "${CMAKE_CURRENT_BINARY_DIR}/font-raster-evidence")
endif()

add_executable(ControllerContractTests tests/test_controller_contracts.cpp)
target_link_libraries(ControllerContractTests PRIVATE CrawlmasterRuntime)
add_dependencies(ControllerContractTests Assets)
if(UNIX AND NOT APPLE)
    find_program(XVFB_RUN_EXECUTABLE xvfb-run)
endif()

add_executable(RngProcessReplayTests tests/test_rng_process_replay.cpp)
target_link_libraries(RngProcessReplayTests PRIVATE CrawlmasterRuntime)
add_dependencies(RngProcessReplayTests Assets)
set(RNG_REPLAY_DIR "${CMAKE_CURRENT_BINARY_DIR}/rng-process-replay")
add_test(NAME RngReplayWrite COMMAND ${CMAKE_COMMAND} -E make_directory "${RNG_REPLAY_DIR}")
set_tests_properties(RngReplayWrite PROPERTIES FIXTURES_SETUP RngReplayFixture)
add_test(NAME RngReplayCheckpointWrite COMMAND RngProcessReplayTests write
    "${RNG_REPLAY_DIR}/save.json" "${RNG_REPLAY_DIR}/expected.txt" "${RNG_REPLAY_DIR}/config.json")
set_tests_properties(RngReplayCheckpointWrite PROPERTIES
    FIXTURES_REQUIRED RngReplayFixture FIXTURES_SETUP RngReplayCheckpoint)
add_test(NAME RngReplayCheckpointContinue COMMAND RngProcessReplayTests continue
    "${RNG_REPLAY_DIR}/save.json" "${RNG_REPLAY_DIR}/expected.txt" "${RNG_REPLAY_DIR}/config.json")
set_tests_properties(RngReplayCheckpointContinue PROPERTIES FIXTURES_REQUIRED RngReplayCheckpoint)

add_executable(UiStateRasterTests tests/test_ui_state_raster.cpp)
target_link_libraries(UiStateRasterTests PRIVATE CrawlmasterRuntime)
add_dependencies(UiStateRasterTests Assets)
if(XVFB_RUN_EXECUTABLE)
    add_test(NAME UiStateRasterTests COMMAND ${XVFB_RUN_EXECUTABLE} -a
        $<TARGET_FILE:UiStateRasterTests> "${CMAKE_CURRENT_BINARY_DIR}/ui-state-raster-evidence")
endif()
if(XVFB_RUN_EXECUTABLE)
    add_test(NAME ControllerContractTests
        COMMAND ${XVFB_RUN_EXECUTABLE} -a $<TARGET_FILE:ControllerContractTests>)
else()
    add_test(NAME ControllerContractTests COMMAND ControllerContractTests)
endif()

# [v0.9.3] 빌드 디렉터리에서 TestHarness를 직접 실행해도 회귀 테스트가 최신 자산을 사용하도록 동기화한다.
add_dependencies(TestHarness Assets)

install(TARGETS Crawlmaster
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT Runtime
)
install(DIRECTORY assets/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/crawlmaster/assets
    COMPONENT Runtime
)
install(FILES README.md THIRD_PARTY_NOTICES.md FONT_PROVENANCE.md DEPENDENCY_MANIFEST.spdx.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/crawlmaster
    COMPONENT Runtime
)
install(DIRECTORY licenses/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/crawlmaster/licenses
    COMPONENT Runtime
)

set(CPACK_PACKAGE_NAME "Crawlmaster")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Crawlmaster pre-release vertical-slice demo candidate")
set(CRAWLMASTER_PACKAGE_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
if(NOT CRAWLMASTER_PACKAGE_ARCH)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CRAWLMASTER_PACKAGE_ARCH "x86_64")
    else()
        set(CRAWLMASTER_PACKAGE_ARCH "x86")
    endif()
endif()
set(CPACK_PACKAGE_FILE_NAME "Crawlmaster-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CRAWLMASTER_PACKAGE_ARCH}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/package")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()
include(CPack)
