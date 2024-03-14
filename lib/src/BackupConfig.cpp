#include "krico/backup/BackupConfig.h"
#include "krico/backup/exception.h"
#include "krico/backup/io.h"
#include "krico/backup/TemporaryFile.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <chrono>
#include <cstdint>

using namespace krico::backup;
using namespace std::chrono;
namespace fs = std::filesystem;

namespace {
    void ltrim(std::string &s) {
        s.erase(s.begin(), std::ranges::find_if(s, [](const unsigned char ch) {
            return !std::isspace(ch);
        }));
    }

    void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](const unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    void to_lower(std::string &s) {
        std::ranges::transform(s, s.begin(),
                               [](const unsigned char c) { return std::tolower(c); });
    }
}

BackupConfig::BackupConfig(std::filesystem::path file): file_(std::move(file)) {
    initialize();
    parse();
}

void BackupConfig::initialize() const {
    const auto status = STATUS(file_);
    if (status.type() == std::filesystem::file_type::not_found) {
        spdlog::trace("Initializing BackupConfig[{}]", file_.string());
        std::ofstream out{file_};

        out << "# Created " << system_clock::now() << std::endl;
    } else if (status.type() == std::filesystem::file_type::regular) {
        spdlog::trace("Found BackupConfig[{}]", file_.string());
    } else {
        const std::string type{std::to_string(static_cast<int>(status.type()))};
        THROW_EXCEPTION("Failed to initialize BackupConfig[" + file_.string() + "] (file_type=" + type + ")");
    }
}

std::optional<std::string> BackupConfig::get(const std::string &section,
                                             const std::string &subSection,
                                             const std::string &variable) {
    std::string sectionName = section;
    to_lower(sectionName);
    std::stringstream key;
    if (!sectionName.empty()) key << sectionName << '.';
    if (!subSection.empty()) key << subSection << '.';
    key << variable;
    return get(key.str());
}

void BackupConfig::set(const std::string &section,
                       const std::string &subSection,
                       const std::string &variable,
                       const std::string &value) {
    std::string sectionName = section;
    to_lower(sectionName);
    if (std::ranges::any_of(sectionName, [](const auto &c) { return !(std::isalnum(c) || c == '-'); })) {
        THROW_EXCEPTION("Invalid section '" + section + "' (only alphanumeric and '-')");
    }
    if (variable.empty()) {
        THROW_EXCEPTION("Variable cannot be empty");
    }
    if (std::ranges::any_of(subSection, [](const auto &c) { return c == '\n' || c == '\0' || c == '"' || c == '\\'; })) {
        THROW_EXCEPTION("Invalid subSection '" + subSection + "' (cannot contain new-line, null byte, '\"' or '\\')");
    }
    if (!std::isalpha(variable.front())) {
        THROW_EXCEPTION("Variable name '" + variable + "' must start an alphabetic character");
    }
    if (std::ranges::any_of(variable, [](auto &c) { return !(std::isalnum(c) || c == '-'); })) {
        THROW_EXCEPTION("Variable '" + variable + "' (start with alphabetic followed by only alphanumeric and '-')");
    }
    uint32_t sectionLine = 0;
    uint32_t valueLine = 0;

    if (const auto foundSection = sections_.find(sectionName); foundSection != sections_.end()) {
        const auto &ssMap = foundSection->second.subSections_;
        if (const auto foundSubSection = ssMap.find(subSection); foundSubSection != ssMap.end()) {
            const auto &ss = foundSubSection->second;
            sectionLine = ss.lineNo_;
            if (const auto foundVar = ss.values_.find(variable); foundVar != ss.values_.end()) {
                valueLine = foundVar->second.lineNo_;
            }
        }
    }

    TemporaryFile tmp(file_.parent_path(), file_.filename(), ".tmp");
    std::ifstream in(file_);
    std::ofstream out(tmp.file());
    std::string line;
    uint32_t lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (sectionLine != 0) {
            if (valueLine == 0) {
                if (sectionLine == lineNo) {
                    out << line << std::endl;
                    out << "\t" << variable << " = " << value << std::endl;
                    continue;
                }
            } else {
                if (valueLine == lineNo) {
                    out << "\t" << variable << " = " << value << std::endl;
                    continue;
                }
            }
        }
        out << line << std::endl;
    }
    if (sectionLine == 0) {
        out << '[' << sectionName;
        if (!subSection.empty()) {
            out << ' ' << '"' << subSection << '"';
        }
        out << ']' << std::endl;
        out << "\t" << variable << " = " << value << std::endl;
    }
    RENAME_FILE(tmp.file(), file_);
    parse();
}

