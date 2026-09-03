#include "core/Persistence.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <system_error>
#include <atomic>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace crawl {
namespace {

std::atomic_bool forceDirectorySyncFailure{false};
std::atomic_bool forcePostCommitSyncFailure{false};

bool syncFile(const std::filesystem::path& path) {
#ifdef _WIN32
    const int descriptor = _wopen(path.wstring().c_str(), _O_RDONLY | _O_BINARY);
    if (descriptor < 0) return false;
    const bool ok = _commit(descriptor) == 0;
    _close(descriptor);
    return ok;
#else
    const int descriptor = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (descriptor < 0) return false;
    const bool ok = ::fsync(descriptor) == 0;
    ::close(descriptor);
    return ok;
#endif
}

bool writeAndSync(const std::filesystem::path& path, const std::string& contents) {
#ifdef _WIN32
    const int descriptor = _wopen(path.wstring().c_str(),
                                  _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY,
                                  _S_IREAD | _S_IWRITE);
    if (descriptor < 0) return false;
    std::size_t offset = 0;
    while (offset < contents.size()) {
        const unsigned int remaining = static_cast<unsigned int>(
            std::min<std::size_t>(contents.size() - offset, 1U << 30));
        const int written = _write(descriptor, contents.data() + offset, remaining);
        if (written <= 0) {
            _close(descriptor);
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    const bool ok = _commit(descriptor) == 0;
    _close(descriptor);
    return ok;
#else
    const int descriptor = ::open(path.c_str(),
                                  O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW,
                                  0600);
    if (descriptor < 0) return false;
    std::size_t offset = 0;
    while (offset < contents.size()) {
        const ssize_t written = ::write(descriptor, contents.data() + offset,
                                        contents.size() - offset);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) {
            ::close(descriptor);
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    const bool ok = ::fsync(descriptor) == 0;
    ::close(descriptor);
    return ok;
#endif
}

bool replaceFile(const std::filesystem::path& source, const std::filesystem::path& destination) {
#ifdef _WIN32
    return MoveFileExW(source.wstring().c_str(), destination.wstring().c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return ::rename(source.c_str(), destination.c_str()) == 0;
#endif
}

bool syncDirectory(const std::filesystem::path& directory) {
    if (forceDirectorySyncFailure.load()) return false;
#ifndef _WIN32
    const int descriptor = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (descriptor < 0) return false;
    const bool synced = ::fsync(descriptor) == 0;
    ::close(descriptor);
    return synced;
#else
    static_cast<void>(directory);
    return true;
#endif
}

std::filesystem::path absoluteEnvPath(const char* name) {
    const char* value = std::getenv(name);
    if (!value || *value == '\0') return {};
    std::filesystem::path path(value);
    return path.is_absolute() ? path : std::filesystem::path{};
}

} // namespace

std::filesystem::path Persistence::userDataDirectory() {
#ifdef _WIN32
    if (auto appData = absoluteEnvPath("APPDATA"); !appData.empty()) {
        return appData / "Crawlmaster";
    }
#else
    if (auto xdgData = absoluteEnvPath("XDG_DATA_HOME"); !xdgData.empty()) {
        return xdgData / "crawlmaster";
    }
    if (auto home = absoluteEnvPath("HOME"); !home.empty()) {
        return home / ".local" / "share" / "crawlmaster";
    }
#endif
    return std::filesystem::temp_directory_path() / "crawlmaster-user-data";
}

std::filesystem::path Persistence::defaultSavePath() {
    return userDataDirectory() / "save.json";
}

std::filesystem::path Persistence::defaultConfigPath() {
    return userDataDirectory() / "config.json";
}

PersistenceResult Persistence::atomicWriteText(const std::filesystem::path& path,
                                               const std::string& contents) {
    if (path.empty()) {
        return {PersistenceStatus::IoError, path, "저장 경로가 비어 있습니다."};
    }

    std::error_code error;
    const auto parent = path.has_parent_path() ? path.parent_path() : std::filesystem::current_path();
    std::filesystem::create_directories(parent, error);
    if (error) {
        return {PersistenceStatus::IoError, path, "저장 디렉터리를 만들 수 없습니다: " + error.message()};
    }
    if (!syncDirectory(parent)) {
        return {PersistenceStatus::IoError, path,
                "저장 전 디렉터리 동기화 검사에 실패했습니다."};
    }

    if (std::filesystem::is_symlink(std::filesystem::symlink_status(path, error))) {
        return {PersistenceStatus::IoError, path, "심볼릭 링크 저장 대상은 허용하지 않습니다."};
    }
    error.clear();

    const std::filesystem::path temporary = path.string() + ".tmp";
    const std::filesystem::path backup = path.string() + ".bak";
    const std::filesystem::path backupTemporary = path.string() + ".bak.tmp";

    if (!writeAndSync(temporary, contents)) {
        std::filesystem::remove(temporary, error);
        return {PersistenceStatus::IoError, path, "임시 파일 쓰기 또는 동기화에 실패했습니다."};
    }

    const bool hadOriginal = std::filesystem::exists(path, error);
    if (hadOriginal) {
        error.clear();
        std::filesystem::copy_file(path, backupTemporary,
                                   std::filesystem::copy_options::overwrite_existing, error);
        if (error || !syncFile(backupTemporary) || !replaceFile(backupTemporary, backup)) {
            std::filesystem::remove(temporary, error);
            std::filesystem::remove(backupTemporary, error);
            return {PersistenceStatus::IoError, path, "백업 회전에 실패했습니다."};
        }
    }

    if (!replaceFile(temporary, path)) {
        std::filesystem::remove(temporary, error);
        return {PersistenceStatus::IoError, path, "원자 파일 교체에 실패했습니다."};
    }

    if (forcePostCommitSyncFailure.load() || !syncDirectory(parent)) {
        return {PersistenceStatus::CommittedDurabilityUnknown, path,
                "파일 교체는 완료됐지만 디렉터리 crash durability를 확인하지 못했습니다."};
    }
    return {PersistenceStatus::Saved, path, {}};
}

PersistenceResult Persistence::quarantine(const std::filesystem::path& path,
                                          std::filesystem::path& quarantinePath) {
    if (!std::filesystem::exists(path)) {
        return {PersistenceStatus::NotFound, path, "격리할 파일이 없습니다."};
    }

    const auto stamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const auto parent = path.has_parent_path() ? path.parent_path() : std::filesystem::current_path();
    if (!syncDirectory(parent)) {
        return {PersistenceStatus::IoError, path, "격리 전 디렉터리 동기화 검사에 실패했습니다."};
    }
    const std::string stem = path.stem().string();
    const std::string extension = path.extension().empty() ? ".json" : path.extension().string();

    for (int suffix = 0; suffix < 100; ++suffix) {
        std::string filename = stem + ".corrupt-" + std::to_string(stamp);
        if (suffix > 0) filename += "-" + std::to_string(suffix);
        quarantinePath = parent / (filename + extension);
        if (std::filesystem::exists(quarantinePath)) continue;

        std::error_code error;
        std::filesystem::rename(path, quarantinePath, error);
        if (!error) {
            if (!syncDirectory(parent)) {
                return {PersistenceStatus::CommittedDurabilityUnknown, quarantinePath,
                        "격리는 완료됐지만 디렉터리 crash durability를 확인하지 못했습니다."};
            }
            return {PersistenceStatus::Corrupt, quarantinePath, "손상 파일을 격리했습니다."};
        }
        return {PersistenceStatus::IoError, path, "손상 파일 격리에 실패했습니다: " + error.message()};
    }

    return {PersistenceStatus::IoError, path, "고유한 손상 파일 이름을 만들 수 없습니다."};
}

void Persistence::setDirectorySyncFailureForTests(bool enabled) {
    forceDirectorySyncFailure.store(enabled);
}

void Persistence::setPostCommitSyncFailureForTests(bool enabled) {
    forcePostCommitSyncFailure.store(enabled);
}

} // namespace crawl
