/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/net/socket.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/net/tls_credentials.h>
#include <modem/pdn.h>64:ff9b::334b:4e67
#include <modem/lte_lc.h>
#include <modem/modem_key_mgmt.h>
#include <nrf_modem_at.h>

#define HTTPS_PORT 443

#define HTTPS_HOSTNAME "v4v6.ipv6-test.com"
//#define HTTPS_HOSTNAME "v4.ipv6-test.com"

// #define HTTP_HEAD                                                              \
// 	"HEAD / HTTP/1.1\r\n"                                                  \
// 	"Host: " HTTPS_HOSTNAME ":443\r\n"                                     \
// 	"Connection: close\r\n\r\n"

// #define HTTP_HEAD_LEN (sizeof(HTTP_HEAD) - 1)

#define HTTP_GET                                                              \
	"GET /api/myip.php HTTP/1.1\r\n"                                                  \
	"Host: " HTTPS_HOSTNAME ":443\r\n"                                     \
	"Connection: close\r\n\r\n"

#define HTTP_GET_LEN (sizeof(HTTP_GET) - 1)

#define HTTP_HDR_END "\r\n\r\n"

#define RECV_BUF_SIZE 4096
#define TLS_SEC_TAG 42

static const char send_buf[] = HTTP_GET;
static char recv_buf[RECV_BUF_SIZE];

static const char cert[] = {
	#include "../cert/USERTrust_RSA_Certification_Authority.pem"
};

BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");

/* Provision certificate to modem */
int cert_provision(void)
{
	int err;
	bool exists;
	int mismatch;

	/* It may be sufficient for you application to check whether the correct
	 * certificate is provisioned with a given tag directly using modem_key_mgmt_cmp().
	 * Here, for the sake of the completeness, we check that a certificate exists
	 * before comparing it with what we expect it to be.
	 */
	err = modem_key_mgmt_exists(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (err) {
		printk("Failed to check for certificates err %d\n", err);
		return err;
	}

	if (exists) {
		mismatch = modem_key_mgmt_cmp(TLS_SEC_TAG,
					      MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					      cert, strlen(cert));
		if (!mismatch) {
			printk("Certificate match\n");
			return 0;
		}

		printk("Certificate mismatch\n");
		err = modem_key_mgmt_delete(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			printk("Failed to delete existing certificate, err %d\n", err);
		}
	}

	printk("Provisioning certificate\n");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(TLS_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, sizeof(cert) - 1);
	if (err) {
		printk("Failed to provision certificate, err %d\n", err);
		return err;
	}

	return 0;
}

static const char * const event_str[] = {
	[PDN_EVENT_CNEC_ESM] = "ESM",
	[PDN_EVENT_ACTIVATED] = "activated",
	[PDN_EVENT_DEACTIVATED] = "deactivated",
	[PDN_EVENT_IPV6_UP] = "IPv6 up",
	[PDN_EVENT_IPV6_DOWN] = "IPv6 down",
};

void pdn_event_handler(uint8_t cid, enum pdn_event event, int reason)
{
	switch (event) {
	case PDN_EVENT_CNEC_ESM:
		printk("Event: PDP context %d, %s\n", cid, pdn_esm_strerror(reason));
		break;
	default:
		printk("Event: PDP context %d %s\n", cid, event_str[event]);
		break;
	}
}

/* Setup TLS options on a given socket */
int tls_setup(int fd)
{
	int err;
	int verify;

	/* Security tag that we have provisioned the certificate with */
	const sec_tag_t tls_sec_tag[] = {
		TLS_SEC_TAG,
	};

#if defined(CONFIG_SAMPLE_TFM_MBEDTLS)
	err = tls_credential_add(tls_sec_tag[0], TLS_CREDENTIAL_CA_CERTIFICATE, cert, sizeof(cert));
	if (err) {
		return err;
	}
#endif

	/* Set up TLS peer verification */
	enum {
		NONE = 0,
		OPTIONAL = 1,
		REQUIRED = 2,
	};

	verify = REQUIRED;

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		printk("Failed to setup peer verification, err %d\n", errno);
		return err;
	}

	/* Associate the socket with the security tag
	 * we have provisioned the certificate with.
	 */
	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag,
			 sizeof(tls_sec_tag));
	if (err) {
		printk("Failed to setup TLS sec tag, err %d\n", errno);
		return err;
	}

	err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, HTTPS_HOSTNAME, sizeof(HTTPS_HOSTNAME) - 1);
	if (err) {
		printk("Failed to setup TLS hostname, err %d\n", errno);
		return err;
	}
	return 0;
}

