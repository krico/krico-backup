#include "krico/backup/TemporaryFile.h"
#include "krico/backup/exception.h"
#include <spdlog/spdlog.h>
#include <cstring>
#include <unistd.h>
#include <cstdio>

using namespace krico::backup;
namespace fs = std::filesystem;

namespace {
    fs::path create_temporary_file(const fs::path &dir, const std::string &prefix, const std::string &suffix) {
        const fs::path filepath = dir / (prefix + "XXXXXX" + suffix);
        char filename[filepath.string().size() + 1];
        std::strcpy(filename, filepath.c_str());
        const int fd = mkstemps(filename, static_cast<int>(suffix.size()));
        if (fd == -1) {
            THROW_ERRNO("create_temporary_file '" + filepath.string() + "'");
        }
        close(fd);
        return fs::path{filename};
    }
}

TemporaryFile::TemporaryFile(const fs::path &dir, const std::string &prefix, const std::string &suffix)
    : file_(create_temporary_file(dir, prefix, suffix)) {
}

TemporaryFile::TemporaryFile(const args_t &args): TemporaryFile(args.dir, args.prefix, args.suffix) {
}

TemporaryFile::TemporaryFile() : TemporaryFile(fs::temp_directory_path()) {
}

TemporaryFile::~TemporaryFile() {
    if (!file_.empty()) std::remove(file_.c_str());
}

const fs::path &TemporaryFile::file() const {
    return file_;
}
