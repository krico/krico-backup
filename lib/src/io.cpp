#include "krico/backup/io.h"
#include <spdlog/spdlog.h>

using namespace krico::backup;
using namespace krico;
namespace fs = std::filesystem;

namespace {
    fs::path dotDot{".."};
}

bool backup::is_lexical_sub_path(const std::filesystem::path &path, const std::filesystem::path &base) {
    const auto &relative = path.lexically_normal().lexically_relative(base);
    if (const auto &b = relative.begin(); b != relative.end() && *b == dotDot) {
        return false;
    }
    return true;
}

fs::path backup::lexically_relative_symlink_target(const fs::path &link, const fs::path &target, const fs::path &base) {
    if (target.is_relative() || !is_lexical_sub_path(target, base)) {
        // Not 100% sure if this can be fixed for backup, so keep it as is for now
        return target;
    }
    return target.lexically_normal().lexically_relative(link.parent_path());
}
