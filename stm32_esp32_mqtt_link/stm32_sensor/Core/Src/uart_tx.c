#include "uart_tx.h"
#include "main.h"

extern UART_HandleTypeDef huart3;

void uart_send_frame(const uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)data, len, HAL_MAX_DELAY);
}
