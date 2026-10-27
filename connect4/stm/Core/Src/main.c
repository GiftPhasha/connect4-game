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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    STATE_IDLE,
    STATE_IN_PROGRESS,
    STATE_GAME_OVER
} GameState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RX_BUFFER_SIZE 64
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
GameState current_state = STATE_IDLE;
char board[6][7];
char current_player = 'X';
char winning_player = ' ';

uint8_t rx_byte;
char rx_buffer[RX_BUFFER_SIZE];
uint8_t rx_index = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void ResetBoard(void);
void GetBoardString(char *dest);
int CheckWin(char p);
int CheckDraw(void);
void SendResponse(const char *resp);
void HandleCommand(char *cmd);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void ResetBoard(void) {
    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 7; c++) {
            board[r][c] = '.';
        }
    }
    current_player = 'X';
    winning_player = ' ';
}

void GetBoardString(char *dest) {
    int idx = 0;
    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 7; c++) {
            dest[idx++] = board[r][c];
        }
    }
    dest[42] = '\0';
}

int CheckWin(char p) {
    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 7; c++) {
            if (board[r][c] == p) {
                if (c + 3 < 7 && board[r][c+1] == p && board[r][c+2] == p && board[r][c+3] == p) return 1;
                if (r + 3 < 6 && board[r+1][c] == p && board[r+2][c] == p && board[r+3][c] == p) return 1;
                if (r + 3 < 6 && c + 3 < 7 && board[r+1][c+1] == p && board[r+2][c+2] == p && board[r+3][c+3] == p) return 1;
                if (r + 3 < 6 && c - 3 >= 0 && board[r+1][c-1] == p && board[r+2][c-2] == p && board[r+3][c-3] == p) return 1;
            }
        }
    }
    return 0;
}

int CheckDraw(void) {
    for (int c = 0; c < 7; c++) {
        if (board[0][c] == '.') return 0;
    }
    return 1;
}

void SendResponse(const char *resp) {
    char buffer[64];
    int len = snprintf(buffer, sizeof(buffer), "%s\r\n", resp);
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, (size_t)len, HAL_MAX_DELAY);
}

