/**
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Aeybel Varghese
 *
 * @file scpi_server.c
 * @brief SCPI Command Server
 */

#define SCPI_STACK_SIZE 2048
#define SCPI_THREAD_PRIO 5

#define SCPI_INPUT_BUFFER_LENGTH 256
#define SCPI_ERROR_QUEUE_SIZE 16

#include "scpi_server.h"

#include <zephyr/app_version.h>
#include <zephyr/console/console.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <string.h>

#include <scpi/scpi.h>

// SCPI Thread Globals
static struct k_thread scpi_thread_data;
K_THREAD_STACK_DEFINE(scpi_stack, SCPI_STACK_SIZE);

// SCPI Parser Globals
static scpi_t scpi_ctx;
static char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];
static scpi_error_t scpi_error_queue[SCPI_ERROR_QUEUE_SIZE];
extern const scpi_unit_def_t scpi_units_def[];

// Device ID Globals
static char device_id[32];

// ========== Utilities ==========

/**
 * @brief Get serial number/device id
 * TODO: It might be worthwhile to move this elsewhere outside of scpi server logic?
 *
 * @param out Buffer to store id
 * @param out_len  Size of buffer
 */
void get_serial(char *dst, size_t len)
{
    uint8_t id[16];
    ssize_t id_len = hwinfo_get_device_id(id, sizeof(id));

    if (id_len <= 0)
    {
        snprintf(dst, len, "UNKNOWN");
        return;
    }

    // convert to hex string
    char *p = dst;
    for (int i = 0; i < id_len && (p - dst) < len - 2; i++)
    {
        p += sprintf(p, "%02X", id[i]);
    }
}

// ========== SCPI Library Command Callbacks ==========

static scpi_result_t My_CoreTstQ(scpi_t *context)
{

    SCPI_ResultInt32(context, 0);

    return SCPI_RES_OK;
}

// SCPI Command Table
scpi_command_t scpi_commands[] = {
    // IEEE 488.2 Common Commands
    {
        .pattern = "*CLS",
        .callback = SCPI_CoreCls,
    },
    {
        .pattern = "*ESE",
        .callback = SCPI_CoreEse,
    },
    {
        .pattern = "*ESE?",
        .callback = SCPI_CoreEseQ,
    },
    {
        .pattern = "*ESR?",
        .callback = SCPI_CoreEsrQ,
    },
    {
        .pattern = "*IDN?",
        .callback = SCPI_CoreIdnQ,
    },
    {
        .pattern = "*OPC",
        .callback = SCPI_CoreOpc,
    },
    {
        .pattern = "*OPC?",
        .callback = SCPI_CoreOpcQ,
    },
    {
        .pattern = "*RST",
        .callback = SCPI_CoreRst,
    },
    {
        .pattern = "*SRE",
        .callback = SCPI_CoreSre,
    },
    {
        .pattern = "*SRE?",
        .callback = SCPI_CoreSreQ,
    },
    {
        .pattern = "*STB?",
        .callback = SCPI_CoreStbQ,
    },
    {
        .pattern = "*TST?",
        .callback = My_CoreTstQ,
    },
    {
        .pattern = "*WAI",
        .callback = SCPI_CoreWai,
    },
    // Required SCPI Commands
    {
        .pattern = "SYSTem:ERRor[:NEXT]?",
        .callback = SCPI_SystemErrorNextQ,
    },
    {
        .pattern = "SYSTem:ERRor:COUNt?",
        .callback = SCPI_SystemErrorCountQ,
    },
    {
        .pattern = "SYSTem:VERSion?",
        .callback = SCPI_SystemVersionQ,
    },
    {
        .pattern = "STATus:QUEStionable[:EVENt]?",
        .callback = SCPI_StatusQuestionableEventQ,
    },
    {
        .pattern = "STATus:QUEStionable:ENABle",
        .callback = SCPI_StatusQuestionableEnable,
    },
    {
        .pattern = "STATus:QUEStionable:ENABle?",
        .callback = SCPI_StatusQuestionableEnableQ,
    },
    {
        .pattern = "STATus:PRESet",
        .callback = SCPI_StatusPreset,
    },
    SCPI_CMD_LIST_END};

/**
 * @brief SCPI Parser Interface callback that outputs library data to stdout
 *
 * @param context SCPI parser context
 * @param data Data to write
 * @param len Length of data
 * @return Bytes written
 */
size_t scpi_write(scpi_t *context, const char *data, size_t len)
{
    (void)context;
    return fwrite(data, 1, len, stdout);
}

scpi_interface_t scpi_interface = {
    .write = scpi_write,
    .error = NULL,
    .reset = NULL,
};

// ========== SCPI Threads ==========

/**
 * @brief Zephyr thread that runs the SCPI Server/Parser
 *
 * @param p1 Unused
 * @param p2 Unused
 * @param p3 Unused
 */
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

        // Feed in the command to the parser
        SCPI_Input(&scpi_ctx, line, len);
        SCPI_Input(&scpi_ctx, "\n", 1);
    }
}

/**
 * @brief Initializes the SCPI Server
 */
void scpi_server_start(void)
{
    // Initializations
    get_serial(device_id, sizeof(device_id));
    console_getline_init();

    // Init scpi parser library
    SCPI_Init(&scpi_ctx, scpi_commands, &scpi_interface, scpi_units_def,
              CONFIG_AWG_MANAFACTURER, // Manufacturer
              CONFIG_AWG_MODEL,        // Model
              device_id,               // Serial
              APP_VERSION_STRING,      // Firmware
              scpi_input_buffer, sizeof(scpi_input_buffer), scpi_error_queue, SCPI_ERROR_QUEUE_SIZE);

    // Spawn server thread
    k_thread_create(&scpi_thread_data, scpi_stack, SCPI_STACK_SIZE, scpi_thread, NULL, NULL, NULL, SCPI_THREAD_PRIO, 0,
                    K_NO_WAIT);
}
