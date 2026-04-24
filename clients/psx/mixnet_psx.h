/* PSYQ / hardware glue — call after mixnet_nav_render_screen (e.g. FntPrint, tile text). */
#ifndef MIXNET_PSX_H
#define MIXNET_PSX_H

/* Implement in your build; default weak stub can be an empty function in the same link unit. */
void mixnet_psx_blt_screen(const char* text, int n_bytes);

void mixnet_psx_init_video(void);
void mixnet_psx_pump(void);

#endif
