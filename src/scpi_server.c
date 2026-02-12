/**
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Aeybel Varghese
 *
 * @file scpi_server.c
 * @brief SCPI Command Server
 */

#define SCPI_STACK_SIZE 2048
#define SCPI_THREAD_PRIO 5

#include "scpi_server.h"

#include <zephyr/console/console.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <string.h>

static struct k_thread scpi_thread_data;
K_THREAD_STACK_DEFINE(scpi_stack, SCPI_STACK_SIZE);

static void scpi_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while (1)
    {
        // grabs a line from console and send it to parser
        char *line = console_getline();
        if (!line)
        {
            continue;
        }

        size_t len = strlen(line);
        if (len == 0)
        {
            continue;
        }

        // TODO: Echo back for now
        printk("%s\n", line);
    }
}

void scpi_server_start(void)
{
    console_getline_init();

    // Spawn server thread
    k_thread_create(&scpi_thread_data, scpi_stack, SCPI_STACK_SIZE, scpi_thread, NULL, NULL, NULL, SCPI_THREAD_PRIO, 0,
                    K_NO_WAIT);
}
