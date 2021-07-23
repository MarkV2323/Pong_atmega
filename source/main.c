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

// Function for setting A2D Pin.
void Set_A2D_Pin(unsigned char pinNum) {
ADMUX = (pinNum <= 0x07) ? pinNum : ADMUX;
// Allow channel to stabilize
static unsigned char i = 0;
for ( i=0; i<15; i++ ) { asm("nop"); }
}

// globals
unsigned char tmpA = 0x00; // Gather input from A.
unsigned char tmpD = 0x00; // Write output to LEDs and speaker.

// menu
// unsigned char enableMenu = 0x01;

// game vars
unsigned char gameStart = 0x00; // 0 for off, 1 for on to start game.
unsigned char gameType = 0x00; // Game type to be spawned. (1, 2 or 3)
unsigned char gameOver = 0x00; // 1 for gameOver, 0 for game online.
unsigned char gameReset = 0x00;

// controller vars
unsigned char enableReset = 0x00; // 1 for on, allows reset back to menu.
unsigned char enableBall = 0x00; // 1 for on, ball can start playing.
unsigned char enableJoy = 0x00; // 1 for on, controlls player 1.
unsigned char enableBut = 0x00; // 1 for on, controlls player 2.
unsigned char enableAI  = 0x00; // 1 for on, controlls AI.

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
enum SM2_States { SM2_Start, SM2_Off, SM2_Game1, SM2_Game2, SM2_Game3,
                  SM2_GameOn} sm2_state;
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
                gameStart = 1;
                gameType = 1;
                state = SM2_GameOn;
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
                gameStart = 1;
                gameType = 2;
                state = SM2_GameOn;
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
                gameStart = 2;
                gameType = 1;
                state = SM2_GameOn;
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

          case SM2_GameOn:
              if (gameReset == 1) {
                // cleans up
                gameStart = 0;
                gameType = 0;
                buttonPressed = buttonOff;
                // Go back to SM2_Off
                state = SM2_Off;
              } else {
                state = SM2_GameOn;
              }
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
              // Keeps GameStart Off.
              gameStart = 0;
              break;

           case SM2_Game1:
               break;

           case SM2_Game2:
               break;

           case SM2_Game3:
               break;

           case SM2_GameOn:
               break;

           default:
              break;
      }

      return state;
}

// SM for pong game.
enum SM3_States { SM3_Start, SM3_Off, SM3_Play, SM3_GameOver, SM3_CleanUp
                } sm3_state;
int TickFct_Game(int state) {

    // Local varibles
    static unsigned char scoreTimer;

    // Transitions
    switch (state) {
      case SM3_Start:
          // Init varibles.
          scoreTimer = 0;
          // Goes to SM3_Off
          state = SM3_Off;
          break;

      case SM3_Off:
          // Decides settings for SM3_Play, and how to start SM3_Play
          if (gameStart == 1) {
            // Begin a new game.
            scoreTimer = 0;
            state = SM3_Play;
            // Disables main menu.
            // enableMenu = 0;
          } else {
            // Stay in non game phase.
            state = SM3_Off;
          }
          break;

      case SM3_Play:
          // When gameOver is 1, go into GameOver state.
          if (gameOver == 1) {
            state = SM3_GameOver;
          } else {
            state = SM3_Play;
          }

          // If reset is pressed, go str8 to cleanup.
          if (gameReset == 1) {
            state = SM3_CleanUp;
          }

          break;

      case SM3_GameOver:
          // After 10 seconds, move to SM3_CleanUp
          if (scoreTimer == 100) {
            state = SM3_CleanUp;
          } else {
            state = SM3_GameOver;
            scoreTimer++;
          }
          break;

      case SM3_CleanUp:
          // After cleanup is done, go back to SM3_Off
          state = SM3_Off;
          break;

      default:
          break;
    }

    // Actions
    switch (state) {
      case SM3_Start:
          // Do nothing.
          break;

      case SM3_Off:
          // Do nothing.
          break;

      case SM3_Play:
          // Allow reset button to be pressed.
          enableReset = 1;

          // Enable player controllers, enable ball SM.
          if (gameType == 1) {
            // Enable joystick and AI.
            enableJoy = 1;
            enableAI = 1;
            // Enable ball
            enableBall = 1;
          } else if (gameType == 2) {
            // Enable joystick and button.
            enableJoy = 1;
            enableBut = 1;
            // Enable ball
            enableBall = 1;
          } else if (gameType == 3) {
            // Enable both AIs
            // TODO
            // Enable ball
            enableBall = 1;
          }
          break;

      case SM3_GameOver:
          // Stop player controllers from working / being able to move.
          // Flash LED lights on winning side.
          // Start playing music from speaker.
          break;

      case SM3_CleanUp:
          // Disables all controllers.
          enableJoy = 0;
          enableBut = 0;
          enableBall = 0;
          enableAI = 0;
          // Reset scoreTimer back to 0.
          scoreTimer = 0;
          // Reset matrix back to defaults.
          frames_x[0] = 0x70;
          frames_x[1] = 0x00;
          frames_x[2] = 0x00;
          frames_x[3] = 0x00;
          frames_x[4] = 0x0E;
          // Re enables Menu
          enableReset = 0;

          break;

      default:
          break;
    }

    return state;
}

