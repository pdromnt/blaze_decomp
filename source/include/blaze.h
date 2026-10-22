#ifndef BLAZE_H
#define BLAZE_H
#include <stddef.h>
#include <stdint.h>
#include "blaze_audio.h"

enum { BLAZE_WIDTH = 320, BLAZE_HEIGHT = 240 };
typedef struct {
    uint16_t width, height, stride, visible_height;
    int16_t origin_x, origin_y, transparent;
    size_t pixels, expanded;
} BlazeSprite;
/* One file owner. Sprite offsets refer only to this allocation. */
typedef struct {
    uint8_t *bytes, *expanded; /* Immutable top-down pixels derived from DAR runs. */
    size_t size, count;
    BlazeSprite *sprites;
} BlazeArchive;

/* Initialize owners to zero before first use. Replacement is transactional. */
int blaze_archive_read(BlazeArchive *out, const char *path, char *error, size_t capacity);
int blaze_archive_decode(BlazeArchive *out, const uint8_t *bytes, size_t size,
                         char *error, size_t capacity);
void blaze_archive_free(BlazeArchive *archive);
int blaze_file_read(const char *path, uint8_t **bytes, size_t *size);
int blaze_path(char *out, size_t capacity, const char *root, const char *relative);
uint16_t blaze_le16(const uint8_t *bytes);
uint32_t blaze_le32(const uint8_t *bytes);

typedef struct {
    uint8_t pixels[BLAZE_WIDTH * BLAZE_HEIGHT];
    uint8_t palette[1024]; /* B,G,R,unused, matching DAR bytes */
} BlazeFrame;
void blaze_draw(BlazeFrame *frame, const BlazeArchive *archive, size_t sprite,
                int x, int y, const uint8_t *rop);
void blaze_draw_rotated(BlazeFrame *frame, const BlazeArchive *archive, size_t sprite,
                        int x, int y, int pivot_x, int pivot_y, unsigned angle, int scale);
void blaze_draw_scaled(BlazeFrame *frame,const BlazeArchive *archive,size_t sprite,
    int x,int y,int scale_x,int scale_y,const uint8_t *rop,const uint8_t *colors);
/* Immutable span snapshot for ordinary/scaled drawing; caller releases it. */
int blaze_frame_capture(BlazeArchive *out,const BlazeFrame *frame);
void blaze_color_map(uint8_t out[256],const uint8_t destination[1024],const uint8_t source[1024],unsigned rgb,unsigned percent);
void blaze_color_table(uint8_t out[256],const uint8_t palette[1024],unsigned rgb,unsigned percent);
void blaze_draw_wave(BlazeFrame *frame,const BlazeArchive *archive,size_t sprite,
    int x,int y,unsigned age,const uint8_t *colors);
void blaze_draw_quad(BlazeFrame *frame,const BlazeArchive *archive,size_t sprite,const int32_t points[8]);
void blaze_text(BlazeFrame *frame, const BlazeArchive *font, const char *text,
                int x, int y, int alignment, const uint8_t *rop);

/* On disk: drop flags, actor type, pending flag, tile x, tile y (0x4075b8).
   Names retained for now; parameter may legitimately be actor type zero. */
typedef struct { uint32_t kind, x, y, parameter, flags; } BlazeSpawn;
typedef struct {
    BlazeArchive objects, tiles;
    uint32_t *map[3];
    size_t width[3], height[3];
    BlazeSpawn spawns[128];
    unsigned stage, difficulty;
} BlazeLevel;
int blaze_level_open(BlazeLevel *level, const char *root, unsigned stage,
                     unsigned difficulty, char *error, size_t capacity);
void blaze_level_close(BlazeLevel *level);

/* Authoritative legacy numeric records. Slots formerly containing owner/player
   pointers remain zero; native functions receive those references explicitly.
   No guest image or serialized mirror is kept. Offsets are byte offsets. */
enum { BLAZE_PLAYER_BYTES=0x49e8, BLAZE_ACTOR_WORDS=0x6c/4, BLAZE_ACTOR_DRAWS=256*32 };
typedef struct { uint32_t words[BLAZE_PLAYER_BYTES/4]; } BlazePlayer;
typedef struct {
    int32_t power, action, mount_x[3], mount_y[3];
    int enabled;
    char name[32];
} BlazeWeapon;
typedef struct {
    BlazeWeapon weapons[11];
    unsigned ship, mode, weapon;
    char boss_music[5][80];
} BlazeLoadout;
typedef struct {
    uint16_t offset, count, stride, sprite;
    int16_t expansion;
} BlazePool;
extern const BlazePool blaze_player_pools[];
extern const size_t blaze_player_pool_count;
int32_t blaze_signed(uint32_t value);
const BlazeSprite *blaze_level_sprite(const BlazeLevel *level, unsigned id);
int blaze_player_reset(BlazePlayer *player, const BlazeLevel *level,
                        const BlazeArchive *font, const BlazeLoadout *loadout);
