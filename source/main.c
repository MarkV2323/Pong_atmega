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
unsigned char tmpA = 0x00; // Gather input from A.
unsigned char tmpD = 0x00; // Write output to LEDs and speaker.

// Contains frames of the current board.
unsigned char frames_x[] = {0x70, 0x00, 0x00, 0x00, 0x0E};
unsigned char frames_y[] = {0x1E, 0x1D, 0x0B, 0x17, 0x0F}; // dnt modifiy
unsigned char frames_index = 0;

// SM for drawing game onto Matrix.
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
              frames_index = 0;
            } else {
              // draw and increment frame
              transmit_data(frames_y[frames_index], 2);
              transmit_data(frames_x[frames_index], 1);
              transmit_data(0x00, 1);
              frames_index = frames_index + 1;
            }
            break;

           default:
            break;
      }

      return state;
}

// SM for main menu on LEDs.
enum ButtonPress { buttonOff, buttonOn } buttonpress;
enum SM2_States { SM2_Start, SM2_Off, SM2_Game1, SM2_Game2, SM2_Game3
                } sm2_state;
int TickFct_MainMenu(int state) {

    // Local varibles
    static unsigned char idleTimer;
    static int buttonPressed;

    // Read Input from tmpA
    tmpA = ~PINA & 0xFE; // exculuding A0 (joystick)

      // Transitions
      switch (state) {
          case SM2_Start:
              // Init LEDs to flash
              buttonPressed = buttonOff;
              idleTimer = 0;
              tmpD = (tmpD | 0x77); // write LEDs to on, save contents of tmpD.
              PORTD = tmpD;
              // Go to Off state.
              state = SM2_Off;
              break;

          case SM2_Off:
              // transition to new state based on button input.
              // Only checks tmpA = 0000 - AAA0
              if (tmpA == 0x00) {
                // no input, set lights to Off
                tmpD = ((tmpD & 0x80) | 0x88);
                state = SM2_Off;
                buttonPressed = buttonOff;
              } else if (tmpA == 0x02) {
                // Button 1, A1 has been activated.
                tmpD = ((tmpD & 0x80) | 0x11);
                state = SM2_Game1;
                buttonPressed = buttonOn;
              } else if (tmpA == 0x04) {
                // Button 2, A2 has been activated.
                tmpD = ((tmpD & 0x80) | 0x33);
                state = SM2_Game2;
                buttonPressed = buttonOn;
              } else if (tmpA == 0x08) {
                // Button 3, A3 has been activated.
                tmpD = ((tmpD & 0x80) | 0x77);
                state = SM2_Game3;
                buttonPressed = buttonOn;
              } else {
                // no buttons activated, treat as non input.
                state = SM2_Off;
              }
              // Write LEDs reguardless of outcome.
              PORTD = tmpD;
              break;

          case SM2_Game1:
              // Only starts game if button was released, and now pressed.
              if (buttonPressed == buttonOff && tmpA == 0x02) {
                // Button 1, A1 has been activated. Start Game.
                tmpD = tmpD & 0x80;
                PORTD = tmpD;
              } else if (buttonPressed == buttonOn && tmpA == 0x00) {
                // Buttons has been released
                buttonPressed = buttonOff;
                state = SM2_Game1;
              } else if (buttonPressed == buttonOn && tmpA != 0x02) {
                // Button is on and is not Button1, must be another button.
                // go back..
                idleTimer = 0;
                buttonPressed = buttonOn;
                state = SM2_Off;
              } else if (buttonPressed == buttonOff && tmpA != 0x02 && tmpA != 0x00) {
                // Button is ff and is not Button1, must be another button.
                // go back..
                idleTimer = 0;
                buttonPressed = buttonOn;
                state = SM2_Off;
              } else {
                // All other input forms.
              }

              // Idle timer automatically sends back to off after 5 seconds.
              if (idleTimer == 50) {
                idleTimer = 0;
                state = SM2_Off;
              }

              // Increment Idle timer reguardless.
              idleTimer++;

              break;

          case SM2_Game2:
              // Only starts game if button was released, and now pressed.
              if (buttonPressed == buttonOff && tmpA == 0x04) {
                // Button 2, A2 has been activated. Start Game.
                tmpD = tmpD & 0x80;
                PORTD = tmpD;
              } else if (buttonPressed == buttonOn && tmpA == 0x00) {
                // Buttons has been released
                buttonPressed = buttonOff;
                state = SM2_Game2;
              } else if (buttonPressed == buttonOn && tmpA != 0x04) {
                // Button is on and is not Button2, must be another button.
                // go back..
                idleTimer = 0;
                buttonPressed = buttonOn;
                state = SM2_Off;
              } else if (buttonPressed == buttonOff && tmpA != 0x04 && tmpA != 0x00) {
                // Button is off and is not Button2, must be another button.
                // go back..
                idleTimer = 0;
                buttonPressed = buttonOn;
                state = SM2_Off;
              } else {
                // All other input forms.
              }

              // Idle timer automatically sends back to off after 5 seconds.
              if (idleTimer == 50) {
                idleTimer = 0;
                state = SM2_Off;
              }

              // Increment Idle timer reguardless.
              idleTimer++;
              break;

          case SM2_Game3:
              // Only starts game if button was released, and now pressed.
              if (buttonPressed == buttonOff && tmpA == 0x08) {
                // Button 2, A2 has been activated. Start Game.
                tmpD = tmpD & 0x80;
                PORTD = tmpD;
              } else if (buttonPressed == buttonOn && tmpA == 0x00) {
                // Buttons has been released
                buttonPressed = buttonOff;
                state = SM2_Game3;
              } else if (buttonPressed == buttonOn && tmpA != 0x08) {
                // Button is on and is not Button2, must be another button.
                // go back..
                idleTimer = 0;
                buttonPressed = buttonOn;
                state = SM2_Off;
              } else if (buttonPressed == buttonOff && tmpA != 0x08 && tmpA != 0x00) {
                // Button is off and is not Button2, must be another button.
                // go back..
                idleTimer = 0;
                buttonPressed = buttonOn;
                state = SM2_Off;
              } else {
                // All other input forms.
              }

              // Idle timer automatically sends back to off after 5 seconds.
              if (idleTimer == 50) {
                idleTimer = 0;
                state = SM2_Off;
              }

              // Increment Idle timer reguardless.
              idleTimer++;
              break;

          default:
              break;
      }

      //Actions
      switch (state) {
           case SM2_Start:
               break;

           case SM2_Off:
              // Resets timers.
              idleTimer = 0;
              break;

           case SM2_Game1:
               break;

           case SM2_Game2:
               break;

           case SM2_Game3:
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
    // DDRB = 0xFF; PORTB = 0x00; // PortB as output
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
	  i++;

    // SM2 (Main Menu LEDs)
    tasks[i].state = SM2_Start;
	  tasks[i].period = 100;
	  tasks[i].elapsedTime = tasks[i].period;
	  tasks[i].TickFct = &TickFct_MainMenu;

    // Setup System Period & Timer to ON.
    TimerSet(tasksGCD);
    TimerOn();
    while (1) {}
    return 1;
}
