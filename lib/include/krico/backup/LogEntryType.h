#pragma once

#include <ostream>

namespace krico::backup {
    enum class LogEntryType : uint8_t {
        NONE = 0,
        Initialized = 1,
        AddDirectory = 2,
        RunBackup = 3,
    };

    inline std::ostream &operator<<(std::ostream &out, const LogEntryType &type) {
        // Try to keep this to max 4 chars (or change setw in command line)
        switch (type) {
            case LogEntryType::NONE:
                return out << "none";
            case LogEntryType::Initialized:
                return out << "init";
            case LogEntryType::AddDirectory:
                return out << "add";
            case LogEntryType::RunBackup:
                return out << "run";
            default:
                return out << "UNKN";
        }
    }
}
