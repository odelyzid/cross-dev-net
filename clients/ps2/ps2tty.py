#!/usr/bin/env python3
"""
ps2tty.py - relay for the PS2 client's "serial-style" UDP channel.

The PS2 app (mixnet_ps2.elf, `-udp` mode) exchanges UDP datagrams on port
19678 with this relay; the relay pipes the byte stream over TCP to mixnetd.
Datagram boundaries are intentionally ignored (the mixnet framing layers are
self-delimiting), which makes the link behave like the serial bridges used
by the other console clients (mixnet_serial_bridge.py).

Usage:
    ps2tty.py --mixnet 192.168.1.10:19677 --ps2 192.168.0.80 [--port 19678]

Requires Python 3.
"""
import argparse
import select
import socket
import sys
import time

MIXNET_PS2_UDP_PORT = 19678


def main():
    ap = argparse.ArgumentParser(description="PS2 UDP-tty <-> mixnetd relay")
    ap.add_argument("--mixnet", default="127.0.0.1:19677",
                    help="mixnetd address host:port (default 127.0.0.1:19677)")
    ap.add_argument("--ps2", default="192.168.0.80",
                    help="PS2 IP (default 192.168.0.80)")
    ap.add_argument("--port", type=int, default=MIXNET_PS2_UDP_PORT,
                    help="local UDP port shared with the PS2 (default %d)"
                         % MIXNET_PS2_UDP_PORT)
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    host, _, port = args.mixnet.partition(":")
    tcp_port = int(port) if port else 19677

    # mixnetd side (TCP client)
    tsock = socket.create_connection((host, tcp_port))
    tsock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    tsock.setblocking(False)
    print("[ps2tty] mixnetd at %s:%d connected" % (host, tcp_port))

    # PS2 side (UDP)
    usock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    usock.bind(("0.0.0.0", args.port))
    usock.setblocking(False)
    print("[ps2tty] udp relay bound on 0.0.0.0:%d (ps2=%s)" % (args.port, args.ps2))

    target = None  # PS2 peer addr, learned from its first datagram
    last_announce = time.time()
    tcp_alive = True

    while True:
        rlist = [usock]
        if tcp_alive:
            rlist.append(tsock)
        r, _, _ = select.select(rlist, [], [], 1.0)

        if tsock in r:
            try:
                data = tsock.recv(4096)
            except BlockingIOError:
                data = None
            except OSError:
                data = b""
            if data == b"":
                print("[ps2tty] mixnetd closed the connection")
                break
            if data:
                if target:
                    usock.sendto(data, target)
                elif args.verbose:
                    print("[ps2tty] no PS2 peer yet, dropped %d bytes" % len(data))

        if usock in r:
            try:
                data, addr = usock.recvfrom(4096)
            except OSError:
                continue
            if target is None:
                target = addr
                print("[ps2tty] PS2 peer at %s:%d" % (addr[0], addr[1]))
            if args.verbose:
                print("[ps2tty] <- %d bytes" % len(data))
            tsock.setblocking(True)
            try:
                tsock.sendall(data)
            except OSError:
                print("[ps2tty] mixnetd link died while writing")
                tcp_alive = False
                break
            finally:
                tsock.setblocking(False)

        now = time.time()
        if target is None and now - last_announce > 5:
            print("[ps2tty] waiting for PS2 %s on udp %d..." % (args.ps2, args.port))
            last_announce = now

    sys.exit(1)


if __name__ == "__main__":
    main()