#ifndef __UART_TX_H
#define __UART_TX_H

#include <stdint.h>

void uart_send_frame(const uint8_t *data, uint16_t len);

#endif /* __UART_TX_H */
