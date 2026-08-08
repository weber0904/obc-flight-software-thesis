#ifndef OBC_CommandSessionSequence_HPP
#define OBC_CommandSessionSequence_HPP

#include "OBC/Components/CommandIngressAuthority/CommandAuthorityTypes.hpp"

namespace OBC {

struct CommandSessionKey {
    FwIndexType ingressPort = 0;
    AuthorityLinkIdentity linkIdentity = AuthorityLinkIdentity::UNKNOWN;
    AuthorityLinkRole linkRole = AuthorityLinkRole::UNKNOWN;
    U32 sessionId = 0;
};

enum class CommandSequenceResult : U32 {
    ACCEPTED = 0,
    REJECTED_NOT_INCREASING = 1,
    REJECTED_TABLE_FULL = 2,
};

enum class CommandSequenceRejectReason : U32 {
    NONE = 0,
    NOT_INCREASING = 1,
    WINDOW_FULL = 2,
};

enum class CommandSessionRejectReason : U32 {
    NONE = 0,
    NOT_OPEN = 1,
    SESSION_MISMATCH = 2,
    ALREADY_OPEN = 3,
    BAD_OPEN_SEQUENCE = 4,
    LEGACY_LIFECYCLE_UNSUPPORTED = 5,
    WINDOW_FULL = 6,
    STALE_REPLAY = 7,
    PERSISTENT_STATE_UNAVAILABLE = 8,
};

class CommandSequenceWindow {
  public:
    static constexpr FwSizeType MAX_SESSIONS = 16;

    CommandSequenceResult evaluateAndAccept(const CommandSessionKey& key, U32 sequenceNumber);

    bool resetSession(const CommandSessionKey& key);

    void resetSource(FwIndexType ingressPort, AuthorityLinkIdentity identity, AuthorityLinkRole role);

    void resetAll();

  private:
    struct SessionEntry {
        bool valid = false;
        CommandSessionKey key;
        U32 lastAccepted = 0;
    };

    static bool keysEqual(const CommandSessionKey& lhs, const CommandSessionKey& rhs);

    SessionEntry* findEntry(const CommandSessionKey& key);

    SessionEntry* allocateEntry(const CommandSessionKey& key);

  private:
    SessionEntry m_entries[MAX_SESSIONS];
};

}  // namespace OBC

#endif
