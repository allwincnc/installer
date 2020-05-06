#ifndef _ARISC_API_H
#define _ARISC_API_H

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




#define ARISC_FW_BASE           (0x00040000) // for ARM CPU it's 0x00040000
#define ARISC_FW_SIZE           ((8+8+32)*1024)
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




#define PG_CH_MAX_CNT       16
#define PG_CH_SLOT_MAX_CNT  4
#define PG_CH_SLOT_MAX      (PG_CH_SLOT_MAX_CNT - 1)

enum
{
    PG_PORT,
    PG_PIN_MSK,
    PG_PIN_MSKN,
    PG_TOGGLES,
    PG_T0,
    PG_T1,
    PG_TICK,
    PG_TIMEOUT,
    PG_CH_DATA_CNT
};

enum
{
    PG_TIMER_TICK,
    PG_ARM_LOCK,
    PG_ARISC_LOCK,
    PG_CH_CNT,
    PG_DATA_CNT
};

#define PG_SHM_BASE         (ARISC_SHM_BASE)
#define PG_SHM_CH_SLOT_BASE (PG_SHM_BASE)
#define PG_SHM_CH_DATA_BASE (PG_SHM_CH_SLOT_BASE + PG_CH_MAX_CNT*4)
#define PG_SHM_DATA_BASE    (PG_SHM_CH_DATA_BASE + PG_CH_MAX_CNT*PG_CH_DATA_CNT*PG_CH_SLOT_MAX_CNT*4)
#define PG_SHM_SIZE         (PG_SHM_DATA_BASE + PG_DATA_CNT*4)




typedef struct {
    uint32_t busy;
    int32_t  pos;
    int32_t  dir;
    uint32_t port[2];
    uint32_t pin_msk[2];
    uint32_t pin_mskn[2];
} _stepgen_ch_t;

enum { STEP, DIR };

#define STEPGEN_CH_MAX_CNT 16




#define PRINT_ERROR(MSG) \
    rtapi_print_msg(RTAPI_MSG_ERR, "%s: "MSG"\n", comp_name)

#define PRINT_ERROR_AND_RETURN(MSG,RETVAL) \
    { PRINT_ERROR(MSG); return RETVAL; }




static uint32_t *_shm_vrt_addr, *_gpio_vrt_addr, *_r_gpio_vrt_addr;

volatile _GPIO_PORT_REG_t *_GPIO[GPIO_PORTS_MAX_CNT] = {0};
static uint32_t _gpio_buf[GPIO_PORTS_MAX_CNT] = {0};

volatile uint32_t * _pgs[PG_CH_MAX_CNT] = {0};
volatile uint32_t * _pgc[PG_CH_MAX_CNT][PG_CH_SLOT_MAX_CNT][PG_CH_DATA_CNT] = {0};
volatile uint32_t * _pgd[PG_DATA_CNT] = {0};

volatile _stepgen_ch_t _sgc[STEPGEN_CH_MAX_CNT] = {0};




static inline
void _spin_lock()
{
    *_pgd[PG_ARM_LOCK] = 1;
    while ( *_pgd[PG_ARISC_LOCK] );
}

static inline
void _spin_unlock()
{
    *_pgd[PG_ARM_LOCK] = 0;
}

static inline
int32_t spin_lock_test(uint32_t usec)
{
    _spin_lock();
    usleep(usec);
    _spin_unlock();
    return 0;
}

