/********************************************************************
 * Description:  arisc.stepgen.c
 *               STEPGEN driver for the Allwinner ARISC firmware
 *
 * Author: MX_Master (mikhail@vydrenko.ru)
 ********************************************************************/

#include "rtapi.h"
#include "rtapi_app.h"
#include "hal.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include "rtapi_math.h"

#include "allwinner_CPU.h"
#include "msg_api.h"
#include "gpio_api.h"
#include "stepgen_api.h"
#include "arisc.stepgen.h"




MODULE_AUTHOR("MX_Master");
MODULE_DESCRIPTION("STEPGEN driver for the Allwinner ARISC firmware");
MODULE_LICENSE("GPL");




#define g *sg[ch]
#define gp sg[ch]

static int32_t comp_id;
static const uint8_t * comp_name = "arisc.stepgen";
static uint8_t cpu_id = ALLWINNER_H3;

static char *CPU = "H3";
RTAPI_MP_STRING(CPU, "Allwinner CPU name");

static char *ctrl_type = "";
RTAPI_MP_STRING(ctrl_type, "channels control type, comma separated");

static stepgen_ch_t *sg;
static uint8_t sg_cnt = 0;
static long last_period = 1000000;




// TOOLS

static void update_pos_scale(uint8_t ch)
{
    if ( g.pos_scale < 0 || (g.pos_scale < 1e-20 && g.pos_scale > -1e-20) ) g.pos_scale = 1.0;
}

static void update_pins(uint8_t ch)
{
    if ( g.step_port != gp.step_port_old    ||
         g.step_pin  != gp.step_pin_old     ||
         g.step_inv  != gp.step_inv_old )
    {
        if ( g.step_port < GPIO_PORTS_CNT && g.step_pin < GPIO_PINS_CNT )
        {
            stepgen_pin_setup(ch, STEP, g.step_port, g.step_pin, g.step_inv);
        }

        gp.step_port_old = g.step_port;
        gp.step_pin_old  = g.step_pin;
        gp.step_inv_old  = g.step_inv;
    }

    if ( g.dir_port != gp.dir_port_old  ||
         g.dir_pin  != gp.dir_pin_old   ||
         g.dir_inv  != gp.dir_inv_old )
    {
        if ( g.dir_port < GPIO_PORTS_CNT && g.dir_pin < GPIO_PINS_CNT )
        {
            stepgen_pin_setup(ch, DIR, g.dir_port, g.dir_pin, g.dir_inv);
        }

        gp.dir_port_old = g.dir_port;
        gp.dir_pin_old  = g.dir_pin;
        gp.dir_inv_old  = g.dir_inv;
    }
}

static void update_dir_times(uint8_t ch)
{
    if ( g.dir_setup != gp.dir_setup_old ||
         g.dir_hold   != gp.dir_hold_old )
    {
        if ( (g.dir_setup + g.dir_hold) > last_period )
        {
            g.dir_setup = last_period / 16;
            g.dir_hold = g.dir_setup;
        }

        stepgen_time_setup(ch, DIR, g.dir_setup, g.dir_hold);

        gp.dir_setup_old = g.dir_setup;
        gp.dir_hold_old  = g.dir_hold;
    }
}




// HAL functions

static void capture_pos(void *arg, long period)
{
    static uint8_t ch;

    for ( ch = sg_cnt; ch--; )
    {
        if ( !g.enable ) continue;

        update_pos_scale(ch);
        g.counts = stepgen_pos_get(ch);

        if ( gp.ctrl_type ) // velocity mode
        {
            g.pos_fb = ((hal_float_t)g.counts) / g.pos_scale;
        }
        else // position mode
        {
            g.pos_fb = (g.counts == gp.step_pos_cmd) ?
                g.pos_cmd :
                ((hal_float_t)g.counts) / g.pos_scale ;
        }
    }
}

