/* 
 * File: main.c
 * Author: Fabien with great help of ChatGPT
 * Created on 20260124
 * Board: HL-K18
 * Display temperature from DS18B20 sensor on a 16x2 characters LCD in 4-bit mode
 * Microcontroller: PIC18F450
 * DS18B20 has a 1-wire data bus
 * DS18B20 already has an on-board 10 k? pull-up to Vcc
 * If inserted in the wrong direction, the DS18B20 chip will be damaged;
 * The correct direction is that the text of the DS18B20 chip is facing the outer edge of the development board
 * Connect the B20 pin to the RA4 GPIO with a jumper
 * Connect the LCD pins RS to RA2, RW to RA3 and EN to RA5 with jumpers
 * DS18B20 and 1602LCD pins against PIC18F450 pins:
 * RS    RA2 Register Select
 * RW    RA3 Read/Write
 * DQ    RA4 Data
 * EN    RA5 ENable
 * D4 E  RD4 Data bit 4
 * D5 F  RD5 Data bit 5
 * D6 G  RD6 Data bit 6
 * D7 DP RD7 Data bit 7
 */

#pragma config DEBUG = ON
//#pragma config FOSC = INTOSC // use internal oscillator
#pragma config LVP=OFF
#pragma config MCLRE = ON
#pragma config OSC = INTIO67 // use internal oscillator, RA6/RA7 as I/O
#pragma config PBADEN=OFF
#pragma config WDT=OFF

#define _XTAL_FREQ 8000000

// Total 8 GPIOs
#define LCD_RS LATAbits.LATA2 // Register Select: 0 = command, 1 = data
#define LCD_RW LATAbits.LATA3 // Read/Write: 0 = write, 1 = read
#define DS18B20_DQ LATAbits.LATA4 // temperature sensor 
#define LCD_EN LATAbits.LATA5 // ENable, high to low validates command or data
#define LCD_D4 LATDbits.LATD4
#define LCD_D5 LATDbits.LATD5
#define LCD_D6 LATDbits.LATD6
#define LCD_D7 LATDbits.LATD7

#include <stdint.h> // for int16_t
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

unsigned char OneWire_Reset(void) {
    TRISAbits.TRISA4 = 0; // output
    LATAbits.LATA4 = 0;
    __delay_us(480);
    TRISAbits.TRISA4 = 1; // input
    __delay_us(70);

    unsigned char presence = PORTAbits.RA4;
    __delay_us(410);

    return presence;
}

void OneWire_WriteBit(unsigned char bit) {
    TRISAbits.TRISA4 = 0; // output
    LATAbits.LATA4 = 0;
    if (bit)
        __delay_us(5);
    else
        __delay_us(60);
        
    TRISAbits.TRISA4 = 1; // input
    if (bit)
        __delay_us(55);
    else
        __delay_us(5);
}

unsigned char OneWire_ReadBit(void) {
    unsigned char bit;
    TRISAbits.TRISA4 = 0; // output
    LATAbits.LATA4 = 0;
    __delay_us(2);
    TRISAbits.TRISA4 = 1; // input
    __delay_us(10);
    bit = PORTAbits.RA4;
    __delay_us(50);
    return bit;
}

unsigned char OneWire_ReadByte(void) {
    unsigned char i;
    unsigned char data = 0;
    for(i = 0; i < 8; i++) {
        data >>= 1;
        if(OneWire_ReadBit())
            data |= 0x80; // binary OR 10000000
    }
    return data;
}

void OneWire_WriteByte(unsigned char data) {
    unsigned char i;
    for(i = 0; i < 8; i++) {
        OneWire_WriteBit(data & 0x01);
        data >>= 1; // right shift (divides by 2) by pushing a 0 from left
    }
}

float DS18B20_ReadTemp(void) {
    unsigned char LSB, MSB;
    int16_t temp; // signed to support negative values

    OneWire_Reset();
    OneWire_WriteByte(0xCC); // skip ROM
    OneWire_WriteByte(0x44); // convert T
    __delay_ms(750); // wait for conversion

    OneWire_Reset();
    OneWire_WriteByte(0xCC); // skip ROM
    OneWire_WriteByte(0xBE); // read scratchpad

    LSB = OneWire_ReadByte();
    MSB = OneWire_ReadByte();

    temp = (MSB << 8) | LSB;
    return (float)temp / 16.0; // convert to Celsius
}

void lcd_pulse(void) {
    // transition ENable high to low validates command or data
    LCD_EN = 1;
    __delay_us(5);
    LCD_EN = 0;
    __delay_us(50);
}

