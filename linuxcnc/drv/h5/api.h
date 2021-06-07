#ifndef _API_H
#define _API_H

#include "rtapi.h"
#include "rtapi_app.h"
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/fsuid.h>
#include <time.h>




#define ARISC_CPU_FREQ          300000000 // Hz
#define ARISC_FW_BASE           (0x00040000) // for ARM CPU it's 0x00040000
#define ARISC_FW_SIZE           ((8+8+64)*1024)
#define ARISC_SHM_SIZE          (4096)
#define ARISC_SHM_BASE          (ARISC_FW_BASE + ARISC_FW_SIZE - ARISC_SHM_SIZE)




#define GPIO_BASE               0x01c20800
#define GPIO_R_BASE             0x01f02c00
#define GPIO_BANK_SIZE          0x24

#define GPIO_PORTS_MAX_CNT      8
#define GPIO_PINS_MAX_CNT       24

enum
{
    GPIO_FUNC_IN,
    GPIO_FUNC_OUT,
    GPIO_FUNC_2,
    GPIO_FUNC_3,
    GPIO_FUNC_RESERVED4,
    GPIO_FUNC_RESERVED5,
    GPIO_FUNC_EINT,
    GPIO_FUNC_DISABLE,
    GPIO_FUNC_CNT
};

enum
{
    GPIO_MULTI_DRIVE_LEVEL0,
    GPIO_MULTI_DRIVE_LEVEL1,
    GPIO_MULTI_DRIVE_LEVEL2,
    GPIO_MULTI_DRIVE_LEVEL3,
    GPIO_MULTI_DRIVE_LEVEL_CNT
};

enum
{
    GPIO_PULL_DISABLE,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN,
    GPIO_PULL_RESERVED3,
    GPIO_PULL_CNT
};

enum { PA, PB, PC, PD, PE, PF, PG, PL };
enum { LOW, HIGH };

#define GPIO_PIN_SET(PORT,PIN_MASK) \
    _GPIO[PORT]->data |= PIN_MASK

#define GPIO_PIN_CLR(PORT,PIN_MASK_NOT) \
    _GPIO[PORT]->data &= PIN_MASK_NOT

#define GPIO_PIN_GET(PORT,PIN_MASK) \
    (_GPIO[PORT]->data & PIN_MASK)

typedef struct
{
    uint32_t config[4];
    uint32_t data;
    uint32_t drive[2];
    uint32_t pull[2];
} _GPIO_PORT_REG_t;




#define PWM_CH_MAX_CNT 16
#define PWM_WASTED_TICKS (160/2) // number of ARISC ticks wasted for calculations

enum
{
    PWM_CH_POS,
    PWM_CH_TICK,
    PWM_CH_TIMEOUT,
    PWM_CH_STATE,
    PWM_CH_WATCHDOG,

    PWM_CH_P_PORT,
    PWM_CH_P_PIN_MSK,
    PWM_CH_P_PIN_MSKN,
    PWM_CH_P_INV,
    PWM_CH_P_T0,
    PWM_CH_P_T1,
    PWM_CH_P_STOP,
    PWM_CH_P_TICK,

    PWM_CH_D_PORT,
    PWM_CH_D_PIN_MSK,
    PWM_CH_D_PIN_MSKN,
    PWM_CH_D,
    PWM_CH_D_INV,
    PWM_CH_D_T0,
    PWM_CH_D_T1,
    PWM_CH_D_CHANGE,

    PWM_CH_DATA_CNT
};

enum
{
    PWM_TIMER_TICK,
    PWM_ARM_LOCK,
    PWM_ARISC_LOCK,
    PWM_CH_CNT,
    PWM_DATA_CNT
};

enum
{
    PWM_CH_STATE_IDLE,
    PWM_CH_STATE_P0,
    PWM_CH_STATE_P1,
    PWM_CH_STATE_D0,
    PWM_CH_STATE_D1
};

#define PWM_SHM_BASE         (ARISC_SHM_BASE)
#define PWM_SHM_DATA_BASE    (PWM_SHM_BASE)
#define PWM_SHM_CH_DATA_BASE (PWM_SHM_DATA_BASE + PWM_DATA_CNT*4)
#define PWM_SHM_SIZE         (PWM_SHM_CH_DATA_BASE + PWM_CH_MAX_CNT*PWM_CH_DATA_CNT*4)




#define ENC_MODULE_ENABLED 1

