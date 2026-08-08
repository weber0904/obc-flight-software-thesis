/*
 * Repo override for active TopCcsds command inventory.
 */

#ifndef OBC_CONFIG_COMMANDDISPATCHERIMPLCFG_HPP_
#define OBC_CONFIG_COMMANDDISPATCHERIMPLCFG_HPP_

enum {
    CMD_DISPATCHER_DISPATCH_TABLE_SIZE = 160,  // !< Active TopCcsds command opcode registry capacity
    CMD_DISPATCHER_SEQUENCER_TABLE_SIZE = 25,  // !< Commands in progress awaiting completion
};

#endif
