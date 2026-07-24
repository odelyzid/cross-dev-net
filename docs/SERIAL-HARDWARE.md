# Serial hardware reference — PS1, Amiga, Genesis

Pinouts, cable wiring, and the full packet opcode catalog for connecting your hardware to mixnetd via the serial bridge.

---

## Wiring philosophy

All consoles connect to a **PC running `mixnet_serial_bridge.py`**, which relays bytes to/from mixnetd. The bridge is framing-agnostic — it just pipes bytes. The console does the framing (`mixnet_line.c` or `mixnet_packet.c`).

```
Console UART  ── 3-wire serial ──▶  PC COM port  ── bridge.py ──TCP──▶  mixnetd
                         115200 8N1, no flow control
```

Only **three wires** are needed: TXD, RXD, GND.

---

## PlayStation 1 — SIO1

The PS1's serial port (SIO1) appears on different physical connectors depending on the model:

| Model | Connector | Location |
|-------|-----------|----------|
| SCPH-100x / 500x | Mini-DIN 8 (round, female) | Back panel, labelled "SERIAL I/O" |
| SCPH-5500+ | Motherboard header JSTM-1 (3×2 pin) | Near the RF modulator |
| SCPH-750x+ | Motherboard pads only | Near the AV multi-out |

### Mini-DIN 8 pinout (SCPH-100x / 500x)

```
     _______
    / 8 7 6 \          View looking INTO the console port
   | 5     4 |         (female on console)
   |   3    |
    \  2 1 /
     ¯¯¯¯¯
```

| Pin | Signal | Direction | Notes |
|-----|--------|-----------|-------|
| 1   | DCD    | ← in      | Not connected in 3-wire |
| 2   | RXD    | ← in      | **Receive data from PC** |
| 3   | TXD    | → out     | **Transmit data to PC** |
| 4   | DTR    | → out     | Optional, asserted by `_sio_control` |
| 5   | GND    | —         | **Signal ground** |
| 6   | DSR    | ← in      | Not connected in 3-wire |
| 7   | RTS    | → out     | Optional, asserted by `_sio_control` |
| 8   | CTS    | ← in      | Not connected in 3-wire |

### 3-wire cable (PS1 → PC USB-serial adapter)

```
PS1 mini-DIN 8               USB-serial adapter (DB9 male)
Pin 2 (RXD) ──────────────── Pin 3 (TXD)
Pin 3 (TXD) ──────────────── Pin 2 (RXD)
Pin 5 (GND) ──────────────── Pin 5 (GND)
```

For the motherboard header (SCPH-5500+), use female dupont jumpers:

```
JSTM-1 header (3×2 pin, console side):
    +-------+
    | DSR o o DCD |  1=DCD, 2=DSR
    | CTS o o RXD |  3=RXD, 4=CTS
    | TXD o o GND |  5=GND, 6=TXD
    +-------+
    | RTS o o DTR |  7=DTR, 8=RTS  (second row on some boards)
    +-------+
```

Only RXD (pin 3), TXD (pin 6), GND (pin 5) are needed.

### PS1 SIO init (for reference)

From `clients/psx/mixnet_stub.c`:

```c
_sio_control(1, 1, CR_RXEN | CR_TXEN | CR_RTS | CR_DTR);   // enable + flow control
_sio_control(1, 2, MR_SB_11 | MR_CHLEN_8 | MR_BR_16);       // 1 stop, 8 bit, 16×
_sio_control(1, 3, 115200u);                                   // 115200 baud
```

---

## Amiga — RS-232 serial

Amiga 500/600/1200 and 2000/3000/4000 all have a built-in RS-232 serial port.

| Connector | Amiga model | Pinout |
|-----------|-------------|--------|
| DB25 female | A500, A2000, A3000, A4000 | Standard RS-232 DCE |
| DB9 male | A600, A1200 | Same signals, compact |

### DB25 pinout (A500 / A2000 / A4000)

```
    ┌───────────────────┐
    \ 14  .....    25  /      View looking INTO the Amiga
     \ 1  ......  13 /       (DB25 female, DCE)
      └───────────────┘
```

