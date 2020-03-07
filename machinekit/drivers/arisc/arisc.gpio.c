/********************************************************************
 * Description:  arisc.gpio.c
 *               GPIO driver for the Allwinner ARISC firmware
 *
 * Author: MX_Master (mikhail@vydrenko.ru)
 ********************************************************************/

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "rtapi.h"
#include "rtapi_app.h"
#include "hal.h"

#include "msg_api.h"
#include "gpio_api.h"
#include "arisc.gpio.h"




MODULE_AUTHOR("MX_Master");
MODULE_DESCRIPTION("GPIO driver for the Allwinner ARISC firmware");
MODULE_LICENSE("GPL");




static int32_t comp_id;
static const uint8_t * comp_name = "arisc.gpio";
static uint8_t cpu_id = ALLWINNER_H3;

static char *CPU = "H3";
RTAPI_MP_STRING(CPU, "Allwinner CPU name");

static int8_t *in = "";
RTAPI_MP_STRING(in, "input pins, comma separated");

static int8_t *out = "";
RTAPI_MP_STRING(out, "output pins, comma separated");

static const char *gpio_name[GPIO_PORTS_CNT] =
    {"PA","PB","PC","PD","PE","PF","PG","PL"};

static hal_u32_t **gpio_port_ID;

static hal_bit_t **gpio_hal_0[GPIO_PORTS_CNT];
static hal_bit_t **gpio_hal_1[GPIO_PORTS_CNT];

static hal_bit_t gpio_hal_0_prev[GPIO_PORTS_CNT][GPIO_PINS_CNT];
static hal_bit_t gpio_hal_1_prev[GPIO_PORTS_CNT][GPIO_PINS_CNT];

static uint32_t gpio_real[GPIO_PORTS_CNT] = {0};
static uint32_t gpio_real_prev[GPIO_PORTS_CNT] = {0};

static uint32_t gpio_out_mask[GPIO_PORTS_CNT] = {0};
static uint32_t gpio_in_mask[GPIO_PORTS_CNT] = {0};

static uint32_t gpio_in_cnt = 0;
static uint32_t gpio_out_cnt = 0;

static const uint32_t gpio_mask[GPIO_PINS_CNT] =
{
    1U<< 0, 1U<< 1, 1U<< 2, 1U<< 3, 1U<< 4, 1U<< 5, 1U<< 6, 1U<< 7,
    1U<< 8, 1U<< 9, 1U<<10, 1U<<11, 1U<<12, 1U<<13, 1U<<14, 1U<<15,
    1U<<16, 1U<<17, 1U<<18, 1U<<19, 1U<<20, 1U<<21, 1U<<22, 1U<<23,
    1U<<24, 1U<<25, 1U<<26, 1U<<27, 1U<<28, 1U<<29, 1U<<30, 1U<<31
};




// HAL functions