void lcd_send_nibble(unsigned char nibble) {
    LCD_D4 = (nibble >> 0) & 1;
    LCD_D5 = (nibble >> 1) & 1;
    LCD_D6 = (nibble >> 2) & 1;
    LCD_D7 = (nibble >> 3) & 1;
    lcd_pulse();
}

void lcd_cmd(unsigned char cmd) {
    LCD_RS = 0; // Register Select 0 = command
    lcd_send_nibble(cmd >> 4); // 4 most significant bits
    lcd_send_nibble(cmd & 0x0F); // 4 least significant bits
    __delay_ms(2);
}

void lcd_put_char(char data) {
    LCD_RS = 1; // Register Select 0 = data
    lcd_send_nibble(data >> 4); // 4 most significant bits
    lcd_send_nibble(data & 0x0F); // 4 least significant bits
    __delay_ms(2);
}

/* 
 * 1602LCD pins against PIC18F450 pins:
 * RS    RA2
 * RW    RA3
 * EN    RA5
 * D4 E  RD4
 * D5 F  RD5
 * D6 G  RD6
 * D7 DP RD7
*/
void lcd_init(void) {
//   TRISA = 0x00;
// set GPIOs as outputs    
    TRISAbits.TRISA2 = 0; // set Register Select as output
    TRISAbits.TRISA3 = 0; // set Read/Write as output
    TRISAbits.TRISA5 = 0; // set ENable output
    TRISDbits.TRISD4 = 0; // set Data bit 4 output
    TRISDbits.TRISD5 = 0; // set Data bit 5 output
    TRISDbits.TRISD6 = 0; // set Data bit 6 output
    TRISDbits.TRISD7 = 0; // set Data bit 7 output
    LCD_EN = 0; // initialise ENable; high to low transition validates command or data
    LCD_RW = 0; // initialize Read/Write; 0 = write, 1 = read
    LCD_RS = 0; // initialize Register Select: 0 = command, 1 = data
    LCD_D4 = 0; // initialize data bit 4
    LCD_D5 = 0; // initialize data bit 5
    LCD_D6 = 0; // initialize data bit 6
    LCD_D7 = 0; // initialize data bit 7
    __delay_ms(20); // wait after power on
    // special sequence
    lcd_send_nibble(0x03);
    __delay_ms(5);
    lcd_send_nibble(0x03);
    __delay_us(150);
    lcd_send_nibble(0x03);
    __delay_us(150);
    lcd_send_nibble(0x02); // 4 bits mode
    lcd_cmd(0x28); // 4 bits, 2 lines, 5x8
    lcd_cmd(0x0C); // display ON, cursor OFF
    lcd_cmd(0x06); // auto increment, no shift
    lcd_cmd(0x01); // clear display
    __delay_ms(2);
}

void lcd_clear(void) {
    lcd_cmd(0x01);
    __delay_ms(2);
}

void lcd_goto(unsigned char row, unsigned char col) {
    unsigned char address;
    if(row == 1)
        address = 0x80 + (col - 1);
    else
        address = 0xC0 + (col - 1);
    lcd_cmd(address);
}

void lcd_put_string(const char *str) {
    while(*str) {
        lcd_put_char(*str++);
    }
}

void lcd_create_char(unsigned char location, unsigned char charmap[]) {
    location &= 0x07; // 8 locations (0-7)
    lcd_cmd(0x40 | (location << 3)); // CGRAM address
    for(int i = 0; i < 8; i++) {
        lcd_put_char(charmap[i]);
    }
}

void main(void) {
    OSCCON = 0b01110010; // 8 MHz internal
    unsigned char thermometer[8] = {
        0b00100, //   X  
        0b01110, //  XXX 
        0b01010, //  X X 
        0b01010, //  X X 
        0b01110, //  XXX 
        0b11111, // XXXXX
        0b11111, // XXXXX
        0b01110  //  XXX     
};
    char buffer[16];
    float temperature;

    lcd_init();
    lcd_clear();
    lcd_create_char(0, thermometer); // create thermometer symbol in CGRAM at location 0
    
    lcd_goto(1,1);
    lcd_put_char(0); // custom character at location 0
    lcd_put_string(" TEMPERATURE");
    while(1) {
        temperature = DS18B20_ReadTemp();
        if (temperature < 0)
            sprintf(buffer, "%2.2f ", -temperature); // use absolute value
        else {
            if (temperature > 0)
                sprintf(buffer, "+%2.2f ", temperature);
            else
                sprintf(buffer, "%2.2f ", temperature);
        }
        lcd_goto(2,1);
        lcd_put_string("                "); // full line of 16 spaces
        lcd_goto(2,1);
        lcd_put_string(buffer);
        lcd_put_char(223);
        lcd_put_string("C");
        __delay_ms(2000);
    }
}