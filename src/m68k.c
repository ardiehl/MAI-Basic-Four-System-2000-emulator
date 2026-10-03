/***************************************************************************
 *  m68k.c
 *
 *  Created: Dec, 10 2011
 *
 *  Armin Diehl <ad@ardiehl.de>
 *
 * Compatibility routines to use musashi 4.x (from mame) with 3.3 like
 * interfaces
 ****************************************************************************/

#include <stdio.h>
#include "m68k.h"
#include "sim.h"

/*
void m68k_pulse_interrupt (int level) {
	m68ki_exception_interrupt(level);
}
*/

/* True while the CPU sits in the state a STOP instruction put it in. It will
   not execute anything until an interrupt above the mask arrives, so a single
   step has, correctly, nothing to do. */
int m68k_is_stopped (void) {
	return (m68ki_cpu.stopped != 0);
}

/*
void m68k_set_int_line (int level, int state) {
	m68k_set_virq(level,state);
}
*/

// TODO: adapt to current version of Musashi


int cpu_save_state(FILE * f)  {
    STATEWRITEVARS("cpu");

    STATEWRITE(id,f);

    STATEWRITELEN(m68ki_cpu.cpu_type,f);     /* CPU Type: 68000, 68008, 68010, 68EC020, 68020, 68EC030, 68030, 68EC040, or 68040 */
//	STATEWRITELEN(m68ki_cpu.dasm_type,f);	 /* disassembly type */
	STATEWRITELEN(m68ki_cpu.dar,f);      /* Data and Address Registers */
	STATEWRITELEN(m68ki_cpu.ppc,f);		   /* Previous program counter */

	STATEWRITELEN(m68ki_cpu.pc,f);           /* Program Counter */
	STATEWRITELEN(m68ki_cpu.sp,f);        /* User, Interrupt, and Master Stack Pointers */
	STATEWRITELEN(m68ki_cpu.vbr,f);          /* Vector Base Register (m68010+) */
	STATEWRITELEN(m68ki_cpu.sfc,f);          /* Source Function Code Register (m68010+) */
	STATEWRITELEN(m68ki_cpu.dfc,f);          /* Destination Function Code Register (m68010+) */
	STATEWRITELEN(m68ki_cpu.cacr,f);         /* Cache Control Register (m68020, unemulated) */
	STATEWRITELEN(m68ki_cpu.caar,f);         /* Cache Address Register (m68020, unemulated) */
	STATEWRITELEN(m68ki_cpu.ir,f);           /* Instruction Register */
	//floatx80 fpr[8];     /* FPU Data Register (m68030/040) */
	//UINT32 fpiar;        /* FPU Instruction Address Register (m68040) */
	//UINT32 fpsr;         /* FPU Status Register (m68040) */
	//UINT32 fpcr;         /* FPU Control Register (m68040) */
	STATEWRITELEN(m68ki_cpu.t1_flag,f);      /* Trace 1 */
	STATEWRITELEN(m68ki_cpu.t0_flag,f);      /* Trace 0 */
	STATEWRITELEN(m68ki_cpu.s_flag,f);       /* Supervisor */
	STATEWRITELEN(m68ki_cpu.m_flag,f);       /* Master/Interrupt state */
	STATEWRITELEN(m68ki_cpu.x_flag,f);       /* Extend */
	STATEWRITELEN(m68ki_cpu.n_flag,f);       /* Negative */
	STATEWRITELEN(m68ki_cpu.not_z_flag,f);   /* Zero, inverted for speedups */
	STATEWRITELEN(m68ki_cpu.v_flag,f);       /* Overflow */
	STATEWRITELEN(m68ki_cpu.c_flag,f);       /* Carry */
	STATEWRITELEN(m68ki_cpu.int_mask,f);     /* I0-I2 */
	STATEWRITELEN(m68ki_cpu.int_level,f);    /* State of interrupt pins IPL0-IPL2 -- ASG: changed from ints_pending */
	STATEWRITELEN(m68ki_cpu.stopped,f);      /* Stopped state */
	STATEWRITELEN(m68ki_cpu.pref_addr,f);    /* Last prefetch address */
	STATEWRITELEN(m68ki_cpu.pref_data,f);    /* Data in the prefetch queue */
	STATEWRITELEN(m68ki_cpu.sr_mask,f);      /* Implemented status register bits */
	STATEWRITELEN(m68ki_cpu.instr_mode,f);   /* Stores whether we are in instruction mode or group 0/1 exception mode */
	STATEWRITELEN(m68ki_cpu.run_mode,f);     /* Stores whether we are processing a reset, bus error, address error, or something else */
	//int    has_pmmu;     /* Indicates if a PMMU available (yes on 030, 040, no on EC030) */
	//int    has_hmmu;     /* Indicates if an Apple HMMU is available in place of the 68851 (020 only) */
	//int    pmmu_enabled; /* Indicates if the PMMU is enabled */
	//int    hmmu_enabled; /* Indicates if the HMMU is enabled */
	//int    has_fpu;      /* Indicates if a FPU is available (yes on 030, 040, may be on 020) */
	//int    fpu_just_reset; /* Indicates the FPU was just reset */
#if 0
	/* Clocks required for instructions / exceptions */
	UINT32 cyc_bcc_notake_b;
	UINT32 cyc_bcc_notake_w;
	UINT32 cyc_dbcc_f_noexp;
	UINT32 cyc_dbcc_f_exp;
	UINT32 cyc_scc_r_true;
	UINT32 cyc_movem_w;
	UINT32 cyc_movem_l;
	UINT32 cyc_shift;
	UINT32 cyc_reset;

	int  initial_cycles;
	int  remaining_cycles;                     /* Number of clocks remaining */
	int  reset_cycles;
#endif
//	STATEWRITELEN(m68ki_cpu.tracing,f);

//	STATEWRITELEN(m68ki_cpu.aerr_address,f);
//	STATEWRITELEN(m68ki_cpu.aerr_write_mode,f);
//	STATEWRITELEN(m68ki_cpu.aerr_fc,f);

	/* Virtual IRQ lines state */
	STATEWRITELEN(m68ki_cpu.virq_state,f);
	STATEWRITELEN(m68ki_cpu.nmi_pending,f);
#if 0
	void (**jump_table)(m68ki_cpu_core *m68k);
	const UINT8* cyc_instruction;
	const UINT8* cyc_exception;

	/* Callbacks to host */
	device_irq_callback int_ack_callback;			  /* Interrupt Acknowledge */
	m68k_bkpt_ack_func bkpt_ack_callback;         /* Breakpoint Acknowledge */
	m68k_reset_func reset_instr_callback;         /* Called when a RESET instruction is encountered */
	m68k_cmpild_func cmpild_instr_callback;       /* Called when a CMPI.L #v, Dn instruction is encountered */
	m68k_rte_func rte_instr_callback;             /* Called when a RTE instruction is encountered */
	m68k_tas_func tas_instr_callback;             /* Called when a TAS instruction is encountered, allows / disallows writeback */

	legacy_cpu_device *device;
	address_space *program;
	m68k_memory_interface memory;
	offs_t encrypted_start;
	offs_t encrypted_end;
#endif
//	STATEWRITELEN(m68ki_cpu.iotemp,f);

	/* ad: bus error handling */
//	STATEWRITELEN(m68ki_cpu.cpu_buserror_address,f);   /* (first) bus error address */
//	STATEWRITELEN(m68ki_cpu.cpu_buserror_instraddr,f); /* address of instruction caused bus error */
//	STATEWRITELEN(m68ki_cpu.cpu_buserror_occurred,f);  /* flag that bus error has occurred from cpu */
//	STATEWRITELEN(m68ki_cpu.cpu_buserror_on_write,f);  /* write or read access */
//	STATEWRITELEN(m68ki_cpu.cpu_buserror_byte_transfer,f);  /* is byte transfer */
//	STATEWRITELEN(m68ki_cpu.cpu_buserror_writeval,f);  /* value to be written */
//	STATEWRITELEN(m68ki_cpu.cpu_buserror_instrfetch,f); /* 1 if error while fetching instruction */
//	STATEWRITELEN(m68ki_cpu.cpu_buserror_ir,f);
//	STATEWRITELEN(m68ki_cpu.cpu_buserror_s_flag,f);     /* Supervisor */

	/* save state data */
//	STATEWRITELEN(m68ki_cpu.save_sr,f);
//	STATEWRITELEN(m68ki_cpu.save_stopped,f);
//	STATEWRITELEN(m68ki_cpu.save_halted,f);
#if 0
	/* PMMU registers */
	UINT32 mmu_crp_aptr, mmu_crp_limit;
	UINT32 mmu_srp_aptr, mmu_srp_limit;
	UINT32 mmu_urp_aptr;	/* 040 only */
	UINT32 mmu_tc;
	UINT16 mmu_sr;
	UINT32 mmu_sr_040;
	UINT32 mmu_atc_tag[MMU_ATC_ENTRIES], mmu_atc_data[MMU_ATC_ENTRIES];
	UINT32 mmu_atc_rr;
	UINT32 mmu_tt0, mmu_tt1;
	UINT32 mmu_itt0, mmu_itt1, mmu_dtt0, mmu_dtt1;
	UINT32 mmu_acr0, mmu_acr1, mmu_acr2, mmu_acr3;

	UINT16 mmu_tmp_sr;      /* temporary hack: status code for ptest and to handle write protection */
	UINT16 mmu_tmp_fc;      /* temporary hack: function code for the mmu (moves) */
	UINT16 mmu_tmp_rw;      /* temporary hack: read/write (1/0) for the mmu */
	UINT32 mmu_tmp_buserror_address;   /* temporary hack: (first) bus error address */
	UINT16 mmu_tmp_buserror_occurred;  /* temporary hack: flag that bus error has occurred from mmu */

	UINT32 ic_address[M68K_IC_SIZE];   /* instruction cache address data */
	UINT16 ic_data[M68K_IC_SIZE];      /* instruction cache content data */

	/* external instruction hook (does not depend on debug mode) */

	instruction_hook_t instruction_hook;
#endif
    return 1;

}

