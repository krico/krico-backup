#include "krico/backup/version.h"
#include "krico/backup/exception.h"
#include "krico/backup/io.h"
#include "krico/backup/BackupDirectory.h"
#include "krico/backup/BackupDirectoryId.h"
#include "krico/backup/BackupRepository.h"
#include "krico/backup/BackupRunner.h"
#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>
#include <chrono>
#include <iostream>

using namespace krico::backup;
using namespace std::chrono;
namespace fs = std::filesystem;

struct base_options {
    fs::path repoPath_;
};

struct subcommand {
    CLI::App &app_;
    const base_options &baseOptions_;
    CLI::App *subCommand_;

    subcommand(CLI::App &app, const base_options &baseOptions, const std::string &name, const std::string &description)
        : app_(app), baseOptions_(baseOptions), subCommand_(app.add_subcommand(name, description)) {
    }
};

struct help_subcommand : subcommand {
    help_subcommand(CLI::App &app, const base_options &baseOptions)
        : subcommand(app, baseOptions, "help", "Print help for all sub commands and exit") {
        subCommand_->silent()->parse_complete_callback([&] { this->help(); });
    }

    void help() const {
        std::cout << app_.help("", CLI::AppFormatMode::All);
        std::cout << std::endl;
        std::cout << "To print help for a specific sub command (for example 'init') try:" << std::endl;
        std::cout << "    " << app_.get_name() << " init -h" << std::endl;
        std::cout << std::endl;
        throw CLI::Success();
    }
};

struct init_subcommand : subcommand {
    init_subcommand(CLI::App &app, const base_options &baseOptions)
        : subcommand(app, baseOptions, "init", "Initialize a backup repository") {
        subCommand_->callback([&] { this->initialize(); });
    }

    void initialize() const {
        if (!exists(baseOptions_.repoPath_)) {
            MKDIRS(baseOptions_.repoPath_);
            std::cout << "Created backup repository '" << baseOptions_.repoPath_.string() << "'" << std::endl;
        }
        const auto backup = BackupRepository::initialize(baseOptions_.repoPath_);
        std::cout << "Initialized backup repository '" << baseOptions_.repoPath_.string() << "'" << std::endl;
    }
};

struct config_subcommand : subcommand {
    std::string name_{};
    CLI::Option *optionList_{nullptr};
    CLI::Option *optionGet_{nullptr};
    std::pair<std::string, std::string> nameValue_{};
    CLI::Option *optionSet_{nullptr};

    config_subcommand(CLI::App &app, const base_options &baseOptions)
        : subcommand(app, baseOptions, "config", "Manage the configuration of a backup repository") {
        optionList_ = subCommand_->add_flag("-l,--list", "List all")->group("Action");
        optionGet_ = subCommand_->add_option("-g,--get", name_, "get value of <name>")
                ->group("Action")->type_name("<name>");
        optionSet_ = subCommand_->add_option("-s,--set", nameValue_, "set value of config property <name> to <value>")
                ->group("Action")->type_size(2)->type_name("<name> <value>");

        subCommand_->callback([&] { this->config(); });
    }

    void config() const {
        const auto actions = subCommand_->get_options([](const auto *o) { return o->get_group() == "Action"; });
        if (std::ranges::all_of(actions, [](const CLI::Option *o) { return o->empty(); })) {
            std::cout << subCommand_->help(app_.get_name()) << std::endl;
            throw CLI::Success();
        }
        if (std::ranges::count_if(actions, [](const CLI::Option *o) { return !o->empty(); }) != 1) {
            throw CLI::ArgumentMismatch("error: only one action at a time");
        }
        BackupRepository repo{baseOptions_.repoPath_};
        auto &config = repo.config();

        if (*optionList_) {
            for (const auto &[k,v]: config.list()) {
                std::cout << k << "=" << v << std::endl;
            }
        } else if (*optionGet_) {
            if (const auto got = config.get(name_)) {
                std::cout << *got << std::endl;
            }
        } else if (*optionSet_) {
            config.set(nameValue_.first, nameValue_.second);
        }

        throw CLI::Success();
    }
};

struct add_subcommand : subcommand {
    fs::path dir_{};
    fs::path sourceDir_{};

    add_subcommand(CLI::App &app, const base_options &baseOptions)
        : subcommand(app, baseOptions, "add", "Add a backed-up directory") {
        subCommand_->add_option("dir", dir_,
                                "Directory under backup repository where the backup of <sourceDir> should reside")
                ->required();
        subCommand_->add_option("sourceDir", sourceDir_, "Directory that will be backed up into <dir>")->required();
        subCommand_->callback([&] { this->add(); });
    }

