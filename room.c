#include <lpc214x.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define LCD (0xFFFF00FF)
#define RS (1<<4)
#define RW (1<<5)
#define EN (1<<6)

// Variable Delay Function:
void delay_ms(uint16_t j) {
    uint16_t x, i;

    // Loop to Generate 1 ms Delay with CCLK = 60 MHz:
    for (i = 0; i < j; i++) {
        for (x = 0; x < 6000; x++);
    }
}

// Display Character Using LCD Command Write Function:
void LCD_CMD(char command) {
    IO0PIN = ((IO0PIN & 0xFFFF00FF) | ((command & 0xF0) << 8));
    IO0SET = 0x00000040; // EN = 1
    IO0CLR = 0x00000030; // Set LCD_RS = 0 & LCD_RW = 0
    delay_ms(5);
    IO0CLR = 0x00000040; // EN = 0
    delay_ms(5);

    IO0PIN = ((IO0PIN & 0xFFFF00FF) | ((command & 0x0F) << 12));
    IO0SET = 0x00000040; // EN = 1
    IO0CLR = 0x00000030; // Set LCD_RS = 0 & LCD_RW = 0
    delay_ms(5);
    IO0CLR = 0x00000040; // EN = 0
    delay_ms(5);
}

// Function to Configure LCD:
void LCD_INIT(void) {
    IO0DIR = 0x0000FFF0; // Set P0.12 to P0.15 LCD Data (D4 to D7); P0.4, 5, 5 as RS, RW and EN as output
    delay_ms(20);
    LCD_CMD(0x02); // Initialize Cursor to Home Position

    LCD_CMD(0x28); // Function Set: 4-Bit, 2 Line, 5*7 Dots
    LCD_CMD(0x0C); // Display ON, Cursor OFF, Blink OFF
    LCD_CMD(0x06); // Set the Entry Mode (Cursor Increment, Display Shift OFF)

    LCD_CMD(0x01); // Clear LCD display; Also, clear DDRAM content
    delay_ms(5);
    LCD_CMD(0x80); // Set DDRAM address or cursor position on display to first line
}

// Display String Using Data Write Function:
void LCD_STRING(char* msg) {
    uint8_t i = 0;
    while (msg[i] != 0) {
        IO0PIN = ((IO0PIN & 0xFFFF00FF) | ((msg[i] & 0xF0) << 8));
        IO0SET = 0x00000050; // Set LCD_RS = 1 & EN = 1
        IO0CLR = 0x00000020; // Set LCD_RW = 0
        delay_ms(2);
        IO0CLR = 0x00000040; // EN = 0
        delay_ms(5);

        IO0PIN = ((IO0PIN & 0xFFFF00FF) | ((msg[i] & 0x0F) << 12));
        IO0SET = 0x00000050; // Set LCD_RS = 1 & EN = 1
        IO0CLR = 0x00000020; // Set LCD_RW = 0
        delay_ms(2);
        IO0CLR = 0x00000040; // EN = 0
        delay_ms(5);

        i++;
    }
}

void initGPIO() {
    IO0DIR &= ~((1 << 10) | (1 << 11)); // P0.10 and P0.11 as inputs (IR sensors)
    IO0DIR |= (1 << 20); // P0.20 as output (LED)
}

void initRTC() {
    CCR = 0x11; // Enable the RTC and Clock Source (CCLK)
    CIIR = 0x01; // Interrupt every second
}

void getTime(int *hour, int *minute) {
    *hour = (HOUR & 0x1F);
    *minute = (MIN & 0x3F);
}

int main() {
    int count_entry = 0;
    int count_exit = 0;
    int is_daytime = 0;

    int entry_sensor, exit_sensor;
    int hour, minute;

    char buffer[10];

    initGPIO();
    initRTC();
    LCD_INIT(); // Initialize LCD

    while (1) {
        getTime(&hour, &minute);
        if (hour >= 5 && hour < 17) {
            is_daytime = 1; // Daytime
        } else {
            is_daytime = 0; // Nighttime
        }

        entry_sensor = (IO0PIN & (1 << 10)) >> 10;
        exit_sensor = (IO0PIN & (1 << 11)) >> 11;

        if (entry_sensor == 1) {
            count_entry++;
        }

        if (exit_sensor == 1) {
            count_exit++;
        }

        if (count_entry > count_exit) {
            if (!is_daytime) {
                IO0SET |= (1 << 20); // Activate LED
                LCD_CMD(0x01);      // Display clear
                sprintf(buffer, "Intruder: %d", count_entry - count_exit);
                LCD_STRING(buffer);
            }
        } else {
            IO0CLR |= (1 << 20); // Deactivate LED
        }
    }

    return 0;
}


