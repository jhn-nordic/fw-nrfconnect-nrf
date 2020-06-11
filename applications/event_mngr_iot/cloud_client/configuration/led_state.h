/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _LED_STATE_H_
#define _LED_STATE_H_

/**
 * @brief LED State
 * @defgroup led_state LED State
 * @{
 */

#include <zephyr/types.h>
#include "led_event.h"

#ifdef __cplusplus
extern "C" {
#endif

enum led_system_state {
	LED_SYSTEM_STATE_IDLE,
	LED_SYSTEM_STATE_CONNECTED,
	LED_SYSTEM_STATE_ERROR,

	LED_SYSTEM_STATE_COUNT
};

enum led_id {
	LED_ID_SYSTEM_STATE,
	LED_ID_COUNT
};

#define LED_UNAVAILABLE CONFIG_DESKTOP_LED_COUNT

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _LED_STATE_H_ */
