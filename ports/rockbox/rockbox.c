/* Native Rockbox host for the 320x240 color click-wheel iPods. */
#include "plugin.h"
#include "bridge.h"
#include "render.h"

#if !defined(HAVE_LCD_COLOR) || LCD_WIDTH != 320 || LCD_HEIGHT != 240 || LCD_DEPTH != 16
#error Bloons currently targets the 320x240 RGB565 iPod Video/Classic LCD
#endif
#if CONFIG_KEYPAD != IPOD_4G_PAD
#error Bloons currently requires the iPod click-wheel keypad
#endif
PLUGIN_HEADER
#if HZ % 50 != 0
#error Bloons requires a Rockbox tick rate divisible by the original 50 FPS
#endif

#define ASSET_FILE PLUGIN_GAMES_DATA_DIR "/bloons.dat"
#define SAVE_FILE PLUGIN_GAMES_DATA_DIR "/bloons.save"
static uint16_t *framebuffer;
static struct bloons_state *game;
static int wheel=24;
/* Queued text is drawn after the framebuffer blit. */
static struct { int x,y; char value[64]; } labels[24];
static unsigned label_count;

void bloons_platform_failure(const char *message) { rb->panicf("%s",message); }
/* Newlib libm's optional errno path needs no reentrant OS state here. */
int *__errno(void) { static int math_errno; return &math_errno; }
void bloons_render_text(int x,int y,const char *text) {
    if(label_count>=24) return;
    labels[label_count].x=x; labels[label_count].y=y;
    rb->strlcpy(labels[label_count].value,text,sizeof(labels[label_count].value)); ++label_count;
}
static void save_profile(void) {
    unsigned char data[104]; int fd;
    bloons_profile_get(game,data);
    fd=rb->open(SAVE_FILE,O_WRONLY|O_CREAT|O_TRUNC,0666);
    if(fd>=0) { if(rb->write(fd,data,sizeof(data))!=(ssize_t)sizeof(data)) rb->splash(HZ,"Save failed"); rb->close(fd); }
}
static void load_profile(void) {
    unsigned char data[104]; int fd=rb->open(SAVE_FILE,O_RDONLY);
    if(fd>=0) {
        if(rb->read(fd,data,sizeof(data))==(ssize_t)sizeof(data)) bloons_profile_set(game,data);
        rb->close(fd);
    }
}
static void draw(void) {
    unsigned i; label_count=0; bloons_render_begin(framebuffer);
    bloons_paint(game,bloons_renderer());
    rb->lcd_set_viewport(NULL); rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_bitmap((const fb_data *)framebuffer,0,0,320,240);
    rb->lcd_set_foreground(LCD_RGBPACK(0,0,0));
    rb->lcd_set_drawmode(DRMODE_FG);
    for(i=0;i<label_count;++i) rb->lcd_putsxy(labels[i].x,labels[i].y,(const unsigned char *)labels[i].value);
    rb->lcd_update();
    rb->lcd_set_drawmode(DRMODE_SOLID);
}
static int choose_level(int level,int maximum) {
    while(1) {
        int event; rb->lcd_clear_display();
        rb->lcd_putsf(0,0,"Bloons: level %d / %d",level,maximum);
        rb->lcd_puts(0,2,"Wheel: choose level"); rb->lcd_puts(0,3,"Select: play   Menu: back");
        rb->lcd_update(); event=rb->button_get(true);
        if(event==BUTTON_SELECT) return level;
        if(event==BUTTON_MENU) return 0;
        if(event==BUTTON_SCROLL_FWD||(event==(BUTTON_SCROLL_FWD|BUTTON_REPEAT))) { if(level<maximum) ++level; }
        if(event==BUTTON_SCROLL_BACK||(event==(BUTTON_SCROLL_BACK|BUTTON_REPEAT))) { if(level>1) --level; }
        if(rb->default_event_handler(event)==SYS_USB_CONNECTED) return -1;
    }
}
static int menu(void) {
    int selection=0,result; struct bloons_status status;
    MENUITEM_STRINGLIST(items,"Bloons",NULL,"Resume","Select level","Unlimited darts",
                        "Reset level","Quit");
    bloons_status(game,&status);
    result=rb->do_menu(&items,&selection,NULL,false);
    if(result==MENU_ATTACHED_USB) return -1;
    if(result==1) {
        int chosen=choose_level(status.level,status.easy?50:status.unlocked);
        if(chosen<0) return -1;
        if(chosen) bloons_start(game,chosen,status.easy);
    } else if(result==2) bloons_start(game,status.level,!status.easy);
    else if(result==3) bloons_start(game,status.level,status.easy);
    else if(result==4) return 1;
    return 0;
}
enum plugin_status plugin_start(const void *parameter) {
    size_t available,game_bytes,asset_bytes; unsigned char *memory; int fd;
    long next; int old_buttons=0,previous_outcome=BLOONS_PLAYING,suppress_fire=0; (void)parameter;
    memory=rb->plugin_get_audio_buffer(&available);
    game_bytes=(bloons_memory_size()+7)&~(size_t)7;
    if(!memory||available<game_bytes+320*240*sizeof(*framebuffer)) { rb->splash(HZ*2,"Not enough memory"); return PLUGIN_ERROR; }
    game=bloons_create(memory,game_bytes,(uint32_t)*rb->current_tick);
    fd=rb->open(ASSET_FILE,O_RDONLY);
    if(fd<0) { rb->splash(HZ*3,"Missing bloons.dat in games data"); return PLUGIN_ERROR; }
    {
        off_t bytes=rb->ffilesize(fd);
        if(bytes<=0||(uint64_t)bytes+1>available-game_bytes-320*240*sizeof(*framebuffer)) { rb->close(fd); return PLUGIN_ERROR; }
        asset_bytes=(size_t)bytes;
    }
    if(rb->read(fd,memory+game_bytes,asset_bytes)!=(ssize_t)asset_bytes) { rb->close(fd); return PLUGIN_ERROR; }
    rb->close(fd);
    framebuffer=(uint16_t *)(memory+game_bytes+((asset_bytes+1)&~(size_t)1));
    if(!game||!bloons_assets_load(memory+game_bytes,asset_bytes)) { rb->splash(HZ*2,"Invalid Bloons assets"); return PLUGIN_ERROR; }
    rb->lcd_set_backdrop(NULL); rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_set_background(LCD_RGBPACK(255,255,255));
    load_profile(); bloons_start(game,1,0); next=*rb->current_tick;
    while(1) {
        int event=rb->button_get(false),buttons=rb->button_status();
        struct bloons_status status; unsigned directions=0;
        bloons_status(game,&status);
        if(rb->default_event_handler(event)==SYS_USB_CONNECTED) { save_profile(); return PLUGIN_USB_CONNECTED; }
        if((buttons&BUTTON_MENU)&&!(old_buttons&BUTTON_MENU)&&(!status.pacman||(buttons&BUTTON_SELECT))) {
            bloons_cancel_fire(game); suppress_fire=1;
            int action=menu(); save_profile();
            if(action) return action<0?PLUGIN_USB_CONNECTED:PLUGIN_OK;
            /* A paused interval must not advance physics or auto-release fire. */
            next=*rb->current_tick; buttons=rb->button_status(); old_buttons=buttons;
        }
#ifdef HAVE_WHEEL_POSITION
        { int position=rb->wheel_status(); if(position>=0) wheel=position; }
#else
        if((event&~BUTTON_REPEAT)==BUTTON_SCROLL_FWD) wheel=(wheel+1)%96;
        if((event&~BUTTON_REPEAT)==BUTTON_SCROLL_BACK) wheel=(wheel+95)%96;
#endif
        if(status.pacman) {
            if(buttons&BUTTON_MENU) directions|=1;
            if(buttons&BUTTON_PLAY) directions|=2;
            if(buttons&BUTTON_LEFT) directions|=4;
            if(buttons&BUTTON_RIGHT) directions|=8;
        }
        if((buttons&BUTTON_SELECT)&&!(old_buttons&BUTTON_SELECT)&&status.outcome!=BLOONS_PLAYING) {
            suppress_fire=1;
            int level=status.level;
            if(status.outcome==BLOONS_WON) ++level;
            else if(status.outcome==BLOONS_COMPLETE) { int action=menu(); if(action) { save_profile(); return action<0?PLUGIN_USB_CONNECTED:PLUGIN_OK; } }
            if(status.outcome!=BLOONS_COMPLETE) bloons_start(game,level,status.easy);
        }
        /* Both target configurations use HZ=100: two OS ticks per Flash frame.
           Do not discard late simulation steps; slower drawing slows wall time
           rather than changing trajectories, charge increments or lifetimes. */
        if(TIME_AFTER_EQ(*rb->current_tick,next)) {
            if(!(buttons&BUTTON_SELECT)) suppress_fire=0;
            bloons_tick(game,wheel,directions,!suppress_fire&&(buttons&BUTTON_SELECT)!=0);
            next+=HZ/50;
            if(status.outcome!=previous_outcome) { save_profile(); previous_outcome=status.outcome; }
            /* Drawing at 25 FPS reduces LCD traffic; the core still ticks at 50. */
            if((next/(HZ/50))%2==0) draw();
        } else rb->sleep(1);
        old_buttons=buttons;
        rb->yield();
    }
}
