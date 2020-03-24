/********************************************************************
 * Description:  arisc_stepgen.h
 *               STEPGEN driver for the Allwinner ARISC firmware
 *
 * Author: MX_Master (mikhail@vydrenko.ru)
 ********************************************************************/

#ifndef _ARISC_STEPGEN_H
#define _ARISC_STEPGEN_H

#include "hal.h"




#define STEPGEN_CH_CNT_MAX 8
#define STEP 0
#define DIR 1




typedef struct
{
    // --- public data ---------------------------
    hal_bit_t *enable; // in

    hal_u32_t *step_space; // in
    hal_u32_t *step_len; // in
    hal_u32_t *step_port; // in
    hal_u32_t *step_pin; // in
    hal_bit_t *step_inv; // in

    hal_u32_t *dir_setup; // in
    hal_u32_t *dir_hold; // in
    hal_u32_t *dir_port; // in
    hal_u32_t *dir_pin; // in
    hal_bit_t *dir_inv; // in

    hal_float_t *pos_scale; // in
    hal_float_t *pos_cmd; // in
    hal_float_t *vel_cmd; // in

    hal_s32_t *counts; // out
    hal_float_t *pos_fb; // out
    hal_float_t *freq; // out

    // --- private data ---------------------------
    hal_bit_t ctrl_type;

    hal_s32_t step_pos_cmd;
    hal_u32_t step_port_old;
    hal_u32_t step_pin_old;
    hal_bit_t step_inv_old;

    hal_u32_t dir_setup_old;
    hal_u32_t dir_hold_old;
    hal_u32_t dir_port_old;
    hal_u32_t dir_pin_old;
    hal_bit_t dir_inv_old;

} stepgen_ch_t;




#endif
