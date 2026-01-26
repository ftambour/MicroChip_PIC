/*
 * Author: Fabien Tambour
 * Created on 20260123
 * Project name:
     tmr0 (Using timer and interrupt)
 * Description:
     Demonstrate using interrupts in C. Turn on/off a LED on a gpio.
 * Test configuration:
     MCU:             PIC12F675
     Dev.Board:       EasyPIC3
     Oscillator:      internal, 4.000 MHz
     Ext. Modules:    -
     Compiler:        XC8
 * NOTES:
     LEDs de l?EasyPIC3 : 1 seule LED câblée sur GP2
     Sur la EasyPIC3, pour le PIC12F675 :
     1 seule LED est physiquement connectée: GP2
     Les autres LEDs PORTA ne sont pas reliées à ce PIC (limitation du boîtier 8 broches)
     Résultat: Seule la LED GP2 peut s?allumer !!!
*/

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>

// Configuration bits (exemple)
#pragma config FOSC = INTRCIO //Internal oscillator
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config MCLRE = OFF
#pragma config BOREN = OFF
#pragma config CP = OFF
#pragma config CPD = OFF

#define _XTAL_FREQ 4000000UL

volatile unsigned int counter = 0; // initialize counter

void __interrupt() isr(void) {
    if (INTCONbits.T0IF) { // Timer0 interruption?
        INTCONbits.T0IF = 0; // Clear Timer0 interruption flag
        counter++;
    }
}

void timer0_init(void)
{
    OPTION_REGbits.T0CS = 0; // Timer internal source (Fosc/4)
    OPTION_REGbits.PSA = 0; // Affect prescaler to Timer0

    // Set prescaler to 1:256
    OPTION_REGbits.PS0 = 1;
    OPTION_REGbits.PS1 = 1;
    OPTION_REGbits.PS2 = 1;

    TMR0 = 0;                // reset Timer0
    INTCONbits.T0IF = 0;     // clear Timer0 interruption flag
    INTCONbits.T0IE = 1;     // enable Timer0 interrupt
    INTCONbits.GIE  = 1;     // enable global interrupts
}

void main(void) {
    ANSEL = 0x00;        // disable ADC
    CMCON = 0x07;        // disable comparators
    TRISIO = 0b00111011; // only GP2 as output
    GPIO2 = 1;           // set LED ON    

    timer0_init();

    while (1) {
      if (counter > 30) {
        GPIO2 = 1; // turn ON LED on RA2
        counter = 0; // reset counter
      }
      else if (counter > 1) {
        GPIO2 = 0; // turn OFF LED on RA2
      }
    }
}