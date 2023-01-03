#include <SI_EFM8BB3_Register_Enums.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <intrins.h>
#include "sched.h"

#define LED2G	0x04
#define LED2R	0x08

/*
 * https://github.com/bradschl/embedded-c-scheduler
 * https://github.com/KirpichenkovPavel/os8051
 */

volatile uint8_t xdata flags = 0x00;
volatile char xdata rx_buffer;

unsigned char xdata malloc_mempool[0x700];
volatile uint8_t xdata ra[2];
volatile struct sched_ctx * xdata ctx;

void xTaskManager(void)
{
	struct sched_task * xdata self = ctx->head;

    IE_EA = false;
	printf("Hello from %s!\n\r", self->args);
    IE_EA = true;

    while (true) {
    	P2 ^= LED2G; // Oszi: Grün
    }
}

void xTaskInit(void)
{
	printf("xTaskInit\r\n");

	/* for c51 we need to initial a mempool once at the beginning! */
	init_mempool(&malloc_mempool, sizeof(malloc_mempool));

	ctx = (struct sched_ctx *)malloc(sizeof(struct sched_ctx));

	if (NULL != ctx) {
		ctx->root = NULL;
		ctx->head = NULL;
	} else {
		printf("xTaskInit: error malloc\r\n");
		while (true);
	}

	/* have at least one task */
	xTaskCreate(xTaskManager, "TaskManager");

	return;
}

static struct sched_task * get_last_linked_task(struct sched_ctx * ctx)
{
	struct sched_task * xdata last;
	for (last = ctx->root; NULL != last; last = last->next) {
		if (NULL == last->next)
			break;
	}

	return last;
}


void link_task(struct sched_ctx *ctx, struct sched_task * task)
{
	task->ctx = ctx;
	task->next = NULL;

	if (NULL == ctx->root) {
		ctx->root = task;
		ctx->head = task;
	} else {
		struct sched_task * xdata last = get_last_linked_task(ctx);
		last->next = task;
	}

	return;
}

int xTaskCreate(TaskFunction_t pxTaskCode, char * pcName)
{
	struct sched_task * xdata task;
	static unsigned int xdata id = 0;

	printf("xTaskCreate: Hello %s!\r\n", pcName);

	task = malloc(sizeof(struct sched_task));
	if (!task) {
		printf("xTaskCreate: error malloc struct sched_task\r\n");
		while (true);
	}

	task->ctx   = NULL;
	task->next  = NULL;
	task->stack = calloc(STACK_SIZE, sizeof(uint8_t));
	if (!task->stack) {
		printf("xTaskCreate: error calloc stack\r\n");
		while (true);
	}
	memset(task->stack, '\0', STACK_SIZE);
	task->sp    = SP;
	task->stack[SP] = (unsigned int)(pxTaskCode) >> 8;
	task->stack[SP-1] = (unsigned int)(pxTaskCode) & 0xff;
	/*
	 * C51 adds push and pop instructions at the beginning and end of the interrupt routines
	 * compare the assembler code to vertify the 12
	 */
	task->sp   += 12;
	task->id    = id++;
	task->func  = pxTaskCode;
	task->args  = pcName;

	link_task(ctx, task);

	return 0;
}

void xTaskDebug()
{
	struct sched_task * xdata task;

	printf("\r\n -- Debug ctx -- \r\n");

	printf("ctx %p\r\n", ctx);
	printf("ctx->root %p\r\n", ctx->root);
	printf("ctx->head %p\r\n", ctx->head);

	printf("\r\n");

	for (task = ctx->root; task != NULL; task = task->next) {
		printf("ctx  %p\r\n", task->ctx);
		printf("next %p\r\n", task->next);
		printf("sp %x\r\n", task->sp);
		printf("stack %p\r\n", task->stack);
		printf("id %d\r\n", task->id);
		printf("func %p\r\n", task->func);
		printf("args %p\r\n", task->args);
		printf("\r\n");
	}

	printf(" ---------- \r\n");
}

void vTaskStartScheduler(void)
{
	struct sched_task * xdata task = ctx->head;
	uint8_t idata * xdata s = 0;

	printf("vTaskStartScheduler\r\n");

	task->sp = SP;
	s[SP] = (unsigned int)(task->func) >> 8;
	s[SP-1] = (unsigned int)(task->func) & 0xff;

	TMR2CN0_TR2 = true;
//	WDTCN = 0xA5;

	return;
}

SI_INTERRUPT (UART0_ISR, UART0_IRQn)
{
	uint8_t platch = SFRPAGE;
	uint8_t ch;

	SFRPAGE = 0x00;

	if (SCON0_RI == 1) {
		SCON0_RI = 0;

		ch = SBUF0;                    // empf. Zeichen uebernehmen
		rx_buffer = ch;
		flags |= (1 << RXBIT);
	}

	SFRPAGE = platch;
}

/*
 * SI_INTERRUPT (TIMER2_ISR, TIMER2_IRQn)
 * void TIMER2_ISR (void) interrupt TIMER2_IRQn
 * https://developer.arm.com/documentation/ka002595/latest
 * using this C51 hopefully saves the registers onto the stack by itsself before and after the ISR
 * The ISR takes about 2,75ms @ 24,5MHz to copy the 256 byte stack back and forth
 */
SI_INTERRUPT (TIMER2_ISR, TIMER2_IRQn)
{
	struct sched_task * xdata task = ctx->head;
	uint8_t idata * xdata s = 0;
	unsigned int xdata i;

	P2 ^= LED2R;  // Oszi: Gelb

//	WDTCN = 0xA5;

	TMR2CN0_TF2H = 0;                            // Timer Interrupt quittieren
	TMR2CN0_TR2 = false;						 // stop the Timer

//	printf("vTaskSchedule\r\n");

	task->sp = SP;

	for (i=8; i<STACK_SIZE; i++)
		task->stack[i] = s[i];

	task = ( ctx->head = task->next == NULL ? ctx->root : task->next );

	ra[0] = *(uint8_t *)SP;
	ra[1] = *(uint8_t *)(SP-1);

	for (i=8; i<STACK_SIZE; i++)
		s[i] = task->stack[i];

	SP += 2;

	*(uint8_t idata *)SP = ra[0];
	*(uint8_t idata *)(SP-1) = ra[1];

	SP = task->sp;

	P2 ^= LED2R;  // Oszi: Gelb

	TMR2CN0_TR2 = true;						 // start the Timer again
}
