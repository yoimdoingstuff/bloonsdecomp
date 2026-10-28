#ifndef BLOONS_ROCKBOX_BRIDGE_H
#define BLOONS_ROCKBOX_BRIDGE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { BLOONS_PLAYING, BLOONS_WON, BLOONS_LOST, BLOONS_COMPLETE };
struct bloons_state;
struct bloons_status {
    int level, target, popped, darts, outcome, pacman, unlocked, easy;
};
struct bloons_painter {
    void (*sprite)(int id, double a, double b, double c, double d,
                   double tx, double ty, int alpha);
    void (*text)(int x, int y, const char *text);
};
size_t bloons_memory_size(void);
struct bloons_state *bloons_create(void *buffer, size_t size, uint32_t seed);
void bloons_start(struct bloons_state *, int level, int easy);
void bloons_tick(struct bloons_state *, int wheel, unsigned directions, int held);
void bloons_cancel_fire(struct bloons_state *);
void bloons_status(const struct bloons_state *, struct bloons_status *);
void bloons_paint(const struct bloons_state *, const struct bloons_painter *);
void bloons_profile_get(const struct bloons_state *, unsigned char out[104]);
int bloons_profile_set(struct bloons_state *, const unsigned char in[104]);
void bloons_platform_failure(const char *message);
#ifdef __cplusplus
}
#endif
#endif
