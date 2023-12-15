#include <lpc214x.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Prototype for delay_ms function
void delay_ms(uint16_t j);

// Custom function to convert integer to string
void intToAscii(int num, char* buffer) {
    sprintf(buffer, "%d", num);
}

// Variable Delay Function:
void delay_ms(uint16_t j) {
    volatile uint16_t x, i;

    // Loop to Generate 1 ms Delay with CCLK = 60 MHz:
    for (i = 0; i < j; i++) {
        for (x = 0; x < 6000; x++);
    }
}

// Display Character Using LCD Command Write Function:
void LCD_CMD(char command) {
    // Upper Nibble of Command:
    IO0PIN = ((IO0PIN & 0xFFFF00FF) | ((command & 0xF0) << 8));
    IO0SET = 0x00000040; // EN = 1
    IO0CLR = 0x00000030; // Set LCD_RS = 0 & LCD_RW = 0
    delay_ms(5); // Call Delay Function
    IO0CLR = 0x00000040; // EN = 0
    delay_ms(5); // Call Delay Function

    // Lower Nibble of Command:
    IO0PIN = ((IO0PIN & 0xFFFF00FF) | ((command & 0x0F) << 12));
    IO0SET = 0x00000040; // EN = 1
    IO0CLR = 0x00000030; // Set LCD_RS = 0 & LCD_RW = 0
    delay_ms(5); // Call Delay Function
    IO0CLR = 0x00000040; // EN = 0
    delay_ms(5); // Call Delay Function
}

// Function to Configure LCD:
void LCD_INIT(void) {
    IO0DIR = 0x0000FFF0; // Set P0.12 to P0.15 LCD Data (D4 to D7); P0.4, 5, 5 as RS, RW and EN as output
    delay_ms(20); // Call Delay Function
    LCD_CMD(0x02); // Initialize Cursor to Home Position

    LCD_CMD(0x28); // Function Set: 4-Bit, 2 Line, 5*7 Dots
    LCD_CMD(0x0C); // Display ON, Cursor OFF, Blink OFF
    LCD_CMD(0x06); // Set the Entry Mode (Cursor Increment, Display Shift OFF)

    LCD_CMD(0x01); // Clear LCD display; Also, clear DDRAM content
    delay_ms(5); // Call Delay Function
    LCD_CMD(0x80); // Set DDRAM address or cursor position on display to first line
}

// Display String Using Data Write Function: 
void LCD_STRING(char* msg) {
    uint8_t i = 0;
    while (msg[i] != 0) {
        IO0PIN = ((IO0PIN & 0xFFFF00FF) | ((msg[i] & 0xF0) << 8));
        IO0SET = 0x00000050; // Set LCD_RS = 1 & EN = 1
        IO0CLR = 0x00000020; // Set LCD_RW = 0
        delay_ms(2); // Call Delay Function
        IO0CLR = 0x00000040; // EN = 0
        delay_ms(5); // Call Delay Function

        IO0PIN = ((IO0PIN & 0xFFFF00FF) | ((msg[i] & 0x0F) << 12));
        IO0SET = 0x00000050; // Set LCD_RS = 1 & EN = 1
        IO0CLR = 0x00000020; // Set LCD_RW = 0
        delay_ms(2); // Call Delay Function
        IO0CLR = 0x00000040; // EN = 0
        delay_ms(5); // Call Delay Function

        i++;
    }
}

void initGPIO() {
    // Initialize IR sensors, LED, and LCD pins as input/output
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

    char buffer[10]; // Buffer for converting integers to strings

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
            if (is_daytime == 0) {
                // Display entry and exit count during daytime
                LCD_CMD(0x01); // Clear LCD display
                LCD_STRING("Entry: ");
                intToAscii(count_entry, buffer);
                LCD_STRING(buffer);
                LCD_STRING("  Exit: ");
                intToAscii(count_exit, buffer);
                LCD_STRING(buffer);
                IO0SET |= (1 << 20); // Activate LED
                delay_ms(2000);
                IO0CLR |= (1 << 20); // Deactivate LED
            }
        } else {
            // Deactivate LED and display alert message during nighttime
            IO0CLR |= (1 << 20);
            LCD_CMD(0x01); // Clear LCD display
            LCD_STRING("Alert: Intruder ");
            delay_ms(2000);
			IO0CLR |= (1 << 20); // Deactivate LED
			
        }
    }

    return 0;
}
