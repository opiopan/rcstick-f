/*
 * project.h
 *
 *  Created on: 2019/01/17
 *      Author: opiopan
 */

#ifndef PROJECT_H_
#define PROJECT_H_

#include <stdint.h>
#include <stm32f0xx.h>
#include <stm32f0xx_hal.h>

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef TRUE
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#endif

#endif /* PROJECT_H_ */
