/* Shared 68mixCross v0 wire tokens (US-ASCII) — all clients + bridges should use these. */
#ifndef MIXNET_PROTO_H
#define MIXNET_PROTO_H

/* Verbs (client -> server) */
#define MX_HELLO "HELLO"
#define MX_JOIN  "JOIN"
#define MX_PART  "PART"
#define MX_WHO   "WHO"
#define MX_ROOMS "ROOMS"
#define MX_MSG   "MSG"
#define MX_PING  "PING"
#define MX_QUIT  "QUIT"

/* Prefixes (server -> client) */
#define MX_P_OK   "OK "
#define MX_P_ERR  "ERR "
#define MX_P_INFO "INFO "
#define MX_P_PRIV "PRIVMSG "

/* Meta for interactive UIs */
#define MX_CMD_QUIT ":quit"

#endif
