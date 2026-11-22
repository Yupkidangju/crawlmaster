# CMakeLists.txt
# Crawlmaster v1.0.0 — AAA Dark Fantasy 5-Floor 3D Dungeon Crawler RPG
cmake_minimum_required(VERSION 3.28)
project(Crawlmaster VERSION 1.0.1 LANGUAGES CXX)

enable_testing()
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

include_directories(include)

include(FetchContent)

# 1. nlohmann/json 3.11.3 (immutable commit pin)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(json)

# 2. SFML 2.6.1 (immutable commit pin, static link)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 69ea0cd863aed1d4092b970b676924a716ff718b
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(SFML)

# 3. miniaudio 0.11.26 (immutable commit pin, Public Domain / MIT-0, zero OS audio dev headers)
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 877ea4a7c2601c7b0f2a44e83dfac42e6470fea6
    SOURCE_SUBDIR resources
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(miniaudio)

# 외부 C 단일 헤더 구현 번역 단위만 경고 억제 적용
if(MSVC)
    set_source_files_properties(src/core/MiniAudioImpl.cpp PROPERTIES COMPILE_OPTIONS "/W0")
else()
    set_source_files_properties(src/core/MiniAudioImpl.cpp PROPERTIES COMPILE_OPTIONS "-w")
endif()

set(RUNTIME_SOURCES
    src/core/Game.cpp
    src/core/GameStateManager.cpp
    src/core/AssetManager.cpp
    src/core/AudioManager.cpp
    src/core/MiniAudioImpl.cpp
    src/core/Persistence.cpp
    src/core/ResourceLocator.cpp
    src/core/SessionRng.cpp
    src/core/LocalizationManager.cpp
    src/controller/TitleState.cpp
    src/controller/GameOverState.cpp
    src/controller/VictoryState.cpp
    src/controller/TownState.cpp
    src/controller/CharacterCreationState.cpp
    src/controller/DungeonState.cpp
    src/controller/QuestJournalState.cpp
    src/controller/BestiaryState.cpp
    src/controller/ShutdownState.cpp
    src/controller/SettingsState.cpp
    src/controller/CharacterInfoState.cpp
    src/controller/CombatState.cpp
    src/controller/CombatStateActions.cpp
    src/view/DungeonRenderer.cpp
    src/view/PartyHudSnapshot.cpp
    src/view/UiSystem.cpp
    src/model/DungeonMap.cpp
    src/model/DungeonWorld.cpp
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

add_library(CrawlmasterRuntime STATIC ${RUNTIME_SOURCES})
target_include_directories(CrawlmasterRuntime SYSTEM PUBLIC "${miniaudio_SOURCE_DIR}")
target_link_libraries(CrawlmasterRuntime PUBLIC
    sfml-graphics sfml-window sfml-system nlohmann_json::nlohmann_json
)
if(UNIX AND NOT APPLE)
    target_link_libraries(CrawlmasterRuntime PUBLIC dl pthread m)
endif()

add_executable(Crawlmaster src/main.cpp)
target_link_libraries(Crawlmaster PRIVATE CrawlmasterRuntime)

add_custom_target(Assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_CURRENT_BINARY_DIR}/assets
    COMMENT "Syncing AAA assets (textures, shaders, audio, fonts, locales) to build output..."
)
add_dependencies(Crawlmaster Assets)
add_test(NAME ResourceVerification COMMAND Crawlmaster --verify-resources)

if(UNIX AND NOT APPLE)
    find_program(XVFB_RUN_EXECUTABLE xvfb-run)
endif()

