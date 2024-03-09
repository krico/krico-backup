#pragma once

#include <filesystem>
#include <optional>

namespace krico::backup {
    //!
    //! A wrapper around an flock() ed file descriptor.  Holds the exclusive lock as long as the object exists
    //!
    class FileLock final {
    public:
        explicit FileLock(std::filesystem::path file);

        ~FileLock();

        FileLock(const FileLock &) = delete;


        FileLock &operator=(const FileLock &) = delete;

        FileLock(FileLock &&) noexcept;

        FileLock &operator=(FileLock &&) noexcept;


        [[nodiscard]] bool locked() const { return fd_ != -1; }

        void unlock();

        static std::optional<FileLock> try_lock(const std::filesystem::path &file) noexcept;

    private:
        std::filesystem::path file_;
        int fd_{-1};

        FileLock(std::filesystem::path file, int fd) noexcept;
    };
}
