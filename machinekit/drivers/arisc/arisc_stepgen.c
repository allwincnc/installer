#include "rtapi.h"
#include "rtapi_app.h"
#include "hal.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include "rtapi_math.h"

#include "arisc_api.h"

MODULE_AUTHOR("MX_Master");
MODULE_DESCRIPTION("STEPGEN driver for the Allwinner ARISC firmware");
MODULE_LICENSE("GPL");




typedef struct
{
    hal_bit_t *enable; // in

    hal_u32_t *step_port; // in
    hal_u32_t *step_pin; // in
    hal_bit_t *step_inv; // in

    hal_u32_t *dir_port; // in
    hal_u32_t *dir_pin; // in
    hal_bit_t *dir_inv; // in

    hal_float_t *pos_scale; // in
    hal_float_t *pos_cmd; // in
    hal_float_t *vel_cmd; // in

    hal_s32_t *counts; // out
    hal_float_t *pos_fb; // out
    hal_float_t *freq; // out
} stepgen_ch_shmem_t;

typedef struct
{
    hal_bit_t ctrl_type;

    hal_s32_t step_pos_cmd;
    hal_u32_t step_port_old;
    hal_u32_t step_pin_old;
    hal_bit_t step_inv_old;

    hal_u32_t dir;
    hal_u32_t dir_port_old;
    hal_u32_t dir_pin_old;
    hal_bit_t dir_inv_old;
} stepgen_ch_priv_t;

#define g *sg[ch]
#define gg sg[ch]
#define gp sgp[ch]

static int32_t comp_id;
static const uint8_t * comp_name = "arisc_stepgen";

static char *ctrl_type = "";
RTAPI_MP_STRING(ctrl_type, "channels control type, comma separated");

static stepgen_ch_shmem_t *sg;
static stepgen_ch_priv_t sgp[STEPGEN_CH_MAX_CNT] = {0};
static uint8_t sg_cnt = 0;




// TOOLS

static void capture_pos(void *arg, long period);
static void update_freq(void *arg, long period);

static inline
int32_t malloc_and_export(const char *comp_name, int32_t comp_id)
{
    int32_t r, ch;
    int8_t *data = ctrl_type, *token, type[STEPGEN_CH_MAX_CNT] = {0};
    char name[HAL_NAME_LEN + 1];

    // get channels count and type
    while ( (token = strtok(data, ",")) != NULL )
    {
        if ( data != NULL ) data = NULL;

        if      ( token[0] == 'P' || token[0] == 'p' ) type[sg_cnt++] = 0;
        else if ( token[0] == 'V' || token[0] == 'v' ) type[sg_cnt++] = 1;
    }
    if ( !sg_cnt ) return 0;
    if ( sg_cnt > STEPGEN_CH_MAX_CNT ) sg_cnt = STEPGEN_CH_MAX_CNT;

    // shared memory allocation
    sg = hal_malloc(sg_cnt * sizeof(stepgen_ch_shmem_t));
    if ( !sg ) PRINT_ERROR_AND_RETURN("hal_malloc() failed", -1);

    // export HAL pins and set default values
#define EXPORT_PIN(IO_TYPE,VAR_TYPE,VAL,NAME,DEFAULT) \
    r += hal_pin_##VAR_TYPE##_newf(IO_TYPE, &(gg.VAL), comp_id,\
    "%s.%d." NAME, comp_name, ch);\
    g.VAL = DEFAULT;

    for ( r = 0, ch = sg_cnt; ch--; )
    {
        EXPORT_PIN(HAL_IN,bit,enable,"enable", 0);
        EXPORT_PIN(HAL_IN,u32,step_port,"step-port", UINT32_MAX);
        EXPORT_PIN(HAL_IN,u32,step_pin,"step-pin", UINT32_MAX);
        EXPORT_PIN(HAL_IN,bit,step_inv,"step-invert", 0);
        EXPORT_PIN(HAL_IN,u32,dir_port,"dir-port", UINT32_MAX);
        EXPORT_PIN(HAL_IN,u32,dir_pin,"dir-pin", UINT32_MAX);
        EXPORT_PIN(HAL_IN,bit,dir_inv,"dir-invert", 0);
        EXPORT_PIN(HAL_IN,float,pos_scale,"position-scale", 1.0);
        EXPORT_PIN(HAL_OUT,float,pos_fb,"position-fb", 0.0);
        EXPORT_PIN(HAL_OUT,float,freq,"frequency", 0.0);
        EXPORT_PIN(HAL_OUT,s32,counts,"counts", 0);

        _stepgen_ch_setup(ch);

        if ( type[ch] ) { EXPORT_PIN(HAL_IN,float,vel_cmd,"velocity-cmd", 0.0); }
        else            { EXPORT_PIN(HAL_IN,float,pos_cmd,"position-cmd", 0.0); }

        gp.ctrl_type = type[ch];

        gp.step_pos_cmd = 0;
        gp.step_inv_old = 0;
        gp.step_pin_old = UINT32_MAX;
        gp.step_port_old = UINT32_MAX;

        gp.dir_inv_old = 0;
        gp.dir_pin_old = UINT32_MAX;
        gp.dir_port_old = UINT32_MAX;
    }
    if ( r ) PRINT_ERROR_AND_RETURN("HAL pins export failed", -1);