void BackupConfig::set(const std::string &key, const std::string &value) {
    const auto firstDotIdx = key.find('.');
    if (firstDotIdx == std::string::npos || firstDotIdx == 0 || firstDotIdx == key.size()) {
        THROW_EXCEPTION("Invalid property key '" + key + "' (section.varname or section.subsection.varname)");
    }
    const std::string section{key.substr(0, firstDotIdx)};
    const auto lastDotIdx = key.rfind('.');
    const std::string subSection{
        firstDotIdx == lastDotIdx ? "" : key.substr(firstDotIdx + 1, lastDotIdx - firstDotIdx - 1)
    };
    const std::string variable{key.substr(lastDotIdx + 1)};
    set(section, subSection, variable, value);
}

namespace {
    struct parser {
        std::ifstream in_;
        std::string origLine_;
        std::string line_;
        uint32_t lineNo_{0};
        std::string section_{};
        std::string subSection_{};
        std::string variableName_{};
        std::string variableValue_{};
        // true if the last call to next resulted in a new variable
        bool newVariable_{false};
        std::vector<std::string> lines_{};

        explicit parser(const fs::path &file): in_{file} {
        }

        bool next() {
            newVariable_ = false;
            while (std::getline(in_, origLine_)) {
                line_ = origLine_;
                lines_.emplace_back(line_);
                ++lineNo_;
                ltrim(line_);
                rtrim(line_);
                if (line_.empty()) continue;
                switch (line_.front()) {
                    case ';':
                    case '#':
                        continue;
                    case '[':
                        section();
                        return true;
                    default:
                        value();
                        newVariable_ = true;
                        return true;
                }
            }
            return false;
        }

        void section() {
            if (line_.back() != ']') {
                THROW_EXCEPTION("Invalid section on line:" + std::to_string(lineNo_) + " '" + origLine_ + "'");
            }
            line_.erase(0, 1);
            line_.erase(line_.size() - 1);
            ltrim(line_);
            rtrim(line_);

            std::string::size_type i = 0;
            for (; i < line_.size(); ++i) {
                if (std::isalnum(line_.at(i))) continue;
                if (line_.at(i) == ' ') break;
            }
            section_ = line_.substr(0, i);
            if (section_.empty()) {
                THROW_EXCEPTION("Emplty section on line:" + std::to_string(lineNo_) + " '" + origLine_ + "'");
            }
            to_lower(section_);
            line_.erase(0, i);
            ltrim(line_);

            if (line_.empty()) {
                subSection_ = "";
                return;
            }
            if (!(line_.front() == '"' && line_.back() == '"')) {
                THROW_EXCEPTION("Invalid sub-section on line:" + std::to_string(lineNo_) + " '" + origLine_ + "'");
            }
            subSection_ = line_.substr(1, line_.size() - 2);
        }

        void value() {
            if (!std::isalpha(line_.front())) {
                THROW_EXCEPTION("Variable name must start an alphabetic character on line:" + std::to_string(lineNo_)
                    + " '" + origLine_ + "'");
            }
            std::string::size_type i = 0;
            for (; i < line_.size(); ++i) {
                if (!(std::isalnum(line_.at(i)) || line_.at(i) == '-')) break;
            }
            variableName_ = line_.substr(0, i);

            if (variableName_.empty()) {
                THROW_EXCEPTION("Emplty variable name on line:" + std::to_string(lineNo_) + " '" + origLine_ + "'");
            }
            line_.erase(0, i);
            ltrim(line_);

            if (line_.empty()) {
                variableValue_ = "true";
                return;
            }
            if (line_.front() != '=') {
                THROW_EXCEPTION("Invalid variable line (missing '=') on line:" + std::to_string(lineNo_)
                    + " '" + origLine_ + "'");
            }
            line_.erase(0, 1);
            ltrim(line_);
            variableValue_ = line_;
        }
    };
}

void BackupConfig::parse() {
    sections_.clear();
    list_.clear();

    parser p{file_};
    while (p.next()) {
        auto &section = sections_.contains(p.section_)
                            ? sections_.at(p.section_)
                            : sections_.emplace(p.section_, p.section_).first->second;
        auto &subSection = section.subSections_.contains(p.subSection_)
                               ? section.subSections_.at(p.subSection_)
                               : section.subSections_.emplace(p.subSection_,
                                                              sub_section(p.lineNo_, p.subSection_)).first->second;
        if (p.newVariable_) {
            subSection.values_.insert_or_assign(p.variableName_, value{p.lineNo_, p.variableValue_});
            std::stringstream key;
            if (!p.section_.empty()) key << p.section_ << ".";
            if (!p.subSection_.empty()) key << p.subSection_ << ".";
            key << p.variableName_;
            list_.insert_or_assign(key.str(), p.variableValue_);
        }
    }
    lines_ = p.lines_;
}
