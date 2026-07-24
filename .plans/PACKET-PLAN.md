# Generic packet structure — implementation plan

**Goal:** Replace the ad-hoc `\n`-delimited text framing with a length-prefixed binary packet layer, shared across all clients and transport paths.

---

## Commit discipline

After every phase below, make a clean commit and push to `origin/main`. Each phase must pass its own verification step before commit.

---

## Phase 1 — Shared packet library + self-test

**Files created:**
- `clients/common/mixnet_packet.h` — types, enums, API declarations
- `clients/common/mixnet_packet.c` — implementation + self-test

**Packet format:**

```
Byte:    0         1         2         3         4...n
        +---------+---------+---------+---------+------------+
        | 0x58    |  type   |  flags  |  length (16-bit BE) |
        +---------+---------+---------+---------+------------+
        |  payload (length bytes)                             |
        +----------------------------------------------------+
```

- **magic (1):** `0x58` — sync marker, also binary-mode discriminator
- **type (1):** packet type enum (see catalog below)
- **flags (1):** bitfield — bit 0 = ACK request, bit 1 = ACK response, rest reserved
- **length (2):** payload length, big-endian (network byte order)
- **payload (N):** raw bytes, meaning depends on `type`
- **No checksum** — relies on TCP or reliable UART

**Type catalog:**

| Range | Purpose | Examples |
|-------|---------|---------|
| 0x01–0x1F | v0 protocol mapping | HELLO, JOIN, MSG, PRIVMSG, PING, QUIT, ERR, INFO, PART, WHO, ROOMS |
| 0x20–0x7F | Reserved for future protocol extensions | — |
| 0x80–0xFF | Application/game types | Game state sync, file chunks, controller input |

**Header types (`mixnet_packet.h`):**

```c
#pragma once
/*...*/

typedef enum {
    PKT_HELLO     = 0x01,
    PKT_HELLO_OK  = 0x02,
    PKT_ERR       = 0x03,
    PKT_JOIN      = 0x04,
    PKT_JOIN_OK   = 0x05,
    PKT_MSG       = 0x06,
    PKT_PRIVMSG   = 0x07,
    PKT_PING      = 0x08,
    PKT_PONG      = 0x09,
    PKT_QUIT      = 0x0A,
    PKT_QUIT_OK   = 0x0B,
    PKT_PART      = 0x0C,
    PKT_PART_OK   = 0x0D,
    PKT_WHO       = 0x0E,
    PKT_WHO_RESP  = 0x0F,
    PKT_ROOMS     = 0x10,
    PKT_ROOMS_RESP= 0x11,
    PKT_INFO      = 0x12,
    PKT_APP_START = 0x80,
} mixnet_pkt_type_t;

typedef struct mixnet_pkt_rx {
    unsigned char buf[4];         // accumulate header
    int pos;                      // bytes accumulated so far
    int stage;                    // 0=header, 1=payload
    int need;                     // bytes still needed in current stage
    unsigned int payload_len;     // decoded length
} mixnet_pkt_rx_t;

// A full packet (header + payload contiguous).
// For max line compat, payload sits inline after the header.
typedef struct {
    unsigned char hdr[4];          // magic+type+flags+length
    unsigned char payload[MIXNET_MAX_LINE];
    int payload_len;               // actual length in this packet
} mixnet_pkt_t;
```

**API:**

```c
void mixnet_pkt_rx_init(mixnet_pkt_rx_t* s);

// Feed one received byte.
// Returns: 1 = full packet in out_pkt, 0 = need more, -1 = error
int mixnet_pkt_rx_byte(mixnet_pkt_rx_t* s, int byte_in, mixnet_pkt_t* out_pkt);

// Serialize a packet via tx callback (like mixnet_write_line).
typedef void (*mixnet_tx_fn)(void* user, int byte);
int mixnet_pkt_send(const mixnet_pkt_t* pkt, mixnet_tx_fn tx, void* user);

// Encode helpers: build a packet header
void mixnet_pkt_start(mixnet_pkt_t* pkt, int type, int flags);
int  mixnet_pkt_append(mixnet_pkt_t* pkt, const void* data, int len); // sets header length

// Convert between binary packet and v0 ASCII line (bidirectional).
int mixnet_pkt_to_text(const mixnet_pkt_t* pkt, char* out, size_t cap);
int mixnet_text_to_pkt(const char* line, mixnet_pkt_t* out_pkt);

// Self-test: returns 1 on pass, 0 on fail.
int mixnet_packet_selftest(void);
```

