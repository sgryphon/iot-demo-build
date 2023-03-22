/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 *      "-----BEGIN CA CERTIFICATE-----\n"
 *      "-----CERTIFICATE-----\n"
 *      "-----END CA CERTIFICATE-----\n"
 */
static const unsigned char ca_certificate[] = {
//#if __has_include("../certs/BaltimoreCyberTrustRoot.crt.pem.h")
#include "../certs/BaltimoreCyberTrustRoot.crt.pem.h"
//#else
//""
//#endif
};

/*
 *      "-----BEGIN PRIVATE KEY-----\n"
 *      "-----KEY-----\n"
 *      "-----END PRIVATE KEY-----\n"
 */
static const unsigned char private_key[] = {
//#if __has_include("../../dev-certs/devices/350457791791735879.key.h")
#include "../../dev-certs/devices/350457791791735879.key.h"
//#else
//""
//#endif
};

/*
 *      "-----BEGIN CLIENT CERTIFICATE-----\n"
 *      "-----CERTIFICATE-----\n"
 *      "-----END CLIENT CERTIFICATE-----\n"
 */
static const unsigned char device_certificate[] = {
//#if __has_include("../../dev-certs/devices/350457791791735879.pem.h")
#include "../../dev-certs/devices/350457791791735879.pem.h"
//#else
//""
//#endif
};

#if CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG != -1

/*
 *      "-----BEGIN CA CERTIFICATE-----\n"
 *      "-----CERTIFICATE-----\n"
 *      "-----END CA CERTIFICATE-----\n"
 */
static const unsigned char ca_certificate_2[] = {
//#if __has_include("../certs/DigiCertGlobalG2TLSRSASHA2562020CA1.crt.pem.h")
#include "../certs/DigiCertGlobalG2TLSRSASHA2562020CA1.crt.pem.h"
//#else
//""
//#endif
};

/*
 *      "-----BEGIN PRIVATE KEY-----\n"
 *      "-----KEY-----\n"
 *      "-----END PRIVATE KEY-----\n"
 */
static const unsigned char private_key_2[] = {
#if __has_include("private-key-2.pem")
#include "private-key-2.pem"
#else
""
#endif
};

/*
 *      "-----BEGIN CLIENT CERTIFICATE-----\n"
 *      "-----CERTIFICATE-----\n"
 *      "-----END CLIENT CERTIFICATE-----\n"
 */
static const unsigned char device_certificate_2[] = {
#if __has_include("client-cert-2.pem")
#include "client-cert-2.pem"
#else
""
#endif
};

#endif /* CONFIG_MQTT_HELPER_SECONDARY_SEC_TAG != -1 */