// SM for joystick player controller.
enum SM4_States { SM4_Start, SM4_Off, SM4_On } sm4_state;
int TickFct_JoyController(int state) {
    // Local vars
    static int joystickInput;

    // Transitions
    switch (state) {
      case SM4_Start:
          // init varibles
          joystickInput = 512;
          // go to SM4_Off
          state = SM4_Off;
          break;

      case SM4_Off:
          // Turns on the player controller when activated
          if (enableJoy == 1) {
            // start player controller.
            state = SM4_On;
          } else {
            state = SM4_Off;
          }
          break;

      case SM4_On:
          // Continues to gather input unless turned off.
          if (enableJoy == 1) {
            state = SM4_On;
          } else {
            state = SM4_Off;
          }
          break;

      default:
          break;
    }

    // Actions
    switch (state) {
      case SM4_Start:
          break;

      case SM4_Off:
          break;

      case SM4_On:
          // Gather input from joystick.
          joystickInput = ADC;

          unsigned char playerRight = frames_x[0];
          unsigned char playerLeft = frames_x[0];
          playerRight = playerRight >> 1;
          playerLeft = playerLeft << 1;

          // Checks type of input.
          if (joystickInput < 200) {
            // Left Move
            // Checks if out of bounds (doesn't move)
            if ( playerLeft == 0xC0 ) {
              // No Move.
            } else {
              // move left
              frames_x[0] = playerLeft;
            }

          } else if (joystickInput > 800) {
            // Right Move
            // Checks if out of bounds (doesn't move)
            if ( playerRight == 0x03 ) {
              // No Move.
            } else {
              // move left
              frames_x[0] = playerRight;
            }

          } else {
            // Joystick not tilted enough to warent move.
          }
          break;

      default:
          break;
    }
    return state;
}

// SM for button player controller.
enum SM5_States { SM5_Start } sm5_state;
int TickFct_ButtonController(int state) {
    // Transitions
    switch (state) {
      case SM5_Start:
          break;
      default:
          break;
    }
    // Actions
    switch (state) {
      case SM5_Start:
          break;
      default:
          break;
    }
    return state;
}

// SM for AI (dumb) controller.
enum SM6_States { SM6_Start } sm6_state;
int TickFct_AIController(int state) {
    // Transitions
    switch (state) {
      case SM6_Start:
          break;
      default:
          break;
    }
    // Actions
    switch (state) {
      case SM6_Start:
          break;
      default:
          break;
    }
    return state;
}

