#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "rtapi.h"
#include "rtapi_app.h"
#include "hal.h"

#include "arisc_api.h"

MODULE_AUTHOR("MX_Master");
MODULE_DESCRIPTION("GPIO driver for the Allwinner ARISC firmware");
MODULE_LICENSE("GPL");




static int32_t comp_id;
static const uint8_t * comp_name = "arisc_gpio";

static int8_t *in = "";
RTAPI_MP_STRING(in, "input pins, comma separated");

static int8_t *out = "";
RTAPI_MP_STRING(out, "output pins, comma separated");

static const char *gpio_name[GPIO_PORTS_MAX_CNT] =
    {"PA","PB","PC","PD","PE","PF","PG","PL"};

static hal_bit_t **gpio_hal_0[GPIO_PORTS_MAX_CNT];
static hal_bit_t **gpio_hal_1[GPIO_PORTS_MAX_CNT];
static hal_bit_t gpio_hal_0_prev[GPIO_PORTS_MAX_CNT][GPIO_PINS_MAX_CNT];
static hal_bit_t gpio_hal_1_prev[GPIO_PORTS_MAX_CNT][GPIO_PINS_MAX_CNT];

static hal_s32_t **gpio_hal_pull[GPIO_PORTS_MAX_CNT];
static hal_s32_t gpio_hal_pull_prev[GPIO_PORTS_MAX_CNT][GPIO_PINS_MAX_CNT];

static hal_u32_t **gpio_hal_drive[GPIO_PORTS_MAX_CNT];
static hal_u32_t gpio_hal_drive_prev[GPIO_PORTS_MAX_CNT][GPIO_PINS_MAX_CNT];

static uint32_t gpio_out_mask[GPIO_PORTS_MAX_CNT] = {0};
static uint32_t gpio_in_mask[GPIO_PORTS_MAX_CNT] = {0};

static uint32_t gpio_in_cnt = 0;
static uint32_t gpio_out_cnt = 0;
static uint32_t gpio_ports_cnt = 0;
static uint32_t gpio_pins_cnt[GPIO_PINS_MAX_CNT] = {0};

static uint32_t pin_msk[GPIO_PINS_MAX_CNT] = {0};




// TOOLS

static void gpio_write(void *arg, long period);
static void gpio_read(void *arg, long period);

