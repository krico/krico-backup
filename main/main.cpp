#include "krico/backup/settings.h"
#include "krico/backup/Backup.h"
#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>
#include <chrono>
#include <iostream>

int main(const int argc, char **argv) {
    using namespace krico::backup;
    namespace fs = std::filesystem;

    settings s{};
    CLI::App app{"Space-efficient backups for personal use", "krico-backup"};
    app.get_formatter()->column_width(40);
    app.get_formatter()->label("REQUIRED", "(required)");
    app.set_version_flag("-v,--version", "krico-backup (version ???)", "Display version and exit");
    argv = app.ensure_utf8(argv);

    fs::path source;
    app.add_option("-s,--source-dir", source, "Directory to backup (source of files)")
            ->required()->type_name("<dir>");
    fs::path target;
    app.add_option("-t,--target-dir", target, "Directory where backup will be written (target)")
            ->required()->type_name("<dir>");

    CLI11_PARSE(app, argc, argv);
    spdlog::set_level(spdlog::level::debug);
    source = absolute(source);
    target = absolute(target);
    Backup backup{source, target};
    backup.run();
    std::cout << "+++ Backup complete +++" << std::endl
            << "Source       : " << source.string() << std::endl
            << "Target       : " << target.string() << std::endl
            << "Backup date  : " << backup.date() << std::endl
            << backup.stats() << std::endl
            << std::endl;
    return 0;
}
