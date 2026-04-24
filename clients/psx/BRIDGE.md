# PS1 <-> mixnetd (PC bridge + serial)

The PlayStation does not run a TCP stack in this project. The console and **mixnetd** (see [`../../server/`](../../server)) are joined by a **PC bridge** that shuffles the same 8-bit stream you already use in [`../common/mixnet_line.c`](../common/mixnet_line.c):

- **PS1 to PC:** `HELLO`, `ROOMS`, `MSG` (lines with `\n`) leave **SIO1** (115200 8N1) as raw bytes.  
- **PC to PS1:** whatever **mixnetd** sends (`INFO`, `OK`, `PRIVMSG` …) is written to the same serial port so `mixnet_nav_on_incoming_line` can show it.

`mixnet://host:port/room` in the navigator is a **label** for the user and for matching your PC config: the bridge is what actually opens a TCP connection to that host. Set defaults in [`../include/mixnet_config.h`](../include/mixnet_config.h) (`MIXNET_DEFAULT_HOST`, `MIXNET_DEFAULT_PORT`, `MIXNET_DEFAULT_ROOM`) or use `:loc` in-app (string entry is pad-focused; for long hosts, prefer editing the config and rebuilding, or a PC-side workflow).

## 1) Start the server

From the repo (see [`../../server/README.md`](../../server)):

```text
path\to\mixnetd.exe
```

Optional: `mixnetd 9000` to listen on another port (then set port in the bridge and in `mixnet_config.h` to match).

## 2) Run the serial bridge (Windows)

- Build the PS1 image: [`build-psyq.bat`](build-psyq.bat) &rarr; `out\mixnet.cpe` (or PS-EXE/ISO from [`pack-pcsx-redux`](pack-pcsx-redux.ps1)).  
- Install Python deps: `pip install -r ..\bridge\requirements.txt`  
- Map the **emulator’s serial** to a **COM** port, or use **TCP serial** if your build exposes one (DuckStation / PCSX-Redux: check the emulator’s **serial / link** settings and the manual for the exact “listen host:port” or COM mapping).

```powershell
cd E:\…\68mixCross\clients\bridge
pip install -r requirements.txt
python mixnet_serial_bridge.py --serial COM5 --server 127.0.0.1 --port 19677
```

**TCP end of an emulator** (example):

```powershell
python mixnet_serial_bridge.py --serial-tcp 127.0.0.1:5678 --server 127.0.0.1 --port 19677
```

Use the same **115200 8N1** line configuration in the bridge as in `mixnet_stub.c` (`_sio_control(…, 115200)`).

## 3) Use the PS1 UI

- **D-pad / face** shortcuts match the in-app help (`:h` on Select, `:g` on Start, `1`…`6`, `PING` on R1, …).  
- **Chat from a PC client:** run another program (e.g. [`../posix/mixnet.c`](../posix/mixnet.c)) against the **same** mixnetd. Those messages are visible in the same rooms as the PlayStation. Your PS1 sends and receives through SIO plus this bridge, not a second stack.  
- **Quit:** `:q` in the app when input is available, or end the homebrew; closing the bridge stops the link.

## 4) Real hardware (optional)

A **link cable** or **UART to USB** adapter on the same SIO pins as the dev kit, **115200 8N1**, connects to the PC COM port the bridge uses. **Use your kit’s schematics;** the repo does not provide PCB routing.

## 5) Troubleshooting

| Symptom | Check |
|--------|--------|
| Black / no `INFO` on PS1 | mixnetd running? Bridge `--server` / `--port`? Same baud **115200**? |
| `mixnetd` no connection | Windows firewall, `0.0.0.0` listen, correct IP (not `127.0.0.1` from another device). |
| Emu shows no serial | Emu serial enabled; correct COM in Device Manager, or use `--serial-tcp` if supported. |
| CPE2X 16-bit error on 64-bit | Expected; use `.cpe` in emulators or [pack-pcsx-redux](pack-pcsx-redux.ps1) for a PS-EXE. |

For PSYQ build details, see [BUILD-PS1.md](BUILD-PS1.md).
