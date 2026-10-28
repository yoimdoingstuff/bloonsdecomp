#ifndef BLOONS_ROCKBOX_RENDER_H
#define BLOONS_ROCKBOX_RENDER_H
#include <stddef.h>
#include <stdint.h>
#include "bridge.h"
#ifdef __cplusplus
extern "C" {
#endif
int bloons_assets_load(const void *data,size_t bytes);
void bloons_render_begin(uint16_t *framebuffer);
const struct bloons_painter *bloons_renderer(void);
/* Text remains with the host's font API; positions are 320x240 pixels. */
void bloons_render_text(int x,int y,const char *text);
#ifdef __cplusplus
}
#endif
#endif
