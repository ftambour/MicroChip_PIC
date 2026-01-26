/*
 * Project name: led_blinking (testing the I/O port)
 * Description: demonstrate blinking of a LED connected on GPIO. Diodes go on
     and off each second.
 * Test configuration:
     MCU:             PIC12F675
     Dev.Board:       EasyPIC3
     Oscillator:      Internal, 4.000 MHz
     Ext. Modules:    -
     Compiler:        XC8
 * NOTES:
     LEDs de l?EasyPIC3 : 1 seule LED câblée sur GP2
     Sur la EasyPIC3, pour le PIC12F675 :
     1 seule LED est physiquement connectée: GP2
     Les autres LEDs PORTA ne sont pas reliées à ce PIC (limitation du boîtier 8 broches)
     Résultat: Seule la LED GP2 peut s?allumer !!!
 
     To achieve 1-second blinking interval, the project must be compiled for 4
     MHz, although it uses chip's internal clock.
*/

// Configuration bits (XC8)
#pragma config FOSC = INTRCIO // Oscillateur interne, GP4/GP5 en I/O
#pragma config WDTE = OFF     // Watchdog OFF
#pragma config PWRTE = ON     // Power-up Timer ON
#pragma config MCLRE = OFF    // GP3 en entrée numérique
#pragma config BOREN = OFF
#pragma config CP = OFF
#pragma config CPD = OFF

#define _XTAL_FREQ 4000000

#include <xc.h>

void main(void){
  ANSEL = 0x00;        // Désactiver les entrées analogiques
  CMCON = 0x07;        // Comparateurs OFF
  // Configuration des directions
  TRISIO = 0x04;       // configure all GPIO pins as output
  TRISIO = 0b00111011; // only GP2 as output
  GPIO2 = 0;           // LED off    
    
  while(1){            // beginning of a repeat loop
    GPIO2 = 1;         // turn ON LED on RA2
    __delay_ms(500);   // wait for 1/2 s
    GPIO2 = 0;         // turn OFF LED on RA2
    __delay_ms(500);   // wait for 1/2 s
  }                    // endless loop (as this condition is always satisfied)
}