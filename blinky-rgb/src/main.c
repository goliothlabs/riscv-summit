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
#include <samples/common/net_connect.h>
#include <zephyr/net/coap.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();

static K_SEM_DEFINE(connected, 0, 1);

static void golioth_on_connect(struct golioth_client *client)
{
    k_sem_give(&connected);
}

void main(void)
{
	int ret;
	int counter = 0;
	int err;

	LOG_DBG("Start Hello sample");

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
		k_msleep(SLEEP_TIME_MS);
	}
}