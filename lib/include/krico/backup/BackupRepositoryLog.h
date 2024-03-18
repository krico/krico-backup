#pragma once

#include "BackupRepository.h"

namespace krico::backup {
    class BackupRepositoryLog {
    public:
        explicit BackupRepositoryLog(const BackupRepository &repository);

    private:
        const BackupRepository &repository_;
    };
}
