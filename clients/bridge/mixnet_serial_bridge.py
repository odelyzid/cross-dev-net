#!/usr/bin/env python3
"""
Bidirectional 8-bit pipe: PS1 SIO (or an emulator's TCP serial) <-> mixnetd TCP.
Lines are framed in software on the console; the bridge is a transparent byte stream.

  pip install pyserial

  python mixnet_serial_bridge.py --serial COM3 --server 127.0.0.1 --port 19677
  python mixnet_serial_bridge.py --serial /dev/ttyUSB0
  python mixnet_serial_bridge.py --serial-tcp 127.0.0.1:5000
"""
from __future__ import annotations

import argparse
import socket
import sys
import threading


def bridge_tcp_to_tcp(emu_sock: socket.socket, srv_sock: socket.socket) -> None:
    def a_to_b() -> None:
        try:
            while True:
                b = emu_sock.recv(4096)
                if not b:
                    return
                srv_sock.sendall(b)
        except (BrokenPipeError, OSError, ConnectionError):
            pass

    def b_to_a() -> None:
        try:
            while True:
                b = srv_sock.recv(4096)
                if not b:
                    return
                emu_sock.sendall(b)
        except (BrokenPipeError, OSError, ConnectionError):
            pass

    t1 = threading.Thread(target=a_to_b, daemon=True)
    t2 = threading.Thread(target=b_to_a, daemon=True)
    t1.start()
    t2.start()
    t1.join()
    t2.join()


def bridge_serial_to_tcp(ser, srv_sock: socket.socket) -> None:
    def ser_to_s() -> None:
        try:
            while True:
                b = ser.read(4096)
                if b:
                    srv_sock.sendall(b)
        except (BrokenPipeError, OSError, ConnectionError, TypeError, ValueError):
            pass

    def s_to_ser() -> None:
        try:
            while True:
                b = srv_sock.recv(4096)
                if not b:
                    return
                ser.write(b)
        except (BrokenPipeError, OSError, ConnectionError):
            pass

    t1 = threading.Thread(target=ser_to_s, daemon=True)
    t2 = threading.Thread(target=s_to_ser, daemon=True)
    t1.start()
    t2.start()
    t1.join()
    t2.join()


def main() -> int:
    p = argparse.ArgumentParser(description="PS1 serial <-> mixnetd TCP (byte pipe)")
    p.add_argument("--server", default="127.0.0.1", help="mixnetd host (same machine as this bridge)")
    p.add_argument("--port", type=int, default=19677, help="mixnetd TCP port (see mixnetd.exe args)")
    p.add_argument("--serial", default="COM1", help="COM port or /dev/tty* (not used with --serial-tcp)")
    p.add_argument("--baud", type=int, default=115200, help="must match mixnet_stub SIO init (115200)")
    p.add_argument(
        "--serial-tcp",
        metavar="HOST:PORT",
        help="Connect to emulator serial server (virtual wire) instead of PySerial",
    )
    a = p.parse_args()

    try:
        srv = socket.create_connection((a.server, a.port))
    except OSError as e:
        print(f"connect mixnetd {a.server}:{a.port} failed: {e}", file=sys.stderr)
        return 1

    if a.serial_tcp:
        h, pstr = a.serial_tcp.rsplit(":", 1)
        try:
            emu = socket.create_connection((h, int(pstr)))
        except OSError as e:
            print(f"connect serial-TCP {a.serial_tcp} failed: {e}", file=sys.stderr)
            srv.close()
            return 1
        print(
            f"OK: {a.serial_tcp} <-> mixnetd {a.server}:{a.port} (press Ctrl+C to stop)",
            file=sys.stderr,
        )
        try:
            bridge_tcp_to_tcp(emu, srv)
        finally:
            emu.close()
            srv.close()
        return 0

    try:
        import serial
    except ImportError:
        print("Install:  pip install pyserial", file=sys.stderr)
        return 1
    try:
        ser = serial.Serial(a.serial, a.baud, timeout=1.0)
    except OSError as e:
        print(f"open {a.serial} @ {a.baud}: {e}", file=sys.stderr)
        srv.close()
        return 1
    print(
        f"OK: {a.serial} @ {a.baud} <-> mixnetd {a.server}:{a.port} (press Ctrl+C to stop)",
        file=sys.stderr,
    )
    try:
        bridge_serial_to_tcp(ser, srv)
    finally:
        ser.close()
        srv.close()
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        raise SystemExit(130) from None
