#include "Servo.h"

/* ============================================================
 * SG90 舵机驱动（TIM2_CH1，PA0）
 *
 * 定时器配置（CubeMX 里填的值）：
 *   TIM2 挂在 APB1 上，APB1 = 36MHz，
 *   但 APB1 分频系数不为 1 时，定时器时钟会被硬件自动 ×2 → 72MHz
 *
 *   目标：PWM 频率 50Hz（周期 20ms = 20000us），1 个计数 = 1us
 *   PSC = 72MHz ÷ 1MHz − 1          = 71
 *   ARR = 20000us ÷ 1us − 1         = 19999
 *   验算：72MHz ÷ (71+1) ÷ (19999+1) = 72e6 ÷ 72 ÷ 20000 = 50Hz ✅
 *
 * 因为 1 计数 = 1us，CCR 的数值就等于脉宽微秒数：
 *   0°   → 500us  → CCR = 500
 *   180° → 2500us → CCR = 2500
 * 实际取 550~2450（两端各留约 4.5°余量），防止顶到机械限位堵转发热。
 * ============================================================ */

static float    s_angle = 90.0f;    /* 上电默认停在中间，避免一上电就甩到底 */
static uint16_t s_ccr   = 1500U;

/* 角度 → CCR
 * 名义映射：0° = 500us，180° = 2500us
 * 两端各留余量：最终限制在 SERVO_MIN_US ~ SERVO_MAX_US
 */
uint16_t Servo_AngleToCCR(float angle)
{
    float us;

    if (angle < 0.0f)
    {
        angle = 0.0f;
    }
    if (angle > 180.0f)
    {
        angle = 180.0f;
    }

    us = 500.0f + angle / 180.0f * 2000.0f;   /* 500us ~ 2500us */

    if (us < (float)SERVO_MIN_US)
    {
        us = (float)SERVO_MIN_US;
    }
    if (us > (float)SERVO_MAX_US)
    {
        us = (float)SERVO_MAX_US;
    }

    return (uint16_t)(us + 0.5f);             /* 四舍五入 */
}

void Servo_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    Servo_SetAngle(90.0f);                    /* 先回中位 */
}

void Servo_SetAngle(float angle)
{
    if (angle < 0.0f)
    {
        angle = 0.0f;
    }
    if (angle > 180.0f)
    {
        angle = 180.0f;
    }

    s_angle = angle;
    s_ccr   = Servo_AngleToCCR(angle);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, s_ccr);
}

float Servo_GetAngle(void)
{
    return s_angle;
}

uint16_t Servo_GetCCR(void)
{
    return s_ccr;
}