void blaze_player_weapon(BlazePlayer *player, const BlazeLoadout *loadout, unsigned weapon);
int blaze_player_restart(BlazePlayer *player,const BlazeLevel *level,const BlazeArchive *font,const BlazeLoadout *loadout);
void blaze_player_clear(BlazePlayer *player);
void blaze_player_move(BlazePlayer *player, const BlazeLevel *level,
                        const int32_t trig[320], unsigned direction, int32_t camera_x);
typedef struct {
    unsigned sprite, angle;
    int32_t x, y, pivot_x, pivot_y, scale;
} BlazeRotatedDraw;
typedef struct {
    unsigned sprite;
    int32_t x,y,scale,palette;
    int blend;
    unsigned rotated,angle;
    int32_t pivot_x,pivot_y;
    int32_t height_scale; /* Zero uses uniform scale; positive overrides Y; -1 is empty Y. */
    unsigned wave_age; /* Zero disables; otherwise original reveal age plus one. */
    unsigned caption,characters,alignment; /* Immutable caption id / reveal count. */
} BlazeActorDraw;
typedef struct {
    uint32_t state,table[256],waves[4][4];
    int32_t streaks[80][2]; /* Application-owned 0xd90c records; survive missions. */
} BlazeRandom;
unsigned blaze_random_next(BlazeRandom *random);
void blaze_random_begin(BlazeRandom *random,uint32_t seed);
typedef struct {
    BlazePlayer player;
    uint32_t actors[256][BLAZE_ACTOR_WORDS];
    BlazeLevel *level;
    const BlazeArchive *font;
    const BlazeLoadout *loadout;
    const BlazeMedia *media; /* Non-owning app/platform audio interface. */
    const int32_t *trig; /* Immutable application-owned 320-entry motion table. */
    int32_t camera_x, camera_y, scroll_speed;
    unsigned tick;
    BlazeRandom *random; /* Application owner survives world teardown. */
    BlazeRotatedDraw shot_draws[128]; /* One frame: at most 2*48 shots + 32 sparks. */
    size_t shot_draw_count,primary_draw_count;
    BlazeActorDraw mount_draw; /* Immutable submission before pass-1 pickups. */
    BlazeActorDraw actor_draws[BLAZE_ACTOR_DRAWS],effect_draws[1536]; /* Bounded recovered pools, including overlapping weapon pickups. */
    size_t actor_draw_count, actor_pass0_count;
    size_t effect_draw_count;
    size_t effect_primary_count,effect_weapon_count,effect_player_count,effect_thunder_count,effect_gbraster_count;
    uint8_t colors[3][256];
    uint8_t backdrop_colors[6][256];
    int32_t backdrop_points[8];
    int32_t backdrop_streaks[80][2]; /* Update-time draw positions, not another state owner. */
    unsigned encounter_phase,backdrop_phase,warning_characters,requested_screen;
    unsigned actor_cursor, enemies_created;
    int active,dying;
} BlazeWorld;
void blaze_world_backdrop_step(BlazeWorld *world);
void blaze_world_backdrop_draw(const BlazeWorld *world,BlazeFrame *frame);
void blaze_sound(BlazeWorld *world,unsigned sound);
int blaze_primary_create(BlazeWorld *world,const int32_t trig[320],uint32_t record[13],int angle_offset);
void blaze_primary_fire(BlazeWorld *world,const int32_t trig[320]);
int blaze_records_overlap(const uint32_t *first,const uint32_t *second);
int blaze_primary_update(BlazeWorld *world,const int32_t trig[320],uint32_t record[13]);
int blaze_sparks_update(BlazeWorld *world);
int blaze_weapon_hit(BlazeWorld *world,const uint32_t *record,unsigned damage,int special);
void blaze_weapon_impact(BlazeWorld *world,const int32_t trig[320],const uint32_t rect[4],unsigned kind);
int blaze_weapon_action(BlazeWorld *world,unsigned action,int *accepted,char *error,size_t capacity);
void blaze_stock_create(BlazeWorld *world);
int blaze_weapons_update(BlazeWorld *world);
int blaze_charge_update(BlazeWorld *world);
int blaze_thunder_update(BlazeWorld *world);
int blaze_gbraster_update(BlazeWorld *world);
int blaze_scatter_bursts_update(BlazeWorld *world);
int blaze_secondary_update(BlazeWorld *world);
int blaze_enemy_trail_create(BlazeWorld *world,const uint32_t rect[4],unsigned angle);
int blaze_enemy_trails_update(BlazeWorld *world);
int blaze_knife_update(BlazeWorld *world);
int blaze_marker_update(BlazeWorld *world);
int blaze_marker_drones_update(BlazeWorld *world);
unsigned blaze_aim(int target_x,int target_y,int source_x,int source_y);
unsigned blaze_turn(const int32_t trig[320],uint32_t angle,int from_x,int from_y,int target_x,int target_y,unsigned step);
int blaze_actor_create(BlazeWorld *world,uint32_t record[BLAZE_ACTOR_WORDS],
    const uint32_t rect[4],unsigned type,unsigned parameter,unsigned source,unsigned flags);
