/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(riscv_blinky, LOG_LEVEL_DBG);

#include <net/golioth/system_client.h>
#include <net/golioth/settings.h>
#include <samples/common/net_connect.h>
#include <zephyr/net/coap.h>

/* 1000 msec = 1 sec */
static int32_t _loop_delay_ms = 1000;

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();

static K_SEM_DEFINE(connected, 0, 1);

static k_tid_t _system_thread = 0;

enum golioth_settings_status on_setting(
		const char *key,
		const struct golioth_settings_value *value)
{
	LOG_DBG("Received setting: key = %s, type = %d", key, value->type);
	if (strcmp(key, "LOOP_DELAY_MS") == 0) {
		/* This setting is expected to be numeric, return an error if it's not */
		if (value->type != GOLIOTH_SETTINGS_VALUE_TYPE_INT64) {
			return GOLIOTH_SETTINGS_VALUE_FORMAT_NOT_VALID;
		}

		/* This setting must be in range [100, 60000], return an error if it's not */
		if (value->i64 < 100 || value->i64 > 60000) {
			return GOLIOTH_SETTINGS_VALUE_OUTSIDE_RANGE;
		}

		/* Setting has passed all checks, so apply it to the loop delay */
		_loop_delay_ms = (int32_t)value->i64;
		LOG_INF("Set loop delay to %d milliseconds", _loop_delay_ms);

		/* Wake up the main thread to use new value */
		k_wakeup(_system_thread);

		return GOLIOTH_SETTINGS_SUCCESS;
	}

	else if (strcmp(key, "LOOP_DELAY_S") == 0) {
		/* Do nothing, this app uses LOOP_DELAY_MS instead of _S */
		return GOLIOTH_SETTINGS_SUCCESS;
	}

	/* If the setting is not recognized, we should return an error */
	return GOLIOTH_SETTINGS_KEY_NOT_RECOGNIZED;
}


static void golioth_on_connect(struct golioth_client *client)
{
	k_sem_give(&connected);

	int err = golioth_settings_register_callback(client, on_setting);

	if (err) {
		LOG_ERR("Failed to register settings callback: %d", err);
	}
}

void main(void)
{
	int ret;
	int counter = 0;
	int err;

	LOG_DBG("Start Hello sample");

	/* Get system thread id so loop delay change event can wake main */
	_system_thread = k_current_get();

	if (!device_is_ready(led.port)) {
		return;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return;
	}

	if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLES_COMMON)) {
			net_connect();
	}

	client->on_connect = golioth_on_connect;
	golioth_system_client_start();

	k_sem_take(&connected, K_FOREVER);

	while (1) {
		LOG_INF("Sending hello! %d", counter);

		err = golioth_send_hello(client);
		if (err) {
				LOG_WRN("Failed to send hello!");
		}
		++counter;

		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return;
		}
		k_sleep(K_MSEC(_loop_delay_ms));
	}
}
