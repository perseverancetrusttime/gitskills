/***************************************************************************
 *
 * Copyright 2015-2020 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#ifndef __SENSOR_HUB_CORE_H__
#define __SENSOR_HUB_CORE_H__

#define SENSOR_HUB_BUFFER_LOCATION __attribute__((used, section(".buffer_section")))

typedef unsigned int (*sensor_hub_core_rx_irq_handler_t)(const void*, unsigned int);
typedef void (*sensor_hub_core_tx_done_irq_handler_t)(const void*, unsigned int);

void sensor_hub_core_register_rx_irq_handler(sensor_hub_core_rx_irq_handler_t irqHandler);
void sensor_hub_core_register_tx_done_irq_handler(sensor_hub_core_tx_done_irq_handler_t irqHandler);

void sensor_hub_enter_full_workload_mode(void);
void sensor_hub_exit_full_workload_mode(void);

#endif