int blaze_actor_register(BlazeWorld *world,const BlazeSpawn *spawn);
int blaze_actor_spawn(BlazeWorld *world,const uint32_t rect[4],unsigned type,unsigned parameter,unsigned flags);
int blaze_actor_attach(BlazeWorld *world,unsigned parent,int offset_x,int offset_y,
    unsigned type,unsigned parameter,unsigned flags);
int blaze_player_contact(BlazeWorld *world,const uint32_t *actor,unsigned damage);
int blaze_damage_effects(BlazeWorld *world);
void blaze_player_regenerate(BlazeWorld *world);
int blaze_player_present_effects(BlazeWorld *world);
int blaze_actor_update(BlazeWorld *world,uint32_t record[BLAZE_ACTOR_WORDS],unsigned pass,
    char *error,size_t capacity);
int blaze_world_begin(BlazeWorld *world, BlazeLevel *level, const BlazeArchive *font,
                        const BlazeLoadout *loadout,BlazeRandom *random,const int32_t trig[320]);
int blaze_world_restart(BlazeWorld *world);
int blaze_player_step(BlazeWorld *world, const int32_t trig[320], unsigned input,
                      char *error, size_t capacity);
/* 1=completed supported frame, 0=explicit unrecovered behavior boundary. */
int blaze_world_step(BlazeWorld *world, const int32_t trig[320], unsigned input,
                     char *error, size_t capacity);
void blaze_player_draw_mount(const BlazeWorld *world,BlazeFrame *frame);
void blaze_world_draw(BlazeWorld *world, BlazeFrame *frame, const uint8_t *rop);
void blaze_world_end(BlazeWorld *world);

enum { BLAZE_UP=1, BLAZE_DOWN=2, BLAZE_RIGHT=0x10, BLAZE_LEFT=0x20,
       BLAZE_CONFIRM=0x40, BLAZE_CANCEL=0x100, BLAZE_ABORT=0x8000,
       BLAZE_PAUSE=0x10000, BLAZE_MAIN=0x20000, BLAZE_QUIT=0x40000 };
typedef enum { BLAZE_INTRO=1, BLAZE_TITLE=2, BLAZE_OPTIONS=6,
               BLAZE_LOADING=5, BLAZE_START=3, BLAZE_GAME=0,
               BLAZE_STAGE_CLEAR=4, BLAZE_ENDING=10,
               BLAZE_SELECT=9, BLAZE_RANKING=14, BLAZE_GAME_OVER=8, BLAZE_BLOCKED=17, BLAZE_EXIT=18 } BlazeScreen;
enum { BLAZE_MUSIC_GROUPS=17 };
typedef struct {char file[80],name[80];unsigned cd;int file_set;} BlazeMusicTrack;
typedef struct {
    BlazeArchive font, screen,game_over_image,menu_image;
    BlazeLevel level_data;
    BlazeWorld world;
    BlazeLoadout loadout;
    BlazeRandom random;
    int32_t trig[320];
    BlazeFrame frame;
    uint8_t *rop;
    char root[1024], error[256];
    BlazeScreen state;
    unsigned tick, previous_input,previous_control,game_over_caption;
    int restart_pending;
    unsigned pending_stage,max_stage,clear_characters,frame_ms;
    unsigned ending_card;
    int ending_rank;
    uint8_t game_over_colors[256],game_over_text_colors[6][256];
    int title_item, select_step, ship, mode, weapon, option_item;
    int level, high_score, music, display,music_pending;
    int level_pending,display_pending,ranking_return,ranking_scores[5],ranking_modes[5];
    /* Borrowed platform service; the app owns menu state and snapshots. */
    void *settings_context;
    int (*apply_display)(void *context,int mode);
    int (*save_options)(void *context,int level,int display,int music);
    int (*save_progress)(void *context,const int scores[5],const int modes[5],unsigned weapons);
    BlazeMedia media;
    BlazeMusicTrack tracks[BLAZE_MUSIC_GROUPS][3];
    unsigned music_choice[2],music_group[2],pause_flags;
    char stage_music_name[80];
} BlazeApp;
/* Open a fresh app, then close even if open fails. Close before reopening. */
/* NULL configuration uses root/dat. Asset and configuration owners are separate. */
int blaze_app_open(BlazeApp *app, const char *root, const char *rop_path,const char *config_directory);
/* Returns 0 on construction/resource failure; unrecovered routes set BLOCKED. */
/* Game-over frame execution; transitions are completed by the application owner. */
int blaze_game_over_tick(BlazeApp *app,unsigned input);
int blaze_stage_clear_tick(BlazeApp *app,unsigned input);
int blaze_progress_save(BlazeApp *app);
void blaze_app_set_media(BlazeApp *app,const BlazeMedia *media);
void blaze_app_pause(BlazeApp *app,unsigned reason,int paused);
void blaze_music_key(BlazeApp *app,const char *key,const char *value);
void blaze_music_load(BlazeApp *app,unsigned group,unsigned slot);
void blaze_music_play(BlazeApp *app,unsigned slot,int loop);
void blaze_music_stop(BlazeApp *app);
int blaze_app_tick(BlazeApp *app, unsigned input);
int blaze_menu_transition(BlazeApp *app);
void blaze_app_close(BlazeApp *app);
#endif