**Verification:**
```bash
gcc -std=c99 -Wall -Wextra -o /tmp/pkt_selftest \
    clients/common/pkt_selftest_wrapper.c clients/common/mixnet_packet.c \
    -Iclients/include -Iclients/common
/tmp/pkt_selftest && echo PASS || echo FAIL
```

**Commit message:** `phase 1: shared packet framing library + self-test`

---

## Phase 2 — Port TCP clients to shared packet layer

**Files modified:**
- `clients/win9x/mixnet.c` — replace inline `\n` scanner with `mixnet_pkt_rx_byte()`
- `clients/posix/mixnet.c` — same
- `clients/amiga/mixnet.c` — same
- `server/src/main.rs` — no changes yet (still speaks v0 text)

**What changes:**
- Replace the per-client `reader_main` byte loop that scans for `\n` with a call to `mixnet_pkt_rx_byte()`
- On full packet, convert back to text via `mixnet_pkt_to_text()` for display (clients still display text)
- TX side: call `mixnet_text_to_pkt()` wrapper before `send()` (optional optimization — clients could send raw text to keep v0 compat with mixnetd)

**Key constraint:** On the TX side, clients **still send ASCII v0 text** because mixnetd doesn't understand binary yet. Only the RX (reader) side switches to packet parsing. This is an interim step — the packet is parsed from the wire, converted to text for display, and everything works.

**Verification:**
```bash
# Build all three TCP clients, connect to mixnetd, exchange messages
gcc -std=c99 -O2 -Wall -o clients/win9x/mixnet.exe clients/win9x/mixnet.c -lwsock32
cc -O2 -pthread -o /tmp/mixnet-posix clients/posix/mixnet.c
# Manual: run mixnetd, connect with each client, verify chat works
```

**Commit message:** `phase 2: port TCP clients to shared packet RX framing`

---

## Phase 3 — Port serial clients to shared packet layer

**Files modified:**
- `clients/psx/mixnet_stub.c` — swap `mixnet_line_rx_byte()` → `mixnet_pkt_rx_byte()`
- `clients/n64/mixnet_stub.c` — same
- `clients/bridge/mixnet_serial_bridge.py` — no changes (transparent byte pipe)

**What changes:**
- PS1 Navigator RX path now uses `mixnet_pkt_rx_byte()`
- PS1 TX path: `mixnet_text_to_pkt()` wraps lines before SIO TX (or keep sending raw v0 text until mixnetd supports binary — same constraint as Phase 2)
- N64 stub: same swap + self-test update

**Verification:**
```bash
# PS1: build with CCPSX or PSYQ, run in emulator with serial-TCP tunnel
# N64: build with mips64-elf-gcc, verify self-test passes
python clients/bridge/mixnet_serial_bridge.py --serial-tcp 127.0.0.1:5000
# Connect emulator serial server, verify PS1 Navigator chat works
```

**Commit message:** `phase 3: port PS1/N64 serial clients to shared packet RX`

---

## Phase 4 — mixnetd binary mode (auto-detect)

**File modified:** `server/src/main.rs`

**What changes:**
- After `TcpListener::accept()`, peek the first byte of the stream
- If `0x58` → binary packet session mode
- Otherwise → existing v0 text path (unchanged)
- Binary mode: parse the 4-byte header + payload, dispatch to existing `handle_*` functions by converting via `pkt_to_text()` or adding typed dispatch
- Session tracking, nick/room state, broadcast — all unchanged (logic is the same, only framing differs)

**Rust implementation notes:**
- No external crates needed (zero deps policy)
- Wrapping reader: `peek()` via `TcpStream::peek()` — peek 1 byte, check for `0x58`, then either branch to binary reader or existing `BufReader::read_line()`
- Binary reader: read 4 header bytes, decode length, read payload, dispatch