void main(void)
{
	int err;
	int fd;
	char *p;
	int bytes;
	size_t off;
	struct addrinfo *res;
	struct addrinfo hints = {
		.ai_family = AF_INET6,
		.ai_socktype = SOCK_STREAM,
	};

	printk("HTTPS client sample started\n\r");

	k_msleep(1000);
	printk("1\n\r");
	k_msleep(1000);
	printk("2\n\r");
	k_msleep(1000);
	printk("3\n\r");

#if !defined(CONFIG_SAMPLE_TFM_MBEDTLS)
	/* Provision certificates before connecting to the LTE network */
	err = cert_provision();
	if (err) {
		return;
	}
#endif

	/* Setup a callback for the default PDP context (zero).
	 * Do this before switching to function mode 1 (CFUN=1)
	 * to receive the first activation event.
	 */
	err = pdn_default_ctx_cb_reg(pdn_event_handler);
	if (err) {
		printk("pdn_default_ctx_cb_reg() failed, err %d\n", err);
		return;
	}

	printk("Waiting for network.. ");
	err = lte_lc_init_and_connect();
	if (err) {
		printk("Failed to connect to the LTE network, err %d\n", err);
		return;
	}
	printk("OK\n");

	char apn[32] = { 0 };
	err = pdn_default_apn_get(apn, sizeof(apn));
	if (err) {
		printk("pdn_default_apn_get() failed, err %d\n", err);
		return;
	}
	printk("Default APN is %s\n", apn);

	err = getaddrinfo(HTTPS_HOSTNAME, NULL, &hints, &res);
	if (err) {
		printk("getaddrinfo() failed, err %d\n", errno);
		return;
	}
	char address_string[64] = { 0 };
	if (res->ai_addr->sa_family == AF_INET6) {
		printk("Host %s address is %s\n", HTTPS_HOSTNAME, inet_ntop(AF_INET6,
			&((const struct sockaddr_in6 *) res->ai_addr)->sin6_addr,
			address_string, sizeof(address_string)));
	} else {
		printk("Host %s IPv4 address is %s\n", HTTPS_HOSTNAME, inet_ntop(AF_INET,
			&((const struct sockaddr_in *) res->ai_addr)->sin_addr,
			address_string, sizeof(address_string)));
	}

	((struct sockaddr_in6 *)res->ai_addr)->sin6_port = htons(HTTPS_PORT);

	if (IS_ENABLED(CONFIG_SAMPLE_TFM_MBEDTLS)) {
		fd = socket(AF_INET6, SOCK_STREAM | SOCK_NATIVE_TLS, IPPROTO_TLS_1_2);
	} else {
		fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TLS_1_2);
	}
	if (fd == -1) {
		printk("Failed to open socket!\n");
		goto clean_up;
	}

	/* Setup TLS socket options */
	err = tls_setup(fd);
	if (err) {
		goto clean_up;
	}

	printk("Connecting to %s\n", HTTPS_HOSTNAME);
	err = connect(fd, res->ai_addr, sizeof(struct sockaddr_in6));
	if (err) {
		printk("connect() failed, err: %d\n", errno);
		goto clean_up;
	}

	off = 0;
	do {
		bytes = send(fd, &send_buf[off], HTTP_GET_LEN - off, 0);
		if (bytes < 0) {
			printk("send() failed, err %d\n", errno);
			goto clean_up;
		}
		off += bytes;
	} while (off < HTTP_GET_LEN);

	printk("Sent %d bytes\n", off);

	off = 0;
	do {
		bytes = recv(fd, &recv_buf[off], RECV_BUF_SIZE - off, 0);
		if (bytes < 0) {
			printk("recv() failed, err %d\n", errno);
			goto clean_up;
		}
		off += bytes;
	} while (bytes != 0 /* peer closed connection */);

	printk("Received %d bytes\n", off);

	/* Make sure recv_buf is NULL terminated (for safe use with strstr) */
	if (off < sizeof(recv_buf)) {
		recv_buf[off] = '\0';
	} else {
		recv_buf[sizeof(recv_buf) - 1] = '\0';
	}

	/* Print HTTP response */
	// p = strstr(recv_buf, "\r\n");
	// if (p) {
	// 	off = p - recv_buf;
	// 	recv_buf[off + 1] = '\0';
	// 	printk("\n>\t %s\n\n", recv_buf);
	// }
	printk("\n> %s\n\n", recv_buf);

	printk("Finished, closing socket.\n");

clean_up:
	freeaddrinfo(res);
	(void)close(fd);

	lte_lc_power_off();
}
