#include "OBC/Components/CommandIngressAuthority/CommandAuthKeystore.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>

namespace OBC {

namespace {

constexpr const char* DEFAULT_COMMAND_AUTH_KEYSTORE_PATH = "config/security/command-auth.ini";
constexpr std::size_t EXPECTED_KEY_HEX_LENGTH = 64U;
constexpr std::size_t MODULE_SERIAL_SIZE = 16U;

std::string trim(const std::string& value) {
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
    const auto end =
        std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

bool decodeHexNibble(char ch, U8& value) {
    if (ch >= '0' && ch <= '9') {
        value = static_cast<U8>(ch - '0');
        return true;
    }
    if (ch >= 'a' && ch <= 'f') {
        value = static_cast<U8>(10 + ch - 'a');
        return true;
    }
    if (ch >= 'A' && ch <= 'F') {
        value = static_cast<U8>(10 + ch - 'A');
        return true;
    }
    return false;
}

bool parseHexKey(const std::string& hex, CommandAuthKeystoreEntry& entry) {
    if (hex.size() != EXPECTED_KEY_HEX_LENGTH) {
        return false;
    }
    const std::size_t byteCount = hex.size() / 2U;
    if (byteCount > CommandAuthConfig::MAX_KEY_BYTES) {
        return false;
    }
    entry.keyLength = static_cast<U8>(byteCount);
    std::memset(entry.keyBytes, 0, sizeof(entry.keyBytes));
    for (std::size_t index = 0U; index < byteCount; ++index) {
        U8 hi = 0U;
        U8 lo = 0U;
        if (!decodeHexNibble(hex[index * 2U], hi) || !decodeHexNibble(hex[index * 2U + 1U], lo)) {
            entry.keyLength = 0U;
            return false;
        }
        entry.keyBytes[index] = static_cast<U8>((hi << 4U) | lo);
    }
    return true;
}

CommandAuthKeystoreEntry* sectionEntry(CommandAuthKeystore& keystore, const std::string& section) {
    if (section == "sband") {
        return &keystore.sband;
    }
    if (section == "uhf") {
        return &keystore.uhf;
    }
    return nullptr;
}

bool validateEntry(const CommandAuthKeystoreEntry& entry, const char* label, std::string& error) {
    if (!entry.valid || entry.keyLength == 0U) {
        std::ostringstream message;
        message << "invalid command auth keystore entry for " << label;
        error = message.str();
        return false;
    }
    return true;
}

}  // namespace

const char* defaultCommandAuthKeystorePath() {
    static const std::string resolved = []() {
        {
            std::ifstream releaseKeystore(DEFAULT_COMMAND_AUTH_KEYSTORE_PATH);
            if (releaseKeystore.is_open()) {
                return std::string(DEFAULT_COMMAND_AUTH_KEYSTORE_PATH);
            }
        }
        std::string sourcePath(__FILE__);
        const std::string suffix = "/OBC/Components/CommandIngressAuthority/CommandAuthKeystore.cpp";
        const std::size_t suffixPos = sourcePath.rfind(suffix);
        if (suffixPos != std::string::npos) {
            sourcePath.resize(suffixPos);
            const std::string sourceTreeCandidate = sourcePath + "/" + DEFAULT_COMMAND_AUTH_KEYSTORE_PATH;
            std::ifstream sourceTreeKeystore(sourceTreeCandidate);
            if (sourceTreeKeystore.is_open()) {
                return sourceTreeCandidate;
            }
        }
        return std::string(DEFAULT_COMMAND_AUTH_KEYSTORE_PATH);
    }();
    return resolved.c_str();
}

bool loadCommandAuthKeystore(const std::string& path, CommandAuthKeystore& keystore, std::string& error) {
    keystore = CommandAuthKeystore();
    error.clear();

    std::ifstream input(path);
    if (!input.is_open()) {
        error = "unable to open command auth keystore";
        return false;
    }

    std::string section;
    std::string line;
    std::size_t lineNumber = 0U;
    while (std::getline(input, line)) {
        lineNumber += 1U;
        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }
        if (trimmed.front() == '[') {
            if (trimmed.back() != ']') {
                error = "invalid command auth keystore section header";
                return false;
            }
            section = trim(trimmed.substr(1, trimmed.size() - 2U));
            if (sectionEntry(keystore, section) == nullptr) {
                std::ostringstream message;
                message << "unsupported command auth keystore section on line " << lineNumber;
                error = message.str();
                return false;
            }
            continue;
        }

        const std::size_t separator = trimmed.find('=');
        if (separator == std::string::npos) {
            std::ostringstream message;
            message << "invalid command auth keystore assignment on line " << lineNumber;
            error = message.str();
            return false;
        }

        const std::string key = trim(trimmed.substr(0, separator));
        const std::string value = trim(trimmed.substr(separator + 1U));
        if (section.empty()) {
            if (key == "module_serial") {
                keystore.moduleSerial = value;
                continue;
            }
            std::ostringstream message;
            message << "unsupported top-level command auth keystore key on line " << lineNumber;
            error = message.str();
            return false;
        }

        CommandAuthKeystoreEntry* entry = sectionEntry(keystore, section);
        if (entry == nullptr) {
            error = "internal command auth keystore section resolution failed";
            return false;
        }

        if (key == "key_hex") {
            if (!parseHexKey(value, *entry)) {
                std::ostringstream message;
                message << "invalid key_hex in command auth keystore on line " << lineNumber;
                error = message.str();
                return false;
            }
        } else {
            std::ostringstream message;
            message << "unsupported command auth keystore key on line " << lineNumber;
            error = message.str();
            return false;
        }
        entry->valid = entry->keyLength != 0U;
    }

    if (keystore.moduleSerial.size() != MODULE_SERIAL_SIZE) {
        error = "module_serial must be 16 bytes";
        return false;
    }
    if (!validateEntry(keystore.sband, "sband", error) || !validateEntry(keystore.uhf, "uhf", error)) {
        return false;
    }
    return true;
}

}  // namespace OBC
