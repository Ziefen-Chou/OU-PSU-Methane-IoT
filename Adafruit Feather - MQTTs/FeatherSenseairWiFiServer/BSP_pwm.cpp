#include "BSP_pwm.h"

Adafruit_ZeroTimer zt4 = Adafruit_ZeroTimer(4);

// Enables PWM on A1 and A2
//    --- Input Params ---
//    None
//    --- Return Params ---
//    None
void PWMInit(void)
{
  // PWM Clock ~118kHz
  zt4.configure(TC_CLOCK_PRESCALER_DIV4, TC_COUNTER_SIZE_8BIT, TC_WAVE_GENERATION_NORMAL_PWM);
  zt4.setPeriodMatch(100U, 0U, 0U);
  zt4.setPeriodMatch(100U, 0U, 1U);
  zt4.PWMout(1U, 0U, A1);
  zt4.PWMout(1U, 1U, A2);
  zt4.enable(1U);

}

// Set PWM Duty Cycle
//    --- Input Params ---
//    uint8_t channel:      A1 or A2
//    uint8_t duty_cycle:   0% -> 100%
//    --- Return Params ---
//    None
void PWMSet(uint8_t channel, uint8_t duty_cycle)
{
  zt4.setPeriodMatch((uint32_t)100U, (uint32_t)duty_cycle, (uint8_t)(channel - 15U));
  
}

// Disables PWM on A1 and A2
//    --- Input Params ---
//    None
//    --- Return Params ---
//    None
void PWMDeInit(void)
{
  zt4.enable(0U);

}