static inline
int32_t malloc_and_export(const char *comp_name, int32_t comp_id)
{
    int8_t* arg_str[2] = {in, out};
    int8_t n, r;
    uint8_t port;
    char name[HAL_NAME_LEN + 1];

    // init some vars
    for ( n = GPIO_PINS_MAX_CNT; n--; ) pin_msk[n] = 1UL << n;

    // shared memory allocation
    for ( port = GPIO_PORTS_MAX_CNT; port--; )
    {
        gpio_hal_0[port] = hal_malloc(GPIO_PINS_MAX_CNT * sizeof(hal_bit_t *));
        gpio_hal_1[port] = hal_malloc(GPIO_PINS_MAX_CNT * sizeof(hal_bit_t *));
        gpio_hal_pull[port] = hal_malloc(GPIO_PINS_MAX_CNT * sizeof(hal_s32_t *));
        gpio_hal_drive[port] = hal_malloc(GPIO_PINS_MAX_CNT * sizeof(hal_u32_t *));

        if ( !gpio_hal_0[port] || !gpio_hal_1[port] ||
             !gpio_hal_pull[port] || !gpio_hal_drive[port] )
        {
            rtapi_print_msg(RTAPI_MSG_ERR,
                "%s: port %s hal_malloc() failed \n",
                comp_name, gpio_name[port]);
            return -1;
        }
    }

    // export HAL pins
    for ( n = 2; n--; )
    {
        if ( !arg_str[n] ) continue;

        int8_t *data = arg_str[n], *token;
        uint8_t pin, found;
        int32_t retval;
        int8_t* type_str = n ? "out" : "in";

        while ( (token = strtok(data, ",")) != NULL )
        {
            if ( data != NULL ) data = NULL;
            if ( strlen(token) < 3 ) continue;

            // trying to find a correct port name
            for ( found = 0, port = GPIO_PORTS_MAX_CNT; port--; )
            {
                if ( 0 == memcmp(token, gpio_name[port], 2) )
                {
                    found = 1;
                    break;
                }
            }

            if ( !found ) continue;

            // trying to find a correct pin number
            pin = (uint8_t) strtoul(&token[2], NULL, 10);

            if ( (pin == 0 && token[2] != '0') || pin >= GPIO_PINS_MAX_CNT ) continue;

            // export pin function
            retval = hal_pin_bit_newf( (n ? HAL_IN : HAL_OUT),
                &gpio_hal_0[port][pin], comp_id,
                "%s.%s-%s", comp_name, token, type_str);

            // export pin inverted function
            retval += hal_pin_bit_newf( (n ? HAL_IN : HAL_OUT),
                &gpio_hal_1[port][pin], comp_id,
                "%s.%s-%s-not", comp_name, token, type_str);

            // export pin pull up/down function
            retval += hal_pin_s32_newf( HAL_IN,
                &gpio_hal_pull[port][pin], comp_id,
                "%s.%s-pull", comp_name, token);

            // export pin multi-drive (open drain) function
            retval += hal_pin_u32_newf( HAL_IN,
                &gpio_hal_drive[port][pin], comp_id,
                "%s.%s-multi-drive-level", comp_name, token);

            if (retval < 0)
            {
                rtapi_print_msg(RTAPI_MSG_ERR, "%s: pin %s export failed \n",
                    comp_name, token);
                return -1;
            }

            // configure GPIO pin
            if ( n )
            {
                gpio_out_cnt++;
                gpio_out_mask[port] |= pin_msk[pin];
                gpio_pin_func_set(port, pin, GPIO_FUNC_OUT, 0);
            }
            else
            {
                gpio_in_cnt++;
                gpio_in_mask[port] |= pin_msk[pin];
                gpio_pin_func_set(port, pin, GPIO_FUNC_IN, 0);
            }

            // disable pull up/down
            gpio_pin_pull_set(port, pin, GPIO_PULL_DISABLE, 0);

            // get/set pin init state
            *gpio_hal_0[port][pin] = gpio_pin_get(port, pin, 0);
            *gpio_hal_1[port][pin] = *gpio_hal_0[port][pin] ? 0 : 1;
            gpio_hal_0_prev[port][pin] = *gpio_hal_0[port][pin];
            gpio_hal_1_prev[port][pin] = *gpio_hal_1[port][pin];

            // get pin pull up/down state
            switch ( gpio_pin_pull_get(port, pin, 0) )
            {
                case GPIO_PULL_UP:      *gpio_hal_pull[port][pin] = 1;
                case GPIO_PULL_DOWN:    *gpio_hal_pull[port][pin] = -1;
                default:                *gpio_hal_pull[port][pin] = 0;
            }
            gpio_hal_pull_prev[port][pin] = *gpio_hal_pull[port][pin];

            // get pin multi-drive (open drain) state
            *gpio_hal_drive[port][pin] = gpio_pin_multi_drive_get(port, pin, 0);
            gpio_hal_drive_prev[port][pin] = *gpio_hal_drive[port][pin];

            // used ports count update
            if ( port >= gpio_ports_cnt ) gpio_ports_cnt = port + 1;
            // used port pins count update
            if ( pin >= gpio_pins_cnt[port] ) gpio_pins_cnt[port] = pin + 1;
        }

    }

    // export HAL functions
    r = 0;
    rtapi_snprintf(name, sizeof(name), "%s.write", comp_name);
    r += hal_export_funct(name, gpio_write, 0, 0, 0, comp_id);
    rtapi_snprintf(name, sizeof(name), "%s.read", comp_name);
    r += hal_export_funct(name, gpio_read, 0, 0, 0, comp_id);
    if ( r )
    {
        rtapi_print_msg(RTAPI_MSG_ERR, "%s: [GPIO] functions export failed\n", comp_name);
        return -1;
    }

    return 0;
}




// HAL functions

