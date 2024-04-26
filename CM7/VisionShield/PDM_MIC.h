/*
 * PDM_MIC.h
 *
 *  Created on: Dec 28, 2022
 *      Author: tj
 */

#ifndef PDM_MIC_H_
#define PDM_MIC_H_

void PDM_MIC_Init(unsigned long gain, unsigned long freq, int gen_sine);
void PDM_MIC_Sample();

#endif /* PDM_MIC_H_ */