void HandleCommand(char *cmd) {
    if (strncmp(cmd, "CONNECT", (size_t)7) == 0) {
        current_state = STATE_IDLE;
        SendResponse("OK");
    }
    else if (strncmp(cmd, "NEW", (size_t)3) == 0) {
        ResetBoard();
        current_state = STATE_IN_PROGRESS;
        char bstr[43];
        GetBoardString(bstr);
        char resp[64];
        snprintf(resp, sizeof(resp), "OK: BOARD: %s", bstr);
        SendResponse(resp);
    }
    else if (strncmp(cmd, "STATE", (size_t)5) == 0) {
        if (current_state == STATE_IDLE) {
            SendResponse("NACK: NOGAME");
            return;
        }
        char bstr[43];
        GetBoardString(bstr);
        char resp[64];
        snprintf(resp, sizeof(resp), "BOARD: %s", bstr);
        SendResponse(resp);
    }
    else if (strncmp(cmd, "MOVE:", (size_t)5) == 0) {
        if (current_state != STATE_IN_PROGRESS) {
            SendResponse("NACK: NOGAME");
            return;
        }
        int col = cmd[5] - '0';
        if (col < 0 || col > 6) {
            SendResponse("NACK: RANGE");
            return;
        }

        // Check if top cell of column is already taken (Column full / occupied)
        if (board[0][col] != '.') {
            SendResponse("NACK: OCCUPIED");
            return;
        }

        int row = -1;
        for (int r = 5; r >= 0; r--) {
            if (board[r][col] == '.') {
                row = r;
                break;
            }
        }

        board[row][col] = current_player;

        if (CheckWin(current_player)) {
            winning_player = current_player;
            char winmsg[16];
            snprintf(winmsg, sizeof(winmsg), "WIN:%c", current_player);
            current_state = STATE_GAME_OVER;
            SendResponse(winmsg);
        } else if (CheckDraw()) {
            current_state = STATE_GAME_OVER;
            SendResponse("DRAW");
        } else {
            current_player = (current_player == 'X') ? 'O' : 'X';
            char bstr[43];
            GetBoardString(bstr);
            char resp[64];
            snprintf(resp, sizeof(resp), "OK: BOARD: %s", bstr);
            SendResponse(resp);
        }
    }
    else if (strncmp(cmd, "RESIGN", (size_t)6) == 0) {
        if (current_state == STATE_IN_PROGRESS) {
            current_state = STATE_GAME_OVER;
            SendResponse("OK: GAMEOVER");
        } else {
            SendResponse("NACK: NOGAME");
        }
    }
    else if (strncmp(cmd, "DISCONNECT", (size_t)10) == 0) {
        current_state = STATE_IDLE;
        SendResponse("OK");
    }
    else {
        SendResponse("NACK: BADCMD");
    }
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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  ResetBoard();
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  static uint32_t last_blink_tick = 0;
  static GPIO_PinState led_state = GPIO_PIN_RESET;

  while (1)
  {
    uint32_t current_tick = HAL_GetTick();

    // LED Management according to your specifications
    if (current_state == STATE_IDLE) {
        // PB0 ON while connected/idle, PB1 and PB2 OFF
        HAL_GPIO_WritePin(GPIOB, LED0_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, LED1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
    }
    else if (current_state == STATE_IN_PROGRESS) {
        // PB0 remains ON during active game
        HAL_GPIO_WritePin(GPIOB, LED0_Pin, GPIO_PIN_SET);

        // PB1 ON when X is playing, PB2 ON when O is playing
        if (current_player == 'X') {
            HAL_GPIO_WritePin(GPIOB, LED1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
        } else {
            HAL_GPIO_WritePin(GPIOB, LED1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET);
        }
    }
    else if (current_state == STATE_GAME_OVER) {
        // PB0 stays ON (connected)
        HAL_GPIO_WritePin(GPIOB, LED0_Pin, GPIO_PIN_SET);

        // PB1 blinks if X won, PB2 blinks if O won
        if (current_tick - last_blink_tick >= 250) {
            last_blink_tick = current_tick;
            led_state = (led_state == GPIO_PIN_RESET) ? GPIO_PIN_SET : GPIO_PIN_RESET;

            if (winning_player == 'X') {
                HAL_GPIO_WritePin(GPIOB, LED1_Pin, led_state);
                HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
            } else if (winning_player == 'O') {
                HAL_GPIO_WritePin(GPIOB, LED2_Pin, led_state);
                HAL_GPIO_WritePin(GPIOB, LED1_Pin, GPIO_PIN_RESET);
            } else {
                // Draw or Resign
                HAL_GPIO_WritePin(GPIOB, LED1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
            }
        }
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED0_Pin|LED1_Pin|LED2_Pin|LED3_Pin
                          |LED4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BTN0_Pin BTN1_Pin BTN2_Pin BTN3_Pin */
  GPIO_InitStruct.Pin = BTN0_Pin|BTN1_Pin|BTN2_Pin|BTN3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED0_Pin LED1_Pin LED2_Pin LED3_Pin
                           LED4_Pin */
  GPIO_InitStruct.Pin = LED0_Pin|LED1_Pin|LED2_Pin|LED3_Pin
                          |LED4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        if (rx_byte == '\n' || rx_byte == '\r') {
            if (rx_index > 0) {
                rx_buffer[rx_index] = '\0';
                HandleCommand(rx_buffer);
                rx_index = 0;
            }
        } else {
            if (rx_index < RX_BUFFER_SIZE - 1) {
                rx_buffer[rx_index++] = rx_byte;
            } else {
                rx_index = 0;
            }
        }
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
