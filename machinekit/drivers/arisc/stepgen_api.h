#ifndef _STEPGEN_API_H
#define _STEPGEN_API_H

#include <stdlib.h>
#include "msg_api.h"




enum
{
    STEPGEN_MSG_PIN_SETUP = 0x20,
    STEPGEN_MSG_TIME_SETUP,
    STEPGEN_MSG_TASK_ADD,
    STEPGEN_MSG_POS_GET,
    STEPGEN_MSG_POS_SET,
    STEPGEN_MSG_CNT
};




/**
 * @brief   setup GPIO pin for the selected channel
 *
 * @param   c               channel id
 * @param   type            0:step, 1:dir
 * @param   port            GPIO port number
 * @param   pin             GPIO pin number
 * @param   invert          invert pin state?
 *
 * @retval  none
 */
void stepgen_pin_setup(uint8_t c, uint8_t type, uint8_t port, uint8_t pin, uint8_t invert)
{
    u32_10_t *tx = (u32_10_t*) msg_buf;

    tx->v[0] = c;
    tx->v[1] = type;
    tx->v[2] = port;
    tx->v[3] = pin;
    tx->v[4] = invert;

    msg_send(STEPGEN_MSG_PIN_SETUP, msg_buf, 5*4, 0);
}

/**
 * @brief   add a new task for the selected channel
 *
 * @param   c           channel id
 * @param   pulses      number of pulses
 *
 * @retval  none
 */
void stepgen_task_add(uint8_t c, int32_t pulses)
{
    u32_10_t *tx = (u32_10_t*) msg_buf;

    tx->v[0] = c;
    tx->v[1] = pulses;

    msg_send(STEPGEN_MSG_TASK_ADD, msg_buf, 2*4, 0);
}

/**
 * @brief   update time values for the current task
 *
 * @param   c       channel id
 * @param   type    0:step, 1:dir
 * @param   t0      pin LOW state duration (in nanoseconds)
 * @param   t1      pin HIGH state duration (in nanoseconds)
 *
 * @retval  none
 */
void stepgen_time_setup(uint8_t c, uint8_t type, uint32_t t0, uint32_t t1)
{
    u32_10_t *tx = (u32_10_t*) msg_buf;

    tx->v[0] = c;
    tx->v[1] = type;
    tx->v[2] = t0;
    tx->v[3] = t1;

    msg_send(STEPGEN_MSG_TIME_SETUP, msg_buf, 4*4, 0);
}

/**
 * @brief   set channel steps position
 * @param   c   channel id
 * @retval  integer 4-bytes
 */
int32_t stepgen_pos_get(uint8_t c)
{
    u32_10_t *tx = (u32_10_t*) msg_buf;

    tx->v[0] = c;

    msg_send(STEPGEN_MSG_POS_GET, msg_buf, 1*4, 0);

    // finite loop, only 999999 tries to read an answer
    uint32_t n = 0;
    for ( n = 999999; n--; )
    {
        if ( msg_read(STEPGEN_MSG_POS_GET, msg_buf, 0) < 0 ) continue;
        else return (int32_t)tx->v[0];
    }

    return 0;
}

/**
 * @brief   set channel steps position
 * @param   c       channel id
 * @param   pos     integer 4-bytes
 * @retval  none
 */
void stepgen_pos_set(uint8_t c, int32_t pos)
{
    u32_10_t *tx = (u32_10_t*) msg_buf;

    tx->v[0] = c;
    tx->v[1] = (uint32_t)pos;

    msg_send(STEPGEN_MSG_POS_SET, msg_buf, 2*4, 0);
}




#endif
