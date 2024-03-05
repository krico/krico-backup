#include "krico/backup/settings.h"
#include <CLI/CLI.hpp>
#include <iostream>

int main(const int argc, char **argv) {
    using namespace krico::backup;
    settings s{};
    CLI::App app{"Space-efficient backups for personal use"};
    argv = app.ensure_utf8(argv);

    std::string filename = "default";
    app.add_option("-f,--file", filename, "A help string");

    CLI11_PARSE(app, argc, argv);
    return 0;
}
