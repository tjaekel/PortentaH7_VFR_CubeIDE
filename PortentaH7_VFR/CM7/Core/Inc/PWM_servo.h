/*
 * PWM_servo.h
 *
 *  Created on: Jan 13, 2023
 *      Author: tj
 */

#ifndef INC_PWM_SERVO_H_
#define INC_PWM_SERVO_H_

#include "stm32h7xx_hal.h"
#include "main.h"
#include "stm32h7xx_hal_tim.h"

void PWM_ServoInit(void);
void PWM_ServoSet(int num, int degree);

#endif /* INC_PWM_SERVO_H_ */