static void gpio_read(void *arg, long period)
{
    static uint32_t port, pin;
    static uint32_t* ports;

    if ( !gpio_in_cnt ) return;

    ports = gpio_all_get();

    for ( port = GPIO_PORTS_CNT; port--; )
    {
        if ( !gpio_in_mask[port] ) continue;

        gpio_real[port] = *(ports+port);

        if ( gpio_real_prev[port] == gpio_real[port] ) continue;

        for ( pin = GPIO_PINS_CNT; pin--; )
        {
            if ( !(gpio_in_mask[port] & gpio_mask[pin]) ) continue;

            if ( gpio_real[port] & gpio_mask[pin] )
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

        gpio_real_prev[port] = gpio_real[port];
    }
}

static void gpio_write(void *arg, long period)
{
    static uint32_t port, pin;
    static uint32_t mask_0_cnt, mask_1_cnt;
    static uint32_t mask_0[GPIO_PORTS_CNT], mask_1[GPIO_PORTS_CNT];

    if ( !gpio_out_cnt ) return;

    mask_1_cnt = 0;
    mask_0_cnt = 0;

    for ( port = GPIO_PORTS_CNT; port--; )
    {
        mask_1[port] = 0;
        mask_0[port] = 0;

        if ( !gpio_out_mask[port] ) continue;

        for ( pin = GPIO_PINS_CNT; pin--; )
        {
            if ( !(gpio_out_mask[port] & gpio_mask[pin]) ) continue;

            if ( *gpio_hal_0[port][pin] != gpio_hal_0_prev[port][pin] )
            {
                gpio_hal_0_prev[port][pin] = *gpio_hal_0[port][pin];

                if ( *gpio_hal_0[port][pin] ) mask_1[port] |= gpio_mask[pin];
                else                          mask_0[port] |= gpio_mask[pin];
            }

            if ( *gpio_hal_1[port][pin] != gpio_hal_1_prev[port][pin] )
            {
                gpio_hal_1_prev[port][pin] = *gpio_hal_1[port][pin];

                if ( *gpio_hal_1[port][pin] ) mask_0[port] |= gpio_mask[pin];
                else                          mask_1[port] |= gpio_mask[pin];
            }
        }

        if ( mask_1[port] ) ++mask_1_cnt;
        if ( mask_0[port] ) ++mask_0_cnt;
    }

    if ( mask_1_cnt ) gpio_all_set(&mask_1[0]);
    if ( mask_0_cnt ) gpio_all_clear(&mask_0[0]);
}




// INIT

static int32_t gpio_malloc_and_export(const char *comp_name, int32_t comp_id)
{
    int8_t* arg_str[2] = {in, out};
    int8_t n, r;
    uint8_t port;
    char name[HAL_NAME_LEN + 1];


    // shared memory allocation
    for ( port = GPIO_PORTS_CNT; port--; )
    {
        gpio_hal_0[port] = hal_malloc(GPIO_PINS_CNT * sizeof(hal_bit_t *));
        gpio_hal_1[port] = hal_malloc(GPIO_PINS_CNT * sizeof(hal_bit_t *));

        if ( !gpio_hal_0[port] || !gpio_hal_1[port] )
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
            for ( found = 0, port = GPIO_PORTS_CNT; port--; )
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

            if ( (pin == 0 && token[2] != '0') || pin >= GPIO_PINS_CNT ) continue;

            // export pin function
            retval = hal_pin_bit_newf( (n ? HAL_IN : HAL_OUT),
                &gpio_hal_0[port][pin], comp_id,
                "%s.%s-%s", comp_name, token, type_str);

            // export pin inverted function
            retval += hal_pin_bit_newf( (n ? HAL_IN : HAL_OUT),
                &gpio_hal_1[port][pin], comp_id,
                "%s.%s-%s-not", comp_name, token, type_str);

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
                gpio_out_mask[port] |= gpio_mask[pin];
                gpio_pin_setup_for_output(port, pin);
            }
            else
            {
                gpio_in_cnt++;
                gpio_in_mask[port] |= gpio_mask[pin];
                gpio_pin_setup_for_input(port, pin);
            }
        }

    }


    // export port ID pins
    gpio_port_ID = hal_malloc(GPIO_PORTS_CNT * sizeof(hal_u32_t));
    if ( !gpio_port_ID )
    {
        rtapi_print_msg(RTAPI_MSG_ERR, "%s: [GPIO] port ID pins malloc failed\n", comp_name);
        return -1;
    }

    for ( r = 0, port = GPIO_PORTS_CNT; port--; )
    {
        r += hal_pin_u32_newf(HAL_OUT, &gpio_port_ID[port], comp_id, "%s", gpio_name[port]);
        if (r) break;
        *gpio_port_ID[port] = port;
    }
    if ( r )
    {
        rtapi_print_msg(RTAPI_MSG_ERR, "%s: [GPIO] port ID pins export failed\n", comp_name);
        return -1;
    }


    // export HAL functions
    r = 0;
    rtapi_snprintf(name, sizeof(name), "%s.write", comp_name);
    r += hal_export_funct(name, gpio_write, 0, 0, 0, comp_id);
    rtapi_snprintf(name, sizeof(name), "%s.read", comp_name);
    r += hal_export_funct(name, gpio_write, 0, 0, 0, comp_id);
    if ( r )
    {
        rtapi_print_msg(RTAPI_MSG_ERR, "%s: [GPIO] functions export failed\n", comp_name);
        return -1;
    }


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
    if ( gpio_malloc_and_export(comp_name, comp_id) ) EXIT;

    // driver ready to work
    hal_ready(comp_id);

    return 0;
}

void rtapi_app_exit(void)
{
    msg_mem_deinit();
    hal_exit(comp_id);
}
