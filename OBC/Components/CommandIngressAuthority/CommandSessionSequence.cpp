#include "OBC/Components/CommandIngressAuthority/CommandSessionSequence.hpp"

namespace OBC {

CommandSequenceResult CommandSequenceWindow::evaluateAndAccept(const CommandSessionKey& key, U32 sequenceNumber) {
    SessionEntry* entry = this->findEntry(key);
    if (entry == nullptr) {
        entry = this->allocateEntry(key);
        if (entry == nullptr) {
            return CommandSequenceResult::REJECTED_TABLE_FULL;
        }
        entry->lastAccepted = sequenceNumber;
        return CommandSequenceResult::ACCEPTED;
    }

    if (sequenceNumber <= entry->lastAccepted) {
        return CommandSequenceResult::REJECTED_NOT_INCREASING;
    }

    entry->lastAccepted = sequenceNumber;
    return CommandSequenceResult::ACCEPTED;
}

bool CommandSequenceWindow::resetSession(const CommandSessionKey& key) {
    SessionEntry* entry = this->findEntry(key);
    if (entry == nullptr) {
        return false;
    }
    *entry = SessionEntry();
    return true;
}

void CommandSequenceWindow::resetSource(FwIndexType ingressPort,
                                        AuthorityLinkIdentity identity,
                                        AuthorityLinkRole role) {
    for (FwSizeType i = 0; i < MAX_SESSIONS; ++i) {
        SessionEntry& entry = this->m_entries[i];
        if (entry.valid && entry.key.ingressPort == ingressPort && entry.key.linkIdentity == identity &&
            entry.key.linkRole == role) {
            entry = SessionEntry();
        }
    }
}

void CommandSequenceWindow::resetAll() {
    for (FwSizeType i = 0; i < MAX_SESSIONS; ++i) {
        this->m_entries[i] = SessionEntry();
    }
}

bool CommandSequenceWindow::keysEqual(const CommandSessionKey& lhs, const CommandSessionKey& rhs) {
    return lhs.ingressPort == rhs.ingressPort && lhs.linkIdentity == rhs.linkIdentity && lhs.linkRole == rhs.linkRole &&
           lhs.sessionId == rhs.sessionId;
}

CommandSequenceWindow::SessionEntry* CommandSequenceWindow::findEntry(const CommandSessionKey& key) {
    for (FwSizeType i = 0; i < MAX_SESSIONS; ++i) {
        if (this->m_entries[i].valid && keysEqual(this->m_entries[i].key, key)) {
            return &this->m_entries[i];
        }
    }
    return nullptr;
}

CommandSequenceWindow::SessionEntry* CommandSequenceWindow::allocateEntry(const CommandSessionKey& key) {
    for (FwSizeType i = 0; i < MAX_SESSIONS; ++i) {
        if (!this->m_entries[i].valid) {
            this->m_entries[i].valid = true;
            this->m_entries[i].key = key;
            return &this->m_entries[i];
        }
    }
    return nullptr;
}

}  // namespace OBC
