#ifndef OBC_CommandAuthorityCatalog_HPP
#define OBC_CommandAuthorityCatalog_HPP

#include "OBC/Components/CommandIngressAuthority/CommandAuthorityTypes.hpp"

namespace OBC {

struct CommandAuthorityCatalogEntry {
    FwOpcodeType opcode;
    const char* name;
    AuthorityCommandClass commandClass;
    AuthorityResourceLabel resource;
    bool uhfBackupAllowed;
};

const CommandAuthorityCatalogEntry* findCommandAuthorityCatalogEntry(FwOpcodeType opcode);

FwSizeType getCommandAuthorityCatalogEntryCount();

AuthorityDecision evaluateCommandAuthority(const AuthorityConfig& config, FwOpcodeType opcode);

}  // namespace OBC

#endif
