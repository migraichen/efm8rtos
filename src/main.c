//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <SI_EFM8BB3_Register_Enums.h>                // SFR declarations
#include <stdio.h>
#include "InitDevice.h"
#include "sched.h"

#define LED1G	0x01
#define LED1R	0x02

static void prvSetupHardware(void);

void ATaskFunction1(void);
void ATaskFunction2(void);

extern volatile struct sched_ctx * xdata ctx;

extern volatile uint8_t xdata flags;
extern volatile char xdata rx_buffer;

//-----------------------------------------------------------------------------
// SiLabs_Startup() Routine
// ----------------------------------------------------------------------------
// This function is called immediately after reset, before the initialization
// code is run in SILABS_STARTUP.A51 (which runs before main() ). This is a
// useful place to disable the watchdog timer, which is enable by default
// and may trigger before main() in some instances.
//-----------------------------------------------------------------------------
void SiLabs_Startup(void)
{
  // Disable the watchdog here
}

//-----------------------------------------------------------------------------
// main() Routine
// ----------------------------------------------------------------------------
// Note: the software watchdog timer is not disabled by default in this
// example, so a long-running program will reset periodically unless
// the timer is disabled or your program periodically writes to it.
//
// Review the "Watchdog Timer" section under the part family's datasheet
// for details. To find the datasheet, select your part in the
// Simplicity Launcher and click on "Data Sheet".
//-----------------------------------------------------------------------------
int main(void)
{
    /* Performasdas any hardware setup necessary. */
    prvSetupHardware();

    /* Send a banner to the serial port, in case it is connected. */
    printf("\n\r");
    printf("EFM-RTOS project -- " __DATE__ " " __TIME__ "\n\r");

    xTaskInit();

    xTaskCreate(ATaskFunction1, "BlinkTask");
    xTaskCreate(ATaskFunction2, "RXTask");

//    xTaskDebug();

    /* Start the created tasks and the watchdog */
    vTaskStartScheduler();

    /* run into this loop until the first timer interrupt is fired */
    while (true);
}

static void prvSetupHardware(void)
{
	SFRPAGE = 0x00;
	enter_DefaultMode_from_RESET();

	SCON0_TI = true;
}

void ATaskFunction1(void)
{
	struct sched_task * xdata self = ctx->head;

	TMR2CN0_TR2 = false;
	printf("Hello from %s!\n\r", self->args);
	TMR2CN0_TR2 = true;

    while (true) {
    	P2 ^= LED1G; // Oszi: Blau
    }
}

void ATaskFunction2(void)
{
	struct sched_task * xdata self = ctx->head;

	TMR2CN0_TR2 = false;
	printf("Hello from %s!\n\r", self->args);
	TMR2CN0_TR2 = true;

    while (true) {
    	P2 ^= LED1R; // Oszi: Rot
    	if (flags & (1 << RXBIT)) {
    	    TMR2CN0_TR2 = false;
    	    putchar(rx_buffer);
    	    TMR2CN0_TR2 = true;
    	    flags &= ~(1 << RXBIT);
    	}
    }
}