**Verification:**
```bash
cargo build --release  # from server/ dir with MinGW PATH
# Run mixnetd, connect with a binary-mode client test, verify messages flow
```

**Commit message:** `phase 4: mixnetd auto-detect binary packet mode on 0x58`

---

## Phase 5 — Genesis integration (hardware test target)

**Files created:**
- `clients/genesis/src/mixnet_ui.c` — minimal SGDK chat UI (scrolling text plane)
- `clients/genesis/src/mixnet_serial.c` — UART TX/RX for flashcart serial

**Files referenced:**
- `clients/common/mixnet_packet.h` / `.c` — included via thin wrappers (same pattern as existing `mixnet_line`)

**What it does:**
- SGDK project gets a new source module `mixnet_ui.c`
- On boot: init UART serial, init packet RX state
- Main loop: poll UART bytes → `mixnet_pkt_rx_byte()` → on full packet, display text via SGDK `VDP_drawText()`
- D-pad/button input: build packets for HELLO, JOIN, MSG, etc. via `mixnet_pkt_send()`
- Requires: Mega EverDrive Pro serial (or similar flashcart with serial-over-USB)

**Verification:**
```bash
build.bat genesis  # from repo root
# Flash clients/genesis/out/rom.bin to EverDrive
# Run bridge.py --serial COMx --server 127.0.0.1 --port 19677
# Also run mixnetd
# Start Genesis — verify chat messages appear on screen
```

**Commit message:** `phase 5: Genesis basic serial chat UI over packet framing`

---

## Phase 6 — Amiga serial path

**File modified:** `clients/amiga/mixnet.c` — add `--serial COMx` option

**What changes:**
- Amiga client gains a serial bridge mode (parallel to existing TCP mode)
- RS-232 init (9600 or 115200, 8N1) via AmigaOS serial.device
- Wire `mixnet_pkt_rx_byte()` in the serial ISR or polling loop
- Existing TCP path unchanged

**Verification:**
```bash
m68k-amigaos-gcc -O2 -std=c99 -o mixnet clients/amiga/mixnet.c -lpthread -lsocket
# On real Amiga: connect RS-232 serial to PC, run bridge.py
# Run: mixnet --serial SER:
```

**Commit message:** `phase 6: Amiga serial bridge mode over packet framing`

---

## Phase 7 — Cleanup + documentation

**Files deleted:**
- `clients/common/mixnet_line.c` / `.h` — no longer needed (everything uses packet layer)
- Remove inline `\n` scanner code from all clients (already replaced in phases 2-3)

**Files updated:**
- `README.md` — update "Client implementation status" table, add packet format description
- `AGENTS.md` — add packet framing notes
- `.cursor/.documentation/cross-net/protocol-v0.mdc` — add binary packet mode section (or create v1 spec)

**Verification:**
```bash
# Full regression: build all clients, run self-tests, connect to mixnetd
build.bat all
python clients/bridge/mixnet_serial_bridge.py --help  # verify bridge still works
# All platforms still functional after deleting line layer
```

**Commit message:** `phase 7: remove legacy line framing, update docs for packet layer`

---

## File tree after all phases

```
clients/common/
  mixnet_packet.h           # packet types + API (new)
  mixnet_packet.c           # packet framing + self-test (new)
  pkt_selftest_wrapper.c    # standalone main() for self-test (new)
  [mixnet_line.c/h]         # deleted in phase 7

server/src/main.rs     # auto-detect 0x58 binary mode (phase 4)

clients/win9x/mixnet.c      # uses mixnet_packet (phase 2)
clients/posix/mixnet.c      # uses mixnet_packet (phase 2)
clients/amiga/mixnet.c      # uses mixnet_packet, +serial mode (phases 2+6)
clients/psx/mixnet_stub.c   # uses mixnet_packet (phase 3)
clients/n64/mixnet_stub.c   # uses mixnet_packet (phase 3)
clients/genesis/src/         # +mixnet_ui.c +mixnet_serial.c (phase 5)

no changes to:
  clients/bridge/       # transparent byte pipe, framing-agnostic
  clients/include/      # protocol tokens unchanged
```
