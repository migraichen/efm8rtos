/***********************************************************************/
/*  This file is part of the C51 Compiler package                      */
/*  Copyright KEIL ELEKTRONIK GmbH 1990 - 2002                         */
/***********************************************************************/
/*                                                                     */
/*  PUTCHAR.C:  This routine is the general character output of C51.   */
/*  You may add this file to a uVision2 project.                       */
/*                                                                     */
/*  To translate this file use C51 with the following invocation:      */
/*     C51 PUTCHAR.C <memory model>                                    */
/*                                                                     */
/*  To link the modified PUTCHAR.OBJ file to your application use the  */
/*  following Lx51 invocation:                                         */
/*     Lx51 <your object file list>, PUTCHAR.OBJ <controls>            */
/*                                                                     */
/***********************************************************************/

#include <SI_EFM8BB3_Register_Enums.h>                // SFR declarations
#include <stdint.h>

/*
 *  Wählt man "odd" als Parität, dann wird das Paritätsbit gesetzt, wenn die Anzahl der übertragenen Einsen in den Nutzdaten gerade ist.
 */

void odd_parity(char c) reentrant
{
	unsigned int i, p;

	p = 0x01;							// odd Parity, even => '0'

	for (i=0; i<sizeof(c)*8; i++) {		// zähle '1' bits
		p ^= (c&0x01);
		c  = (c>>1);
	}

	if (p)								// Anzahl der einsen gerade
		SCON0_TB8 = 1;					// SCON.TB8 setzen
	else								// Anzahl der einsen ungerade
		SCON0_TB8 = 0;					// SCON.TB8 lï¿½schen
}

char putchar (char c)
{
	uint8_t platch = SFRPAGE;
	SFRPAGE = 0x00;

	odd_parity(c);
	while (!SCON0_TI);
	SCON0_TI = 0;
	SBUF0 = c;

	SFRPAGE = platch;

	return c;
}

