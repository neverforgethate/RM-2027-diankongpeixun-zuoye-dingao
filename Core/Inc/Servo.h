#ifndef __SERVO_H
#define __SERVO_H

#include <stdint.h>
#include "tim.h"

/* SG90 舵机：50Hz PWM，脉宽 500~2500us 对应 0~180 度
 *
 * 本工程把 TIM2 配成 1 个计数 = 1us（PSC = 71），
 * 所以写进 CCR 的数值就等于脉宽的微秒数，换算非常直观。
 */

#define SERVO_MIN_US   550U     /* 0 度对应的脉宽（留了余量，防顶限位） */
#define SERVO_MAX_US   2450U    /* 180 度对应的脉宽 */

void  Servo_Init(void);                 /* 启动 PWM 输出 */
void  Servo_SetAngle(float angle);      /* 设置目标角度，内部限幅 0~180 */
float Servo_GetAngle(void);             /* 读取当前记录的角度 */
uint16_t Servo_GetCCR(void);            /* 读取最近一次写入的 CCR */
uint16_t Servo_AngleToCCR(float angle); /* 角度 → CCR（作业 Step 4 的函数） */

#endif
