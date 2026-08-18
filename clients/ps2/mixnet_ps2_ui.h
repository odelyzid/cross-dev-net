/* mixnet_ps2_ui: gsKit renderer + pad input for the Mixnet PS2 client.
 * Renders the navigator text buffer with a self-contained 8x8 bitmap font
 * (1x horizontal, 2x vertical => 8x16 px chars on a 640x448 frame).
 * Pad mapping matches the PS1 client (see mixnet_psx stub):
 *   Up/Down/Left/Right = 1/2/3/4, L1=5, L2=6, Select=:h, Start=:g, R1=PING.
 */
#ifndef MIXNET_PS2_UI_H
#define MIXNET_PS2_UI_H

int mixnet_ps2_ui_init(void);          /* gsKit screen + font texture + pad */
void mixnet_ps2_ui_pad_poll(void);     /* edge-triggered nav keys */
void mixnet_ps2_ui_frame(const char* view, int len, int connected);

#define MIXNET_PS2_FONT_X 1            /* horizontal scale (px per texel) */
#define MIXNET_PS2_FONT_Y 2            /* vertical scale   (px per texel) */
#define MIXNET_PS2_GRID_W 76           /* chars across  ((640-16)/(8*1))  */
#define MIXNET_PS2_GRID_H 27           /* chars down    ((448-16)/(8*2))  */

#endif