| Pin | Signal | Direction | Notes |
|-----|--------|-----------|-------|
| 1   | FG     | —         | Frame ground (shield) |
| 2   | TXD    | → out     | **Transmit data to PC** |
| 3   | RXD    | ← in      | **Receive data from PC** |
| 4   | RTS    | → out     | Optional |
| 5   | CTS    | ← in      | Optional |
| 6   | DSR    | ← in      | Optional |
| 7   | SG     | —         | **Signal ground** |
| 8   | DCD    | ← in      | Optional |
| 20  | DTR    | → out     | Optional |
| 22  | RI     | ← in      | Not used |

### DB9 pinout (A1200 / A600)

```
    ┌───────┐
    \ 1 2 3 /      View looking into the Amiga
     \ 4 5 /       (DB9 male, DCE)
      \ 6 /
       \ 7 /
        \ 8 /
         \ 9 /
          └─┘
```

| Pin | Signal | Direction | Notes |
|-----|--------|-----------|-------|
| 1   | DCD    | ← in      | Optional |
| 2   | RXD    | ← in      | **Receive data from PC** |
| 3   | TXD    | → out     | **Transmit data to PC** |
| 4   | DTR    | → out     | Optional |
| 5   | SG     | —         | **Signal ground** |
| 6   | DSR    | ← in      | Optional |
| 7   | RTS    | → out     | Optional |
| 8   | CTS    | ← in      | Optional |
| 9   | RI     | ← in      | Not used |

### Null-modem 3-wire (Amiga → PC USB-serial)

**Amiga DB25 (female)** → **PC USB-serial adapter (DB9 male or USB)**:

```
Amiga DB25                USB-serial (DB9)
Pin 2 (TXD) ───────────── Pin 3 (RXD)
Pin 3 (RXD) ───────────── Pin 2 (TXD)
Pin 7 (GND) ───────────── Pin 5 (GND)
```

For Amiga DB9 (A1200), the same signals on different pins:

```
Amiga DB9                 USB-serial (DB9)
Pin 3 (TXD) ───────────── Pin 3 (RXD)
Pin 2 (RXD) ───────────── Pin 2 (TXD)
Pin 5 (GND) ───────────── Pin 5 (GND)
```

### AmigaOS serial configuration

The Amiga serial device is opened at 115200 8N1. Example from a C client:

```c
struct MsgPort *mp = CreatePort(NULL, 0);
struct IOExtSer *io = CreateExtIO(mp, sizeof(struct IOExtSer));
OpenDevice("serial.device", 0, (struct IORequest *)io, 0);
io->io_SerFlags = SERF_RAD_BOOGIE | SERF_XDISABLED;  // no handshake
io->io_Baud = 115200;
io->io_CtlChar[0] = 0;  // no EOF
io->io_ExtFlags |= SETF_QUEUEBRK | SETF_TD_6WIRE;
```

For the TCP client (`clients/amiga/mixnet.c`), no serial is used — it connects directly to mixnetd over TCP (AmigaOS + AmiTCP/Roadshow).

---

## Genesis / Mega Drive — flashcart serial

The stock Genesis does **not** have a built-in serial port. Serial requires a flashcart with USB serial passthrough.

| Flashcart | Interface | Appears as |
|-----------|-----------|------------|
| Mega EverDrive Pro | Micro-USB → FTDI | Virtual COM port (PC side) |
| Terraonion Mega SD | Micro-USB → FTDI | Virtual COM port (PC side) |
| Custom UART | Expansion port pins | TTL-level serial |

### EverDrive Pro — no wiring needed

Plug the micro-USB cable from the EverDrive to the PC. The PC sees a COM port at 115200 8N1. The Genesis talks through the EverDrive's FPGA bridge to the USB-serial chip.

The Genesis side `mixnet_line.c` wrappers are at `clients/genesis/mixnet_line.{c,h}`. On-cart code uses SGDK `UART_*` functions or raw I/O to the serial hardware.

### Expansion port pins (for custom hardware)

The Genesis expansion port (32X/CD connector on the bottom) has accessible signals:

```
   Front of console
┌──────────────────────┐
│  1  3  5  7  9  ... │
│  2  4  6  8  10 ... │
└──────────────────────┘
```

