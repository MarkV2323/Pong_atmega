/*	Author: Mark Alan Vincent II
 *  Partner(s) Name: NA
 *	Lab Section: A01
 *	Assignment: Final Project
 *	Youtube:
 *  Exercise Description:
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#include "timer.h"
#endif

// Method for handling writing to register.
// Modified: RCLK (C2 and C4) and SRCLR(C3 and C5) are independent of each other.
void transmit_data(unsigned char data, unsigned char regi) {
    int i;
    for (i = 0; i < 8 ; ++i) {
   	 // Sets SRCLR to 1 allowing data to be set
   	 // Also clears SRCLK in preparation of sending data
     if (regi == 1) {
       PORTC = 0x08;
     } else if (regi == 2) {
       PORTC = 0x20;
     }
   	 // set SER = next bit of data to be sent.
   	 PORTC |= ((data >> i) & 0x01);
   	 // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
   	 PORTC |= 0x02;
    }

    // set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
    if (regi == 1) {
      PORTC |= 0x04;
    } else if (regi == 2) {
      PORTC |= 0x10;
    }

    // clears all lines in preparation of a new transmission
    PORTC = 0x00;
}

// Function for A2D conversion
void A2D_init() {
      ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: Enables analog-to-digital conversion
	// ADSC: Starts analog-to-digital conversion
	// ADATE: Enables auto-triggering, allowing for constant
	//	    analog to digital conversions.
}

// globals
unsigned char tmpA = 0x00;

// Contains frames of the current board.
unsigned char frames_x[] = {0x70, 0x00, 0x00, 0x00, 0x0E};
unsigned char frames_y[] = {0x1E, 0x1D, 0x0B, 0x17, 0x0F}; // dnt modifiy
unsigned char frames_index = 0;
unsigned long count = 0;

enum SM1_States { SM1_Start, SM1_Draw } sm1_state;
int TickFct_DrawGame(int state) {

      // Transitions
      switch (state) {
          case SM1_Start:
              // Go to Draw state.
              state = SM1_Draw;
              break;

          case SM1_Draw:
              // Continue Drawing.
              state = SM1_Draw;
              break;

          default:
              break;
      }

      //Actions
      switch (state) {
           case SM1_Start:
            break;

           case SM1_Draw:
            // Draws 5 Frames, top down.
            if (frames_index == 4) {
              // draw last frame and reset
              transmit_data(frames_y[frames_index], 2);
              transmit_data(frames_x[frames_index], 1);
              transmit_data(0x00, 1);

              // transmit_data(0x0F, 1);
              // transmit_data(0x1E, 2);
              // transmit_data(0x00, 1);
              // transmit_data(0x0F, 2);
              // transmit_data(0x0E, 1);
              // transmit_data(0x00, 1);
              // transmit_data(0x1E, 2);
              frames_index = 0;
            } else {
              // draw and increment frame
              transmit_data(frames_y[frames_index], 2);
              transmit_data(frames_x[frames_index], 1);
              transmit_data(0x00, 1);
              frames_index = frames_index + 1;

              // transmit_data(0x0F, 1);
              // transmit_data(0x1E, 2);
              // transmit_data(0x00, 1);
              // transmit_data(0x0F, 2);
              // transmit_data(0x0E, 1);
              // transmit_data(0x00, 1);
              // transmit_data(0x1E, 2);
            }
            count++;
            if (count == 1000) {
              // shift player1 Left
              frames_x[0] = frames_x[0] >> 1;
              count = 0;
            }
            break;

           default:
            break;
      }

      return state;
}


// Main Funcntion
int main(void) {

    // PORTS
    DDRA = 0x00; PORTA = 0xFF; // PortA as input
    DDRB = 0xFF; PORTB = 0x00; // PortB as output
    DDRC = 0xFF; PORTC = 0x00; // PortC as output
    DDRD = 0xFF; PORTD = 0x00; // PortD as output

    // Start A2D conversion
    // A2D_init();

    // Setup Task List
    unsigned char i = 0;

    // SM1 (Draw SM)
    tasks[i].state = SM1_Start;
	  tasks[i].period = 1;
	  tasks[i].elapsedTime = tasks[i].period;
	  tasks[i].TickFct = &TickFct_DrawGame;
	  //i++;

    // Setup System Period & Timer to ON.
    TimerSet(tasksGCD);
    TimerOn();
    while (1) {}
    return 1;
}
