/**
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Aeybel Varghese
 *
 * @file main.c
 * @brief Main applicaiton logic
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <errno.h>
#include <string.h>

#define MSG_SIZE 128
#define RX_MSGQ_LEN 10

K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, RX_MSGQ_LEN, 4);

static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/**
 * @brief Prints a string message over UARt
 *
 * @param buf Null termminated string to print
 */
void print_uart(char *buf)
{
    int msg_len = strlen(buf);

    for (int i = 0; i < msg_len; i++)
    {
        uart_poll_out(uart_dev, buf[i]);
    }
}

/**
 * @brief IRQ Callback that reads UART RX FIFO and puts the contents
 * on a message queue for other tasks to read
 *
 * @param dev UART Device
 * @param user_data Callback User Data
 */
void uart_callback(const struct device *dev, void *user_data)
{
    uint8_t c;

    if (!uart_irq_update(uart_dev))
    {
        return;
    }

    if (!uart_irq_rx_ready(uart_dev))
    {
        return;
    }

    // Read UART FIFO Contents until its emptied out
    while (uart_fifo_read(uart_dev, &c, 1) == 1)
    {
        if ((c == '\n' || c == '\r') && rx_buf_pos > 0)
        {
            // On a newline, null-terminate the current buffer
            rx_buf[rx_buf_pos] = '\0';

            // Pushes the message and resets buffer
            k_msgq_put(&uart_msgq, &rx_buf, K_FOREVER);
            rx_buf_pos = 0;
        }
        else if (rx_buf_pos < (sizeof(rx_buf) - 1))
        {
            // Otherwise keep populating the buffer
            rx_buf[rx_buf_pos++] = c;
        }
        else
        {
            // Unlikely to happen but print if it does go wrong
            printk("UART message larger than buffer");
        }
    }
}

static int repl_loop(void)
{
    char tx_buf[MSG_SIZE];

    // Configure UART interrupt to execute a callback that receives data
    int ret = uart_irq_callback_user_data_set(uart_dev, uart_callback, NULL);

    if (ret < 0)
    {
        switch (ret)
        {
        case -ENOTSUP:
            printk("Interrupt-driven UART API support not enabled\n");
            break;
        case -ENOSYS:
            printk("UART Device does not support interrupt-driven API\n");
            break;
        default:
            printk("Error setting UART callback: %d\n", ret);
        }

        return ret;
    }

    uart_irq_rx_enable(uart_dev);

    // REPL Loop
    while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0)
    {
        print_uart(tx_buf);
        print_uart("\r\n");
    }

    return 0;
}

int main(void)
{

    if (!device_is_ready(uart_dev))
    {
        printk("UART Device not found!\n");
        return -ENXIO;
    }

    return repl_loop();
}
