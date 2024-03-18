#include "krico/backup/BackupDirectoryId.h"
#include "krico/backup/Digest.h"
#include "krico/backup/Digest.h"

using namespace krico::backup;
namespace fs = std::filesystem;

BackupDirectoryId::BackupDirectoryId(const std::string &id)
    : id_(fs::path{id}.lexically_normal()), relativePath_(id_), idPath_(sha1_sum(id_)) {
}