#if ENC_MODULE_ENABLED
#define ENC_CH_MAX_CNT 8

enum
{
    ENC_CH_BUSY,
    ENC_CH_POS,
    ENC_CH_AB_STATE,

    ENC_CH_A_PORT,
    ENC_CH_A_PIN_MSK,
    ENC_CH_A_INV,
    ENC_CH_A_ALL,
    ENC_CH_A_STATE,

    ENC_CH_B_USE,
    ENC_CH_B_PORT,
    ENC_CH_B_PIN_MSK,
    ENC_CH_B_STATE,

    ENC_CH_Z_USE,
    ENC_CH_Z_PORT,
    ENC_CH_Z_PIN_MSK,
    ENC_CH_Z_INV,
    ENC_CH_Z_ALL,
    ENC_CH_Z_STATE,

    ENC_CH_DATA_CNT
};

enum
{
    ENC_ARM_LOCK,
    ENC_ARISC_LOCK,
    ENC_CH_CNT,
    ENC_DATA_CNT
};

#define ENC_SHM_BASE         (PWM_SHM_BASE + PWM_SHM_SIZE)
#define ENC_SHM_DATA_BASE    (ENC_SHM_BASE)
#define ENC_SHM_CH_DATA_BASE (ENC_SHM_DATA_BASE + ENC_DATA_CNT*4)
#define ENC_SHM_SIZE         (ENC_SHM_CH_DATA_BASE + ENC_CH_MAX_CNT*ENC_CH_DATA_CNT*4 - ENC_SHM_BASE)
#endif




#define PRINT_ERROR(MSG) \
    rtapi_print_msg(RTAPI_MSG_ERR, "%s: "MSG"\n", comp_name)

#define PRINT_ERROR_AND_RETURN(MSG,RETVAL) \
    { PRINT_ERROR(MSG); return RETVAL; }




static uint32_t *_shm_vrt_addr, *_gpio_vrt_addr, *_r_gpio_vrt_addr;

volatile _GPIO_PORT_REG_t *_GPIO[GPIO_PORTS_MAX_CNT] = {0};
static uint32_t _gpio_buf[GPIO_PORTS_MAX_CNT] = {0};

volatile uint32_t * _pwmc[PWM_CH_MAX_CNT][PWM_CH_DATA_CNT] = {0};
volatile uint32_t * _pwmd[PWM_DATA_CNT] = {0};
#if ENC_MODULE_ENABLED
volatile uint32_t * _encc[ENC_CH_MAX_CNT][ENC_CH_DATA_CNT] = {0};
volatile uint32_t * _encd[ENC_DATA_CNT] = {0};
#endif



static inline
void _pwm_spin_lock()
{
    *_pwmd[PWM_ARM_LOCK] = 1;
    if ( ! *_pwmd[PWM_CH_CNT] ) return;
    while ( *_pwmd[PWM_ARISC_LOCK] );
}

static inline
void _pwm_spin_unlock()
{
    *_pwmd[PWM_ARM_LOCK] = 0;
}

#if ENC_MODULE_ENABLED
static inline
void _enc_spin_lock()
{
    *_encd[ENC_ARM_LOCK] = 1;
    if ( ! *_encd[ENC_CH_CNT] ) return;
    while ( *_encd[ENC_ARISC_LOCK] );
}

static inline
void _enc_spin_unlock()
{
    *_encd[ENC_ARM_LOCK] = 0;
}
#endif