static inline
int32_t gpio_pin_pull_set(uint32_t port, uint32_t pin, uint32_t level, uint32_t safe)
{
    if ( safe )
    {
        if ( port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( pin >= GPIO_PINS_MAX_CNT ) return -2;
        if ( level >= GPIO_PULL_CNT ) return -3;
    }
    uint32_t slot = pin/16, pos = pin%16*2;
    _spin_lock();
    _GPIO[port]->pull[slot] &= ~(0b11 << pos);
    _GPIO[port]->pull[slot] |= (level << pos);
    _spin_unlock();
    return 0;
}

static inline
uint32_t gpio_pin_pull_get(uint32_t port, uint32_t pin, uint32_t safe)
{
    if ( safe )
    {
        if ( port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( pin >= GPIO_PINS_MAX_CNT ) return -2;
    }
    uint32_t slot = pin/16, pos = pin%16*2;
    return (_GPIO[port]->pull[slot] >> pos) & 0b11;
}

static inline
int32_t gpio_pin_multi_drive_set(uint32_t port, uint32_t pin, uint32_t level, uint32_t safe)
{
    if ( safe )
    {
        if ( port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( pin >= GPIO_PINS_MAX_CNT ) return -2;
        if ( level >= GPIO_MULTI_DRIVE_LEVEL_CNT ) return -3;
    }
    uint32_t slot = pin/16, pos = pin%16*2;
    _spin_lock();
    _GPIO[port]->drive[slot] &= ~(0b11 << pos);
    _GPIO[port]->drive[slot] |= (level << pos);
    _spin_unlock();
    return 0;
}

static inline
uint32_t gpio_pin_multi_drive_get(uint32_t port, uint32_t pin, uint32_t safe)
{
    if ( safe )
    {
        if ( port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( pin >= GPIO_PINS_MAX_CNT ) return -2;
    }
    uint32_t slot = pin/16, pos = pin%16*2;
    return (_GPIO[port]->drive[slot] >> pos) & 0b11;
}

static inline
int32_t gpio_pin_func_set(uint32_t port, uint32_t pin, uint32_t func, uint32_t safe)
{
    if ( safe )
    {
        if ( port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( pin >= GPIO_PINS_MAX_CNT ) return -2;
        if ( func >= GPIO_FUNC_CNT ) return -3;
    }
    uint32_t slot = pin/8, pos = pin%8*4;
    _spin_lock();
    _GPIO[port]->config[slot] &= ~(0b0111 << pos);
    _GPIO[port]->config[slot] |=    (func << pos);
    _spin_unlock();
    return 0;
}

static inline
uint32_t gpio_pin_func_get(uint32_t port, uint32_t pin, uint32_t safe)
{
    if ( safe )
    {
        if ( port >= GPIO_PORTS_MAX_CNT ) return -1;
        if ( pin >= GPIO_PINS_MAX_CNT ) return -2;
    }
    uint32_t slot = pin/8, pos = pin%8*4;
    return (_GPIO[port]->config[slot] >> pos) & 0b0111;
}

static inline
uint32_t gpio_pin_get(uint32_t port, uint32_t pin, uint32_t safe)
{
    if ( safe )
    {
        if ( port >= GPIO_PORTS_MAX_CNT ) return 0;
        if ( pin >= GPIO_PINS_MAX_CNT ) return 0;
    }
    return _GPIO[port]->data & (1UL << pin) ? HIGH : LOW;
}

static inline
int32_t gpio_pin_set(uint32_t port, uint32_t pin, uint32_t safe)
{
    if ( safe )
    {
        if ( port >= GPIO_PORTS_MAX_CNT ) return 0;
        if ( pin >= GPIO_PINS_MAX_CNT ) return 0;
    }
    _spin_lock();
    _GPIO[port]->data |= (1UL << pin);
    _spin_unlock();
    return 0;
}

static inline
int32_t gpio_pin_clr(uint32_t port, uint32_t pin, uint32_t safe)
{
    if ( safe )
    {
        if ( port >= GPIO_PORTS_MAX_CNT ) return 0;
        if ( pin >= GPIO_PINS_MAX_CNT ) return 0;
    }
    _spin_lock();
    _GPIO[port]->data &= ~(1UL << pin);
    _spin_unlock();
    return 0;
}

static inline
uint32_t gpio_port_get(uint32_t port, uint32_t safe)
{
    if ( safe )
    {
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
    _spin_lock();
    _GPIO[port]->data |= mask;
    _spin_unlock();
    return 0;
}

static inline
int32_t gpio_port_clr(uint32_t port, uint32_t mask, uint32_t safe)
{
    if ( safe )
    {
        if ( port >= GPIO_PORTS_MAX_CNT ) return 0;
    }
    _spin_lock();
    _GPIO[port]->data &= ~mask;
    _spin_unlock();
    return 0;
}

static inline
uint32_t* gpio_all_get(uint32_t safe)
{
    uint32_t port;
    for ( port = GPIO_PORTS_MAX_CNT; port--; )
        _gpio_buf[port] = _GPIO[port]->data;
    return (uint32_t*) &_gpio_buf[0];
}

static inline
int32_t gpio_all_set(uint32_t* mask, uint32_t safe)
{
    uint32_t port;
    _spin_lock();
    for ( port = GPIO_PORTS_MAX_CNT; port--; )
    {
        _GPIO[port]->data |= mask[port];
    }
    _spin_unlock();
    return 0;
}

static inline
int32_t gpio_all_clr(uint32_t* mask, uint32_t safe)
{
    uint32_t port;
    _spin_lock();
    for ( port = GPIO_PORTS_MAX_CNT; port--; )
    {
        _GPIO[port]->data &= ~mask[port];
    }
    _spin_unlock();
    return 0;
}




static inline
int32_t pg_data_set(uint32_t name, uint32_t value, uint32_t safe)
{
    if ( safe )
    {
        if ( name >= PG_DATA_CNT ) return -1;
        if ( name == PG_CH_CNT && value >= PG_CH_MAX_CNT ) return -4;
    }
    _spin_lock();
    *_pgd[name] = value;
    _spin_unlock();
    return 0;
}

static inline
uint32_t pg_data_get(uint32_t name, uint32_t safe)
{
    if ( safe )
    {
        if ( name >= PG_DATA_CNT ) return 0;
    }
    _spin_lock();
    uint32_t value = *_pgd[name];
    _spin_unlock();
    return value;
}

static inline
int32_t pg_ch_data_set(uint32_t c, uint32_t s, uint32_t name, uint32_t value, uint32_t safe)
{
    if ( safe )
    {
        if ( c >= PG_CH_MAX_CNT ) return -1;
        if ( s >= PG_CH_SLOT_MAX_CNT ) return -2;
        if ( name >= PG_CH_DATA_CNT ) return -3;
    }
    _spin_lock();
    *_pgc[c][s][name] = value;
    _spin_unlock();
    return 0;
}

static inline
uint32_t pg_ch_data_get(uint32_t c, uint32_t s, uint32_t name, uint32_t safe)
{
    if ( safe )
    {
        if ( c >= PG_CH_MAX_CNT ) return 0;
        if ( s >= PG_CH_SLOT_MAX_CNT ) return -1;
        if ( name >= PG_CH_DATA_CNT ) return -2;
    }
    _spin_lock();
    uint32_t value = *_pgc[c][s][name];
    _spin_unlock();
    return value;
}

static inline
int32_t pg_ch_slot_set(uint32_t c, uint32_t value, uint32_t safe)
{
    if ( safe )
    {
        if ( c >= PG_CH_MAX_CNT ) return -1;
    }
    _spin_lock();
    *_pgs[c] = value;
    _spin_unlock();
    return 0;
}

static inline
uint32_t pg_ch_slot_get(uint32_t c, uint32_t safe)
{
    if ( safe )
    {
        if ( c >= PG_CH_MAX_CNT ) return 0;
    }
    _spin_lock();
    uint32_t value = *_pgs[c];
    _spin_unlock();
    return value;
}

static inline
void _stepgen_ch_setup(uint32_t c)
{
    if ( _sgc[c].busy ) return;

    _sgc[c].pos = 0;
    _sgc[c].dir = 1;
    _sgc[c].busy = 1;

    uint32_t pg_ch_cnt = *_pgd[PG_CH_CNT];
    if ( c >= *_pgd[PG_CH_CNT] ) pg_ch_cnt = c + 1;

    _spin_lock();
    *_pgd[PG_CH_CNT] = pg_ch_cnt;
    _spin_unlock();
}

static inline
int32_t stepgen_pin_setup(uint32_t c, uint8_t type, uint32_t port, uint32_t pin, uint32_t invert, uint32_t safe)
{
    if ( safe )
    {
        if ( c >= STEPGEN_CH_MAX_CNT ) return -1;
        if ( type >= 2 ) return -2;
        if ( port >= GPIO_PORTS_MAX_CNT ) return -3;
        if ( pin >= GPIO_PINS_MAX_CNT ) return -4;
        _stepgen_ch_setup(c);
    }

    _sgc[c].port[type] = port;
    _sgc[c].pin_msk[type] = 1UL << pin;
    _sgc[c].pin_mskn[type] = ~(_sgc[c].pin_msk[type]);

    gpio_pin_func_set(port, pin, GPIO_FUNC_OUT, safe);
    gpio_pin_pull_set(port, pin, GPIO_PULL_DISABLE, safe);
    if ( invert ) gpio_pin_set(port, pin, safe);
    else gpio_pin_clr(port, pin, safe);

    return 0;
}

static inline
int32_t stepgen_task_add(uint32_t c, int32_t pulses, uint32_t time, uint32_t safe)
{
    if ( safe )
    {
        if ( c >= STEPGEN_CH_MAX_CNT ) return -1;
        if ( !pulses ) return -2;
        _stepgen_ch_setup(c);
    }

    uint32_t s = *_pgs[c];
    uint32_t i = PG_CH_SLOT_MAX_CNT;
    uint32_t t = 0;

    for ( ; *_pgc[c][s][PG_TOGGLES] && i--; )
    {
        t += (*_pgc[c][s][PG_TOGGLES]) * (*_pgc[c][s][PG_T0]);
        s = (s+1) & PG_CH_SLOT_MAX;
    }

    t /= 2;
    if ( *_pgc[c][s][PG_TOGGLES] || t > time ) return -3;

    _sgc[c].pos += pulses;

    uint32_t stp_tgs = 2*((uint32_t)abs(pulses));
    uint32_t dir_new = (pulses > 0) ? 1 : -1;
    uint32_t dir_tgs = (_sgc[c].dir != dir_new) ? 1 : 0;

    time -= t;
    time = (uint32_t) ((uint64_t)time * 450 / 1000);
    time = time / (stp_tgs + dir_tgs);

    *_pgc[c][s][PG_TICK] = *_pgd[PG_TIMER_TICK];

    if ( dir_tgs )
    {
        _sgc[c].dir = dir_new;
        *_pgc[c][s][PG_PORT] = _sgc[c].port[DIR];
        *_pgc[c][s][PG_PIN_MSK] = _sgc[c].pin_msk[DIR];
        *_pgc[c][s][PG_PIN_MSKN] = _sgc[c].pin_mskn[DIR];
        *_pgc[c][s][PG_TIMEOUT] = 0;
        *_pgc[c][s][PG_T0] = time;
        *_pgc[c][s][PG_T1] = time;
        *_pgc[c][s][PG_TOGGLES] = 2;
        s = (s+1) & PG_CH_SLOT_MAX;
        *_pgc[c][s][PG_TOGGLES] = 0;
    }

    *_pgc[c][s][PG_PORT] = _sgc[c].port[STEP];
    *_pgc[c][s][PG_PIN_MSK] = _sgc[c].pin_msk[STEP];
    *_pgc[c][s][PG_PIN_MSKN] = _sgc[c].pin_mskn[STEP];
    *_pgc[c][s][PG_TIMEOUT] = 0;
    *_pgc[c][s][PG_T0] = time;
    *_pgc[c][s][PG_T1] = time;
    *_pgc[c][s][PG_TOGGLES] = 1 + stp_tgs;

    return 0;
}

static inline
int32_t stepgen_pos_get(uint32_t c, uint32_t safe)
{
    if ( safe )
    {
        if ( c >= STEPGEN_CH_MAX_CNT ) return 0;
    }
    return _sgc[c].pos;
}

static inline
int32_t stepgen_pos_set(uint32_t c, int32_t pos, uint32_t safe)
{
    if ( safe )
    {
        if ( c >= STEPGEN_CH_MAX_CNT ) return -1;
    }
    _sgc[c].pos = pos;
    return 0;
}

static inline
int32_t stepgen_cleanup()
{
    uint32_t *p, i;

    _spin_lock();
    for ( i = PG_CH_MAX_CNT, p = (uint32_t*)_pgs[0]; i--; p++ ) *p = 0;
    for ( i = PG_CH_MAX_CNT*PG_CH_SLOT_MAX_CNT*PG_CH_DATA_CNT, p = (uint32_t*)_pgc[0][0][0]; i--; p++ ) *p = 0;
    for ( i = PG_DATA_CNT, p = (uint32_t*)_pgd[0]; i--; p++ ) *p = 0;
    _spin_unlock();

    for ( i = sizeof(_stepgen_ch_t)*STEPGEN_CH_MAX_CNT/4, p = (uint32_t*)_sgc; i--; p++ ) *p = 0;

    return 0;
}




static inline
int32_t shmem_init(const char *comp_name)
{
    int32_t mem_fd;
    uint32_t addr, off, port, ch, name, *p, s;

    // open physical memory file
    seteuid(0);
    setfsuid( geteuid() );
    mem_fd = open("/dev/mem", O_RDWR|O_SYNC);
    if ( mem_fd  < 0 ) PRINT_ERROR_AND_RETURN("ERROR: can't open /dev/mem file\n",-1);
    setfsuid( getuid() );

    // mmap shmem
    addr = PG_SHM_BASE & ~(4096 - 1);
    off = PG_SHM_BASE & (4096 - 1);
    _shm_vrt_addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, addr);
    if (_shm_vrt_addr == MAP_FAILED) PRINT_ERROR_AND_RETURN("ERROR: shm mmap() failed\n",-2);
    p = _shm_vrt_addr + off/4;
    for ( s = 0; s < PG_CH_MAX_CNT; s++, p++ ) _pgs[s] = p;
    for ( ch = 0; ch < PG_CH_MAX_CNT; ch++ ) {
        for ( s = 0; s < PG_CH_SLOT_MAX_CNT; s++ ) {
            for ( name = 0; name < PG_CH_DATA_CNT; name++, p++ ) _pgc[ch][s][name] = p;
        }
    }
    for ( name = 0; name < PG_DATA_CNT; name++, p++ ) _pgd[name] = p;

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
    munmap(_shm_vrt_addr, 4096);
    munmap(_gpio_vrt_addr, 4096);
    munmap(_r_gpio_vrt_addr, 4096);
}




#endif