static void update_freq(void *arg, long period)
{
    static uint8_t ch;
    static int32_t steps;
    static uint32_t step_space, step_period, step_period_min, step_len;
    static hal_float_t period_s;

    last_period = period;
    period_s = ((hal_float_t)period) / ((hal_float_t)1000000000);

    for ( ch = sg_cnt; ch--; )
    {
        if ( !g.enable ) { g.freq = 0; continue; }

        update_pins(ch);
        update_dir_times(ch);
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
            gp.step_pos_cmd = (int32_t)(g.pos_scale * g.pos_cmd);
            steps = gp.step_pos_cmd - g.counts;

            // stop any movement if we have the nearest position to the target
            if ( !steps ) { g.freq = 0; continue; }

            g.freq = (g.pos_cmd - g.pos_fb) / period_s;
        }

        if ( steps )
        {
            step_period = ((uint32_t)period) / ((uint32_t)abs(steps));

            // use 50% duty if any of step timings == 0
            if ( !g.step_len || !g.step_space )
            {
                step_space = step_period / 2;
                step_len = step_space;
            }
            else // use step space/len parameters set by user
            {
                step_period_min = g.step_len + g.step_space;

                // limit the velocity using step space/len parameters
                // this limitation can cause "joint # following error"
                if ( step_period < step_period_min ) step_period = step_period_min;

                step_len = g.step_len;
                step_space = step_period - step_len;
            }

            stepgen_time_setup(ch, STEP, step_space, step_len);
            stepgen_task_add(ch, steps);
        }
    }
}




// INIT

#define EXPORT_PIN(IO_TYPE,VAR_TYPE,VAL,NAME,DEFAULT) \
    r += hal_pin_##VAR_TYPE##_newf(IO_TYPE, &(gp.VAL), comp_id,\
    "%s.%d." NAME, comp_name, ch);\
    g.VAL = DEFAULT;

#define PRINT_ERROR(MSG) \
    rtapi_print_msg(RTAPI_MSG_ERR, "%s: "MSG"\n", comp_name)

#define PRINT_ERROR_AND_RETURN(MSG,RETVAL) \
    { PRINT_ERROR(MSG); return RETVAL; }

static int32_t malloc_and_export(const char *comp_name, int32_t comp_id)
{
    int32_t r, ch;
    int8_t *data = ctrl_type, *token, type[STEPGEN_CH_CNT_MAX] = {0};
    char name[HAL_NAME_LEN + 1];

    // get channels count and type

    while ( (token = strtok(data, ",")) != NULL )
    {
        if ( data != NULL ) data = NULL;

        if      ( token[0] == 'P' || token[0] == 'p' ) type[sg_cnt++] = 0;
        else if ( token[0] == 'V' || token[0] == 'v' ) type[sg_cnt++] = 1;
    }

    if ( !sg_cnt ) return 0;
    if ( sg_cnt > STEPGEN_CH_CNT_MAX ) sg_cnt = STEPGEN_CH_CNT_MAX;

    // shared memory allocation

    sg = hal_malloc(sg_cnt * sizeof(stepgen_ch_t));
    if ( !sg ) PRINT_ERROR_AND_RETURN("hal_malloc() failed", -1);

    // export HAL pins and set default values

    for ( r = 0, ch = sg_cnt; ch--; )
    {
        EXPORT_PIN(HAL_IN,bit,enable,"enable", 0);
        EXPORT_PIN(HAL_IN,u32,step_space,"stepspace", 0);
        EXPORT_PIN(HAL_IN,u32,step_len,"steplen", 0);
        EXPORT_PIN(HAL_IN,u32,dir_setup,"dirsetup", 1000);
        EXPORT_PIN(HAL_IN,u32,dir_hold,"dirhold", 1000);
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

        stepgen_task_add(ch,0); // abort all tasks
        stepgen_pos_set(ch,0);

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

    // export HAL functions

    r = 0;
    rtapi_snprintf(name, sizeof(name), "%s.capture-position", comp_name);
    r += hal_export_funct(name, capture_pos, 0, 1, 0, comp_id);
    rtapi_snprintf(name, sizeof(name), "%s.update-freq", comp_name);
    r += hal_export_funct(name, update_freq, 0, 1, 0, comp_id);
    if ( r ) PRINT_ERROR_AND_RETURN("HAL functions export failed", -1);

    return 0;
}

int32_t rtapi_app_main(void)
{
    // get component id
    if ( (comp_id = hal_init(comp_name)) < 0 )
    {
        rtapi_print_msg(RTAPI_MSG_ERR, "%s: hal_init() failed\n", comp_name);
        return -1;
    }

    #define EXIT { hal_exit(comp_id); return -1; }

    // shared memory allocation and export
    cpu_id = allwinner_cpu_id_get(CPU);
    if ( msg_mem_init(cpu_id, comp_name) ) EXIT;
    if ( malloc_and_export(comp_name, comp_id) ) EXIT;

    // driver ready to work
    hal_ready(comp_id);

    return 0;
}

void rtapi_app_exit(void)
{
    msg_mem_deinit();
    hal_exit(comp_id);
}

#undef EXPORT_PIN
#undef PRINT_ERROR
#undef PRINT_ERROR_AND_RETURN




#undef g
#undef gp
