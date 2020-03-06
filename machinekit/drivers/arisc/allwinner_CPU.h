#ifndef _CPU_H
#define _CPU_H

#include <stdint.h>
#include <string.h>




enum
{
    ALLWINNER_H2,
    ALLWINNER_H3,
    ALLWINNER_H5,
    ALLWINNER_CPU_CNT
};

typedef struct
{
    const char *name;
    uint32_t ARISC_addr;
    uint32_t ARISC_size;

} allwinner_cpu_data_t;




static const allwinner_cpu_data_t cpu_data[ALLWINNER_CPU_CNT] =
{
    {"H2",  0x00040000, (8+8+32)*1024},
    {"H3",  0x00040000, (8+8+32)*1024},
    {"H5",  0x00040000, (8+8+64)*1024}
};




static uint8_t allwinner_cpu_id_get(const char *cpu)
{
    uint8_t c;
    for ( c = ALLWINNER_CPU_CNT; c--; )
    {
        if ( !strcmp(cpu, cpu_data[c].name) ) return c;
    }

    return ALLWINNER_H3;
}




#endif