    void add() const {
        BackupRepository repo{baseOptions_.repoPath_};
        const BackupDirectory directory = repo.add_directory(dir_, sourceDir_);
        std::cout << "Added " << directory.id().relative_path() << " as backup of " << sourceDir_ << std::endl;
    }
};

struct list_subcommand : subcommand {
    list_subcommand(CLI::App &app, const base_options &baseOptions)
        : subcommand(app, baseOptions, "list", "List backed-up directories") {
        subCommand_->callback([&] { this->list(); });
    }

    void list() const {
        for (BackupRepository repo{baseOptions_.repoPath_}; const auto *backupDirectory: repo.list_directories()) {
            std::cout << backupDirectory->id().relative_path().string() << " -> "
                    << backupDirectory->sourceDir().string() << std::endl;
        }
    }
};

struct run_subcommand : subcommand {
    run_subcommand(CLI::App &app, const base_options &baseOptions)
        : subcommand(app, baseOptions, "run", "Run the backup for this repository") {
        subCommand_->callback([&] { this->run_backup(); });
    }

    void run_backup() const {
        for (BackupRepository repo{baseOptions_.repoPath_}; const auto *backupDirectory: repo.list_directories()) {
            std::cout << "Running backup of '" << backupDirectory->id().relative_path().string() << "'"
                    << " from '" << backupDirectory->sourceDir().string() << "'" << std::endl;
            auto summary = repo.run_backup(*backupDirectory);
            std::cout << summary << std::endl;
        }
    }
};

struct log_subcommand : subcommand {
    uint32_t number_{0};
    CLI::Option *optionNumber_{nullptr};
    uint32_t skip_{0};
    CLI::Option *optionSkip_{nullptr};
    CLI::Option *optionFull_{nullptr};
    std::string hash_{};
    CLI::Option *optionHash_{nullptr};
    CLI::Option *optionOne_{nullptr};
    CLI::Option *optionFileList_{nullptr};

    log_subcommand(CLI::App &app, const base_options &baseOptions)
        : subcommand(app, baseOptions, "log", "Print the log of what happened in the backup repository") {
        optionNumber_ = subCommand_->add_option("-n,--number", number_, "Limit the number of log records to output")
                ->type_name("<number>");
        optionSkip_ = subCommand_->add_option("-s,--skip", skip_, "Skip the first <number> records")
                ->type_name("<number>");
        optionFull_ = subCommand_->add_flag("-f,--full", "Print all information about each log record");
        optionOne_ = subCommand_->add_flag("-1", "Print a single result (equivalent to '-n 1')");
        optionFileList_ = subCommand_->add_flag("--file-list", "Print the list of files of every 'run' log record\n"
                                                "Best if used in combination with `-n` or `-1` since could be a lot of output");
        optionHash_ = subCommand_->add_option("hash", hash_, "Start printing logs from this log <hash>.\n"
                                              "A full hash such as 00223f175b5efa40724916ac50176e5fd5204fd2\n"
                                              "A partial hash like 00223f17 (must be unique)");
        subCommand_->callback([&] { this->log(); });
    }

    Digest::result start_hash(BackupRepositoryLog &log) const {
        if (*optionHash_) {
            if (const auto found = log.findHash(hash_); found.empty()) {
                throw exception("No hash found matching '" + hash_ + "'");
            } else {
                if (found.size() != 1) {
                    std::stringstream ss;
                    for (const auto &h: found) {
                        ss << std::endl << h.str();
                    }
                    throw exception("More than one hash found matching '" + hash_ + "'" + ss.str());
                }
                return found.at(0);
            }
        }
        return log.head();
    }

