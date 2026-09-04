#ifndef PERSISTENCE_HPP
#define PERSISTENCE_HPP

#include <filesystem>
#include <string>

namespace crawl {

enum class PersistenceStatus {
    Saved,
    Loaded,
    NotFound,
    RecoveredFromBackup,
    CommittedDurabilityUnknown,
    Corrupt,
    UnsupportedVersion,
    IoError,
    RecoveryPending
};

struct PersistenceResult {
    PersistenceStatus status = PersistenceStatus::IoError;
    std::filesystem::path path;
    std::string message;

    [[nodiscard]] bool succeeded() const {
        return status == PersistenceStatus::Saved ||
               status == PersistenceStatus::Loaded ||
               status == PersistenceStatus::RecoveredFromBackup ||
               status == PersistenceStatus::CommittedDurabilityUnknown;
    }

    operator bool() const { return succeeded(); }
    [[nodiscard]] bool durabilityConfirmed() const {
        return status != PersistenceStatus::CommittedDurabilityUnknown;
    }
};

class Persistence {
public:
    static std::filesystem::path userDataDirectory();
    static std::filesystem::path defaultSavePath();
    static std::filesystem::path defaultConfigPath();

    static PersistenceResult atomicWriteText(const std::filesystem::path& path,
                                             const std::string& contents);
    static PersistenceResult quarantine(const std::filesystem::path& path,
                                        std::filesystem::path& quarantinePath);
    static void setDirectorySyncFailureForTests(bool enabled);
    static void setPostCommitSyncFailureForTests(bool enabled);
};

} // namespace crawl

#endif // PERSISTENCE_HPP
