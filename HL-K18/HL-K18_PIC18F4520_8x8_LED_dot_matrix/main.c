/* 
 * File:   main.c
 * Author: Fabien
 * Created on 20260126
 * Board: HL-K18
 * MCU: PIC18F4520
 * Display drawing on the 8x8 LEDs dots matrix with multiplexing
 * /!\ Warning: 64 LEDs @20 mA = 1,28 A ! -> do not switch on all LEDs, use multiplexing, 1 line at a time
 * Refresh at an interval of 2 ms max to benefit from retinal persistence.
 * To use the 8*8 LED dot matrix module, set S4 all four toggles ON, and all S2 8 toggles OFF
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xc.h>

#pragma config OSC = HS
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config PBADEN = OFF
#pragma config MCLRE = ON

#define _XTAL_FREQ 10000000UL // quartz 10 MHz

// 8x8 LEDs matrix columns
#define BIT_PORTA_OUT_COL8 (1 << 1)
#define BIT_PORTA_OUT_COL7 (1 << 2)
#define BIT_PORTA_OUT_COL6 (1 << 3)
#define BIT_PORTA_OUT_COL5 (1 << 5)
#define BIT_PORTE_OUT_COL4 (1 << 0)
#define BIT_PORTE_OUT_COL3 (1 << 1)
#define BIT_PORTE_OUT_COL2 (1 << 2)
#define BIT_PORTC_OUT_COL1 (1 << 0)

// 8x8 LEDs matrix lines
#define BIT_PORTD_OUT_LINE8 (1 << 0)
#define BIT_PORTD_OUT_LINE7 (1 << 1)
#define BIT_PORTD_OUT_LINE6 (1 << 2)
#define BIT_PORTD_OUT_LINE5 (1 << 3)
#define BIT_PORTD_OUT_LINE4 (1 << 4)
#define BIT_PORTD_OUT_LINE3 (1 << 5)
#define BIT_PORTD_OUT_LINE2 (1 << 6)
#define BIT_PORTD_OUT_LINE1 (1 << 7)

volatile uint8_t* const col_lats[8] = {
    &LATA, // col 7
    &LATA, // col 6
    &LATA, // col 5
    &LATA, // col 4
    &LATE, // col 3
    &LATE, // col 2
    &LATE, // col 1
    &LATC  // col 0
};

const uint8_t col_bits[8] = {
    BIT_PORTA_OUT_COL8,
    BIT_PORTA_OUT_COL7,
    BIT_PORTA_OUT_COL6,
    BIT_PORTA_OUT_COL5,
    BIT_PORTE_OUT_COL4,
    BIT_PORTE_OUT_COL3,
    BIT_PORTE_OUT_COL2,
    BIT_PORTC_OUT_COL1
};

volatile uint8_t display[8];
volatile uint8_t current_line = 0;

void Timer0_Init(void)
{
    T0CON = 0b11000100;
    /*
       TMR0ON = 1
       T08BIT = 1 (8-bit mode)
       T0CS   = 0 (internal clock, Fosc/4)
       PSA    = 1 (prescaler OFF)
       PS     = 010 (1:8)
    */
    TMR0 = 64; // preload
    INTCONbits.TMR0IF = 0;
    INTCONbits.TMR0IE = 1;
    INTCONbits.GIE    = 1;
}

void LED_8x8_dot_matrix_Init(void) {
    ADCON1 = 0x0F; // all digital

// set lines and columns port as output (0)
    // columns
    TRISA &= ~(BIT_PORTA_OUT_COL5 | BIT_PORTA_OUT_COL6 | BIT_PORTA_OUT_COL7 | BIT_PORTA_OUT_COL8);
//    TRISC &= ~(BIT_PORTC_OUT_COL1 | BIT_PORTC_OUT_SPEAKER);
    TRISC &= ~(BIT_PORTC_OUT_COL1);
    TRISE &= ~(BIT_PORTE_OUT_COL2 | BIT_PORTE_OUT_COL3 | BIT_PORTE_OUT_COL4);
    // lines
    TRISD &= ~(BIT_PORTD_OUT_LINE1 | BIT_PORTD_OUT_LINE2 | BIT_PORTD_OUT_LINE3 | BIT_PORTD_OUT_LINE4
            | BIT_PORTD_OUT_LINE5 | BIT_PORTD_OUT_LINE6 | BIT_PORTD_OUT_LINE7 | BIT_PORTD_OUT_LINE8);

// switch all LEDs OFF (0)
    // columns
    LATA &= ~(BIT_PORTA_OUT_COL5 | BIT_PORTA_OUT_COL6 | BIT_PORTA_OUT_COL7 | BIT_PORTA_OUT_COL8);
//    TRISC &= ~(BIT_PORTC_OUT_COL1 | BIT_PORTC_OUT_SPEAKER);
    LATC &= ~(BIT_PORTC_OUT_COL1);
    LATE &= ~(BIT_PORTE_OUT_COL2 | BIT_PORTE_OUT_COL3 | BIT_PORTE_OUT_COL4);
    // lines
    LATD &= ~(BIT_PORTD_OUT_LINE1 | BIT_PORTD_OUT_LINE2 | BIT_PORTD_OUT_LINE3 | BIT_PORTD_OUT_LINE4
            | BIT_PORTD_OUT_LINE5 | BIT_PORTD_OUT_LINE6 | BIT_PORTD_OUT_LINE7 | BIT_PORTD_OUT_LINE8);
}