function(add_crawl_test TARGET_NAME SOURCE_FILE USE_XVFB)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})
    target_link_libraries(${TARGET_NAME} PRIVATE CrawlmasterRuntime)
    target_compile_definitions(${TARGET_NAME} PRIVATE
        CRAWLMASTER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    add_dependencies(${TARGET_NAME} Assets)
    if(${USE_XVFB} AND XVFB_RUN_EXECUTABLE)
        add_test(NAME ${TARGET_NAME}
            COMMAND ${XVFB_RUN_EXECUTABLE} -a $<TARGET_FILE:${TARGET_NAME}> ${ARGN})
    else()
        add_test(NAME ${TARGET_NAME}
            COMMAND $<TARGET_FILE:${TARGET_NAME}> ${ARGN})
    endif()
    set_tests_properties(${TARGET_NAME} PROPERTIES ENVIRONMENT "CRAWLMASTER_HEADLESS=1")
endfunction()

add_crawl_test(TestHarness src/test_harness.cpp FALSE --run-all)
add_crawl_test(HudContractTests tests/test_hud_contracts.cpp FALSE)
add_crawl_test(CombatContractTests tests/test_combat_contracts.cpp FALSE)
add_crawl_test(ContentContractTests tests/test_content_contracts.cpp FALSE)
add_crawl_test(AgencyContractTests tests/test_agency_contracts.cpp FALSE)
add_crawl_test(LocalizationContractTests tests/test_localization_contracts.cpp FALSE)
add_crawl_test(AudioContractTests tests/test_audio_contracts.cpp FALSE)
add_crawl_test(FontRasterTests tests/test_font_raster.cpp TRUE "${CMAKE_CURRENT_BINARY_DIR}/font-raster-evidence")
add_crawl_test(ControllerContractTests tests/test_controller_contracts.cpp TRUE)
add_crawl_test(UiStateRasterTests tests/test_ui_state_raster.cpp TRUE "${CMAKE_CURRENT_BINARY_DIR}/ui-state-raster-evidence")

# 독립 프로세스 v5 체크포인트 RNG 리플레이 & 구버전(v1~v4) 안전 격리 테스트
add_executable(RngProcessReplayTests tests/test_rng_process_replay.cpp)
target_link_libraries(RngProcessReplayTests PRIVATE CrawlmasterRuntime)
add_dependencies(RngProcessReplayTests Assets)
set(RNG_REPLAY_DIR "${CMAKE_CURRENT_BINARY_DIR}/rng-process-replay")
add_test(NAME RngReplayWrite COMMAND ${CMAKE_COMMAND} -E make_directory "${RNG_REPLAY_DIR}")
set_tests_properties(RngReplayWrite PROPERTIES FIXTURES_SETUP RngReplayFixture)

add_test(NAME RngReplayCheckpointWrite COMMAND RngProcessReplayTests write
    "${RNG_REPLAY_DIR}/save.json" "${RNG_REPLAY_DIR}/expected.txt" "${RNG_REPLAY_DIR}/config.json")
set_tests_properties(RngReplayCheckpointWrite PROPERTIES
    FIXTURES_REQUIRED RngReplayFixture FIXTURES_SETUP RngReplayCheckpoint
    ENVIRONMENT "CRAWLMASTER_HEADLESS=1")

if(XVFB_RUN_EXECUTABLE)
    add_test(NAME RngReplayCheckpointContinue COMMAND ${XVFB_RUN_EXECUTABLE} -a
        $<TARGET_FILE:RngProcessReplayTests> continue
        "${RNG_REPLAY_DIR}/save.json" "${RNG_REPLAY_DIR}/expected.txt" "${RNG_REPLAY_DIR}/config.json")
else()
    add_test(NAME RngReplayCheckpointContinue COMMAND $<TARGET_FILE:RngProcessReplayTests> continue
        "${RNG_REPLAY_DIR}/save.json" "${RNG_REPLAY_DIR}/expected.txt" "${RNG_REPLAY_DIR}/config.json")
endif()
set_tests_properties(RngReplayCheckpointContinue PROPERTIES
    FIXTURES_REQUIRED RngReplayCheckpoint
    ENVIRONMENT "CRAWLMASTER_HEADLESS=1")

add_test(NAME LegacySaveQuarantineTest COMMAND RngProcessReplayTests legacy-quarantine
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/save_v1.json"
    "${RNG_REPLAY_DIR}/legacy-copy.json" "${RNG_REPLAY_DIR}/unused.json")
set_tests_properties(LegacySaveQuarantineTest PROPERTIES
    FIXTURES_REQUIRED RngReplayFixture
    ENVIRONMENT "CRAWLMASTER_HEADLESS=1")

install(TARGETS Crawlmaster
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT Runtime
)
install(DIRECTORY assets/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/crawlmaster/assets
    COMPONENT Runtime
)
install(FILES README.md THIRD_PARTY_NOTICES.md FONT_PROVENANCE.md ASSET_PROVENANCE.md DEPENDENCY_MANIFEST.spdx.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/crawlmaster
    COMPONENT Runtime
)
install(DIRECTORY licenses/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/crawlmaster/licenses
    COMPONENT Runtime
)

set(CPACK_PACKAGE_NAME "Crawlmaster")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Crawlmaster v1.0.0 AAA Dark Fantasy 3D Dungeon Crawler RPG")
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
