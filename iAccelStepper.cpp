#include "iAccelStepper.h"
#include "driverlib/timer.h"

volatile boolean state[3];
static iAccelStepper* me[3];
static unsigned int all_instances;
static unsigned long ulPeriod;

void iAccelStepper::ISR(void) {
  TimerIntClear(g_ulTIMERBase[id], TIMER_TIMA_TIMEOUT);
  digitalWrite(_pin[0], state[id]);
  if(state[id]) {
    if(_direction == DIRECTION_CW)
      // Clockwise
      ++_currentPos;
    else
      // Anticlockwise
      --_currentPos;
    // prepare for the next period
    computeNewSpeed();
    digitalWrite(_pin[1], _direction);
    TimerLoadSet(g_ulTIMERBase[id], TIMER_A, ulPeriod);
    TimerEnable(g_ulTIMERBase[id], TIMER_A);
  } else {
    // switch off timer at the falling edge
    if(_stepInterval == 0) {
      TimerDisable(g_ulTIMERBase[id], TIMER_A);
      running = false;
    } else {
      TimerLoadSet(g_ulTIMERBase[id], TIMER_A, _stepInterval - ulPeriod);
      TimerEnable(g_ulTIMERBase[id], TIMER_A);
    }
  }
  state[id] ^= 1;
}

void timerISR0(void) { me[0]->ISR(); }
void timerISR1(void) { me[1]->ISR(); }
void timerISR2(void) { me[2]->ISR(); }
//void timerISR3(void) { me[3]->ISR(); }
//void timerISR4(void) { me[4]->ISR(); }

typedef void (*ISR_ptr_t)(void);
//ISR_ptr_t timerISR_ptr[5] = { &timerISR0, &timerISR1, timerISR2, timerISR3, timerISR4 };
ISR_ptr_t timerISR_ptr[3] = { timerISR0, timerISR1, timerISR2 };

void iAccelStepper::begin(uint8_t pin1, uint8_t pin2, uint8_t pin3)
{
  //                                        STEP  DIR
  AccelStepper::begin(AccelStepper::DRIVER, pin1, pin2);
  //                         ENABLE
  AccelStepper::setEnablePin(pin3);
  AccelStepper::setPinsInverted(false, false, false, false, true);

  ulPeriod = 2 * clockCyclesPerMicrosecond();

  if(all_instances < 3) {
    id = all_instances;
    // Configure timer
    SysCtlPeripheralEnable(g_ulTIMERPeriph[id]);

    TimerConfigure(g_ulTIMERBase[id], TIMER_CFG_ONE_SHOT);
    TimerIntEnable(g_ulTIMERBase[id], TIMER_TIMA_TIMEOUT);
    TimerIntRegister(g_ulTIMERBase[id], TIMER_A, timerISR_ptr[id]);

    me[id] = this;
    state[id] = false;
    running = false;
    ++all_instances;
  }
}

void iAccelStepper::move(long relative)
{
  moveTo(_currentPos + relative);
}

void iAccelStepper::moveTo(long absolute)
{
  AccelStepper::moveTo(absolute);

  if(!running && (distanceToGo() != 0)) {
    running = true;
    // enable driver
//    enableOutputs();
    digitalWrite(_pin[1], _direction);
    computeNewSpeed();
    state[id] = true;
    digitalWrite(_pin[0], true);
    TimerLoadSet(g_ulTIMERBase[id], TIMER_A, ulPeriod);
    if(_direction == DIRECTION_CW)
      // Clockwise
      ++_currentPos;
    else
      // Anticlockwise
      --_currentPos;
    TimerEnable(g_ulTIMERBase[id], TIMER_A);
  }
}