// SM for speaker.
enum SM7_States { SM7_Start } sm7_state;
int TickFct_Speaker(int state) {
    // Transitions
    switch (state) {
      case SM7_Start:
          break;
      default:
          break;
    }
    // Actions
    switch (state) {
      case SM7_Start:
          break;
      default:
          break;
    }
    return state;
}

// SM for winning LED flashing
enum SM8_States { SM8_Start } sm8_state;
int TickFct_Celebrate(int state) {
    // Transitions
    switch (state) {
      case SM8_Start:
          break;
      default:
          break;
    }
    // Actions
    switch (state) {
      case SM8_Start:
          break;
      default:
          break;
    }
    return state;
}

// SM for ball object
enum SM9_States { SM9_Start } sm9_state;
int TickFct_Ball(int state) {
    // Transitions
    switch (state) {
      case SM9_Start:
          break;
      default:
          break;
    }
    // Actions
    switch (state) {
      case SM9_Start:
          break;
      default:
          break;
    }
    return state;
}

enum SM10_States { SM10_Start, SM10_rOff, SM10_On, SM10_Cleanup } sm10_state;
int TickFct_ResetButton(int state) {
    static unsigned int cleanupCount = 0;
    // Transitions
    switch (state) {
      case SM10_Start:
          // init & go to SM10_rOff
          state = SM10_rOff;
          break;
      case SM10_rOff:
          // If enabled, moves to on.
          if (enableReset == 1) {
            state = SM10_On;
          } else {
            state = SM10_rOff;
          }
          break;
      case SM10_On:
          if (enableReset == 0 && gameReset == 0) {
            state = SM10_rOff;
          } else if (gameReset == 1) {
            state = SM10_Cleanup;
          } else {
            state = SM10_On;
          }
          break;
      case SM10_Cleanup:
          state = SM10_rOff;
          break;
      default:
          break;
    }

    // Actions
    switch (state) {
      case SM10_Start:
          break;
      case SM10_rOff:
          break;
      case SM10_On:
          // Check if button has been pressed.
          tmpA = ~PINA & 0xFE; // exculuding A0 (joystick)
          if (tmpA == 0x08) { // A3 online
            // reset current game, go back to menu.
            gameReset = 1;
          } else {
            // nothing
          }
          break;
      case SM10_Cleanup:
          gameReset = 0;
          cleanupCount = 0;
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
    A2D_init();

    // Set A2D on A0
    Set_A2D_Pin(0x00);

    // Setup Task List
    unsigned char i = 0;

    // SM1 (Draw SM)
    tasks[i].state = SM1_Start;
	  tasks[i].period = 1;
	  tasks[i].elapsedTime = tasks[i].period;
	  tasks[i].TickFct = &TickFct_DrawGame;
	  i++;

    // SM2 (Main Menu LEDs SM)
    tasks[i].state = SM2_Start;
	  tasks[i].period = 100;
	  tasks[i].elapsedTime = tasks[i].period;
	  tasks[i].TickFct = &TickFct_MainMenu;
	  i++;

    // SM3 (Game SM)
    tasks[i].state = SM3_Start;
	  tasks[i].period = 100;
	  tasks[i].elapsedTime = tasks[i].period;
	  tasks[i].TickFct = &TickFct_Game;
	  i++;

    // SM4 (Joystick SM)
    tasks[i].state = SM4_Start;
	  tasks[i].period = 500;
	  tasks[i].elapsedTime = tasks[i].period;
	  tasks[i].TickFct = &TickFct_JoyController;
    i++;

    // SM10 (Reset Button SM)
    tasks[i].state = SM10_Start;
	  tasks[i].period = 100;
	  tasks[i].elapsedTime = tasks[i].period;
	  tasks[i].TickFct = &TickFct_ResetButton;

    // Setup System Period & Timer to ON.
    TimerSet(tasksGCD);
    TimerOn();
    while (1) {}
    return 1;
}