#undef EXPORT_PIN

    // export HAL functions
    r = 0;
    rtapi_snprintf(name, sizeof(name), "%s.capture-position", comp_name);
    r += hal_export_funct(name, capture_pos, 0, 1, 0, comp_id);
    rtapi_snprintf(name, sizeof(name), "%s.update-freq", comp_name);
    r += hal_export_funct(name, update_freq, 0, 1, 0, comp_id);
    if ( r ) PRINT_ERROR_AND_RETURN("HAL functions export failed", -1);

    return 0;
}

static inline
void update_pos_scale(uint8_t ch)
{
    if ( g.pos_scale < 0 || (g.pos_scale < 1e-20 && g.pos_scale > -1e-20) ) g.pos_scale = 1.0;
}

static inline
void update_pins(uint8_t ch)
{
    if ( g.step_port != gp.step_port_old    ||
         g.step_pin  != gp.step_pin_old     ||
         g.step_inv  != gp.step_inv_old )
    {
        if ( g.step_port < GPIO_PORTS_MAX_CNT && g.step_pin < GPIO_PINS_MAX_CNT )
        {
            stepgen_pin_setup(ch, STEP, g.step_port, g.step_pin, g.step_inv, 0);
            gp.step_port_old = g.step_port;
            gp.step_pin_old  = g.step_pin;
            gp.step_inv_old  = g.step_inv;
        }
        else
        {
            g.step_port = gp.step_port_old;
            g.step_pin = gp.step_pin_old;
            g.step_inv = gp.step_inv_old;
        }
    }

    if ( g.dir_port != gp.dir_port_old  ||
         g.dir_pin  != gp.dir_pin_old   ||
         g.dir_inv  != gp.dir_inv_old )
    {
        if ( g.dir_port < GPIO_PORTS_MAX_CNT && g.dir_pin < GPIO_PINS_MAX_CNT )
        {
            stepgen_pin_setup(ch, DIR, g.dir_port, g.dir_pin, g.dir_inv, 0);
            gp.dir_port_old = g.dir_port;
            gp.dir_pin_old  = g.dir_pin;
            gp.dir_inv_old  = g.dir_inv;
        }
        else
        {
            g.dir_port = gp.dir_port_old;
            g.dir_pin = gp.dir_pin_old;
            g.dir_inv = gp.dir_inv_old;
        }
    }
}




// HAL functions

static
void capture_pos(void *arg, long period)
{
    static uint8_t ch;

    for ( ch = sg_cnt; ch--; )
    {
        if ( !g.enable ) continue;

        update_pos_scale(ch);
        g.counts = stepgen_pos_get(ch, 0);
        g.pos_fb = ((hal_float_t)g.counts) / g.pos_scale;
    }
}

static
void update_freq(void *arg, long period)
{
    static uint32_t ch;
    static int32_t steps;
    static hal_float_t period_s;

    period_s = ((hal_float_t)period) / ((hal_float_t)1000000000);

    for ( ch = sg_cnt; ch--; )
    {
        if ( !g.enable ) { g.freq = 0; continue; }

        update_pins(ch);
        update_pos_scale(ch);

        if ( gp.ctrl_type ) // velocity mode
        {
            // stop any movement if velocity value is too small
            if ( g.vel_cmd < 1e-20 && g.vel_cmd > -1e-20 ) { g.freq = 0; continue; }

            g.freq = g.pos_scale * g.vel_cmd;
            gp.step_pos_cmd = g.counts + (int32_t)(g.freq * period_s);
            steps = gp.step_pos_cmd - g.counts;
        }
        else // position mode
        {
            gp.step_pos_cmd = (int32_t)round(g.pos_scale * g.pos_cmd);
            steps = gp.step_pos_cmd - g.counts;

            // stop any movement if we have the nearest position to the target
            if ( !steps ) { g.freq = 0; continue; }

            g.freq = (g.pos_cmd - g.pos_fb) / period_s;
        }

        if ( steps ) stepgen_task_add(ch, steps, (uint32_t)period, 0);
    }
}




// INIT

int32_t rtapi_app_main(void)
{
    if ( (comp_id = hal_init(comp_name)) < 0 )
        PRINT_ERROR_AND_RETURN("ERROR: hal_init() failed\n",-1);

    if ( shmem_init(comp_name) || malloc_and_export(comp_name, comp_id) )
    {
        hal_exit(comp_id);
        return -1;
    }

    hal_ready(comp_id);

    return 0;
}

void rtapi_app_exit(void)
{
    stepgen_cleanup();
    shmem_deinit();
    hal_exit(comp_id);
}