void LED_8x8_dot_matrix_shut_all_lines(void) {
    LATD &= ~(BIT_PORTD_OUT_LINE1 | BIT_PORTD_OUT_LINE2 | BIT_PORTD_OUT_LINE3 | BIT_PORTD_OUT_LINE4
            | BIT_PORTD_OUT_LINE5 | BIT_PORTD_OUT_LINE6 | BIT_PORTD_OUT_LINE7 | BIT_PORTD_OUT_LINE8);
}

void LED_8x8_dot_matrix_shut_line(uint8_t y) {
    LATD &= ~(1 << (7 - y));
}

void LED_8x8_dot_matrix_light_line(uint8_t y) {
    LATD |= (1 << (7 - y));
}

void LED_8x8_dot_matrix_shut_all_columns(void) {
    LATA &= ~(BIT_PORTA_OUT_COL5 | BIT_PORTA_OUT_COL6 | BIT_PORTA_OUT_COL7 | BIT_PORTA_OUT_COL8);
//    TRISC &= ~(BIT_PORTC_OUT_COL1 | BIT_PORTC_OUT_SPEAKER);
    LATC &= ~(BIT_PORTC_OUT_COL1);
    LATE &= ~(BIT_PORTE_OUT_COL2 | BIT_PORTE_OUT_COL3 | BIT_PORTE_OUT_COL4);
}

void LED_8x8_dot_matrix_light_columns(uint8_t row) {
    LED_8x8_dot_matrix_shut_all_columns();
    for (uint8_t x = 0; x < 8; x++) {
        if (row & (1 << x)) {
            *col_lats[x] |= col_bits[x];
        }
    }
}

void LED_8x8_dot_matrix_shut_column(uint8_t x) {
    *col_lats[x] &= ~col_bits[x];
}

void __interrupt() ISR(void) {
    if (INTCONbits.TMR0IF) {
        TMR0 = 230; // reload
        LED_8x8_dot_matrix_shut_all_lines();
        LED_8x8_dot_matrix_shut_all_columns();

        LED_8x8_dot_matrix_light_columns(display[current_line]);
        LED_8x8_dot_matrix_light_line(current_line);

        current_line++;
        if (current_line >= 8)
            current_line = 0;

        INTCONbits.TMR0IF = 0;
    }
}

void main(void) {
    LED_8x8_dot_matrix_Init();
    Timer0_Init();
    
    //smiley
    display[0] = 0b00111100;
    display[1] = 0b01100010;
    display[2] = 0b11110101;
    display[3] = 0b10100001;
    display[4] = 0b10100101;
    display[5] = 0b10011001;
    display[6] = 0b01000010;
    display[7] = 0b00111100;

    __delay_ms(3000);

    while(1) {
        // squares 1
        display[0] = 0b11111111;
        display[1] = 0b11000001;
        display[2] = 0b10111101;
        display[3] = 0b10100101;
        display[4] = 0b10100101;
        display[5] = 0b10111101;
        display[6] = 0b10000001;
        display[7] = 0b11111111;
        __delay_ms(500);

        display[0] = 0b00000000;
        display[1] = 0b01111110;
        display[2] = 0b01000010;
        display[3] = 0b01011010;
        display[4] = 0b01011010;
        display[5] = 0b01000010;
        display[6] = 0b01111110;
        display[7] = 0b00000000;
        __delay_ms(500);
    }
}