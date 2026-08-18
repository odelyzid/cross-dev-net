/*
 * mixnet_ps2_net: IOP boot + network stack init for the PS2 client.
 * Embedded IRX byte arrays come from bin2c of the ps2sdk release modules:
 *   $(PS2SDK)/iop/irx/ps2dev9.irx  -> DEV9_irx[] (sample convention)
 *   $(PS2SDK)/iop/irx/netman.irx   -> NETMAN_irx[]
 *   $(PS2SDK)/iop/irx/smap.irx     -> SMAP_irx[]
 *   $(PS2SDK)/iop/irx/ps2ip-nm.irx -> PS2IP_NM_irx[]
 */
#include "mixnet_ps2_net.h"
#include <kernel.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <debug.h>
#include <netman.h>
#include <ps2ip.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <string.h>

extern unsigned char DEV9_irx[];
extern unsigned int size_DEV9_irx;
extern unsigned char NETMAN_irx[];
extern unsigned int size_NETMAN_irx;
extern unsigned char SMAP_irx[];
extern unsigned int size_SMAP_irx;
extern unsigned char PS2IP_NM_irx[];
extern unsigned int size_PS2IP_NM_irx;

static char s_err[128];
static int s_net_ok;

const char* mixnet_ps2_net_status(void) {
	return s_err[0] ? s_err : (s_net_ok ? "network up" : "network init pending");
}

static void EthStatusCheckCb(s32 alarm_id, u16 time, void* common) {
	(void)alarm_id; (void)time;
	iWakeupThread(*(int*)common);
}

static int wait_valid_net_state(int (*check)(void)) {
	int tid = GetThreadId();
	int i;
	for (i = 0; i < 10; i++) {
		if (check()) return 0;
		SetAlarm(1000 * 16, &EthStatusCheckCb, &tid);
		SleepThread();
	}
	return -1;
}

static int eth_link_up(void) {
	return NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0) ==
	       NETMAN_NETIF_ETH_LINK_STATE_UP;
}

static void net_fail(const char* why) {
	strncpy(s_err, why, sizeof s_err - 1);
	s_err[sizeof s_err - 1] = '\0';
}

int mixnet_ps2_net_init(void) {
	struct ip4_addr IP, NM, GW;
	int ret;

	s_err[0] = '\0';
	if (s_net_ok) return 0;

	scr_printf("mixnet_ps2: IOP reset...\n");
	sceSifInitRpc(0);
	while (!SifIopReset("", 0)) {}
	while (!SifIopSync()) {}
	sceSifInitRpc(0);
	if (SifLoadFileInit() < 0) scr_printf("mixnet_ps2: SifLoadFileInit\n");
	if (SifInitIopHeap() < 0) scr_printf("mixnet_ps2: SifInitIopHeap\n");
	sbv_patch_enable_lmb();

	ret = SifExecModuleBuffer(DEV9_irx, size_DEV9_irx, 0, NULL, NULL);
	if (ret < 0) { net_fail("DEV9 module failed"); return -1; }
	ret = SifExecModuleBuffer(NETMAN_irx, size_NETMAN_irx, 0, NULL, NULL);
	if (ret < 0) { net_fail("NETMAN module failed"); return -1; }
	ret = SifExecModuleBuffer(SMAP_irx, size_SMAP_irx, 0, NULL, NULL);
	if (ret < 0) { net_fail("SMAP module failed"); return -1; }
	ret = SifExecModuleBuffer(PS2IP_NM_irx, size_PS2IP_NM_irx, 0, NULL, NULL);
	if (ret < 0) { net_fail("PS2IP-NM module failed"); return -1; }

	if (NetManInit() < 0) { net_fail("NetManInit failed"); return -1; }

	if (NetManSetLinkMode(NETMAN_NETIF_ETH_LINK_MODE_AUTO) < 0) {
		net_fail("NetManSetLinkMode failed");
		return -1;
	}

	IP4_ADDR(&IP, 192, 168, 0, 80);
	IP4_ADDR(&NM, 255, 255, 255, 0);
	IP4_ADDR(&GW, 192, 168, 0, 1);
	if (ps2ipInit(&IP, &NM, &GW) < 0) { net_fail("ps2ipInit failed"); return -1; }

	if (wait_valid_net_state(&eth_link_up) != 0) {
		net_fail("no Ethernet link (cable?)");
		return -1;
	}

	scr_printf("mixnet_ps2: network ready\n");
	s_net_ok = 1;
	return 0;
}