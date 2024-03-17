#include "krico/backup/BackupDirectory.h"
#include "krico/backup/exception.h"
#include "krico/backup/io.h"
#include <spdlog/spdlog.h>
#include <fstream>


using namespace krico::backup;
namespace fs = std::filesystem;

BackupDirectory::BackupDirectory(BackupRepository &repository, BackupDirectoryId id)
    : repository_(repository), id_(std::move(id)),
      dir_(repository_.dir() / id_.relative_path()),
      metaDir_(repository_.directoriesDir() / id_.id_path()),
      configured_(false) {
    const fs::path sourceFile{metaDir_ / SOURCE_FILE};
    const auto sourceStatus = STATUS(sourceFile);
    if (sourceStatus.type() != fs::file_type::not_found) {
        THROW_EXCEPTION("Existing source file '" + sourceFile.string() + "' (use other constructor)");
    }
    const fs::path targetFile{metaDir_ / TARGET_FILE};
    const auto tagetStatus = STATUS(targetFile);
    if (tagetStatus.type() != fs::file_type::not_found) {
        THROW_EXCEPTION("Existing target file '" + targetFile.string() + "' (use other constructor)");
    }
}

BackupDirectory::BackupDirectory(BackupRepository &repository, const std::filesystem::path &directoryMetaDir)
    : repository_(repository),
      id_(readId(directoryMetaDir)),
      dir_(repository_.dir() / id_.relative_path()),
      metaDir_(repository_.directoriesDir() / id_.id_path()),
      configured_(true) {
    const fs::path sorceFile{directoryMetaDir / SOURCE_FILE};
    spdlog::debug("Reading [{}]", sorceFile.string());
    std::string line;
    if (std::ifstream in{sorceFile}; std::getline(in, line)) {
        sourceDir_ = line;
    } else {
        THROW_EXCEPTION("Failed to read source file '" + sorceFile.string() + "'");
    }
}

const std::filesystem::path &BackupDirectory::sourceDir() const {
    if (configured_) return sourceDir_;
    THROW_EXCEPTION("Backup directory '" + id_.str() + "' is not configured.");
}

void BackupDirectory::configure(const std::filesystem::path &sourceDir) {
    if (configured_) {
        THROW_EXCEPTION("Backup directory '" + id_.str() + "' is already configured.");
    }
    if (!is_directory(metaDir_)) {
        spdlog::debug("Creating directory [{}]", metaDir_.string());
        MKDIRS(metaDir_);
    }
    const fs::path targetFile{metaDir_ / TARGET_FILE};
    spdlog::debug("Writing [{}] '{}'", targetFile.string(), id_.relative_path().string());
    if (std::ofstream out{targetFile}; out) {
        out << id_.relative_path().string() << std::endl;
    } else {
        THROW_EXCEPTION("Failed to write target file '" + targetFile.string() + "'");
    }

    const fs::path sourceFile{metaDir_ / SOURCE_FILE};
    spdlog::debug("Writing [{}] '{}'", sourceFile.string(), sourceDir.string());
    if (std::ofstream out{sourceFile}; out) {
        out << sourceDir.string() << std::endl;
    } else {
        THROW_EXCEPTION("Failed to write source file '" + sourceFile.string() + "'");
    }

    if (!is_directory(dir_)) {
        spdlog::debug("Creating directory [{}]", dir_.string());
        MKDIRS(dir_);
    }
    sourceDir_ = sourceDir;
    configured_ = true;
}

BackupDirectoryId BackupDirectory::readId(const std::filesystem::path &directoryMetaDir) {
    const fs::path targetFile{directoryMetaDir / TARGET_FILE};
    spdlog::debug("Reading [{}]", targetFile.string());
    std::string line;
    if (std::ifstream in{targetFile}; std::getline(in, line)) {
        return BackupDirectoryId(line);
    }
    THROW_EXCEPTION("Failed to read target file '" + targetFile.string() + "'");
}
