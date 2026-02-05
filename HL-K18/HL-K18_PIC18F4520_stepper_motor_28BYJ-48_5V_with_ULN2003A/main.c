/*
 * File: main.c
 * Author: Fabien with great help of ChatGPT
 * Created on 20260205
 * Board: HL-K18
 * 28BYJ-48 5 V stepper motor through ULN2003A Darlington array
 * MCU: PIC18F4520
 * 1 turn = 4096 half-steps
 * Set the jumpers for COL5..8
 * 28BYJ-48 pin # Color  Function PIC18F4520 pin
 * 1              blue   coil 4   COL5 -> RE0
 * 2              pink   coil 2   COL6 -> RE1
 * 3              yellow coil 3   COL7 -> RE2
 * 4              orange coil 1   COL8 -> RC0
 * 5              red    Vcc 5 V
 */

#pragma config DEBUG = OFF
//#pragma config FOSC = INTOSC // use internal oscillator
#pragma config LVP=OFF
#pragma config MCLRE = ON
#pragma config OSC = INTIO67 // use internal oscillator, RA6/RA7 as I/O
//#pragma config OSC = HS // use external oscillator
#pragma config PBADEN=OFF
#pragma config WDT=OFF

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

#define COIL1 LATCbits.LATC0 // RC0
#define COIL2 LATEbits.LATE1 // RE1
#define COIL3 LATEbits.LATE2 // RE2
#define COIL4 LATEbits.LATE0 // RE0
#define _XTAL_FREQ 8000000
//#define _XTAL_FREQ 10000000

// Half-steps sequence
// If the motor vibrates without turning, use the other stepTable
//const unsigned char stepTable[8][4] = {
//    // RC0  RE2  RE1  RE0
//    { 1,   0,   0,   0 },  // A
//    { 1,   1,   0,   0 },  // A+B
//    { 0,   1,   0,   0 },  // B
//    { 0,   1,   1,   0 },  // B+C
//    { 0,   0,   1,   0 },  // C
//    { 0,   0,   1,   1 },  // C+D
//    { 0,   0,   0,   1 },  // D
//    { 1,   0,   0,   1 }   // D+A
//};
const unsigned char stepTable[8][4] = {
//    RC0  RE1  RE2  RE0
    { 1,   0,   0,   0 },  // 1
    { 1,   0,   1,   0 },  // 1 + 3
    { 0,   0,   1,   0 },  // 3
    { 0,   1,   1,   0 },  // 3 + 2
    { 0,   1,   0,   0 },  // 2
    { 0,   1,   0,   1 },  // 2 + 4
    { 0,   0,   0,   1 },  // 4
    { 1,   0,   0,   1 }   // 4 + 1
};

static unsigned char stepIndex = 0;

void Stepper_Init(void) {
    TRISCbits.TRISC0 = 0; // RC0 as output
    TRISEbits.TRISE0 = 0; // RE0 as output
    TRISEbits.TRISE1 = 0; // RE1 as output
    TRISEbits.TRISE2 = 0; // RE2 as output

    COIL1 = 0;
    COIL2 = 0;
    COIL3 = 0;
    COIL4 = 0;
}

void Stepper_ApplyStep(unsigned char step) {
    COIL1 = stepTable[step][0];
    COIL2 = stepTable[step][1];
    COIL3 = stepTable[step][2];
    COIL4 = stepTable[step][3];
}

void Stepper_Stop(void) {
    COIL1 = 0;
    COIL2 = 0;
    COIL3 = 0;
    COIL4 = 0;
}

void Stepper_StepForward(void) {
    stepIndex++;
    if(stepIndex >= 8) stepIndex = 0;
    Stepper_ApplyStep(stepIndex);
}

void Stepper_StepBackward(void) {
    if(stepIndex == 0) stepIndex = 7;
    else stepIndex--;
    Stepper_ApplyStep(stepIndex);
}

void Stepper_RotateCCW(unsigned int steps) {
    while(steps--) {
        Stepper_StepForward();
        __delay_ms(3); // adjust speed here
    }
}

void Stepper_RotateCW(unsigned int steps) {
    while(steps--) {
        Stepper_StepBackward();
        __delay_ms(3); // adjust speed here
    }
}

void main(void) {
    ADCON1 = 0x0F; // all digital
    OSCCON = 0b01110010; // only for 8 MHz internal oscillator

    Stepper_Init();

    while(1) {
        Stepper_RotateCW(4096);     // ~1 rotation
        Stepper_Stop();
        __delay_ms(1000);

        Stepper_RotateCCW(4096);    // ~1 rotation back
        Stepper_Stop();
        __delay_ms(1000);
    }
}