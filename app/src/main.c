/*
 * Class A LoRaWAN sample application
 *
 * Copyright (c) 2020 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include "secrets.h"

#define DELAY K_MSEC(10000)

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_class_a);

char data[] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd'};

static void dl_callback(uint8_t port, uint8_t flags, int16_t rssi, int8_t snr, uint8_t len,
			const uint8_t *hex_data)
{
	LOG_INF("Port %d, Pending %d, RSSI %ddB, SNR %ddBm, Time %d", port,
		flags & LORAWAN_DATA_PENDING, rssi, snr, !!(flags & LORAWAN_TIME_UPDATED));
	if (hex_data) {
		LOG_HEXDUMP_INF(hex_data, len, "Payload: ");
	}
}

static void lorwan_datarate_changed(enum lorawan_datarate dr)
{
	uint8_t unused, max_size;

	lorawan_get_payload_sizes(&unused, &max_size);
	LOG_INF("New Datarate: DR_%d, Max Payload %d", dr, max_size);
}

int main(void)
{
	const struct device *lora_dev;
	struct lorawan_join_config join_cfg;
	uint32_t dev_addr = LORAWAN_DEV_ADDR;
	uint8_t app_skey[] = LORAWAN_APP_SKEY;
	uint8_t nwk_skey[] = LORAWAN_NWK_SKEY;
	uint8_t app_eui[] = LORAWAN_APP_EUI;
    uint8_t dev_eui[] = LORAWAN_DEV_EUI;
	int ret;

	struct lorawan_downlink_cb downlink_cb = {
		.port = LW_RECV_PORT_ANY,
		.cb = dl_callback
	};

	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s: device not ready.", lora_dev->name);
		return 0;
	}


	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("settings init failed: %d", ret);
		return 0;
	}

	ret = settings_load();
	if (ret) {
		LOG_ERR("settings load failed: %d", ret);
	}



uint16_t mask[] = { 0xFF00, 0x0000, 0x0000, 0x0000, 0x0002, 0x0000 };
ret = lorawan_start();
	if (ret < 0) {
		LOG_ERR("lorawan_start failed: %d", ret);
		return 0;
	}

    lorawan_enable_adr(false);
    ret = lorawan_set_datarate(LORAWAN_DR_4);   
    if (ret < 0) {
        LOG_ERR("lorawan_set_datarate failed: %d", ret);
        return 0;
    }

    lorawan_set_channels_mask(mask, ARRAY_SIZE(mask));	

	lorawan_register_downlink_callback(&downlink_cb);
	lorawan_register_dr_changed_callback(lorwan_datarate_changed);

	join_cfg.mode = LORAWAN_ACT_ABP;
	join_cfg.dev_eui = dev_eui;
	join_cfg.abp.dev_addr = dev_addr;
	join_cfg.abp.app_skey = app_skey;
	join_cfg.abp.nwk_skey = nwk_skey;
	join_cfg.abp.app_eui = app_eui;

	LOG_INF("Provisioned via ABP");
	ret = lorawan_join(&join_cfg);
	if (ret < 0) {
		 LOG_ERR("lorawan_join failed: %d", ret);
		 return 0;
	}

	LOG_INF("Sending data...");
	while (1) {
		ret = lorawan_send(2, data, sizeof(data),
				   LORAWAN_MSG_UNCONFIRMED);

		if (ret == -EAGAIN) {
			LOG_ERR("lorawan_send failed: %d. Continuing...", ret);
			k_sleep(DELAY);
			continue;
		}

		if (ret < 0) {
			LOG_ERR("lorawan_send failed: %d", ret);
			return 0;
		}

		LOG_INF("Data sent!");
		k_sleep(DELAY);
	}
}