static inline
int32_t gpio_pin_pull_set(uint32_t port, uint32_t pin, uint32_t level, uint32_t safe)
{
    if ( safe ) {
        if ( port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( pin >= GPIO_PINS_MAX_CNT ) return -2;
        if ( level >= GPIO_PULL_CNT ) return -3;
    }
    uint32_t slot = pin/16, pos = pin%16*2;
    _pwm_spin_lock();
#if ENC_MODULE_ENABLED
    _enc_spin_lock();
#endif
    _GPIO[port]->pull[slot] &= ~(0b11 << pos);
    _GPIO[port]->pull[slot] |= (level << pos);
#if ENC_MODULE_ENABLED
    _enc_spin_unlock();
#endif
    _pwm_spin_unlock();
    return 0;
}

static inline
uint32_t gpio_pin_pull_get(uint32_t port, uint32_t pin, uint32_t safe)
{
    if ( safe ) {
        if ( port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( pin >= GPIO_PINS_MAX_CNT ) return -2;
    }
    uint32_t slot = pin/16, pos = pin%16*2;
    return (_GPIO[port]->pull[slot] >> pos) & 0b11;
}

static inline
int32_t gpio_pin_multi_drive_set(uint32_t port, uint32_t pin, uint32_t level, uint32_t safe)
{
    if ( safe ) {
        if ( port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( pin >= GPIO_PINS_MAX_CNT ) return -2;
        if ( level >= GPIO_MULTI_DRIVE_LEVEL_CNT ) return -3;
    }
    uint32_t slot = pin/16, pos = pin%16*2;
    _pwm_spin_lock();
#if ENC_MODULE_ENABLED
    _enc_spin_lock();
#endif
    _GPIO[port]->drive[slot] &= ~(0b11 << pos);
    _GPIO[port]->drive[slot] |= (level << pos);
#if ENC_MODULE_ENABLED
    _enc_spin_unlock();
#endif
    _pwm_spin_unlock();
    return 0;
}

static inline
uint32_t gpio_pin_multi_drive_get(uint32_t port, uint32_t pin, uint32_t safe)
{
    if ( safe ) {
        if ( port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( pin >= GPIO_PINS_MAX_CNT ) return -2;
    }
    uint32_t slot = pin/16, pos = pin%16*2;
    return (_GPIO[port]->drive[slot] >> pos) & 0b11;
}

static inline
int32_t gpio_pin_func_set(uint32_t port, uint32_t pin, uint32_t func, uint32_t safe)
{
    if ( safe ) {
        if ( port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( pin >= GPIO_PINS_MAX_CNT ) return -2;
        if ( func >= GPIO_FUNC_CNT ) return -3;
    }
    uint32_t slot = pin/8, pos = pin%8*4;
    _pwm_spin_lock();
#if ENC_MODULE_ENABLED
    _enc_spin_lock();
#endif
    _GPIO[port]->config[slot] &= ~(0b0111 << pos);
    _GPIO[port]->config[slot] |=    (func << pos);
#if ENC_MODULE_ENABLED
    _enc_spin_unlock();
#endif
    _pwm_spin_unlock();
    return 0;
}

static inline
uint32_t gpio_pin_func_get(uint32_t port, uint32_t pin, uint32_t safe)
{
    if ( safe ) {
        if ( port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( pin >= GPIO_PINS_MAX_CNT ) return -2;
    }
    uint32_t slot = pin/8, pos = pin%8*4;
    return (_GPIO[port]->config[slot] >> pos) & 0b0111;
}

static inline
uint32_t gpio_pin_get(uint32_t port, uint32_t pin, uint32_t safe)
{
    if ( safe ) {
        if ( port >= GPIO_PORTS_MAX_CNT ) return 0;
        if ( pin >= GPIO_PINS_MAX_CNT ) return 0;
    }
    return _GPIO[port]->data & (1UL << pin) ? HIGH : LOW;
}

static inline
int32_t gpio_pin_set(uint32_t port, uint32_t pin, uint32_t safe)
{
    if ( safe ) {
        if ( port >= GPIO_PORTS_MAX_CNT ) return 0;
        if ( pin >= GPIO_PINS_MAX_CNT ) return 0;
    }
    _pwm_spin_lock();
#if ENC_MODULE_ENABLED
    _enc_spin_lock();
#endif
    _GPIO[port]->data |= (1UL << pin);
#if ENC_MODULE_ENABLED
    _enc_spin_unlock();
#endif
    _pwm_spin_unlock();
    return 0;
}

static inline
int32_t gpio_pin_clr(uint32_t port, uint32_t pin, uint32_t safe)
{
    if ( safe ) {
        if ( port >= GPIO_PORTS_MAX_CNT ) return 0;
        if ( pin >= GPIO_PINS_MAX_CNT ) return 0;
    }
    _pwm_spin_lock();
#if ENC_MODULE_ENABLED
    _enc_spin_lock();
#endif
    _GPIO[port]->data &= ~(1UL << pin);
#if ENC_MODULE_ENABLED
    _enc_spin_unlock();
#endif
    _pwm_spin_unlock();
    return 0;
}

static inline
uint32_t gpio_port_get(uint32_t port, uint32_t safe)
{
    if ( safe ) {
        if ( port >= GPIO_PORTS_MAX_CNT ) return 0;
    }
    return _GPIO[port]->data;
}

static inline
int32_t gpio_port_set(uint32_t port, uint32_t mask, uint32_t safe)
{
    if ( safe )
    {
        if ( port >= GPIO_PORTS_MAX_CNT ) return 0;
    }
    _pwm_spin_lock();
#if ENC_MODULE_ENABLED
    _enc_spin_lock();
#endif
    _GPIO[port]->data |= mask;
#if ENC_MODULE_ENABLED
    _enc_spin_unlock();
#endif
    _pwm_spin_unlock();
    return 0;
}

static inline
int32_t gpio_port_clr(uint32_t port, uint32_t mask, uint32_t safe)
{
    if ( safe )
    {
        if ( port >= GPIO_PORTS_MAX_CNT ) return 0;
    }
    _pwm_spin_lock();
#if ENC_MODULE_ENABLED
    _enc_spin_lock();
#endif
    _GPIO[port]->data &= ~mask;
#if ENC_MODULE_ENABLED
    _enc_spin_unlock();
#endif
    _pwm_spin_unlock();
    return 0;
}

static inline
uint32_t* gpio_all_get(uint32_t safe)
{
    uint32_t port;
    for ( port = GPIO_PORTS_MAX_CNT; port--; ) _gpio_buf[port] = _GPIO[port]->data;
    return (uint32_t*) &_gpio_buf[0];
}

static inline
int32_t gpio_all_set(uint32_t* mask, uint32_t safe)
{
    uint32_t port;
    _pwm_spin_lock();
#if ENC_MODULE_ENABLED
    _enc_spin_lock();
#endif
    for ( port = GPIO_PORTS_MAX_CNT; port--; ) _GPIO[port]->data |= mask[port];
#if ENC_MODULE_ENABLED
    _enc_spin_unlock();
#endif
    _pwm_spin_unlock();
    return 0;
}

static inline
int32_t gpio_all_clr(uint32_t* mask, uint32_t safe)
{
    uint32_t port;
    _pwm_spin_lock();
#if ENC_MODULE_ENABLED
    _enc_spin_lock();
#endif
    for ( port = GPIO_PORTS_MAX_CNT; port--; )
    {
        _GPIO[port]->data &= ~mask[port];
    }
#if ENC_MODULE_ENABLED
    _enc_spin_unlock();
#endif
    _pwm_spin_unlock();
    return 0;
}




static inline
int32_t pwm_cleanup(uint32_t safe)
{
    uint32_t c, d;

    if ( safe ) _pwm_spin_lock();

    for ( c = PWM_CH_MAX_CNT; c--; ) {
        // reset configured pins state
        if ( *_pwmc[c][PWM_CH_P_PORT] < GPIO_PORTS_MAX_CNT && *_pwmc[c][PWM_CH_P_PIN_MSK] ) {
            if ( *_pwmc[c][PWM_CH_P_INV] ) {
                _GPIO[ *_pwmc[c][PWM_CH_P_PORT] ]->data |= *_pwmc[c][PWM_CH_P_PIN_MSK];
            } else {
                _GPIO[ *_pwmc[c][PWM_CH_P_PORT] ]->data &= *_pwmc[c][PWM_CH_P_PIN_MSKN];
            }
        }
        if ( *_pwmc[c][PWM_CH_D_PORT] < GPIO_PORTS_MAX_CNT && *_pwmc[c][PWM_CH_D_PIN_MSK] ) {
            if ( *_pwmc[c][PWM_CH_D_INV] ) {
                _GPIO[ *_pwmc[c][PWM_CH_D_PORT] ]->data |= *_pwmc[c][PWM_CH_D_PIN_MSK];
            } else {
                _GPIO[ *_pwmc[c][PWM_CH_D_PORT] ]->data &= *_pwmc[c][PWM_CH_D_PIN_MSKN];
            }
        }
        // reset channel's data
        for ( d = PWM_CH_DATA_CNT; d--; ) *_pwmc[c][d] = 0;
    }

    // reset module's data
    for ( d = PWM_DATA_CNT; d--; ) *_pwmd[d] = 0;

    if ( safe ) _pwm_spin_unlock();

    return 0;
}

static inline
int32_t pwm_data_set(uint32_t name, uint32_t value, uint32_t safe)
{
    if ( safe ) {
        if ( name >= PWM_DATA_CNT ) return -1;
        if ( name == PWM_CH_CNT && value >= PWM_CH_MAX_CNT ) return -2;
    }
    _pwm_spin_lock();
    *_pwmd[name] = value;
    _pwm_spin_unlock();
    return 0;
}

static inline
uint32_t pwm_data_get(uint32_t name, uint32_t safe)
{
    if ( safe ) {
        if ( name >= PWM_DATA_CNT ) return 0;
    }
    _pwm_spin_lock();
    uint32_t value = *_pwmd[name];
    _pwm_spin_unlock();
    return value;
}

static inline
int32_t pwm_ch_data_set(uint32_t c, uint32_t name, uint32_t value, uint32_t safe)
{
    if ( safe ) {
        if ( c >= PWM_CH_MAX_CNT ) return -1;
        if ( name >= PWM_CH_DATA_CNT ) return -2;
    }
    _pwm_spin_lock();
    *_pwmc[c][name] = value;
    _pwm_spin_unlock();
    return 0;
}

static inline
uint32_t pwm_ch_data_get(uint32_t c, uint32_t name, uint32_t safe)
{
    if ( safe ) {
        if ( c >= PWM_CH_MAX_CNT ) return 0;
        if ( name >= PWM_CH_DATA_CNT ) return 0;
    }
    _pwm_spin_lock();
    uint32_t value = *_pwmc[c][name];
    _pwm_spin_unlock();
    return value;
}

static inline
int32_t pwm_ch_pins_setup (
    uint32_t c,
    uint32_t p_port, uint32_t p_pin, uint32_t p_inv,
    uint32_t d_port, uint32_t d_pin, uint32_t d_inv,
    uint32_t safe
) {
    if ( safe ) {
        if ( c >= PWM_CH_MAX_CNT ) return -1;
        if ( p_port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( p_pin >= GPIO_PINS_MAX_CNT ) return -1;
        if ( d_port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( d_pin >= GPIO_PINS_MAX_CNT ) return -1;
    }

    gpio_pin_func_set(p_port, p_pin, GPIO_FUNC_OUT, safe);
    gpio_pin_pull_set(p_port, p_pin, GPIO_PULL_DISABLE, safe);
    if ( p_inv ) gpio_pin_set(p_port, p_pin, safe);
    else         gpio_pin_clr(p_port, p_pin, safe);

    gpio_pin_func_set(d_port, d_pin, GPIO_FUNC_OUT, safe);
    gpio_pin_pull_set(d_port, d_pin, GPIO_PULL_DISABLE, safe);
    if ( d_inv ) gpio_pin_set(d_port, d_pin, safe);
    else         gpio_pin_clr(d_port, d_pin, safe);

    _pwm_spin_lock();
    *_pwmc[c][PWM_CH_P_PORT] = p_port;
    *_pwmc[c][PWM_CH_P_PIN_MSK] = 1UL << p_pin;
    *_pwmc[c][PWM_CH_P_PIN_MSKN] = ~(1UL << p_pin);
    *_pwmc[c][PWM_CH_P_INV] = p_inv;
    *_pwmc[c][PWM_CH_D_PORT] = d_port;
    *_pwmc[c][PWM_CH_D_PIN_MSK] = 1UL << d_pin;
    *_pwmc[c][PWM_CH_D_PIN_MSKN] = ~(1UL << d_pin);
    *_pwmc[c][PWM_CH_D_INV] = d_inv;
    *_pwmc[c][PWM_CH_D] = 0;
    _pwm_spin_unlock();

    return 0;
}

static inline
int32_t pwm_ch_times_setup (
    uint32_t c,
    int32_t p_freq_mHz, int32_t p_duty_s32, uint32_t p_duty_max_time_ns,
    uint32_t d_hold_ns, uint32_t d_setup_ns,
    uint32_t safe
) {
    uint32_t p_t0, p_t1, p_t1_max, p_period, d_t0, d_t1;
    int32_t d;

    if ( safe ) {
        if ( c >= PWM_CH_MAX_CNT ) return -1;

        if ( !d_hold_ns ) d_hold_ns = 50000;
        if ( !d_setup_ns ) d_setup_ns = 50000;

        // disable channel if it's pins wasn't setup properly
        if (  *_pwmc[c][PWM_CH_P_PORT] >= GPIO_PORTS_MAX_CNT ||
              *_pwmc[c][PWM_CH_D_PORT] >= GPIO_PORTS_MAX_CNT ||
            !(*_pwmc[c][PWM_CH_P_PIN_MSK]) ||
            !(*_pwmc[c][PWM_CH_D_PIN_MSK])
        ) {
            _pwm_spin_lock();
            *_pwmc[c][PWM_CH_STATE] = PWM_CH_STATE_IDLE;
            _pwm_spin_unlock();
            return 0;
        }
    }

    // stop channel if frequency/duty_cycle == 0
    if ( !p_freq_mHz || !p_duty_s32 ) {
        _pwm_spin_lock();
        if ( *_pwmc[c][PWM_CH_STATE] ) *_pwmc[c][PWM_CH_P_STOP] = 1;
        _pwm_spin_unlock();
        return 0;
    }

    d = (p_freq_mHz < 0 ? -1 : 1) * (p_duty_s32 < 0 ? -1 : 1);

    p_duty_s32 = p_duty_s32 < 0 ? -p_duty_s32 : p_duty_s32;
    p_freq_mHz = p_freq_mHz < 0 ? -p_freq_mHz : p_freq_mHz;

    p_period = (uint32_t) ( ((uint64_t)ARISC_CPU_FREQ) * ((uint64_t)1000) / ((uint64_t)p_freq_mHz) );
    p_period = p_period < (2*PWM_WASTED_TICKS) ? 0 : p_period - (2*PWM_WASTED_TICKS);
    p_t1 = (uint32_t) ( ((uint64_t)p_period) * ((uint64_t)p_duty_s32) / ((uint64_t)INT32_MAX) );

    if ( p_duty_max_time_ns ) {
        p_t1_max = ((uint64_t)ARISC_CPU_FREQ) * ((uint64_t)p_duty_max_time_ns) / ((uint64_t)1000000000);
        if ( p_t1 > p_t1_max ) p_t1 = p_t1_max;
    }

    p_t0 = p_period - p_t1;

    d_t0 = ARISC_CPU_FREQ / (1000000000 / d_hold_ns);
    d_t0 = d_t0 < PWM_WASTED_TICKS ? 0 : d_t0 - PWM_WASTED_TICKS;
    d_t1 = ARISC_CPU_FREQ / (1000000000 / d_setup_ns);
    d_t1 = d_t1 < PWM_WASTED_TICKS ? 0 : d_t1 - PWM_WASTED_TICKS;

    _pwm_spin_lock();

    *_pwmc[c][PWM_CH_D_CHANGE] = (d > 0 &&  (*_pwmc[c][PWM_CH_D])) ||
                                 (d < 0 && !(*_pwmc[c][PWM_CH_D])) ? 1 : 0;

    switch ( *_pwmc[c][PWM_CH_STATE] ) {
        case PWM_CH_STATE_IDLE: { *_pwmc[c][PWM_CH_STATE] = PWM_CH_STATE_P0; break; }
        case PWM_CH_STATE_P0: { *_pwmc[c][PWM_CH_TIMEOUT] = p_t0; break; }
        case PWM_CH_STATE_P1: { *_pwmc[c][PWM_CH_TIMEOUT] = p_t1; break; }
        case PWM_CH_STATE_D0: { *_pwmc[c][PWM_CH_TIMEOUT] = d_t0; break; }
        case PWM_CH_STATE_D1: { *_pwmc[c][PWM_CH_TIMEOUT] = d_t1; break; }
    }

    *_pwmc[c][PWM_CH_P_T0] = p_t0;
    *_pwmc[c][PWM_CH_P_T1] = p_t1;
    *_pwmc[c][PWM_CH_D_T0] = d_t0;
    *_pwmc[c][PWM_CH_D_T1] = d_t1;

    _pwm_spin_unlock();

    return 0;
}

// get channel's position in milli-pulses
static inline
int64_t pwm_ch_pos_get(uint32_t c, uint32_t safe)
{
    int64_t pos = 0, a = 0;
    int32_t pos32;
    uint32_t tc, tt;

    if ( safe ) {
        if ( c >= PWM_CH_MAX_CNT ) return 0;
    }

    _pwm_spin_lock();

    pos32 = (int32_t) *_pwmc[c][PWM_CH_POS];
    pos = (int64_t) pos32;
    pos *= 1000;
    tt = *_pwmc[c][PWM_CH_P_T0] + *_pwmc[c][PWM_CH_P_T1];

    if ( tt &&
         ( *_pwmc[c][PWM_CH_STATE] == PWM_CH_STATE_P0 ||
           *_pwmc[c][PWM_CH_STATE] == PWM_CH_STATE_P1 )
    ) {
        tc = *_pwmd[PWM_TIMER_TICK] - *_pwmc[c][PWM_CH_P_TICK];
        if ( tc > tt ) tc = tt;
        a = 1000 * tc / tt;
        a = *_pwmc[c][PWM_CH_D] ? 1000 - a : a - 1000;
    }

    _pwm_spin_unlock();

    return pos + a;
}




#if ENC_MODULE_ENABLED
static inline
int32_t enc_cleanup(uint32_t safe)
{
    uint32_t c, d;
    if ( safe ) _enc_spin_lock();
    for ( c = ENC_CH_MAX_CNT; c--; ) {
        for ( d = ENC_CH_DATA_CNT; d--; ) *_encc[c][d] = 0;
    }
    for ( d = ENC_DATA_CNT; d--; ) *_encd[d] = 0;
    if ( safe ) _enc_spin_unlock();
    return 0;
}

static inline
int32_t enc_data_set(uint32_t name, uint32_t value, uint32_t safe)
{
    if ( safe ) {
        if ( name >= ENC_DATA_CNT ) return -1;
        if ( name == ENC_CH_CNT && value >= ENC_CH_MAX_CNT ) return -2;
    }
    _enc_spin_lock();
    *_encd[name] = value;
    _enc_spin_unlock();
    return 0;
}

static inline
uint32_t enc_data_get(uint32_t name, uint32_t safe)
{
    if ( safe ) {
        if ( name >= ENC_DATA_CNT ) return 0;
    }
    _enc_spin_lock();
    uint32_t value = *_encd[name];
    _enc_spin_unlock();
    return value;
}

static inline
int32_t enc_ch_data_set(uint32_t c, uint32_t name, uint32_t value, uint32_t safe)
{
    if ( safe ) {
        if ( c >= ENC_CH_MAX_CNT ) return -1;
        if ( name >= ENC_CH_DATA_CNT ) return -2;
    }
    _enc_spin_lock();
    *_encc[c][name] = (name == ENC_CH_POS) ? (int32_t)value : value;
    _enc_spin_unlock();
    return 0;
}

static inline
uint32_t enc_ch_data_get(uint32_t c, uint32_t name, uint32_t safe)
{
    if ( safe ) {
        if ( c >= ENC_CH_MAX_CNT ) return 0;
        if ( name >= ENC_CH_DATA_CNT ) return 0;
    }
    _enc_spin_lock();
    uint32_t value = *_encc[c][name];
    _enc_spin_unlock();
    return value;
}

static inline
int32_t enc_ch_pins_setup(
    uint32_t c,
    uint32_t a_port, uint32_t a_pin, uint32_t a_inv, uint32_t a_all,
    uint32_t b_port, uint32_t b_pin,
    uint32_t z_port, uint32_t z_pin, uint32_t z_inv, uint32_t z_all,
    uint32_t safe
) {
    uint32_t b_use = 0, z_use = 0;

    if ( safe )
    {
        if ( c >= ENC_CH_MAX_CNT ) return -1;
        if ( a_port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( a_pin >= GPIO_PINS_MAX_CNT ) return -1;
    }

    if ( b_port < GPIO_PORTS_MAX_CNT && b_pin < GPIO_PINS_MAX_CNT ) b_use = 1;
    if ( z_port < GPIO_PORTS_MAX_CNT && z_pin < GPIO_PINS_MAX_CNT ) z_use = 1;

    gpio_pin_func_set(a_port, a_pin, GPIO_FUNC_IN, safe);
    gpio_pin_pull_set(a_port, a_pin, GPIO_PULL_DISABLE, safe);
    if ( b_use ) {
        gpio_pin_func_set(b_port, b_pin, GPIO_FUNC_IN, safe);
        gpio_pin_pull_set(b_port, b_pin, GPIO_PULL_DISABLE, safe);
    }
    if ( z_use ) {
        gpio_pin_func_set(z_port, z_pin, GPIO_FUNC_IN, safe);
        gpio_pin_pull_set(z_port, z_pin, GPIO_PULL_DISABLE, safe);
    }

    _enc_spin_lock();
    *_encc[c][ENC_CH_A_PORT] = a_port;
    *_encc[c][ENC_CH_A_PIN_MSK] = 1UL << a_pin;
    *_encc[c][ENC_CH_A_INV] = a_inv;
    *_encc[c][ENC_CH_A_ALL] = a_all;
    *_encc[c][ENC_CH_B_USE] = b_use;
    if ( b_use ) {
        *_encc[c][ENC_CH_B_PORT] = b_port;
        *_encc[c][ENC_CH_B_PIN_MSK] = 1UL << b_pin;
    }
    *_encc[c][ENC_CH_Z_USE] = z_use;
    if ( b_port < GPIO_PORTS_MAX_CNT && b_pin < GPIO_PINS_MAX_CNT ) {
        *_encc[c][ENC_CH_Z_PORT] = z_port;
        *_encc[c][ENC_CH_Z_PIN_MSK] = 1UL << z_pin;
        *_encc[c][ENC_CH_Z_INV] = z_inv;
        *_encc[c][ENC_CH_Z_ALL] = z_all;
    }
    _enc_spin_unlock();

    return 0;
}

static inline
int32_t enc_ch_pos_get(uint32_t c, uint32_t safe)
{
    if ( safe ) {
        if ( c >= ENC_CH_MAX_CNT ) return 0;
    }
    _enc_spin_lock();
    int32_t value = (int32_t) *_encc[c][ENC_CH_POS];
    _enc_spin_unlock();
    return value;
}

static inline
int32_t enc_ch_pos_set(uint32_t c, int32_t pos, uint32_t safe)
{
    if ( safe ) {
        if ( c >= ENC_CH_MAX_CNT ) return -1;
    }
    _enc_spin_lock();
    *_encc[c][ENC_CH_POS] = (uint32_t) pos;
    _enc_spin_unlock();
    return 0;
}
#endif



static inline
int32_t shmem_init(const char *comp_name)
{
    int32_t mem_fd;
    uint32_t addr, off, port, ch, name, *p;

    // open physical memory file
    seteuid(0);
    setfsuid( geteuid() );
    mem_fd = open("/dev/mem", O_RDWR|O_SYNC);
    if ( mem_fd  < 0 ) PRINT_ERROR_AND_RETURN("ERROR: can't open /dev/mem file\n",-1);
    setfsuid( getuid() );

    // mmap shmem
    addr = ARISC_SHM_BASE & ~(ARISC_SHM_SIZE - 1);
    off = ARISC_SHM_BASE & (ARISC_SHM_SIZE - 1);
    _shm_vrt_addr = mmap(NULL, ARISC_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, addr);
    if (_shm_vrt_addr == MAP_FAILED) PRINT_ERROR_AND_RETURN("ERROR: shm mmap() failed\n",-1);
    p = _shm_vrt_addr + off/4;
    for ( name = 0; name < PWM_DATA_CNT; name++, p++ ) _pwmd[name] = p;
    for ( ch = 0; ch < PWM_CH_MAX_CNT; ch++ ) {
        for ( name = 0; name < PWM_CH_DATA_CNT; name++, p++ ) _pwmc[ch][name] = p;
    }
#if ENC_MODULE_ENABLED
    for ( name = 0; name < ENC_DATA_CNT; name++, p++ ) _encd[name] = p;
    for ( ch = 0; ch < ENC_CH_MAX_CNT; ch++ ) {
        for ( name = 0; name < ENC_CH_DATA_CNT; name++, p++ ) _encc[ch][name] = p;
    }
#endif

    // mmap gpio
    addr = GPIO_BASE & ~(4096 - 1);
    off = GPIO_BASE & (4096 - 1);
    _gpio_vrt_addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, addr);
    if (_gpio_vrt_addr == MAP_FAILED) PRINT_ERROR_AND_RETURN("ERROR: gpio mmap() failed\n",-3);
    for ( port = PA; port <= PG; ++port )
    {
        _GPIO[port] = (_GPIO_PORT_REG_t *)(_gpio_vrt_addr + (off + port*0x24)/4);
    }

    // mmap r_gpio (PL)
    addr = GPIO_R_BASE & ~(4096 - 1);
    off = GPIO_R_BASE & (4096 - 1);
    _r_gpio_vrt_addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, addr);
    if (_r_gpio_vrt_addr == MAP_FAILED) PRINT_ERROR_AND_RETURN("ERROR: r_gpio mmap() failed\n",-4);
    _GPIO[PL] = (_GPIO_PORT_REG_t *)(_r_gpio_vrt_addr + off/4);

    // no need to keep phy memory file open after mmap
    close(mem_fd);

    return 0;
}

static inline
void shmem_deinit(void)
{
    munmap(_shm_vrt_addr, ARISC_SHM_SIZE);
    munmap(_gpio_vrt_addr, 4096);
    munmap(_r_gpio_vrt_addr, 4096);
}




#endif
