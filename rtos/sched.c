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

volatile struct sched_ctx * xdata ctx;

void xTaskManager(void)
{
	struct sched_task * xdata self = ctx->head;

	IE_EA = false;
	printf("Hello from %s!\n\r", self->args);
	IE_EA = true;

	while (true) {
		P2 ^= LED2G; // Oszi: Gr?n
	}
}

void xTaskInit(void)
{
	printf("xTaskInit\r\n");

	/* for c51 we need to initial a mempool once at the beginning! */
	init_mempool(&malloc_mempool, sizeof(malloc_mempool));

	ctx = (struct sched_ctx *) malloc(sizeof(struct sched_ctx));

	if (NULL != ctx) {
		ctx->root = NULL;
		ctx->head = NULL;
	} else {
		printf("xTaskInit: error malloc\r\n");
		while (true)
			;
	}

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

int xTaskCreate(TaskFunction_t pxTaskCode, char * pcName, uint8_t priority, unsigned int delay)
{
	struct sched_task * xdata task;
	static unsigned int xdata id = 0;

	printf("xTaskCreate: Hello %s!\r\n", pcName);

	task = malloc(sizeof(struct sched_task));
	if (!task) {
		printf("xTaskCreate: error malloc struct sched_task\r\n");
		while (true)
			;
	}

	task->ctx = NULL;
	task->next = NULL;
	task->stack = calloc(STACK_SIZE, sizeof(uint8_t));
	if (!task->stack) {
		printf("xTaskCreate: error calloc stack\r\n");
		while (true)
			;
	}
	memset(task->stack, '\0', STACK_SIZE);
	task->sp = SP;
	task->stack[SP] = (unsigned int) (pxTaskCode) >> 8;
	task->stack[SP - 1] = (unsigned int) (pxTaskCode) & 0xff;
	/*
	 * C51 adds push and pop instructions at the beginning and end of the interrupt routines
	 * compare the assembler code to vertify the 13
	 */
	task->sp += 13;
	task->id = id++;
	task->priority = priority;
	task->delay = delay;
	task->func = pxTaskCode;
	task->args = pcName;

	link_task(ctx, task);

	return 0;
}

void xTaskDebug(void)
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
		printf("sp 0x%01x\r\n", task->sp);
		printf("stack %p\r\n", task->stack+8);
		printf("id %d\r\n", task->id);
		printf("priority %d\r\n", (unsigned int)task->priority);
		printf("delay %d\r\n", task->delay);
		printf("func %s %p\r\n", task->args, task->func);
		printf("args %p\r\n", task->args);
		printf("\r\n");
	}

	printf(" ---------- \r\n");
}

void vTaskStartScheduler(void)
{
	struct sched_task * xdata task = ctx->head;
	uint8_t idata * xdata s = 0;

	/*
	 * Have at least one task with the lowest priority.
	 * This is the idle task to execute if there is nothing else to do
	 */
	xTaskCreate(xTaskManager, "TaskManager", 1, 0);

    xTaskDebug();

	printf("vTaskStartScheduler\r\n");

	/* manipulate the stackpointer so on return we will jump to the first function */
	s[SP] = (unsigned int) (task->func) >> 8;
	s[SP - 1] = (unsigned int) (task->func) & 0xff;

	TMR2CN0_TR2 = true;
//	WDTCN = 0xA5;

	return;
}

SI_INTERRUPT (UART0_ISR, UART0_IRQn) {
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

void vTasksTick(void)
{
	struct sched_task * xdata task;

	for (task = ctx->root; task != NULL; task = task->next)
		if (task->delay)
			task->delay--;

	return;
}

//struct sched_task * xSwitchContext(struct sched_task * task)
//{
//	/* Simple round robin scheduler */
//	return (ctx->head = task->next == NULL ? ctx->root : task->next);
//}

struct sched_task * xSwitchContext(void)
{
	struct sched_task * xdata t, * xdata ret;
	uint8_t priority = 0;

	/* Simple round robin scheduler */
//	return (ctx->head = task->next == NULL ? ctx->root : task->next);

	/* schedule the highest priority task that is not on delay */
	for (t = ctx->root; t != NULL; t = t->next) {
		if (t->delay == 0 && t->priority > priority) {
			priority = t->priority;
			ret = ctx->head = t;
		}
	}

	return ret;
}

/*
 * SI_INTERRUPT (TIMER2_ISR, TIMER2_IRQn)
 * void TIMER2_ISR (void) interrupt TIMER2_IRQn
 * https://developer.arm.com/documentation/ka002595/latest
 * using this C51 hopefully saves the registers onto the stack by itsself before and after the ISR
 * The ISR takes about 375us @ 24,5MHz to copy the 256 byte stack back and forth
 */
/*
SI_INTERRUPT (TIMER2_ISR, TIMER2_IRQn) {
	struct sched_task * xdata task = ctx->head;

	uint8_t idata * dptr;
	uint8_t xdata * xptr;
	uint8_t j;

	TMR2CN0_TF2H = 0;                            // Timer Interrupt quittieren

//	P2 ^= LED2R;  // Oszi: Gelb

//	WDTCN = 0xA5;

	TMR2CN0_TR2 = false;						 // stop the Timer

//	printf("vTaskSchedule\r\n");

	task->sp = SP;

	P2 ^= LED2R;  // Oszi: Gelb
	memcpy(task->stack + REGS_SIZE, (uint8_t idata *) REGS_SIZE,
			STACK_SIZE - REGS_SIZE);
	P2 ^= LED2R;  // Oszi: Gelb

	task = (ctx->head = task->next == NULL ? ctx->root : task->next);

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

	TMR2CN0_TR2 = true;						 // start the Timer again

//	P2 ^= LED2R;  // Oszi: Gelb
}
*/
