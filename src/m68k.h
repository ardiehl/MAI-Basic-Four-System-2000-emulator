/***************************************************************************
 *  m68k.h
 *
 *  Created: Dec, 10 2011
 *
 *  Armin Diehl <ad@ardiehl.de>
 *
 * Compatibility routines to use musashi 4.x (from mame) with 3.3 like
 * interfaces
 ****************************************************************************/

#ifndef M68K_H
#define M68K_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "Musashi/m68k.h"
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wunused-variable\"")

#include "Musashi/m68kcpu.h"
_Pragma("GCC diagnostic pop")

extern m68ki_cpu_core m68ki_cpu;

/* mapping to the routine names in sim.c */
#define cpu_read_byte m68k_read_memory_8
#define cpu_read_word m68k_read_memory_16
#define cpu_read_long m68k_read_memory_32
#define cpu_write_byte m68k_write_memory_8
#define cpu_write_word m68k_write_memory_16
#define cpu_write_long m68k_write_memory_32


/* I/O line states, came from mame */
enum line_state
{
        CLEAR_LINE = 0,                         /* clear (a fired or held) line */
        ASSERT_LINE,                            /* assert an interrupt immediately */
        HOLD_LINE,                              /* hold interrupt line until acknowledged */
        PULSE_LINE                              /* pulse interrupt line instantaneously (only for NMI, RESET) */
};


// these are from the mame version of musashi
#define UINT32 uint32_t
#define UINT16 uint16_t
#define UINT8 uint8_t
#define INT32 int32_t
#define INT16 int16_t
#define INT8 int8_t

int  m68k_is_stopped (void);
//void m68k_set_int_line (int level, int state);

#define m68k_set_int_line(level,state) m68k_set_virq(level,state)

//void m68k_pulse_interrupt (int level);

#define m68k_pulse_interrupt(level) m68ki_exception_interrupt(level)


#define DASMFLAG_LENGTHMASK             0x0000ffff      /* the low 16-bits contain the actual length */

int cpu_save_state(FILE * f);
int cpu_load_state(FILE * f);

#endif /* M68K__HEADER */
