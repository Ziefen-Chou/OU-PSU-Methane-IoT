/* Adafruit ZeroTimer Implementation for PWM    */
/* Adafruit ZeroTimer library must be installed */

#ifndef _BSP_PWM_H_
#define _BSP_PWM_H_

#include "Adafruit_ZeroTimer.h"

void PWMInit(void);
void PWMSet(uint8_t channel, uint8_t duty_cycle);
void PWMDeInit(void);

#endif

