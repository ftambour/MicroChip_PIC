/* 
 * File: main.c
 * Author: Fabien with great help of ChatGPT
 * Created on 20260202
 * Board: HL-K18
 * Display temperature and humidity from DHT22 (AM2303) sensor on a 16x2 characters LCD in 4-bit mode
 * Microcontroller: PIC18F4520
 * DHT22 has a 1-wire data bus
 * DHT22 already has an on-board 10 kR pull-up to Vcc
 * If inserted in the wrong direction, the DHT22 chip will be damaged;
 * Connect the DATA pin to the RA4 GPIO with a jumper
 * NB: RA4 is a special open drain TTL input (not a standard GPIO)
 * Never write 1 to RA4
 * If you have a DS18B20 sensor on the HL-K18 board you don't need to remove it
 * Connect the LCD pins RS to RA2, RW to RA3 and EN to RA5 with jumpers
 * DHT22 and 1602LCD pins against PIC18F4520 pins:
 * RS    RA2 Register Select
 * RW    RA3 Read/Write
 * DQ    RA4 DHT22 data
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
//#pragma config OSC = HS // use external oscillator
#pragma config PBADEN=OFF
#pragma config WDT=OFF

#define _XTAL_FREQ 8000000
//#define _XTAL_FREQ 10000000

// Total 8 GPIOs
#define DHT22_DATA PORTAbits.RA4 // temperature+humidity sensor pin state
#define DHT22_TRIS TRISAbits.TRISA4
#define LCD_RS LATAbits.LATA2 // Register Select: 0 = command, 1 = data
#define LCD_RW LATAbits.LATA3 // Read/Write: 0 = write, 1 = read
#define LCD_EN LATAbits.LATA5 // ENable, high to low validates command or data
#define LCD_D4 LATDbits.LATD4
#define LCD_D5 LATDbits.LATD5
#define LCD_D6 LATDbits.LATD6
#define LCD_D7 LATDbits.LATD7

#include <stdint.h> // for int16_t
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

unsigned int humidity;
signed int temperature;

void DHT22_Start(void) {
  DHT22_DATA = 0;
  DHT22_TRIS = 0; // Output low
  __delay_ms(2);  // >1 ms
  DHT22_TRIS = 1; // Input, pull-up pulls high
  __delay_us(30);
}

char DHT22_CheckResponse(void) {
  __delay_us(40);
  if (!DHT22_DATA) {
    __delay_us(80);
    if (DHT22_DATA) {
      __delay_us(80);
      return 1;
    }
  }
  return 0;
}

unsigned char DHT22_ReadByte(void) {
  unsigned char i, data = 0;
  unsigned int timeout;
  for (i = 0; i < 8; i++) {
    timeout = 10000;
    while (!DHT22_DATA && timeout--); // Wait for HIGH
    if (timeout == 0) return 0;
    __delay_us(40); // sampling point

    data <<= 1;
    if (DHT22_DATA)
        data |= 1;

    while (DHT22_DATA); // Wait for LOW (bit end))
    timeout = 10000;
    while (DHT22_DATA && timeout--);
    if (!timeout) return 0;
  }
  return data;
}

char DHT22_ReadData(void) {
  unsigned char hH, hL, tH, tL, checksum;
  unsigned char sum;

  DHT22_Start();
  if (!DHT22_CheckResponse())
    return 0;
  hH = DHT22_ReadByte();
  hL = DHT22_ReadByte();
  tH = DHT22_ReadByte();
  tL = DHT22_ReadByte();
  checksum = DHT22_ReadByte();
  sum = hH + hL + tH + tL;
  if (sum != checksum)
    return 0;
  humidity = ((unsigned int)hH << 8) | hL;
  temperature = ((signed int)tH << 8) | tL;
  if (temperature & 0x8000) {
    temperature &= 0x7FFF;
    temperature = -temperature;
  }
  return 1;
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
    if (row < 1 || row > 2) return;
    if (col < 1 || col > 16) return;
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
    ADCON1 = 0x0F; // all digital
    OSCCON = 0b01110010; // only for 8 MHz internal oscillator
    //thermometer custom character icon
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
    //humidity custom character icon
    unsigned char drop[8] = {
        0b00000,
        0b00000,
        0b00000,
        0b00100, //   X
        0b01110, //  XXX 
        0b11111, // XXXXX
        0b11111, // XXXXX
        0b01110  //  XXX     
};
    char buffer[16]; //LCD line

    lcd_init();
    lcd_clear();
    lcd_create_char(0, thermometer); // create thermometer symbol in CGRAM at location 0
    lcd_create_char(1, drop); // create drop symbol in CGRAM at location 1
    
    while(1) {
        if (DHT22_ReadData()) {
            float hum = humidity / 10.0;
            float temp = temperature / 10.0;
            // Avoid sprintf with float
            // Split integer and decimal parts
            int h_int = (int)hum;
            int h_dec = abs((int)((hum - h_int) * 100));
            int t_int = (int)temp;
            int t_dec = abs((int)((temp - t_int) * 100));
            
            lcd_goto(1,1);
            lcd_put_string("                "); // full 16 spaces line
            lcd_goto(1,1); // row,col
            lcd_put_char(0); // custom character at location 0
            lcd_put_string(" TMP ");
            if (temp < 0)
                sprintf(buffer, "%d.%02d ", t_int, t_dec);
            else {
                if (temp > 0)
                    sprintf(buffer, "+%d.%02d ", t_int, t_dec);
                else
                    sprintf(buffer, "%d.%02d ", t_int, t_dec);
            }
            lcd_put_string(buffer);
            lcd_put_char(223);
            lcd_put_string("C");
        
            lcd_goto(2,1);
            lcd_put_string("                "); // full 16 spaces line
            lcd_goto(2,1); // row,col
            lcd_put_char(1); // custom character at location 1
            lcd_put_string(" HUM ");
            // sprintf(buffer, "%2.2f ", hum);
            sprintf(buffer, "%d.%02d ", h_int, h_dec);
            lcd_put_string(buffer);
            lcd_put_string("%");
        }
        else {
            lcd_goto(1,1);
            lcd_put_string("                "); // full 16 spaces line
            lcd_goto(1,1); // row,col
            lcd_put_string("ERROR: DHT22");
            lcd_goto(2,1); // row,col
            lcd_put_string("not responding");
        }
        __delay_ms(5000);

    }
}