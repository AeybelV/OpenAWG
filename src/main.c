/**
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Aeybel Varghese
 *
 * @file main.c
 * @brief Main applicaiton logic
 */

#include "scpi_server.h"

#include <zephyr/console/console.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{

    printk("\n\n========== OpenAWG ==========\n\n");

    scpi_server_start();

    printk("Welcome to OpenAWG\n");

    while (1)
    {
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