    void log() const {
        static constexpr auto WIDTH = 10;
        BackupRepository repo{baseOptions_.repoPath_};
        auto &log = repo.repositoryLog();
        if (log.head().is_zero()) {
            // Should never happen (since init creates an entry)
            std::cout << "No log entries in this repository" << std::endl;
            return;
        }
        uint32_t first{0};
        if (*optionSkip_) first = skip_;
        uint32_t last{std::numeric_limits<uint32_t>::max()};
        if (*optionNumber_) {
            last = number_ + first;
        }
        if (*optionOne_) {
            last = 1 + first;
        }
        uint32_t count = 0;
        auto prev = start_hash(log);
        do {
            if (count++ == last) break;
            const LogHeader &headerEntry = log.getRecord(prev);

            if (count > first) {
                const auto type = headerEntry.type();
                std::cout << std::left << std::setw(5) << std::setfill(' ') << type << prev.str() << std::endl;
                system_clock::time_point ts{headerEntry.ts()};
                const auto tt = system_clock::to_time_t(ts);

                std::cout << std::left << std::setw(WIDTH) << "Date:" << std::ctime(&tt);
                switch (type) {
                    case InitRecord::log_entry_type: {
                        const auto &entry = log_record_cast<InitRecord>(headerEntry);
                        std::cout << std::left << std::setw(WIDTH) << "Author:" << entry.author() << std::endl;
                    }
                    break;
                    case AddDirectoryRecord::log_entry_type: {
                        const auto &entry = log_record_cast<AddDirectoryRecord>(headerEntry);
                        std::cout << std::left << std::setw(WIDTH) << "Author:" << entry.author() << std::endl;
                        std::cout << std::endl;
                        std::cout << "  Added \"" << entry.directoryId() << "\""
                                << " as backup of " << entry.sourceDir() << "" << std::endl;

                        if (*optionFull_) {
                            constexpr auto W = 12;
                            std::cout << std::endl;
                            std::cout << std::setw(W) << "Directory:" << entry.directoryId() << std::endl;
                            std::cout << std::setw(W) << "SourceDir:" << entry.sourceDir().string() << std::endl;
                        }
                    }
                    break;
                    case RunBackupRecord::log_entry_type: {
                        const auto &entry = log_record_cast<RunBackupRecord>(headerEntry);
                        std::cout << std::left << std::setw(WIDTH) << "Author:" << entry.author() << std::endl;
                        const auto &summary = entry.summary();
                        std::cout << std::left << std::setw(WIDTH) << "Checksum:" << summary.checksum().str() <<
                                std::endl;
                        std::cout << std::endl;
                        const auto total = summary.numCopiedFiles()
                                           + summary.numHardLinkedFiles()
                                           + summary.numSymlinks();
                        const auto elapsed = duration_cast<nanoseconds>(summary.endTime() - summary.startTime());
                        std::cout << "  Backed up " << summary.directoryId().relative_path()
                                << " (" << total << " entries)"
                                << " in " << std::format("{0:%T}", elapsed) << std::endl;

                        if (*optionFull_) {
                            constexpr auto W = 12;
                            std::cout << std::endl;
                            std::cout << summary << std::endl;
                        }
                        if (*optionFileList_) {
                            //TODO: file-list should be structured (not just plain text)
                            std::cout << std::endl;
                            if (const auto *backupDirectory = repo.get_directory(summary.directoryId())) {
                                auto summaryFile = summary.summaryFile(backupDirectory->metaDir());
                                if (std::ifstream in{summaryFile}; in) {
                                    std::cout << in.rdbuf();
                                } else {
                                    throw exception("Failed to open '" + summaryFile.string() + "'");
                                }
                            } else {
                                std::cerr << "ERROR: \"" << summary.directoryId().str() << "\" NOT FOUND" << std::endl;
                                std::cout << "N/A" << std::endl;
                            }
                        }
                    }
                    break;
                    default:
                        std::cout << "UNKNOWN LOG ENTRY TYPE" << std::endl;
                        break;
                }
            }
            prev = headerEntry.prev();
            if (!prev.is_zero() && count != last) std::cout << std::endl;
        } while (!prev.is_zero());
    }
};

class krico_backup {
public:
    krico_backup() {
        app_.get_formatter()->column_width(30);
        app_.get_formatter()->label("REQUIRED", "(required)");
        app_.failure_message(CLI::FailureMessage::simple);
        app_.set_version_flag("-v,--version", "krico-backup (version " KRICO_BACKUP_VERSION ")",
                              "Display version and exit");
        app_.require_subcommand(1, 1);
        app_.add_option("-C", baseOptions_.repoPath_,
                        "Run as if the program was started in <path> instead of the current directory"
        )->type_name("<path>");
    }

    int run(const int argc, char **argv) {
        try {
            CLI11_PARSE(app_, argc, argv);
            return 0;
        } catch (const exception &e) {
            std::cerr << "fatal: " << e.what() << std::endl;
            return 1;
        } catch (...) {
            std::cerr << "fatal: unknown error..." << std::endl;
            return 1;
        }
    }

private:
    CLI::App app_{"Space-efficient backups for personal use", "krico-backup"};
    base_options baseOptions_{.repoPath_ = absolute(fs::path{})};
    init_subcommand init_{app_, baseOptions_};
    config_subcommand config_{app_, baseOptions_};
    add_subcommand add_{app_, baseOptions_};
    list_subcommand list_{app_, baseOptions_};
    run_subcommand run_{app_, baseOptions_};
    log_subcommand log_{app_, baseOptions_};
    help_subcommand help_{app_, baseOptions_};
};

int main(const int argc, char **argv) {
    krico_backup kb{};
    return kb.run(argc, argv);
}
