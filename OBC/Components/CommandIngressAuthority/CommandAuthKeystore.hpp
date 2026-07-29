#ifndef OBC_COMMAND_AUTH_KEYSTORE_HPP
#define OBC_COMMAND_AUTH_KEYSTORE_HPP

#include "OBC/Components/CommandIngressAuthority/CommandAuthorityTypes.hpp"

#include <string>

namespace OBC {

struct CommandAuthKeystoreEntry {
    bool valid = false;
    U8 keyLength = 0U;
    U8 keyBytes[CommandAuthConfig::MAX_KEY_BYTES] = {};
};

struct CommandAuthKeystore {
    std::string moduleSerial;
    CommandAuthKeystoreEntry sband = {};
    CommandAuthKeystoreEntry uhf = {};
};

const char* defaultCommandAuthKeystorePath();

bool loadCommandAuthKeystore(const std::string& path, CommandAuthKeystore& keystore, std::string& error);

}  // namespace OBC

#endif
