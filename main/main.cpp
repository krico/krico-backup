#include "krico/backup/version.h"
#include "krico/backup/exception.h"
#include "krico/backup/Backup.h"
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
            BackupRunner runner{*backupDirectory};
            runner.run();
        }
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
    help_subcommand help_{app_, baseOptions_};
};

int main(const int argc, char **argv) {
    krico_backup kb{};
    return kb.run(argc, argv);
}
