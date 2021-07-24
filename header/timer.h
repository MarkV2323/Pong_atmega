// Must include
#include <avr/interrupt.h>
#define tasksSize 7
#define tasksGCD 1

// Important Stuff
// Task Structure
typedef struct Task {
    int state;
    unsigned long period;
    unsigned long elapsedTime;
    int (*TickFct) (int);
} Task;

// Globals for Task
// unsigned char tasksSize = 2;
Task tasks[tasksSize];



// TimerISR() sets this to 1. C programmer should clear to 0.
volatile unsigned char TimerFlag = 0;

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, to 0. Default 1ms
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1 ms ticks.

// TimerOn Function
void TimerOn() {
    // AVR timer/counter controller register TCCR1
    TCCR1B = 0x0B;
    // bit3 = 0: CTC mode (clear timer on compare)
    // bit2bit1bit0=011: pre-scaler /64
    // 00001011 : 0x0B
    // SO, 8 MHz clock or 8mill / 64 = 125,000 ticks / second
    // => TCNT1 register will count at 125,000 ticks / second

    // AVR output compare register OCR1A.
    OCR1A = 125;
    // Timer interrupt will be gen when TCNT1 == OCR1A
    // We want 1 ms tick, 0.001 s * 125,000 t/s = 125
    // When TCNT1 register equals 125, 1 ms has passed.

    // AVR timer interrupt mask register
    TIMSK1 = 0x02; // bit1: OCIE1A -- enable compare match interrupt

    // Init avr counter
    TCNT1 = 0;

    _avr_timer_cntcurr = _avr_timer_M;
    // TimerISR will be called every _avr_timer_cntcurr ms

    // enable global interrupts
    SREG |= 0x80; // 0x80: 1000 0000

}

// TimerOff Function
void TimerOff() {
    TCCR1B = 0x00; //bit3bit1bit0 0-000: timer off
}

// TimerISR Function
void TimerISR() {
    unsigned char i;
    for (i = 0; i < tasksSize; ++i) {
        if (tasks[i].elapsedTime >= tasks[i].period) { // If task is ready
            // execute the tick function and update state varible.
            tasks[i].state = tasks[i].TickFct(tasks[i].state);
            tasks[i].elapsedTime = 0; // Reset elapsed time
        }
        tasks[i].elapsedTime += tasksGCD; // Update elapsed time
    }
}

// C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
    // CPU auto calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
    _avr_timer_cntcurr--; // count down to 0 rather than up to TOP
    if (_avr_timer_cntcurr == 0) { // efficient compare
        TimerISR(); // Call the ISR that the user will use
        _avr_timer_cntcurr = _avr_timer_M;
    }

}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
    _avr_timer_M = M;
    _avr_timer_cntcurr = _avr_timer_M;
}
// END OF TIMER FUNCTIONS
