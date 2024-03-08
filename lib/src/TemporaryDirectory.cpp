#include "krico/backup/TemporaryDirectory.h"
#include "krico/backup/exception.h"
#include <spdlog/spdlog.h>
#include <cstring>
#include <unistd.h>
#include <cstdio>

using namespace krico::backup;
namespace fs = std::filesystem;

namespace {
    fs::path create_temporary_dir(const fs::path &dir, const std::string &prefix) {
        const fs::path dirpath = dir / (prefix + "XXXXXX");
        char dirname[dirpath.string().size() + 1];
        std::strcpy(dirname, dirpath.c_str());

        if (!mkdtemp(dirname)) {
            THROW_ERRNO("create_temporary_dir '" + dirpath.string() + "'");
        }

        return fs::path{dirname};
    }
}

TemporaryDirectory::TemporaryDirectory(const fs::path &dir, const std::string &prefix)
    : dir_(create_temporary_dir(dir, prefix)) {
}

TemporaryDirectory::TemporaryDirectory(const args_t &args): TemporaryDirectory(args.dir, args.prefix) {
}

TemporaryDirectory::TemporaryDirectory() : TemporaryDirectory(fs::temp_directory_path()) {
}

TemporaryDirectory::~TemporaryDirectory() {
    if (!dir_.empty()) fs::remove_all(dir_.c_str());
}

const fs::path &TemporaryDirectory::dir() const {
    return dir_;
}
