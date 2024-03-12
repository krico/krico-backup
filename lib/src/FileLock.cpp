#include "krico/backup/FileLock.h"
#include "krico/backup/exception.h"
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <sys/file.h>

using namespace krico::backup;
namespace fs = std::filesystem;

namespace {
    void check_close(const int fd) {
        if (close(fd)) {
            auto ec = std::make_error_code(static_cast<std::errc>(errno));
            spdlog::error("Failed to close [fd={}][ec={}]: {}", fd, ec.value(), ec.message());
        }
    }
}

FileLock::FileLock(): file_(), fd_(-1) {
}

FileLock::FileLock(fs::path file) : file_(std::move(file)) {
    fd_ = open(file_.c_str(), O_CREAT | O_RDONLY, S_IRWXU);
    if (fd_ == -1) {
        THROW_ERRNO("Failed to open '" + file_.string() + "'");
    }
    if (flock(fd_, LOCK_EX | LOCK_NB) == 0) {
        spdlog::debug("Acquired lock  [fd={}][file={}]", fd_, file_.string());
    } else {
        const auto ec = std::make_error_code(static_cast<std::errc>(errno));
        check_close(fd_);
        fd_ = -1;
        THROW_ERROR_CODE("Failed to acquire lock [file=" + file_.string() + "]", ec);
    }
}

FileLock::FileLock(std::filesystem::path file, const int fd) noexcept : file_(std::move(file)), fd_(fd) {
}

FileLock::FileLock(FileLock &&rhs) noexcept : file_(std::move(rhs.file_)), fd_(std::exchange(rhs.fd_, -1)) {
}

FileLock &FileLock::operator=(FileLock &&rhs) noexcept {
    if (fd_ != -1) {
        spdlog::debug("Releasing lock [fd={}][file={}] (move-assign)", fd_, file_.string());
        check_close(fd_);
    }
    file_ = std::move(rhs.file_);
    fd_ = std::exchange(rhs.fd_, -1);
    return *this;
}

FileLock::~FileLock() {
    if (fd_ != -1) {
        spdlog::debug("Releasing lock [fd={}][file={}]", fd_, file_.string());
        check_close(fd_);
        fd_ = -1;
    }
}

void FileLock::unlock() {
    if (fd_ != -1) {
        spdlog::debug("Releasing lock [fd={}][file={}] (unlock)", fd_, file_.string());
        check_close(fd_);
        fd_ = -1;
    }
}

std::optional<FileLock> FileLock::try_lock(const std::filesystem::path &file) noexcept {
    const int fd = open(file.c_str(), O_CREAT | O_RDONLY, S_IRWXU);
    if (fd == -1) {
        spdlog::debug("Failed to open '{}'", file.string());
        return std::nullopt;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
        spdlog::debug("Acquired lock  [fd={}][file={}] (try_lock)", fd, file.string());
        return FileLock{file, fd};
    }
    const auto ec = std::make_error_code(static_cast<std::errc>(errno));
    check_close(fd);
    spdlog::debug("Failed to acquire lock [file={}] (try_lock)", file.string());
    return std::nullopt;
}