/*
#include <lpc214x.h>
#include<stdio.h> // Declares functions that deal with standard I/O
#include<stdint.h> // Declare sets of integer types having specified widths
#include<stdlib.h> 


// Variable Delay Function:
void delay_ms(uint16_t j)
{
    uint16_t x, i;

	// Loop to Generate 1 ms Delay with CCLK = 60 MHz:
	for(i=0;i<j;i++)
	{
    	for(x=0;x<6000;x++);
	}
}

// Function to initialize UART for LCD display
void initUART() {
	// Execution Starts Here:
	PINSEL0 = 0x00000005; // Set P0.0 Pin as TxD and P0.1 Pin as RxD (UART0)
	U0LCR = 0x83; // Set 8 Bits - NO Parity - 1 Stop Bit for Transmission - DLAB = 1
	U0DLL = 97; // Set BAUD Rate as 9600 bps at 15 MHz VPB Clock (PCLK)
	U0LCR = 0x03; // Disable Access to Divisor Latches 
}
					
	// Infinite Loop (An embedded program does not stop):
	while(1)
	{
		while(!(U0LSR & 0x01)); // Wait until UART0 is ready with the received data
	  	data = U0RBR; // Read the received data from U0RBR
		while(!(U0LSR & 0x20)); // Wait until UART0 is ready to send character    
		U0THR = data; // Sent a character to UART Transmit Shift Register
	} 	

// Function to send character over UART
void sendChar( unsigned char c) {
    while(!(U0LSR & 0x20));			  	
    U0THR = c;
}

// Function to initialize GPIO for IR sensors and buzzer
void initGPIO() {
    // Initialize IR sensors and buzzer pins as input/output
    IO0DIR &= ~((1<<10) | (1<<11)); // P0.10 and P0.11 as inputs (IR sensors)
    IO0DIR |= (1<<20); // P0.20 as output (buzzer)
}

// Function to initialize RTC
void initRTC() {
    CCR = 0x11; // Enable the RTC and Clock Source (CCLK)
    CIIR = 0x01; // Interrupt every second
}

// Function to get current time from RTC
void getTime(int *hour, int *minute) {
    *hour = (HOUR & 0x1F); // Masking to get hours
    *minute = (MIN & 0x3F); // Masking to get minutes
}

int main() {
    int count_entry = 0;
    int count_exit = 0;
    int is_daytime = 0;
    
    // Moved the variable declarations to the beginning of the block
    int entry_sensor, exit_sensor;
    int hour, minute;
    
    //initUART(); // Initialize UART for LCD
    initGPIO(); // Initialize GPIO for sensors and buzzer
    initRTC(); // Initialize RTC
    
    while(1) {
	    // Check if it's daytime or nighttime
        getTime(&hour, &minute);
        if(hour >= 5 && hour < 17) {
            is_daytime = 1; // Daytime
        } else {
            is_daytime = 0; // Nighttime
        }
        // Read values from IR sensors
        entry_sensor = (IO0PIN & (1<<10)) >> 10;
        exit_sensor = (IO0PIN & (1<<11)) >> 11;
        
        // Check if a p-erson enters the room
        if(entry_sensor == 1) {
            count_entry++;
        }
        
        // Check if a person exits the room
        if(exit_sensor == 1) {
            count_exit++;
        }
        
        // Check if someone is still in the room
        if(count_entry > count_exit) {
            if(is_daytime == 0) {							  
				//while(!(U0LSR & 0x20)); // Wait until UART0 is ready to send character    
		        //U0THR = 'c'; // Sent a character to UART Transmit Shift Register
			    //delay_ms(1000);
                // Alert during nighttime if someone is in the room
				
                sendChar('S'); sendChar('o'); sendChar('m'); sendChar('e');
                sendChar('b'); sendChar('o'); sendChar('d'); sendChar('y');
                sendChar(' '); sendChar('i'); sendChar('s');
                sendChar(' '); sendChar('i'); sendChar('n');
                sendChar(' '); sendChar('t'); sendChar('h'); sendChar('e');
                sendChar(' '); sendChar('r'); sendChar('o'); sendChar('o');
                sendChar('m'); sendChar('\n');
                  
                // Activate buzzer alarm
                IO0SET |= (1<<20);
            }
        } else {
            IO0CLR |= (1<<20); // Deactivate buzzer alarm if no one is in the room
        }
        

    }
    
    return 0;
}

*/
