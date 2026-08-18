# mixnet_ps2: Mixnet Navigator for PlayStation 2 (classic ELF).
# Needs the ps2dev toolchain (PS2DEV, PS2SDK) + gsKit (GSKIT).
#   make            -> mixnet_ps2.elf
# Recommended env on Windows (Git Bash / cmd):
#   PS2DEV=E:\Emulation\ps2dev  PS2SDK=%PS2DEV%\ps2sdk  GSKIT=%PS2DEV%\gsKit
#   PATH=<msys64>\mingw32\bin;<PS2DEV>\bin;<PS2DEV>\ee\bin;<PS2DEV>\iop\bin;<msys64>\usr\bin

EE_BIN = mixnet_ps2.elf
EE_OBJS = mixnet_ps2_main.o mixnet_ps2_net.o mixnet_ps2_ui.o \
          mixnet_ps2_link.o mixnet_ps2_tcp.o mixnet_ps2_udp.o \
          mixnet_navigator.o mixnet_line.o mixnet_packet.o \
          DEV9_irx.o NETMAN_irx.o SMAP_irx.o PS2IP_NM_irx.o
EE_LIBS = -lnetman -lps2ip -ldebug -lpatches -lpad -lpacket -ldma -lgskit -ldmakit

GSKIT ?= $(PS2DEV)/gsKit
EE_INCS += -I$(GSKIT)/include
EE_LDFLAGS += -L$(GSKIT)/lib

all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)
	@echo "mixnet_ps2 build OK: $(EE_BIN)"

clean:
	rm -f $(EE_BIN) $(EE_OBJS) DEV9_irx.c NETMAN_irx.c SMAP_irx.c PS2IP_NM_irx.c

# shared sources (cross-client framing + navigator UI)
mixnet_navigator.o: ../psx/mixnet_navigator.c ../psx/mixnet_navigator.h
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
mixnet_line.o: ../common/mixnet_line.c ../common/mixnet_line.h
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
mixnet_packet.o: ../common/mixnet_packet.c ../common/mixnet_packet.h
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

# network driver modules, embedded with bin2c
BIN2C = $(PS2SDK)/bin/bin2c
DEV9_irx.c: $(PS2SDK)/iop/irx/ps2dev9.irx
	$(BIN2C) $< DEV9_irx.c DEV9_irx
NETMAN_irx.c: $(PS2SDK)/iop/irx/netman.irx
	$(BIN2C) $< NETMAN_irx.c NETMAN_irx
SMAP_irx.c: $(PS2SDK)/iop/irx/smap.irx
	$(BIN2C) $< SMAP_irx.c SMAP_irx
PS2IP_NM_irx.c: $(PS2SDK)/iop/irx/ps2ip-nm.irx
	$(BIN2C) $< PS2IP_NM_irx.c PS2IP_NM_irx

run: $(EE_BIN)
	ps2client execee host:$(EE_BIN)

reset:
	ps2client reset

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal