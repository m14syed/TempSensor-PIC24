/*
 * File:   CourseProjT1.c
 * Author: Mohammed Sohaib Syed (501105890)
 *
 * Created on November 21, 2023, 9:21 AM
 */


#include "xc.h"
#include "stdio.h"
#include "string.h"
#include <p24fxxxx.h>

#pragma config POSCMOD = NONE    //Primary Oscillator Select->Primary oscillator disabled
#pragma config FNOSC = FRCPLL   //Oscillator Select->Fast RC Oscillator with PLL module (FRCPLL)
#pragma config FCKSM = CSDCMD  /*Clock Switching and Monitor->Clock switching and Fail-Safe Clock Monitor are disabled */
#pragma config FWDTEN = OFF    //Watchdog Timer Enable->Watchdog Timer is disabled
#pragma config JTAGEN = OFF    //JTAG Port Enable->JTAG port is disabled

#define CLOCK_SystemFrequencyGet()        (32000000UL)
#define CLOCK_PeripheralFrequencyGet()    (CLOCK_SystemFrequencyGet() / 2)
#define CLOCK_InstructionFrequencyGet()   (CLOCK_SystemFrequencyGet() / 2)
#define BAUDRATE 9600
#define MODE 16
#define BRGVAL (int) ((CLOCK_PeripheralFrequencyGet()/BAUDRATE)/MODE)-1
#define ADC_NSTEPS 1024  //Number of steps = 2^N
#define zero 0.5         //0°C in Voltage

void ConfigIntUART1();
void SendDataBuffer(const char  *buffer, uint32_t size);
void ADCinit(void);
uint16_t ADCread(void);
unsigned char Rxdata[1024];

int main(void){
    int16_t  ADCcode; //to hold the 16-bit ADC output code
    float   f_ADCcode; //to hold the a float format of ADC output
    AD1PCFG = 0xFFFE; //RB0 as analog pins
    TRISB = 0x0001; //RB0 set as input
    CNPU1 = 0x0000;  //no weak pull-up
    TRISE = 0x0000; //Fan DC is set as output
    _RE0 = 0; //Fan DC is initialized as 0
    _RE9 = 0;
    
    //Timer2 Configuration
    T2CON = 0x8030; // TMR2 on, prescale 1:256
    PR2 = 31249; // set period register for delay= 500 msec
    
    U1MODEbits.UARTEN = 0; //Disable UART1 for configuration
    ConfigIntUART1(); //Config UART1 
    ADCinit();     //Call ADC Configuration/initialization function
    
    U1MODEbits.UARTEN = 1; // Enable UART
    U1STAbits.UTXEN = 1; // Enable UART TX
    
    sprintf(Rxdata, "Mohammed Sohaib Syed \r\n\r\n");
    SendDataBuffer(Rxdata, strlen(Rxdata));
    while(1){
        ADCcode= ADCread(); // read conversion result (get ADC value at ADC1BUF0)
        f_ADCcode = (5.0/ ADC_NSTEPS) * (ADCcode); //convert back to equivalent analog
        f_ADCcode = (f_ADCcode - zero) / 0.01;  // Convert voltage to temperature; TMP36 has a 500mV offset (0.5V at 0°C) and 10mV/°C sensitivity
        //load the digital and a analog values into Rxdata as char-type data
        sprintf(Rxdata, "\r\nCurrent temperature is: %.1f Celsius \r\n\r\n", (double) f_ADCcode); 
        SendDataBuffer(Rxdata, strlen(Rxdata)); //transmit to the virtual terminal 
        if (f_ADCcode < 30){
            _RE0=0;
            _RE9=1;
            sprintf(Rxdata, "\r\nClockwise Rotation\r\n\r\n"); 
            SendDataBuffer(Rxdata, strlen(Rxdata)); //transmit to the virtual terminal 
        } 
        else{
            _RE0=1;
            _RE9=0;
            sprintf(Rxdata, "\r\nCounter Clockwise Rotation\r\n\r\n"); 
            SendDataBuffer(Rxdata, strlen(Rxdata)); //transmit to the virtual terminal 
        }
        while(TMR2);
    }
    U1MODEbits.UARTEN = 0;
    return 0;
}

void ConfigIntUART1(){
    U1MODE=0; //UART disabled/Low Speed/8-bit data, no parity/1 Stop bit
    U1BRG = BRGVAL; // Baud Rate setting 
    U1MODEbits.UARTEN = 1; //Enable UART1

}

void SendDataBuffer(const char *buffer,uint32_t size){
    while(size){
        while(!U1STAbits.TRMT);   //Wait until transmission is completed
        U1TXREG= *buffer;//Transmit the next character
        buffer++;
        size--;
    }
    while(!U1STAbits.TRMT);   //Wait until transmission is completed
}

void ADCinit(void){
    AD1CON2 = 0;  // Configure A/D voltage reference, and buffer fill modes. 
                  // Vr+ and Vr- from AVdd and AVss, Inputs are not scanned,
                  // Interrupt after every sample 
    AD1CHS = 0x0000; // Configure input channels, S/H+ input is AN0. 
    AD1CSSL = 0x0000; // NO inputs are scanned.
  
    AD1CON1 = 0x00E0; // Configure sample clock source and conversion trigger mode.
                      // Integer right-aligned format, auto conversion, Manual sampling  
                      
    AD1CON3 = 0x1F19; // Configure sample time = 31Tad, ADCS=25, Fcy=16Mhz, Fad=625KHz
    AD1CON1bits.ADON = 1; // Turn on A/D
}

uint16_t ADCread(void){
   AD1CON1bits.SAMP = 1; // Start sampling (manual sampling)  
   while (!AD1CON1bits.DONE); /* wait to complete conversion. */
   return ADC1BUF0;
}