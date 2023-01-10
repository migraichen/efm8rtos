#ifndef __SCHED_H_
#define __SCHED_H_

#define STACK_SIZE	256
#define REGS_SIZE	8

#define RXBIT 0

typedef uint8_t CoreReg_t;
typedef void (*TaskFunction_t)(void *);

struct sched_task {
	/* Linked list storage */
	struct sched_ctx * ctx;
	struct sched_task * next;
	CoreReg_t sp;
	uint8_t * stack;
	unsigned int id;
	uint8_t priority;
	unsigned int delay;
	TaskFunction_t func;
	char * args;
};

struct sched_ctx {
	struct sched_task * root;
	struct sched_task * head;
};

void xTaskInit(void);
int xTaskCreate(TaskFunction_t pxTaskCode, char * pcName, uint8_t priority, unsigned int delay);
void xTaskDebug(void);
void vTaskStartScheduler(void);
//struct sched_task * xSwitchContext(struct sched_task * task) reentrant;
struct sched_task * xSwitchContext(void);

#endif
