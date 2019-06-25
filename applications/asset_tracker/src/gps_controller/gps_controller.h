/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file
 *
 * @brief   GPS module for asset tracker
 */

#ifndef GPS_CONTROLLER_H__
#define GPS_CONTROLLER_H__

#include <zephyr.h>

#ifdef __cplusplus
extern "C" {
#endif

int gps_control_init(gps_trigger_handler_t handler);

void gps_control_on_trigger(void);

void gps_control_stop(u32_t delay_ms);

void gps_control_start(u32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif /* GPS_CONTROLLER_H__ */
