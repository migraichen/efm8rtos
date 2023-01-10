#pragma SRC
#include <SI_EFM8BB3_Register_Enums.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <intrins.h>
#include "sched.h"

#define LED2R	0x08

extern volatile struct sched_ctx * xdata ctx;

//extern struct sched_task * xSwitchContext(struct sched_task * task) reentrant;
extern struct sched_task * xSwitchContext(void);
extern void vTasksTick(void);

void vTaskSchedule(unsigned int ticks)
{
	struct sched_task * xdata task = ctx->head;

	uint8_t idata * dptr;
	uint8_t xdata * xptr;
	uint8_t j;

	IE_EA = false;

	task->sp = SP;

	P2 ^= LED2R;  // Oszi: Gelb
	memcpy(task->stack + REGS_SIZE, (uint8_t idata *) REGS_SIZE, STACK_SIZE - REGS_SIZE);
	P2 ^= LED2R;  // Oszi: Gelb

	task->delay = ticks;

	task = xSwitchContext();

	//
	// memcpy takes about 100us and this loop about 250us, but we cannot use memcpy here
	// because it invokes a LCALL before we manipulate the stack, so on return we end up in nirvana
	//

	P2 ^= LED2R;  // Oszi: Gelb
	xptr = task->stack + REGS_SIZE;
	dptr = REGS_SIZE;
	for ((uint8_t) j = STACK_SIZE - REGS_SIZE; j; j--)
		*(dptr++) = *(xptr++);
	P2 ^= LED2R;  // Oszi: Gelb

	SP = task->sp;

	IE_EA = true;

	return;
}

/*
 * SI_INTERRUPT (TIMER2_ISR, TIMER2_IRQn)
 * void TIMER2_ISR (void) interrupt TIMER2_IRQn
 * https://developer.arm.com/documentation/ka002595/latest
 * using this C51 hopefully saves the registers onto the stack by itsself before and after the ISR
 * The ISR takes about 375us @ 24,5MHz to copy the 256 byte stack back and forth
 */

SI_INTERRUPT (TIMER2_ISR, TIMER2_IRQn)
{
	struct sched_task * xdata task = ctx->head;

	uint8_t idata * dptr;
	uint8_t xdata * xptr;
	uint8_t j;

	TMR2CN0_TF2H = 0;                            // Timer Interrupt quittieren

//	P2 ^= LED2R;  // Oszi: Gelb

//	WDTCN = 0xA5;

//	TMR2CN0_TR2 = false;						 // stop the Timer

//	printf("vTaskSchedule\r\n");

	task->sp = SP;

	P2 ^= LED2R;  // Oszi: Gelb
	memcpy(task->stack + REGS_SIZE, (uint8_t idata *) REGS_SIZE, STACK_SIZE - REGS_SIZE);
	P2 ^= LED2R;  // Oszi: Gelb

	vTasksTick();

//	task = (ctx->head = task->next == NULL ? ctx->root : task->next);

	task = xSwitchContext();

	//
	// memcpy takes about 100us and this loop about 250us, but we cannot use memcpy here
	// because it invokes a LCALL before we manipulate the stack, so on return we end up in nirvana
	//

	P2 ^= LED2R;  // Oszi: Gelb
	xptr = task->stack + REGS_SIZE;
	dptr = REGS_SIZE;
	for ((uint8_t) j = STACK_SIZE - REGS_SIZE; j; j--)
		*(dptr++) = *(xptr++);
	P2 ^= LED2R;  // Oszi: Gelb

	SP = task->sp;

//	TMR2CN0_TR2 = true;						 // start the Timer again

//	P2 ^= LED2R;  // Oszi: Gelb
}