static
void gpio_read(void *arg, long period)
{
    static uint32_t port, pin, port_state;

    if ( !gpio_in_cnt ) return;

    for ( port = gpio_ports_cnt; port--; )
    {
        if ( !gpio_in_mask[port] ) continue;

        port_state = gpio_port_get(port, 0);

        for ( pin = gpio_pins_cnt[port]; pin--; )
        {
            if ( !(gpio_in_mask[port] & pin_msk[pin]) ) continue;

            if ( port_state & pin_msk[pin] )
            {
                *gpio_hal_0[port][pin] = 1;
                *gpio_hal_1[port][pin] = 0;
            }
            else
            {
                *gpio_hal_0[port][pin] = 0;
                *gpio_hal_1[port][pin] = 1;
            }
        }
    }
}

static
void gpio_write(void *arg, long period)
{
    static uint32_t port, pin, mask_0, mask_1;

    if ( !gpio_in_cnt && !gpio_out_cnt ) return;

    for ( port = gpio_ports_cnt; port--; )
    {
        if ( !gpio_in_mask[port] && !gpio_out_mask[port] ) continue;

        mask_0 = 0;
        mask_1 = 0;

        for ( pin = gpio_pins_cnt[port]; pin--; )
        {
            if ( !(gpio_in_mask[port] & pin_msk[pin]) &&
                 !(gpio_out_mask[port] & pin_msk[pin]) ) continue;

            // set pin pull up/down state
            if ( gpio_hal_pull_prev[port][pin] != *gpio_hal_pull[port][pin] )
            {
                if ( *gpio_hal_pull[port][pin] > 0 )
                {
                    *gpio_hal_pull[port][pin] = 1;
                    gpio_pin_pull_set(port, pin, GPIO_PULL_UP, 0);
                }
                else if ( *gpio_hal_pull[port][pin] < 0 )
                {
                    *gpio_hal_pull[port][pin] = -1;
                    gpio_pin_pull_set(port, pin, GPIO_PULL_DOWN, 0);
                }
                else gpio_pin_pull_set(port, pin, GPIO_PULL_DISABLE, 0);
                gpio_hal_pull_prev[port][pin] = *gpio_hal_pull[port][pin];
            }

            // set pin multi-drive (open drain) state
            if ( gpio_hal_drive_prev[port][pin] != *gpio_hal_drive[port][pin] )
            {
                *gpio_hal_drive[port][pin] &= (GPIO_PULL_CNT - 1);
                gpio_pin_multi_drive_set(port, pin, *gpio_hal_drive[port][pin], 0);
                gpio_hal_drive_prev[port][pin] = *gpio_hal_drive[port][pin];
            }

            if ( !(gpio_out_mask[port] & pin_msk[pin]) ) continue;

            if ( *gpio_hal_0[port][pin] != gpio_hal_0_prev[port][pin] )
            {
                if ( *gpio_hal_0[port][pin] )
                {
                    *gpio_hal_1[port][pin] = 0;
                    mask_1 |= pin_msk[pin];
                }
                else
                {
                    *gpio_hal_1[port][pin] = 1;
                    mask_0 |= pin_msk[pin];
                }
                gpio_hal_0_prev[port][pin] = *gpio_hal_0[port][pin];
                gpio_hal_1_prev[port][pin] = *gpio_hal_1[port][pin];
            }

            if ( *gpio_hal_1[port][pin] != gpio_hal_1_prev[port][pin] )
            {
                if ( *gpio_hal_1[port][pin] )
                {
                    *gpio_hal_0[port][pin] = 0;
                    mask_0 |= pin_msk[pin];
                }
                else
                {
                    *gpio_hal_0[port][pin] = 1;
                    mask_1 |= pin_msk[pin];
                }
                gpio_hal_1_prev[port][pin] = *gpio_hal_1[port][pin];
                gpio_hal_0_prev[port][pin] = *gpio_hal_0[port][pin];
            }
        }

        if ( mask_0 ) gpio_port_clr(port, mask_0, 0);
        if ( mask_1 ) gpio_port_set(port, mask_1, 0);
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
    shmem_deinit();
    hal_exit(comp_id);
}
