/*	Author: Mark Alan Vincent II
 *  Partner(s) Name: NA
 *	Lab Section: A01
 *	Assignment: Final Project
 *	Youtube:
 *  Exercise Description:
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <time.h>
#include <stdlib.h>
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
unsigned char tmpMove;     // tracks moves made by player 1.
unsigned char p1Score;     // Contains p1 Score.
unsigned char p2Score;     // Contains p2 Score.

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
// Do not modifiy frames_y[], SM1, DrawGame() depends on frames_y[].
// Only modifiy if also modifiying SM1.
unsigned char frames_x[] = {0x70, 0x20, 0x00, 0x00, 0x0E};
unsigned char frames_y[] = {0x1E, 0x1D, 0x1B, 0x17, 0x0F};
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
                tmpD = (tmpD & 0x80);
                PORTD = tmpD;
                // Clears LEDs
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
                tmpD = (tmpD & 0x80);
                PORTD = tmpD;
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
                tmpD = (tmpD & 0x80);
                PORTD = tmpD;
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
              } else if (gameOver == 1) {
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
          // After 5 seconds, move to SM3_CleanUp
          if (scoreTimer == 50) {
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
          // Disables all controllers.
          enableJoy = 0;
          enableBut = 0;
          enableBall = 0;
          enableAI = 0;

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
          frames_x[1] = 0x20;
          frames_x[2] = 0x00;
          frames_x[3] = 0x00;
          frames_x[4] = 0x0E;

          // Re enables Menu
          enableReset = 0;

          // resets GameOver
          gameOver = 0;

          // Clear LEDs
          tmpD = (tmpD & 0x80);
          PORTD = tmpD;

          break;

      default:
          break;
    }

    return state;
}

// SM for joystick player controller.
enum playerMoved { moveRight, moveLeft, moveNull} sm4_movement;
enum SM4_States { SM4_Start, SM4_Off, SM4_On } sm4_state;
int TickFct_JoyController(int state) {
    // Local vars
    static int joystickInput;

    // Transitions
    switch (state) {
      case SM4_Start:
          // init varibles
          joystickInput = 512;
          tmpMove = moveNull;
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
              tmpMove = moveNull;
            } else {
              // move left
              frames_x[0] = playerLeft;
              tmpMove = moveLeft;
            }

          } else if (joystickInput > 800) {
            // Right Move
            // Checks if out of bounds (doesn't move)
            if ( playerRight == 0x03 ) {
              // No Move.
              tmpMove = moveNull;
            } else {
              // move left
              frames_x[0] = playerRight;
              tmpMove = moveRight;
            }

          } else {
            // Joystick not tilted enough to warent move.
            tmpMove = moveNull;
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
enum SM6_States { SM6_Start, SM6_Off, SM6_On } sm6_state;
int TickFct_AIController(int state) {
    // Local vars
    static unsigned int randomNum;

    // Transitions
    switch (state) {
      case SM6_Start:
          randomNum = 0;
          state = SM6_Off;
          break;
      case SM6_Off:
          // Turns on the AI (dumb) controller when activated
          if (enableAI == 1) {
            // start player controller.
            state = SM6_On;
          } else {
            state = SM6_Off;
          }
          break;
      case SM6_On:
          if (enableAI == 1) {
            // start player controller.
            state = SM6_On;
          } else {
            state = SM6_Off;
          }
          break;
      default:
          break;
    }

    // Actions
    switch (state) {
      case SM6_Start:
          break;

      case SM6_Off:
          break;

      case SM6_On:
          // Picks a move (gens a randomNum between 0 and 9)
          randomNum = rand() % 10;

          // Calculates possible moves
          unsigned char aiRight = frames_x[4];
          unsigned char aiLeft = frames_x[4];
          aiRight = aiRight >> 1;
          aiLeft = aiLeft << 1;

          // Checks what move to make.
          if (randomNum < 4) {
            // Don't move

          } else if (randomNum < 7) {

            // try move left.
            if ( aiLeft == 0xC0 ) {
              // No Move.
            } else {
              // move left
              frames_x[4] = aiLeft;
            }

          } else if (randomNum < 10) {

            // try move right.
            if ( aiRight == 0x03 ) {
              // No Move.
            } else {
              // move left
              frames_x[4] = aiRight;
            }

          } else {

          }
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
enum SM9_Directions { north, south, northRight, northLeft, southRight,
                      southLeft } sm9_directions;
enum SM9_ContactType { centerT, rightT, leftT, noneT } sm9_contactType;
enum SM9_States { SM9_Start, SM9_Spawn, SM9_Move, SM9_Goal } sm9_state;
int TickFct_Ball(int state) {
    // Local vars
    static unsigned char spawnTimer;
    static unsigned char goalScored;
    static unsigned char contactType;

    static unsigned char tmpX;            // Used for current X position.
    static unsigned char tmpDirection;    // Used for current direction.
    static unsigned char tmpYlevel;       // Used for current Y position.
    static unsigned char tmpMask;         // Used for finding contact.

    // Used in calculating possible move
    static unsigned char possibleX;
    static unsigned char possibleY;


    // Transitions
    switch (state) {

      case SM9_Start:
          // init vars
          spawnTimer = 0;
          goalScored = 0;
          tmpDirection = south;
          contactType = noneT;
          tmpYlevel = 1;
          tmpX = 0x20; // Row 2, init spawn.
          // move to Spawn
          if (enableBall == 1) {
            // spawn the ball.
            tmpX = 0x20; // Row 2, init spawn.
            tmpYlevel = 1;
            frames_x[tmpYlevel] = tmpX;
            // Reset players.
            frames_x[0] = 0x70;
            frames_x[4] = 0x0E;
            state = SM9_Spawn;
          }
          break;

      case SM9_Spawn:
          // check if ball is still alive.
          if (enableBall == 0) {
            // reset spawnTimer
            spawnTimer = 0;
            state = SM9_Start;
          }
          // wait 3 second following Joystick player b4 calculating a move.
          if (spawnTimer >= 6) {
            // move
            state = SM9_Move;
          } else if (spawnTimer < 6) {
            // move with joystick player.
            if (tmpMove == moveNull) {
              // no movement tracked, stay in position.
            } else if (tmpMove == moveRight) {
              // right movement tracked, move right 1 bit.
              // gets current positionof ball.
              tmpX = frames_x[1];
              // moves 1 bit to right.
              tmpX = tmpX >> 1;
              // write new tmpX
              frames_x[1] = tmpX;
            } else if (tmpMove == moveLeft) {
              // left movement tracked, move left 1 bit.
              // gets current positionof ball.
              tmpX = frames_x[1];
              // moves 1 bit to left.
              tmpX = tmpX << 1;
              // write new tmpX
              frames_x[1] = tmpX;
            }

            // Increment spawnTimer.
            spawnTimer++;
          }

          break;

      case SM9_Move:
          // check if ball is still alive.
          if (enableBall == 0) {
            // reset spawnTimer
            spawnTimer = 0;
            state = SM9_Start;
          } else {
            // continue Moving
            state = SM9_Move;
          }

          // check if goal has been scored.
          if (goalScored == 1) {
            // Player 1 scored, add point, respawn ball, execute goalScore SMs.
            p1Score++;
            // Write LED score.
            switch (p1Score) {
              case 0:
                tmpD = (tmpD | 0x00);
                PORTD = tmpD;
                break;
              case 1:
                tmpD = (tmpD | 0x01);
                PORTD = tmpD;
                break;
              case 2:
                tmpD = (tmpD | 0x02);
                PORTD = tmpD;
                break;
              case 3:
                tmpD = (tmpD | 0x04);
                PORTD = tmpD;
                break;
              default:
                break;
            }
            // Check for game over.
            if (p1Score == 3) {
              // Game is over!
              gameOver = 1;
              p1Score = 0;
              p2Score = 0;
            }
            // Go back to start.
            state = SM9_Start;

          } else if (goalScored == 2) {
            // Player 2 scored, add point, respawn ball, execute goalScore SMs.
            p2Score++;
            // Write LED score.
            // Write LED score.
            switch (p2Score) {
              case 0:
                tmpD = (tmpD | 0x00);
                PORTD = tmpD;
                break;
              case 1:
                tmpD = (tmpD | 0x10);
                PORTD = tmpD;
                break;
              case 2:
                tmpD = (tmpD | 0x20);
                PORTD = tmpD;
                break;
              case 3:
                tmpD = (tmpD | 0x40);
                PORTD = tmpD;
                break;
              default:
                break;
            }
            // Check for game over.
            if (p2Score == 3) {
              // Game is over!
              gameOver = 1;
              p1Score = 0;
              p2Score = 0;
            }

            // Go back to start.
            state = SM9_Start;
          }
          break;

      case SM9_Goal:
          break;

      default:
          break;
    }

    // Actions
    switch (state) {
      case SM9_Start:
          break;

      case SM9_Spawn:
          break;

      case SM9_Move:
          // Based on the current direction, calculate the possible move.
          tmpX = frames_x[tmpYlevel]; // Get current x pos of ball.
          possibleX = tmpX;           // init possibleX
          possibleY = tmpYlevel;      // init possibleY

          // Calculates direction to move.
          if ((possibleY - 1 == 0) && (tmpDirection == north) ) {
              // Check if contact was made.
              tmpMask = (frames_x[0] & frames_x[1]);
              if (tmpMask == frames_x[1]) {
                // Contact was made with top player paddle.
                // Direction is now south.
                tmpDirection = south;
              } else {
                // No Contact was made, must be a goal.
                // Goal on top player.
                goalScored = 1;
              }

          } else if ((possibleY + 1 == 4) && (tmpDirection == south)) {
              // Check if contact was made.
              tmpMask = (frames_x[3] & frames_x[4]);
              if (tmpMask == frames_x[3]) {
                // Contact was made with top player paddle.
                // Direction is now north.
                tmpDirection = north;
              } else {
                // No Contact was made, must be a goal.
                // Goal on bottom player.
                goalScored = 2;
              }

          } else {
              // continue moving in direction.
              tmpMask = 0x00;
              goalScored = 0;
          }

          // Calculates possible move.
          switch (tmpDirection) {

            case north:
                // Move north
                possibleY--;  // decrement Y val.
                break;

            case south:
                // move south
                possibleY++;  // increment Y val.
                break;

            case northRight:
                // Move north right
                possibleY--;                // decrement Y val.
                possibleX = possibleX >> 1; // shift right by 1.
                break;

            case northLeft:
                // move north left
                possibleY--;                // decrement Y val.
                possibleX = possibleX << 1; // shift left by 1.
                break;

            case southRight:
                // move south right
                possibleY++;                // increment Y val.
                possibleX = possibleX >> 1; // shift right by 1.
                break;

            case southLeft:
                // move south left
                possibleY++;                // increment Y val.
                possibleX = possibleX << 1; // shift left by 1.
                break;

            default:
                break;
          }

          // Writes new location to frames.
          if (goalScored == 0) {
              frames_x[tmpYlevel] = 0x00;         // Clears last position.
              tmpX = possibleX;                   // Sets new current X
              tmpYlevel = possibleY;              // Sets new current Y
              frames_x[tmpYlevel] = tmpX;         // Writes new location.
          } else if (goalScored == 1) {
              // Goal has been scored on p1.
              frames_x[tmpYlevel] = 0x00;         // Clears last position.
              tmpX = possibleX;                   // Sets new current X
              tmpYlevel = possibleY;              // Sets new current Y
              // Writes new location with ball and player.
              frames_x[0] = (frames_x[0] | tmpX);

          } else if (goalScored == 2) {
              // Goal has been scored on p2.
              frames_x[tmpYlevel] = 0x00;         // Clears last position.
              tmpX = possibleX;                   // Sets new current X
              tmpYlevel = possibleY;              // Sets new current Y
              // Writes new location with ball and player.
              frames_x[4] = (frames_x[4] | tmpX);
          }

          // End of calculating move
          break;

      case SM9_Goal:
          break;

      default:
          break;
    }
    return state;
}

enum SM10_States { SM10_Start, SM10_rOff, SM10_On, SM10_Cleanup } sm10_state;
int TickFct_ResetButton(int state) {
    // static unsigned int cleanupCount = 0;
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
          // cleanupCount = 0;
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

    // SM10 (Reset Button SM)
    tasks[i].state = SM10_Start;
	  tasks[i].period = 100;
	  tasks[i].elapsedTime = tasks[i].period;
	  tasks[i].TickFct = &TickFct_ResetButton;
    i++;

    // SM4 (Joystick SM)
    tasks[i].state = SM4_Start;
	  tasks[i].period = 500;
	  tasks[i].elapsedTime = tasks[i].period;
	  tasks[i].TickFct = &TickFct_JoyController;
    i++;

    // SM6 (Dumb AI SM)
    tasks[i].state = SM6_Start;
	  tasks[i].period = 500;
	  tasks[i].elapsedTime = tasks[i].period;
	  tasks[i].TickFct = &TickFct_AIController;
    i++;

    // 9SM (Ball SM)
    tasks[i].state = SM9_Start;
    tasks[i].period = 500;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_Ball;

    // Setup System Period & Timer to ON.
    TimerSet(tasksGCD);
    TimerOn();
    while (1) {}
    return 1;
}
