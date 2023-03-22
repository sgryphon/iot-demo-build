/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <modem/modem_key_mgmt.h>

#include CONFIG_AZURE_IOT_HUB_SAMPLE_MODEM_CERTIFICATES

int credentials_provision(void)
{
	int err = 0;

	if (sizeof(ca_certificate) > 1) {
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					   ca_certificate,
					   sizeof(ca_certificate) - 1);
		if (err) {
			return err;
		}
	}

	if (sizeof(device_certificate) > 1) {
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
					   device_certificate,
					   sizeof(device_certificate) - 1);
		if (err) {
			return err;
		}
	}

	if (sizeof(private_key) > 1) {
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
					   private_key,
					   sizeof(private_key) - 1);
		if (err) {
			return err;
		}
	}

	/* Secondary security tag entries. */

#if CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG != -1

	if (sizeof(ca_certificate_2) > 1) {
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					   ca_certificate_2,
					   sizeof(ca_certificate_2) - 1);
		if (err) {
			return err;
		}
	}

	if (sizeof(device_certificate_2) > 1) {
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
					   device_certificate_2,
					   sizeof(device_certificate_2) - 1);
		if (err) {
			return err;
		}
	}

	if (sizeof(private_key_2) > 1) {
		err = modem_key_mgmt_write(CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG,
					   MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
					   private_key_2,
					   sizeof(private_key_2) - 1);
		if (err) {
			return err;
		}
	}

#endif /* CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG != -1 */

	return err;
}
