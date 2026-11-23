/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Servo.h"
#include "OLED.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* 进阶 A：两个按键，每次 ±15°，限幅 0~180 */
#define KEY_STEP_DEG     15.0f
#define KEY_DEBOUNCE_MS  20U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* 定角序列：0° → 45° → 90° → 135° → 180° */
static const float s_seq[5] = {0.0f, 45.0f, 90.0f, 135.0f, 180.0f};

static uint8_t  s_seq_idx = 0;      /* 序列走到第几个点 */
static uint32_t s_seq_ms  = 0;      /* 上一次切点的时间 */
static uint8_t  s_manual  = 0;      /* 1 = 已进入按键手动模式，序列停止 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ---------- 进阶 A：PA1(+15°) / PA2(-15°) 按键 ----------
 * 这两个脚没在 CubeMX 里配，直接在这里初始化成"输入 + 内部上拉"：
 * 松开为高电平，按下被短到 GND 变成低电平。
 * 主循环每 20ms 采样一次，检测"高→低"的跳变，天然滤掉了抖动。
 */
static uint8_t  s_key_prev[2]  = {1, 1};
static uint32_t s_key_last[2]  = {0, 0};

static void Keys_Init(void)
{
  GPIO_InitTypeDef g = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  g.Pin   = GPIO_PIN_1 | GPIO_PIN_2;
  g.Mode  = GPIO_MODE_INPUT;
  g.Pull  = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &g);
}

/* 返回 +1（加角度）、-1（减角度）或 0（没按） */
static int8_t Keys_Poll(void)
{
  static uint32_t s_last_sample = 0;
  uint32_t now = HAL_GetTick();

  if ((now - s_last_sample) < KEY_DEBOUNCE_MS)
  {
    return 0;                       /* 20ms 采样周期 */
  }
  s_last_sample = now;

  {
    const uint16_t pins[2] = {GPIO_PIN_1, GPIO_PIN_2};
    const int8_t   dirs[2] = {+1, -1};     /* PA1 加，PA2 减 */
    int8_t ret = 0;

    for (uint8_t i = 0; i < 2; i++)
    {
      uint8_t now_low = (HAL_GPIO_ReadPin(GPIOA, pins[i]) == GPIO_PIN_RESET) ? 1 : 0;

      if (now_low && !s_key_prev[i])       /* 高 → 低：按下 */
      {
        ret = dirs[i];
      }
      s_key_prev[i] = now_low;
    }
    return ret;
  }
}

/* 把当前状态刷到 OLED 三行 + 串口（作业 Step 6/7 规定的格式） */
static void Show_State(void)
{
  float    target = Servo_GetAngle();
  uint16_t ccr    = Servo_GetCCR();

  OLED_ShowString(1, 1,  "Target:");
  OLED_ShowNum(1, 9, (uint32_t)target, 3);
  OLED_ShowString(1, 12, "deg");

  OLED_ShowString(2, 1,  "CCR   :");
  OLED_ShowNum(2, 9, ccr, 4);

  OLED_ShowString(3, 1,  "Pulse :");
  OLED_ShowNum(3, 9, ccr / 1000, 1);
  OLED_ShowString(3, 10, ".");
  OLED_ShowNum(3, 11, ccr % 1000, 3);
  OLED_ShowString(3, 14, "ms");

  printf("Target=%3d  CCR=%4d  Pulse=%d.%03dms\r\n",
         (int)target, ccr, ccr / 1000, ccr % 1000);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  Servo_Init();          /* 启动 TIM2_CH1 的 PWM，并回到 90° 中位 */

  printf("\r\n===== SG90 Servo Demo =====\r\n");
  printf("PWM 50Hz  PSC=71  ARR=19999  1 tick = 1us\r\n");

  Keys_Init();           /* 进阶 A：PA1 加 15°，PA2 减 15° */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    int8_t step = Keys_Poll();

    if (step != 0)
    {
      /* ---- 进阶 A：手动模式，按键改角度，立即生效 ---- */
      s_manual = 1;
      Servo_SetAngle(Servo_GetAngle() + (float)step * KEY_STEP_DEG);
      Show_State();
    }
    else if (!s_manual && (HAL_GetTick() - s_seq_ms) >= 1000U)
    {
      /* ---- 必做：定角序列，每点停 1 秒 ---- */
      s_seq_ms = HAL_GetTick();
      Servo_SetAngle(s_seq[s_seq_idx]);
      s_seq_idx = (uint8_t)((s_seq_idx + 1) % 5);
      Show_State();
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* printf 重定向：GCC（CLion）用 _write，不是 Keil 的 __io_putchar */
int _write(int fd, char *ptr, int len)
{
  (void)fd;
  HAL_UART_Transmit(&huart1, (uint8_t *)ptr, (uint16_t)len, 100);
  return len;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