int cpu_load_state(FILE * f) {
    STATEREADVARS("cpu");

    //if (fread(&idRead,sizeof(idRead),1,f) != 1) return 0;
    //if (memcmp(&id,&idRead,sizeof(id)) != 0) return 0;
    STATEREADID(f);

    STATEREADLEN(m68ki_cpu.cpu_type,f);     /* CPU Type: 68000, 68008, 68010, 68EC020, 68020, 68EC030, 68030, 68EC040, or 68040 */
//	STATEREADLEN(m68ki_cpu.dasm_type,f);	 /* disassembly type */
	STATEREADLEN(m68ki_cpu.dar,f);      /* Data and Address Registers */
	STATEREADLEN(m68ki_cpu.ppc,f);		   /* Previous program counter */

	STATEREADLEN(m68ki_cpu.pc,f);           /* Program Counter */
	STATEREADLEN(m68ki_cpu.sp,f);        /* User, Interrupt, and Master Stack Pointers */
	STATEREADLEN(m68ki_cpu.vbr,f);          /* Vector Base Register (m68010+) */
	STATEREADLEN(m68ki_cpu.sfc,f);          /* Source Function Code Register (m68010+) */
	STATEREADLEN(m68ki_cpu.dfc,f);          /* Destination Function Code Register (m68010+) */
	STATEREADLEN(m68ki_cpu.cacr,f);         /* Cache Control Register (m68020, unemulated) */
	STATEREADLEN(m68ki_cpu.caar,f);         /* Cache Address Register (m68020, unemulated) */
	STATEREADLEN(m68ki_cpu.ir,f);           /* Instruction Register */
	//floatx80 fpr[8];     /* FPU Data Register (m68030/040) */
	//UINT32 fpiar;        /* FPU Instruction Address Register (m68040) */
	//UINT32 fpsr;         /* FPU Status Register (m68040) */
	//UINT32 fpcr;         /* FPU Control Register (m68040) */
	STATEREADLEN(m68ki_cpu.t1_flag,f);      /* Trace 1 */
	STATEREADLEN(m68ki_cpu.t0_flag,f);      /* Trace 0 */
	STATEREADLEN(m68ki_cpu.s_flag,f);       /* Supervisor */
	STATEREADLEN(m68ki_cpu.m_flag,f);       /* Master/Interrupt state */
	STATEREADLEN(m68ki_cpu.x_flag,f);       /* Extend */
	STATEREADLEN(m68ki_cpu.n_flag,f);       /* Negative */
	STATEREADLEN(m68ki_cpu.not_z_flag,f);   /* Zero, inverted for speedups */
	STATEREADLEN(m68ki_cpu.v_flag,f);       /* Overflow */
	STATEREADLEN(m68ki_cpu.c_flag,f);       /* Carry */
	STATEREADLEN(m68ki_cpu.int_mask,f);     /* I0-I2 */
	STATEREADLEN(m68ki_cpu.int_level,f);    /* State of interrupt pins IPL0-IPL2 -- ASG: changed from ints_pending */
	STATEREADLEN(m68ki_cpu.stopped,f);      /* Stopped state */
	STATEREADLEN(m68ki_cpu.pref_addr,f);    /* Last prefetch address */
	STATEREADLEN(m68ki_cpu.pref_data,f);    /* Data in the prefetch queue */
	STATEREADLEN(m68ki_cpu.sr_mask,f);      /* Implemented status register bits */
	STATEREADLEN(m68ki_cpu.instr_mode,f);   /* Stores whether we are in instruction mode or group 0/1 exception mode */
	STATEREADLEN(m68ki_cpu.run_mode,f);     /* Stores whether we are processing a reset, bus error, address error, or something else */
	//int    has_pmmu;     /* Indicates if a PMMU available (yes on 030, 040, no on EC030) */
	//int    has_hmmu;     /* Indicates if an Apple HMMU is available in place of the 68851 (020 only) */
	//int    pmmu_enabled; /* Indicates if the PMMU is enabled */
	//int    hmmu_enabled; /* Indicates if the HMMU is enabled */
	//int    has_fpu;      /* Indicates if a FPU is available (yes on 030, 040, may be on 020) */
	//int    fpu_just_reset; /* Indicates the FPU was just reset */
#if 0
	/* Clocks required for instructions / exceptions */
	UINT32 cyc_bcc_notake_b;
	UINT32 cyc_bcc_notake_w;
	UINT32 cyc_dbcc_f_noexp;
	UINT32 cyc_dbcc_f_exp;
	UINT32 cyc_scc_r_true;
	UINT32 cyc_movem_w;
	UINT32 cyc_movem_l;
	UINT32 cyc_shift;
	UINT32 cyc_reset;

	int  initial_cycles;
	int  remaining_cycles;                     /* Number of clocks remaining */
	int  reset_cycles;
#endif
//	STATEREADLEN(m68ki_cpu.tracing,f);

//	STATEREADLEN(m68ki_cpu.aerr_address,f);
//	STATEREADLEN(m68ki_cpu.aerr_write_mode,f);
//	STATEREADLEN(m68ki_cpu.aerr_fc,f);

	/* Virtual IRQ lines state */
	STATEREADLEN(m68ki_cpu.virq_state,f);
	STATEREADLEN(m68ki_cpu.nmi_pending,f);
#if 0
	void (**jump_table)(m68ki_cpu_core *m68k);
	const UINT8* cyc_instruction;
	const UINT8* cyc_exception;

	/* Callbacks to host */
	device_irq_callback int_ack_callback;			  /* Interrupt Acknowledge */
	m68k_bkpt_ack_func bkpt_ack_callback;         /* Breakpoint Acknowledge */
	m68k_reset_func reset_instr_callback;         /* Called when a RESET instruction is encountered */
	m68k_cmpild_func cmpild_instr_callback;       /* Called when a CMPI.L #v, Dn instruction is encountered */
	m68k_rte_func rte_instr_callback;             /* Called when a RTE instruction is encountered */
	m68k_tas_func tas_instr_callback;             /* Called when a TAS instruction is encountered, allows / disallows writeback */

	legacy_cpu_device *device;
	address_space *program;
	m68k_memory_interface memory;
	offs_t encrypted_start;
	offs_t encrypted_end;
#endif
//	STATEREADLEN(m68ki_cpu.iotemp,f);

	/* ad: bus error handling */
//	STATEREADLEN(m68ki_cpu.cpu_buserror_address,f);   /* (first) bus error address */
//	STATEREADLEN(m68ki_cpu.cpu_buserror_instraddr,f); /* address of instruction caused bus error */
//	STATEREADLEN(m68ki_cpu.cpu_buserror_occurred,f);  /* flag that bus error has occurred from cpu */
//	STATEREADLEN(m68ki_cpu.cpu_buserror_on_write,f);  /* write or read access */
//	STATEREADLEN(m68ki_cpu.cpu_buserror_byte_transfer,f);  /* is byte transfer */
//	STATEREADLEN(m68ki_cpu.cpu_buserror_writeval,f);  /* value to be written */
//	STATEREADLEN(m68ki_cpu.cpu_buserror_instrfetch,f); /* 1 if error while fetching instruction */
//	STATEREADLEN(m68ki_cpu.cpu_buserror_ir,f);
//	STATEREADLEN(m68ki_cpu.cpu_buserror_s_flag,f);     /* Supervisor */

	/* save state data */
//	STATEREADLEN(m68ki_cpu.save_sr,f);
//	STATEREADLEN(m68ki_cpu.save_stopped,f);
//	STATEREADLEN(m68ki_cpu.save_halted,f);
#if 0
	/* PMMU registers */
	UINT32 mmu_crp_aptr, mmu_crp_limit;
	UINT32 mmu_srp_aptr, mmu_srp_limit;
	UINT32 mmu_urp_aptr;	/* 040 only */
	UINT32 mmu_tc;
	UINT16 mmu_sr;
	UINT32 mmu_sr_040;
	UINT32 mmu_atc_tag[MMU_ATC_ENTRIES], mmu_atc_data[MMU_ATC_ENTRIES];
	UINT32 mmu_atc_rr;
	UINT32 mmu_tt0, mmu_tt1;
	UINT32 mmu_itt0, mmu_itt1, mmu_dtt0, mmu_dtt1;
	UINT32 mmu_acr0, mmu_acr1, mmu_acr2, mmu_acr3;

	UINT16 mmu_tmp_sr;      /* temporary hack: status code for ptest and to handle write protection */
	UINT16 mmu_tmp_fc;      /* temporary hack: function code for the mmu (moves) */
	UINT16 mmu_tmp_rw;      /* temporary hack: read/write (1/0) for the mmu */
	UINT32 mmu_tmp_buserror_address;   /* temporary hack: (first) bus error address */
	UINT16 mmu_tmp_buserror_occurred;  /* temporary hack: flag that bus error has occurred from mmu */

	UINT32 ic_address[M68K_IC_SIZE];   /* instruction cache address data */
	UINT16 ic_data[M68K_IC_SIZE];      /* instruction cache content data */

	/* external instruction hook (does not depend on debug mode) */

	instruction_hook_t instruction_hook;
#endif
    return 1;
}