| Pin | Signal | Notes |
|-----|--------|-------|
| 1   | +5V    | Power |
| 4   | GND    | Ground |
| 20  | TXD    | 68000 SIO serial out |
| 22  | RXD    | 68000 SIO serial in |
| 24  | GND    | Ground |

The 68000's built-in SIO is memory-mapped at `$FFE0C1` (SIO data A) and `$FFE0C3` (SIO command/status A). SGDK does not expose these directly; you'd read/write via `*(volatile u8*)0xFFE0C1`.

---

## Application data flow

All three platforms use the same byte-oriented protocol stack:

```
┌────────────────────┐
│  User application  │  ← protocol verbs (HELLO, JOIN, MSG, …)
├────────────────────┤
│  mixnet_packet.c   │  ← binary framing or mixnet_line.c for v0 text TX
├────────────────────┤
│  mixnet_serial_    │  ← PC-side; pipes bytes between COM and TCP
│  bridge.py         │
├────────────────────┤
│  mixnetd (server)  │  ← TCP hub, auto-detects binary vs text on first byte
└────────────────────┘
```

Each platform implements the bottom hardware UART layer:
- **PS1:** `sio_putb()` / `sio_pump_incoming()` via PSYQ `_sio_control()`
- **Amiga:** `serial.device` (or direct TCP socket)
- **Genesis:** SGDK UART or raw SIO register I/O

---

## Packet opcodes (wire format type field)

Reference for the `type` byte in the binary packet header (`0x58 type flags len_hi len_lo payload...`).

| Hex | Dec | Name | Payload | Direction |
|-----|-----|------|---------|-----------|
| 0x01 | 1   | `PKT_HELLO` | nickname | client → server |
| 0x02 | 2   | `PKT_HELLO_OK` | session_id (hex) | server → client |
| 0x03 | 3   | `PKT_ERR` | error code text | server → client |
| 0x04 | 4   | `PKT_JOIN` | room name | client → server |
| 0x05 | 5   | `PKT_JOIN_OK` | room name | server → client |
| 0x06 | 6   | `PKT_MSG` | chat text | client → server |
| 0x07 | 7   | `PKT_PRIVMSG` | `<nick> <text>` | server → client |
| 0x08 | 8   | `PKT_PING` | (empty) | client → server |
| 0x09 | 9   | `PKT_PONG` | (empty) | server → client |
| 0x0A | 10  | `PKT_QUIT` | (empty) | client → server |
| 0x0B | 11  | `PKT_QUIT_OK` | (empty) | server → client |
| 0x0C | 12  | `PKT_PART` | (empty) | client → server |
| 0x0D | 13  | `PKT_PART_OK` | (empty) | server → client |
| 0x0E | 14  | `PKT_WHO` | (empty) | client → server |
| 0x0F | 15  | `PKT_WHO_RESP` | `<room> <csv nicks>` | server → client |
| 0x10 | 16  | `PKT_ROOMS` | (empty) | client → server |
| 0x11 | 17  | `PKT_ROOMS_RESP` | csv room list | server → client |
| 0x12 | 18  | `PKT_INFO` | info text | server → client |
| 0x80+| 128+ | Application-defined | game state, file chunks | free |

Only the first byte of any packet determines the mode:
- `0x58` → binary packet mode (all subsequent bytes parsed as packets)
- Any other value → v0 text mode (`\n`-delimited ASCII)

---

## Bridge command reference

```
# Physical serial (PS1 via null-modem cable, Amiga, etc.)
python clients/bridge/mixnet_serial_bridge.py --serial COM3 --baud 115200 \
    --server 127.0.0.1 --port 19677

# Emulator serial tunnel (DuckStation / PCSX-Redux)
python clients/bridge/mixnet_serial_bridge.py --serial-tcp 127.0.0.1:5678 \
    --server 127.0.0.1 --port 19677
```

---

## See also

- `clients/bridge/mixnet_serial_bridge.py` — the Python bridge source
- `clients/common/mixnet_packet.h` — packet type enum and API
- `.cursor/.documentation/cross-net/packet-v1.mdc` — binary packet format spec
- `clients/psx/BRIDGE.md` — PS1-specific bridge setup and troubleshooting
