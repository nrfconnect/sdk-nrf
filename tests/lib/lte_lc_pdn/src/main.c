/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fff.h>
#include <nrf_modem_at.h>

#include <modem/lte_lc.h>

DEFINE_FFF_GLOBALS;

#define CID_0				0
#define CID_1				1
#define CID_2				2
#define CID_11				11
#define CID_12				12
#define CID_MAX				0x7f
#define CID_MIN				-0x80

#define CELLULAR_PROFILE_ID_0		0
#define CELLULAR_PROFILE_ID_1		1

#define AUTH_USR_DEFAULT		"usr"
#define AUTH_PWD_DEFAULT		"pwd"

#define IPv4_DNS_PRIMARY_STR		"192.168.1.23"
#define IPv4_DNS_SECONDARY_STR		"192.168.1.24"
#define IPv6_DNS_PRIMARY_STR		"1111:2222:3:fff::55"
#define IPv6_DNS_SECONDARY_STR		"2222:3333:4:eee:7777::66"

#define CMD_CGDCONT_CID			"AT+CGDCONT=%u"
#define CMD_CGDCONT_CID_FAM		"AT+CGDCONT=%u,%s"
#define CMD_CGDCONT_CID_FAM_APN		"AT+CGDCONT=%u,%s,%s"
#define CMD_CGDCONT_CID_FAM_APN_OPT	"AT+CGDCONT=%u,%s,%s,,,,%u,,,,%u,%u"

#define CMD_CGACT			"AT+CGACT=%u,%u"
#define CMD_CGAUTH			"AT+CGAUTH=%u,%d,%s,%s"

#define RESP_AT_CMD_ERROR		"ERROR"
#define CMD_XGETPDNID			"AT%%XGETPDNID=%u"
#define RESP_XGETPDNID			"%XGETPDNID: 1"

#define FAM_IPV4_STR			"IP"
#define FAM_IPV6_STR			"IPV6"
#define FAM_IPV4V6_STR			"IPV4V6"
#define FAM_NONIP_STR			"Non-IP"
#define APN_DEFAULT			"apn0"

#define OPT_IPv4_ADDR_ALLOC_EXP		0xbe
#define OPT_NSLPI_EXP			0x1
#define OPT_SECURE_PCO_EXP		0xC0

#define CMD_CELLULARPRFL_SET		"AT%%CELLULARPRFL=2,%d,%d,%d"
#define CMD_CELLULARPRFL_REMOVE		"AT%%CELLULARPRFL=2,%d"
#define CMD_CELLULARPRFL_NOTIF		"AT%%CELLULARPRFL=1"
#define CELLULARPRFL_CP_ID		0
#define CELLULARPRFL_ACT		2
#define CELLULARPRFL_ACT_NBIOT_LTEM	(LTE_LC_ACT_NBIOT | LTE_LC_ACT_LTEM)
#define CELLULARPRFL_SIM_SLOT		LTE_LC_UICC_PHYSICAL
#define CELLULARPRFL_SIM_SLOT_SOFTSIM	LTE_LC_UICC_SOFTSIM

FAKE_VALUE_FUNC(void *, k_malloc, size_t);
FAKE_VOID_FUNC(k_free, void *);
FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_scanf, const char *, const char *, ...);
FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_printf, const char *, ...);
FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_cmd, void *, size_t, const char *, ...);
FAKE_VALUE_FUNC(int, z_impl_zsock_inet_pton, net_sa_family_t, const char *, void *);

/* Defined in modules/pdn.c, we redefine it here. */
struct pdn {
	sys_snode_t node; /* list handling */
	int8_t context_id;
};

static struct pdn pdn0;
static struct pdn pdn1;
static struct pdn pdn2;
static struct pdn pdn11;
static struct pdn pdn12;

static struct lte_lc_evt pdn_evt;
static uint8_t pdn_detach_cids[4];
static size_t pdn_detach_cid_count;

static const char ipv4_dns_primary[] = {192, 168, 1, 23};
static const char ipv4_dns_secondary[] = {192, 168, 1, 24};
static const char ipv6_dns_primary[] = {17, 17, 34, 34, 0, 3, 15, 255, 0, 0, 0, 0, 0, 0, 0, 85};
static const char ipv6_dns_secondary[] = {
	34, 34, 51, 51, 0, 4, 14, 238, 119, 119, 0, 0, 0, 0, 0, 102
};

extern void on_cgev(const char *notif);
extern void on_cnec_esm(const char *notif);
extern void on_cellularprfl(const char *notif);

static void *k_malloc_custom(size_t size)
{
	return malloc(size);
}

static void k_free_custom(void *ptr)
{
	free(ptr);
}

static void *k_malloc_NULL(size_t size)
{
	return NULL;
}

static void *k_malloc_PDN0(size_t size)
{
	TEST_ASSERT_EQUAL(sizeof(struct pdn), size);

	return &pdn0;
}

static void *k_malloc_PDN1(size_t size)
{
	TEST_ASSERT_EQUAL(sizeof(struct pdn), size);

	return &pdn1;
}

static void *k_malloc_PDN2(size_t size)
{
	TEST_ASSERT_EQUAL(sizeof(struct pdn), size);

	return &pdn2;
}

static void *k_malloc_PDN11(size_t size)
{
	TEST_ASSERT_EQUAL(sizeof(struct pdn), size);

	return &pdn11;
}

static void *k_malloc_PDN12(size_t size)
{
	TEST_ASSERT_EQUAL(sizeof(struct pdn), size);

	return &pdn12;
}

static void k_free_PDN0(void *data)
{
	TEST_ASSERT_EQUAL_PTR(&pdn0, data);
}

static void k_free_PDN1(void *data)
{
	TEST_ASSERT_EQUAL_PTR(&pdn1, data);
}

static void k_free_PDN2(void *data)
{
	TEST_ASSERT_EQUAL_PTR(&pdn2, data);
}

static int z_impl_zsock_inet_pton_custom(net_sa_family_t family, const char *src, void *dst)
{
	switch (family) {
	case NET_AF_INET:
		if (memcmp(src, IPv4_DNS_PRIMARY_STR, strlen(IPv4_DNS_PRIMARY_STR)) == 0) {
			memcpy(dst, ipv4_dns_primary, sizeof(ipv4_dns_primary));
			return 1;
		}
		if (memcmp(src, IPv4_DNS_SECONDARY_STR, strlen(IPv4_DNS_SECONDARY_STR)) == 0) {
			memcpy(dst, ipv4_dns_secondary, sizeof(ipv4_dns_secondary));
			return 1;
		}
		break;
	case NET_AF_INET6:
		if (memcmp(src, IPv6_DNS_PRIMARY_STR, strlen(IPv6_DNS_PRIMARY_STR)) == 0) {
			memcpy(dst, ipv6_dns_primary, sizeof(ipv6_dns_primary));
			return 1;
		}
		if (memcmp(src, IPv6_DNS_SECONDARY_STR, strlen(IPv6_DNS_SECONDARY_STR)) == 0) {
			memcpy(dst, ipv6_dns_secondary, sizeof(ipv6_dns_secondary));
			return 1;
		}
		break;
	}

	return 0;
}

static int nrf_modem_at_printf_eshutdown(const char *cmd, va_list args)
{
	ARG_UNUSED(cmd);
	ARG_UNUSED(args);

	return -ESHUTDOWN;
}

static int nrf_modem_at_scanf_eshutdown(const char *cmd, const char *fmt, va_list args)
{
	ARG_UNUSED(cmd);
	ARG_UNUSED(fmt);
	ARG_UNUSED(args);

	return -ESHUTDOWN;
}

static int nrf_modem_at_cmd_eshutdown(void *buf, size_t len, const char *cmd, va_list args)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(cmd);
	ARG_UNUSED(args);

	return -ESHUTDOWN;
}

static int nrf_modem_at_printf_error(const char *cmd, va_list args)
{
	ARG_UNUSED(cmd);
	ARG_UNUSED(args);

	return 65536; /* Modem respond "ERROR" */
}

static int nrf_modem_at_scanf_no_match(const char *cmd, const char *fmt, va_list args)
{
	ARG_UNUSED(cmd);
	ARG_UNUSED(fmt);
	ARG_UNUSED(args);

	return 0;
}

static int nrf_modem_at_cmd_error(void *buf, size_t len, const char *cmd, va_list args)
{
	ARG_UNUSED(cmd);
	ARG_UNUSED(args);

	TEST_ASSERT_TRUE(len >= sizeof(RESP_AT_CMD_ERROR));

	memcpy(buf, RESP_AT_CMD_ERROR, sizeof(RESP_AT_CMD_ERROR));

	return 65536; /* Modem respond "ERROR" */
}

static int nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv4(const char *cmd, va_list args)
{
	uint8_t cid;
	char *fam;

	TEST_ASSERT_EQUAL_STRING(CMD_CGDCONT_CID_FAM, cmd);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	fam = va_arg(args, char *);
	TEST_ASSERT_EQUAL_STRING(FAM_IPV4_STR, fam);

	return 0;
}

static int nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv6(const char *cmd, va_list args)
{
	uint8_t cid;
	char *fam;

	TEST_ASSERT_EQUAL_STRING(CMD_CGDCONT_CID_FAM, cmd);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	fam = va_arg(args, char *);
	TEST_ASSERT_EQUAL_STRING(FAM_IPV6_STR, fam);

	return 0;
}

static int nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv4v6(const char *cmd, va_list args)
{
	uint8_t cid;
	char *fam;

	TEST_ASSERT_EQUAL_STRING(CMD_CGDCONT_CID_FAM, cmd);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	fam = va_arg(args, char *);
	TEST_ASSERT_EQUAL_STRING(FAM_IPV4V6_STR, fam);

	return 0;
}

static int nrf_modem_at_printf_cmd_cgdcont_cid_fam_nonip(const char *cmd, va_list args)
{
	uint8_t cid;
	char *fam;

	TEST_ASSERT_EQUAL_STRING(CMD_CGDCONT_CID_FAM, cmd);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	fam = va_arg(args, char *);
	TEST_ASSERT_EQUAL_STRING(FAM_NONIP_STR, fam);

	return 0;
}

static int nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv4_apn(const char *cmd, va_list args)
{
	uint8_t cid;
	const char *apn;
	char *fam;

	TEST_ASSERT_EQUAL_STRING(CMD_CGDCONT_CID_FAM_APN, cmd);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	fam = va_arg(args, char *);
	TEST_ASSERT_EQUAL_STRING(FAM_IPV4_STR, fam);

	apn = va_arg(args, const char *);
	TEST_ASSERT_EQUAL_STRING(APN_DEFAULT, apn);

	return 0;
}

static int nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv4_apn_opt(const char *cmd, va_list args)
{
	uint8_t cid;
	const char *apn;
	char *fam;
	uint8_t opt_ip4_addr_alloc;
	uint8_t opt_nslpi;
	uint8_t opt_secure_pco;

	TEST_ASSERT_EQUAL_STRING(CMD_CGDCONT_CID_FAM_APN_OPT, cmd);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	fam = va_arg(args, char *);
	TEST_ASSERT_EQUAL_STRING(FAM_IPV4_STR, fam);

	apn = va_arg(args, const char *);
	TEST_ASSERT_EQUAL_STRING(APN_DEFAULT, apn);

	opt_ip4_addr_alloc = va_arg(args, int);
	TEST_ASSERT_EQUAL(OPT_IPv4_ADDR_ALLOC_EXP, opt_ip4_addr_alloc);

	opt_nslpi = va_arg(args, int);
	TEST_ASSERT_EQUAL(OPT_NSLPI_EXP, opt_nslpi);

	opt_secure_pco = va_arg(args, int);
	TEST_ASSERT_EQUAL(OPT_SECURE_PCO_EXP, opt_secure_pco);

	return 0;
}

static int nrf_modem_at_printf_cgdcont_cid1(const char *cmd, va_list args)
{
	uint8_t cid;

	TEST_ASSERT_EQUAL_STRING(CMD_CGDCONT_CID, cmd);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	return 0;
}

static int nrf_modem_at_printf_cgdcont_cid2(const char *cmd, va_list args)
{
	uint8_t cid;

	TEST_ASSERT_EQUAL_STRING(CMD_CGDCONT_CID, cmd);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_2, cid);

	return 0;
}

static int nrf_modem_at_printf_cgauth(const char *cmd, va_list args)
{
	uint8_t cid;
	enum lte_lc_pdn_auth method;
	const char *usr;
	const char *pwd;

	TEST_ASSERT_EQUAL_STRING("AT+CGAUTH=%u,%d,%s,%s", cmd);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	method = va_arg(args, int);
	TEST_ASSERT_EQUAL(LTE_LC_PDN_AUTH_PAP, method);

	usr = va_arg(args, char *);
	TEST_ASSERT_EQUAL_STRING(AUTH_USR_DEFAULT, usr);

	pwd = va_arg(args, char *);
	TEST_ASSERT_EQUAL_STRING(AUTH_PWD_DEFAULT, pwd);

	return 0;
}

static int nrf_modem_at_printf_cgact_activate_fam_unknown(const char *cmd, va_list args)
{
	int activate;
	uint8_t cid;

	TEST_ASSERT_EQUAL_STRING(CMD_CGACT, cmd);

	activate = va_arg(args, int);
	TEST_ASSERT_EQUAL(true, activate);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	on_cgev("+CGEV: ME PDN ACT 1\r\n");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_ACTIVATED, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(CID_1, pdn_evt.pdn.cid);

	return 0;
}

static int nrf_modem_at_printf_cgact_activate_ipv4(const char *cmd, va_list args)
{
	int activate;
	uint8_t cid;

	TEST_ASSERT_EQUAL_STRING(CMD_CGACT, cmd);

	activate = va_arg(args, int);
	TEST_ASSERT_EQUAL(true, activate);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	memset(&pdn_evt, 0, sizeof(struct lte_lc_evt));

	/* Bad format */
	on_cgev("+CGEV: ME PDN ACT1\r\n");
	TEST_ASSERT_EQUAL(0, pdn_evt.type);
	TEST_ASSERT_EQUAL(0, pdn_evt.pdn.cid);

	/* Different CID */
	on_cgev("+CGEV: ME PDN ACT 0\r\n");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_ACTIVATED, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(0, pdn_evt.pdn.cid);

	on_cgev("+CGEV: ME PDN ACT 1,0\r\n");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_ACTIVATED, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(CID_1, pdn_evt.pdn.cid);
	TEST_ASSERT_EQUAL(0, pdn_evt.pdn.esm_err);

	return 0;
}

static int nrf_modem_at_printf_cgact_activate_ipv6(const char *cmd, va_list args)
{
	int activate;
	uint8_t cid;

	TEST_ASSERT_EQUAL_STRING(CMD_CGACT, cmd);

	activate = va_arg(args, int);
	TEST_ASSERT_EQUAL(true, activate);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	memset(&pdn_evt, 0, sizeof(struct lte_lc_evt));

	/* Bad format */
	on_cgev("+CGEV: ME PDN ACT1\r\n");
	TEST_ASSERT_EQUAL(0, pdn_evt.type);
	TEST_ASSERT_EQUAL(0, pdn_evt.pdn.cid);

	/* Different CID */
	on_cgev("+CGEV: ME PDN ACT 0\r\n");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_ACTIVATED, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(0, pdn_evt.pdn.cid);

	on_cgev("+CGEV: ME PDN ACT 1,1\r\n");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_ACTIVATED, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(CID_1, pdn_evt.pdn.cid);
	TEST_ASSERT_EQUAL(1, pdn_evt.pdn.esm_err);

	return 0;
}

static int nrf_modem_at_printf_cgact_activate_esm_cid1(const char *cmd, va_list args)
{
	int activate;
	uint8_t cid;

	TEST_ASSERT_EQUAL_STRING(CMD_CGACT, cmd);

	activate = va_arg(args, int);
	TEST_ASSERT_EQUAL(true, activate);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	on_cgev("+CGEV: ME PDN ACT 1\r\n");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_ACTIVATED, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(CID_1, pdn_evt.pdn.cid);

	on_cnec_esm("+CNEC_ESM: 2,1");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_ESM_ERROR, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(CID_1, pdn_evt.pdn.cid);
	TEST_ASSERT_EQUAL(2, pdn_evt.pdn.esm_err);

	return 0;
}

static int nrf_modem_at_printf_cgact_activate_esm_cid2(const char *cmd, va_list args)
{
	int activate;
	uint8_t cid;

	TEST_ASSERT_EQUAL_STRING(CMD_CGACT, cmd);

	activate = va_arg(args, int);
	TEST_ASSERT_EQUAL(true, activate);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_2, cid);

	on_cgev("+CGEV: ME PDN ACT 2\r\n");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_ACTIVATED, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(2, pdn_evt.pdn.cid);

	on_cnec_esm("+CNEC_ESM: 2,2");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_ESM_ERROR, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(CID_2, pdn_evt.pdn.cid);
	TEST_ASSERT_EQUAL(2, pdn_evt.pdn.esm_err);

	return 0;
}

static int nrf_modem_at_printf_cgact_activate_esm_bad_format(const char *cmd, va_list args)
{
	int activate;
	uint8_t cid;

	TEST_ASSERT_EQUAL_STRING(CMD_CGACT, cmd);

	activate = va_arg(args, int);
	TEST_ASSERT_EQUAL(true, activate);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	on_cgev("+CGEV: ME PDN ACT 1\r\n");

	on_cnec_esm("+CNEC_ESM: 2,0");
	on_cnec_esm("+CNEC_ESM: 2");

	on_cnec_esm("+CNEC_ESM: 0,1");

	return 0;
}

static int nrf_modem_at_printf_cgact_deactivate(const char *cmd, va_list args)
{
	int activate;
	uint8_t cid;

	TEST_ASSERT_EQUAL_STRING(CMD_CGACT, cmd);

	activate = va_arg(args, int);
	TEST_ASSERT_EQUAL(false, activate);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	on_cgev("+CGEV: ME PDN DEACT 1\r\n");

	return 0;
}

static int nrf_modem_at_cmd_xgetpdnid(void *buf, size_t len, const char *cmd, va_list args)
{
	uint8_t cid;

	TEST_ASSERT_NOT_NULL(buf);
	TEST_ASSERT_EQUAL_STRING(CMD_XGETPDNID, cmd);
	TEST_ASSERT_TRUE(len >= sizeof(RESP_XGETPDNID));

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	memcpy(buf, RESP_XGETPDNID, sizeof(RESP_XGETPDNID));

	return 0;
}

#define RESP_XGETPDNID_BAD_RESP "%XGETPDNID"

static int nrf_modem_at_cmd_cgetpdnid_bad_resp(void *buf, size_t len, const char *cmd,
	va_list args)
{
	uint8_t cid;

	TEST_ASSERT_NOT_NULL(buf);
	TEST_ASSERT_EQUAL_STRING(CMD_XGETPDNID, cmd);
	TEST_ASSERT_TRUE(len >= sizeof(RESP_XGETPDNID));

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	memcpy(buf, RESP_XGETPDNID_BAD_RESP, sizeof(RESP_XGETPDNID_BAD_RESP));

	return 0;
}

#define CMD_XNEWCID "AT%XNEWCID?"
#define RESP_XNEWCID "%%XNEWCID: %d"

static int nrf_modem_at_scanf_xnewcid_1(const char *cmd, const char *fmt, va_list args)
{
	int *cid;

	TEST_ASSERT_EQUAL_STRING(CMD_XNEWCID, cmd);
	TEST_ASSERT_EQUAL_STRING(RESP_XNEWCID, fmt);

	cid = va_arg(args, int *);
	*cid = CID_1;

	return 1;
}

static int nrf_modem_at_scanf_xnewcid_2(const char *cmd, const char *fmt, va_list args)
{
	int *cid;

	TEST_ASSERT_EQUAL_STRING(CMD_XNEWCID, cmd);
	TEST_ASSERT_EQUAL_STRING(RESP_XNEWCID, fmt);

	cid = va_arg(args, int *);
	*cid = CID_2;

	return 1;
}

static int nrf_modem_at_scanf_xnewcid_11(const char *cmd, const char *fmt, va_list args)
{
	int *cid;

	TEST_ASSERT_EQUAL_STRING(CMD_XNEWCID, cmd);
	TEST_ASSERT_EQUAL_STRING(RESP_XNEWCID, fmt);

	cid = va_arg(args, int *);
	*cid = CID_11;

	return 1;
}

static int nrf_modem_at_scanf_xnewcid_12(const char *cmd, const char *fmt, va_list args)
{
	int *cid;

	TEST_ASSERT_EQUAL_STRING(CMD_XNEWCID, cmd);
	TEST_ASSERT_EQUAL_STRING(RESP_XNEWCID, fmt);

	cid = va_arg(args, int *);
	*cid = CID_12;

	return 1;
}

static int nrf_modem_at_scanf_xnewcid_too_big(const char *cmd, const char *fmt, va_list args)
{
	int *cid;

	TEST_ASSERT_EQUAL_STRING(CMD_XNEWCID, cmd);
	TEST_ASSERT_EQUAL_STRING(RESP_XNEWCID, fmt);

	cid = va_arg(args, int *);
	*cid = CID_MAX + 1;

	return 1;
}

static int nrf_modem_at_scanf_xnewcid_too_small(const char *cmd, const char *fmt, va_list args)
{
	int *cid;

	TEST_ASSERT_EQUAL_STRING(CMD_XNEWCID, cmd);
	TEST_ASSERT_EQUAL_STRING(RESP_XNEWCID, fmt);

	cid = va_arg(args, int *);
	*cid = CID_MIN - 1;

	return 1;
}

static int nrf_modem_at_scanf_custom_cgdcont(const char *cmd, const char *fmt, va_list args)
{
	int *apn;

	TEST_ASSERT_EQUAL_STRING("AT+CGDCONT?", cmd);
	TEST_ASSERT_EQUAL_STRING("+CGDCONT: 0,\"%*[^\"]\",\"%63[^\"]\"", fmt);

	apn = va_arg(args, int *);
	*apn = 1;

	return 1;
}

#define AT_CMD_PDN_CONTEXT_READ_INFO_PARSE_LINE1 \
	"+CGCONTRDP: %*u,,\"%*[^\"]\",\"\",\"\",\"%15[0-9.]\",\"%15[0-9.]\",,,,,%u"
#define AT_CMD_PDN_CONTEXT_READ_INFO_PARSE_LINE2 \
	"+%*[^+]" \
	"+CGCONTRDP: %*u,,\"%*[^\"]\",\"\",\"\",\"%39[0-9A-Fa-f:]\",\"%39[0-9A-Fa-f:]\",,,,,%u"

static int nrf_modem_at_scanf_custom_cgcontrdp(const char *cmd, const char *fmt, va_list args)
{
	char *dns1;
	char *dns2;
	unsigned int *mtu;

	TEST_ASSERT_EQUAL_STRING("AT+CGCONTRDP=0", cmd);

	dns1 = va_arg(args, char *);
	dns2 = va_arg(args, char *);
	mtu = va_arg(args, unsigned int *);

	switch (nrf_modem_at_scanf_fake.call_count) {
	case 1:
		TEST_ASSERT_EQUAL_STRING(AT_CMD_PDN_CONTEXT_READ_INFO_PARSE_LINE1, fmt);
		strcpy(dns1, IPv4_DNS_PRIMARY_STR);
		strcpy(dns2, IPv4_DNS_SECONDARY_STR);
		*mtu = 1500;
		return 3;
	case 2:
		TEST_ASSERT_EQUAL_STRING(AT_CMD_PDN_CONTEXT_READ_INFO_PARSE_LINE2, fmt);
		strcpy(dns1, IPv6_DNS_PRIMARY_STR);
		strcpy(dns2, IPv6_DNS_SECONDARY_STR);
		*mtu = 1600;
		return 3;
	default:
		TEST_ASSERT(false);
		break;
	}

	return -EBADMSG;
}

static int nrf_modem_at_scanf_custom_cgcontrdp_match_ipv4(
	const char *cmd, const char *fmt, va_list args)
{
	char *dns1;
	char *dns2;
	unsigned int *mtu;

	TEST_ASSERT_EQUAL_STRING("AT+CGCONTRDP=0", cmd);

	dns1 = va_arg(args, char *);
	dns2 = va_arg(args, char *);
	mtu = va_arg(args, unsigned int *);

	switch (nrf_modem_at_scanf_fake.call_count) {
	case 1:
		TEST_ASSERT_EQUAL_STRING(AT_CMD_PDN_CONTEXT_READ_INFO_PARSE_LINE1, fmt);
		strcpy(dns1, IPv4_DNS_PRIMARY_STR);
		strcpy(dns2, IPv4_DNS_SECONDARY_STR);
		*mtu = 1500;
		return 3;
	case 2:
		TEST_ASSERT_EQUAL_STRING(AT_CMD_PDN_CONTEXT_READ_INFO_PARSE_LINE2, fmt);
		return -EBADMSG;
	default:
		TEST_ASSERT(false);
		break;
	}

	return -EBADMSG;
}

static int nrf_modem_at_scanf_custom_cgcontrdp_match_ipv6(
	const char *cmd, const char *fmt, va_list args)
{
	char *dns1;
	char *dns2;
	unsigned int *mtu;

	TEST_ASSERT_EQUAL_STRING("AT+CGCONTRDP=0", cmd);

	dns1 = va_arg(args, char *);
	dns2 = va_arg(args, char *);
	mtu = va_arg(args, unsigned int *);

	switch (nrf_modem_at_scanf_fake.call_count) {
	case 1:
		TEST_ASSERT_EQUAL_STRING(AT_CMD_PDN_CONTEXT_READ_INFO_PARSE_LINE1, fmt);
		strcpy(dns1, IPv6_DNS_PRIMARY_STR);
		strcpy(dns2, IPv6_DNS_SECONDARY_STR);
		*mtu = 1600;
		return 3;
	case 2:
		TEST_ASSERT_EQUAL_STRING(AT_CMD_PDN_CONTEXT_READ_INFO_PARSE_LINE2, fmt);
		return -EBADMSG;
	default:
		TEST_ASSERT(false);
		break;
	}

	return -EBADMSG;
}

static int nrf_modem_at_scanf_custom_cgcontrdp_no_match(
	const char *cmd, const char *fmt, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT+CGCONTRDP=0", cmd);

	return -EBADMSG;
}

static int nrf_modem_at_printf_cellularprfl(const char *cmd, va_list args)
{
	int cp_id;
	int act;
	int sim_slot;

	if (strncmp(cmd, CMD_CELLULARPRFL_NOTIF, sizeof(CMD_CELLULARPRFL_NOTIF) - 1) == 0) {
		return 0;
	}

	TEST_ASSERT_EQUAL_STRING(CMD_CELLULARPRFL_SET, cmd);

	cp_id = va_arg(args, int);
	TEST_ASSERT_EQUAL(CELLULARPRFL_CP_ID, cp_id);

	act = va_arg(args, int);
	TEST_ASSERT_EQUAL(CELLULARPRFL_ACT, act);

	sim_slot = va_arg(args, int);
	TEST_ASSERT_EQUAL(CELLULARPRFL_SIM_SLOT, sim_slot);

	return 0;
}

static int nrf_modem_at_printf_cellularprfl_softsim(const char *cmd, va_list args)
{
	int cp_id;
	int act;
	int sim_slot;

	if (strncmp(cmd, CMD_CELLULARPRFL_NOTIF,
		    sizeof(CMD_CELLULARPRFL_NOTIF) - 1) == 0) {
		return 0;
	}

	TEST_ASSERT_EQUAL_STRING(CMD_CELLULARPRFL_SET, cmd);

	cp_id = va_arg(args, int);
	TEST_ASSERT_EQUAL(CELLULARPRFL_CP_ID, cp_id);

	act = va_arg(args, int);
	TEST_ASSERT_EQUAL(CELLULARPRFL_ACT, act);

	sim_slot = va_arg(args, int);
	TEST_ASSERT_EQUAL(CELLULARPRFL_SIM_SLOT_SOFTSIM, sim_slot);

	return 0;
}

static int nrf_modem_at_printf_cellularprfl_multiple_act(const char *cmd, va_list args)
{
	int cp_id;
	int act;

	if (strncmp(cmd, CMD_CELLULARPRFL_NOTIF, sizeof(CMD_CELLULARPRFL_NOTIF) - 1) == 0) {
		return 0;
	}

	TEST_ASSERT_EQUAL_STRING(CMD_CELLULARPRFL_SET, cmd);

	cp_id = va_arg(args, int);
	TEST_ASSERT_EQUAL(CELLULARPRFL_CP_ID, cp_id);

	act = va_arg(args, int);
	TEST_ASSERT_EQUAL(CELLULARPRFL_ACT_NBIOT_LTEM, act);

	return 0;
}

static int nrf_modem_at_printf_cellularprfl_remove_cid0(const char *cmd, va_list args)
{
	int cp_id;

	TEST_ASSERT_EQUAL_STRING(CMD_CELLULARPRFL_REMOVE, cmd);

	cp_id = va_arg(args, int);
	TEST_ASSERT_EQUAL(CELLULAR_PROFILE_ID_0, cp_id);

	return 0;
}

static int nrf_modem_at_printf_cellularprfl_remove_cid2(const char *cmd, va_list args)
{
	int cp_id;

	TEST_ASSERT_EQUAL_STRING(CMD_CELLULARPRFL_REMOVE, cmd);

	cp_id = va_arg(args, int);
	TEST_ASSERT_EQUAL(2, cp_id);

	return -EINVAL;
}

/* Unified lte_lc event handler that extracts PDN events */
static void lte_lc_event_handler(const struct lte_lc_evt *const evt)
{
	/* Only handle PDN and cellular profile events. */

	switch (evt->type) {
	case LTE_LC_EVT_PDN:
		switch (evt->pdn.type) {
		case LTE_LC_EVT_PDN_ACTIVATED:
		case LTE_LC_EVT_PDN_DEACTIVATED:
		case LTE_LC_EVT_PDN_IPV6_UP:
		case LTE_LC_EVT_PDN_IPV6_DOWN:
		case LTE_LC_EVT_PDN_SUSPENDED:
		case LTE_LC_EVT_PDN_RESUMED:
		case LTE_LC_EVT_PDN_NETWORK_DETACH:
		case LTE_LC_EVT_PDN_APN_RATE_CONTROL_ON:
		case LTE_LC_EVT_PDN_APN_RATE_CONTROL_OFF:
		case LTE_LC_EVT_PDN_CTX_DESTROYED:
		case LTE_LC_EVT_PDN_ESM_ERROR:
			break;
		default:
			TEST_ASSERT(false);

			break;
		}

		break;
	case LTE_LC_EVT_CELLULAR_PROFILE_ACTIVE:
		break;
	default:
		TEST_ASSERT(false);

		break;
	}

	if ((evt->type == LTE_LC_EVT_PDN) &&
	    (evt->pdn.type == LTE_LC_EVT_PDN_NETWORK_DETACH) &&
	    (pdn_detach_cid_count < ARRAY_SIZE(pdn_detach_cids))) {
		pdn_detach_cids[pdn_detach_cid_count++] = evt->pdn.cid;
	}

	pdn_evt = *evt;
}

void setUp(void)
{
	RESET_FAKE(k_malloc);
	RESET_FAKE(k_free);
	RESET_FAKE(nrf_modem_at_printf);
	RESET_FAKE(nrf_modem_at_scanf);
	RESET_FAKE(nrf_modem_at_cmd);

	memset(&pdn_evt, 0, sizeof(struct lte_lc_evt));
	memset(pdn_detach_cids, 0, sizeof(pdn_detach_cids));

	pdn_detach_cid_count = 0;

	z_impl_zsock_inet_pton_fake.custom_fake = z_impl_zsock_inet_pton_custom;
	k_malloc_fake.custom_fake = k_malloc_custom;
	k_free_fake.custom_fake = k_free_custom;

	lte_lc_register_handler(lte_lc_event_handler);

	/* Reset call count after lte_lc_register_handler() called it once. */
	k_malloc_fake.call_count = 0;
}

void tearDown(void)
{
	/* Ensure that the correct k_free is called */
	k_free_fake.custom_fake = k_free_custom;

	lte_lc_deregister_handler(lte_lc_event_handler);
}

void test_lte_lc_pdp_context_create_eshutdown(void)
{
	int ret;
	uint8_t cid = 0;

	k_malloc_fake.custom_fake = k_malloc_PDN1;
	k_free_fake.custom_fake = k_free_PDN1;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_eshutdown;

	ret = lte_lc_pdn_ctx_create(&cid);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
}

void test_lte_lc_pdp_context_create_ebadmsg(void)
{
	int ret;
	uint8_t cid = 0;

	k_malloc_fake.custom_fake = k_malloc_PDN1;
	k_free_fake.custom_fake = k_free_PDN1;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_no_match;

	ret = lte_lc_pdn_ctx_create(&cid);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_pdp_context_create_efault(void)
{
	int ret;
	uint8_t cid = 0;

	ret = lte_lc_pdn_ctx_create(NULL);
	TEST_ASSERT_EQUAL(-EFAULT, ret);

	k_malloc_fake.custom_fake = k_malloc_PDN1;
	k_free_fake.custom_fake = k_free_PDN1;

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_xnewcid_too_big;

	ret = lte_lc_pdn_ctx_create(&cid);
	TEST_ASSERT_EQUAL(-EFAULT, ret);

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_xnewcid_too_small;

	ret = lte_lc_pdn_ctx_create(&cid);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_pdp_context_create_enomem(void)
{
	int ret;
	uint8_t cid = 1;

	k_malloc_fake.custom_fake = k_malloc_NULL;

	ret = lte_lc_pdn_ctx_create(&cid);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_lte_lc_pdp_context_create_at_cmd_scanf_eshutdown(void)
{
	int ret;
	uint8_t cid = 0;

	k_malloc_fake.custom_fake = k_malloc_PDN1;
	k_free_fake.custom_fake = k_free_PDN1;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_eshutdown;

	ret = lte_lc_pdn_ctx_create(&cid);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
}

void test_lte_lc_pdp_context_create_cid1(void)
{
	int ret;
	uint8_t cid = CID_1;

	k_malloc_fake.custom_fake = k_malloc_PDN1;
	k_free_fake.custom_fake = k_free_PDN1;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_xnewcid_1;

	ret = lte_lc_pdn_ctx_create(&cid);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_lte_lc_pdp_context_create_cid2(void)
{
	int ret;
	uint8_t cid = CID_2;

	k_malloc_fake.custom_fake = k_malloc_PDN2;
	k_free_fake.custom_fake = k_free_PDN2;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_xnewcid_2;

	ret = lte_lc_pdn_ctx_create(&cid);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_lte_lc_pdp_context_configure_einval(void)
{
	int ret;
	/* APN_STR_MAX_LEN + 1 */
	static const char apn_toobig[] = "loremipsumdolorsitametconsectet "
					 "uradipiscingelitnamaceleifendaug";
	enum lte_lc_pdn_family fam = LTE_LC_PDN_FAM_IPV4;
	struct lte_lc_pdn_pdp_context_opts opt;

	ret = lte_lc_pdn_ctx_configure(CID_1, apn_toobig, fam, &opt);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	fam = 4; /* Invalid */

	ret = lte_lc_pdn_ctx_configure(CID_1, APN_DEFAULT, fam, &opt);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_pdp_context_configure_eshutdown(void)
{
	int ret;
	enum lte_lc_pdn_family fam = LTE_LC_PDN_FAM_IPV4;
	struct lte_lc_pdn_pdp_context_opts opt;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_eshutdown;

	ret = lte_lc_pdn_ctx_configure(CID_1, APN_DEFAULT, fam, &opt);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
}

void test_lte_lc_pdp_context_configure_enoexec(void)
{
	int ret;
	enum lte_lc_pdn_family fam = LTE_LC_PDN_FAM_IPV4;
	struct lte_lc_pdn_pdp_context_opts opt;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_error;

	ret = lte_lc_pdn_ctx_configure(CID_1, APN_DEFAULT, fam, &opt);
	TEST_ASSERT_EQUAL(-ENOEXEC, ret);
}

void test_lte_lc_pdp_context_configure(void)
{
	int ret;
	struct lte_lc_pdn_pdp_context_opts opt = {
		.ip4_addr_alloc =  OPT_IPv4_ADDR_ALLOC_EXP,
		.nslpi = OPT_NSLPI_EXP,
		.secure_pco = OPT_SECURE_PCO_EXP
	};

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv4;

	ret = lte_lc_pdn_ctx_configure(CID_1, NULL, LTE_LC_PDN_FAM_IPV4, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv6;

	ret = lte_lc_pdn_ctx_configure(CID_1, NULL, LTE_LC_PDN_FAM_IPV6, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv4v6;

	ret = lte_lc_pdn_ctx_configure(CID_1, NULL, LTE_LC_PDN_FAM_IPV4V6, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cmd_cgdcont_cid_fam_nonip;

	ret = lte_lc_pdn_ctx_configure(CID_1, NULL, LTE_LC_PDN_FAM_NONIP, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv4_apn;

	ret = lte_lc_pdn_ctx_configure(CID_1, APN_DEFAULT, LTE_LC_PDN_FAM_IPV4, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv4_apn_opt;

	ret = lte_lc_pdn_ctx_configure(CID_1, APN_DEFAULT, LTE_LC_PDN_FAM_IPV4, &opt);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_lte_lc_pdp_context_auth_set_einval(void)
{
	int ret;
	static const char user[] = AUTH_USR_DEFAULT;
	static const char password[] = "pwd";

	ret = lte_lc_pdn_ctx_auth_set(CID_1, LTE_LC_PDN_AUTH_PAP, NULL, password);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = lte_lc_pdn_ctx_auth_set(CID_1, LTE_LC_PDN_AUTH_PAP, user, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = lte_lc_pdn_ctx_auth_set(CID_1, LTE_LC_PDN_AUTH_CHAP, NULL, password);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = lte_lc_pdn_ctx_auth_set(CID_1, LTE_LC_PDN_AUTH_CHAP, user, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}


void test_lte_lc_pdp_context_auth_set_enoexec(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_error;

	ret = lte_lc_pdn_ctx_auth_set(CID_1, LTE_LC_PDN_AUTH_PAP, AUTH_USR_DEFAULT,
				      AUTH_PWD_DEFAULT);
	TEST_ASSERT_EQUAL(-ENOEXEC, ret);
}

void test_lte_lc_pdp_context_auth_set_eshutdown(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_eshutdown;

	ret = lte_lc_pdn_ctx_auth_set(CID_1, LTE_LC_PDN_AUTH_PAP, AUTH_USR_DEFAULT,
				      AUTH_PWD_DEFAULT);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
}

void test_lte_lc_pdp_context_auth_set(void)
{
	int ret;

	ret = lte_lc_pdn_ctx_auth_set(CID_1, LTE_LC_PDN_AUTH_NONE, NULL, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgauth;

	ret = lte_lc_pdn_ctx_auth_set(CID_1, LTE_LC_PDN_AUTH_PAP, AUTH_USR_DEFAULT,
				      AUTH_PWD_DEFAULT);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_lte_lc_pdn_activate_esm_timeout(void)
{
	int ret;
	int esm = 1;
	enum lte_lc_pdn_family fam;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_ipv4;

	ret = lte_lc_pdn_activate(CID_1, &esm, &fam);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(0, esm);
}

void test_lte_lc_pdn_activate_eshutdown(void)
{
	int ret;
	int esm = 1;
	enum lte_lc_pdn_family fam;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_eshutdown;

	ret = lte_lc_pdn_activate(CID_1, &esm, &fam);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);

	TEST_ASSERT_EQUAL(0, esm);
}

void test_lte_lc_pdn_activate_error(void)
{
	int ret;
	int esm = 1;
	enum lte_lc_pdn_family fam;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_error;

	ret = lte_lc_pdn_activate(CID_1, &esm, &fam);
	TEST_ASSERT_EQUAL(-ENOEXEC, ret);

	TEST_ASSERT_EQUAL(0, esm);
}

void test_lte_lc_pdn_activate_esm_bad_format(void)
{
	int ret;
	int esm = 1;
	enum lte_lc_pdn_family fam;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_ipv4;

	ret = lte_lc_pdn_activate(CID_1, NULL, &fam);
	TEST_ASSERT_EQUAL(0, ret);

	/* ESM unchanged */
	TEST_ASSERT_EQUAL(1, esm);

	TEST_ASSERT_EQUAL(CID_1, pdn_evt.pdn.cid);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_ACTIVATED, pdn_evt.pdn.type); /* Activated*/

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_esm_bad_format;

	ret = lte_lc_pdn_activate(CID_1, &esm, &fam);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(0, esm);
	TEST_ASSERT_EQUAL(0, pdn_evt.pdn.esm_err);

	TEST_ASSERT_EQUAL(CID_1, pdn_evt.pdn.cid);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_ESM_ERROR, pdn_evt.pdn.type); /* Activated*/
}

void test_lte_lc_pdn_activate(void)
{
	int ret;
	int esm = 1;
	enum lte_lc_pdn_family fam;

	/* Events are checked in the fake functions */

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_fam_unknown;

	ret = lte_lc_pdn_activate(CID_1, NULL, NULL);
	TEST_ASSERT_EQUAL(0, ret);
	/* ESM unchanged */
	TEST_ASSERT_EQUAL(1, esm);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_ipv4;

	ret = lte_lc_pdn_activate(CID_1, NULL, &fam);
	TEST_ASSERT_EQUAL(0, ret);
	/* ESM unchanged */
	TEST_ASSERT_EQUAL(1, esm);
	TEST_ASSERT_EQUAL(LTE_LC_PDN_FAM_IPV4, fam);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_ipv6;

	ret = lte_lc_pdn_activate(CID_1, NULL, &fam);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(1, esm);
	TEST_ASSERT_EQUAL(LTE_LC_PDN_FAM_IPV6, fam);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_esm_cid1;

	ret = lte_lc_pdn_activate(CID_1, &esm, &fam);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(2, esm);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_esm_cid2;

	memset(&pdn_evt, 0, sizeof(struct lte_lc_evt));

	ret = lte_lc_pdn_activate(CID_2, &esm, &fam);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(2, esm);
}

void test_lte_lc_pdn_deactivate_eshutdown(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_eshutdown;

	ret = lte_lc_pdn_deactivate(CID_1);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
}

void test_lte_lc_pdn_deactivate_error(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_error;

	ret = lte_lc_pdn_deactivate(CID_1);
	TEST_ASSERT_EQUAL(-ENOEXEC, ret);
}

void test_lte_lc_pdn_deactivate(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_deactivate;

	ret = lte_lc_pdn_deactivate(CID_1);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_DEACTIVATED, pdn_evt.pdn.type);
}

void test_lte_lc_pdn_suspended(void)
{
	on_cgev("+CGEV: ME PDN SUSPENDED " STRINGIFY(CID_0) "\r\n");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_SUSPENDED, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(CID_0, pdn_evt.pdn.cid);
}

void test_lte_lc_pdn_resumed(void)
{
	on_cgev("+CGEV: ME PDN RESUMED " STRINGIFY(CID_1) "\r\n");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_RESUMED, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(CID_1, pdn_evt.pdn.cid);
}

void test_lte_lc_pdn_detach(void)
{
	on_cgev("+CGEV: NW DETACH\r\n");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_NETWORK_DETACH, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(-1, pdn_evt.pdn.cellular_profile_id);
}

void test_lte_lc_pdn_detach_me_no_cp_id(void)
{
	on_cgev("+CGEV: ME DETACH\r\n");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_NETWORK_DETACH, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(-1, pdn_evt.pdn.cellular_profile_id);
}

void test_lte_lc_pdn_detach_me_with_cp_id(void)
{
	on_cgev("+CGEV: ME DETACH " STRINGIFY(CELLULAR_PROFILE_ID_0) "\r\n");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_NETWORK_DETACH, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(CELLULAR_PROFILE_ID_0,
			 pdn_evt.pdn.cellular_profile_id);
}

void test_lte_lc_pdn_detach_nw_with_cp_id(void)
{
	int ret;
	uint8_t cid = 0;

	/* Create CID_11 and CID_12 contexts. */
	k_malloc_fake.custom_fake = k_malloc_PDN11;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_xnewcid_11;

	ret = lte_lc_pdn_ctx_create(&cid);
	TEST_ASSERT_EQUAL(0, ret);

	k_malloc_fake.custom_fake = k_malloc_PDN12;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_xnewcid_12;

	ret = lte_lc_pdn_ctx_create(&cid);
	TEST_ASSERT_EQUAL(0, ret);

	on_cgev("+CGEV: NW DETACH " STRINGIFY(CELLULAR_PROFILE_ID_1) "\r\n");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_NETWORK_DETACH, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(CELLULAR_PROFILE_ID_1, pdn_evt.pdn.cellular_profile_id);
}

void test_lte_lc_pdn_detach_all_cids(void)
{
	int ret;
	uint8_t cid = 0;

	/* Create CID_1 context. */
	k_malloc_fake.custom_fake = k_malloc_PDN1;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_xnewcid_1;

	ret = lte_lc_pdn_ctx_create(&cid);
	TEST_ASSERT_EQUAL(0, ret);

	/* Create CID_2 context. */
	k_malloc_fake.custom_fake = k_malloc_PDN2;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_xnewcid_2;

	ret = lte_lc_pdn_ctx_create(&cid);
	TEST_ASSERT_EQUAL(0, ret);

	/* Create CID_11 context. */
	k_malloc_fake.custom_fake = k_malloc_PDN11;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_xnewcid_11;

	ret = lte_lc_pdn_ctx_create(&cid);
	TEST_ASSERT_EQUAL(0, ret);

	/* Create CID_12 context. */
	k_malloc_fake.custom_fake = k_malloc_PDN12;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_xnewcid_12;

	ret = lte_lc_pdn_ctx_create(&cid);
	TEST_ASSERT_EQUAL(0, ret);

	on_cgev("+CGEV: NW DETACH\r\n");
	on_cgev("+CGEV: NW DETACH " STRINGIFY(CELLULAR_PROFILE_ID_1) "\r\n");

	TEST_ASSERT_EQUAL(4, pdn_detach_cid_count);
	TEST_ASSERT_EQUAL(CID_1, pdn_detach_cids[0]);
	TEST_ASSERT_EQUAL(CID_2, pdn_detach_cids[1]);
	TEST_ASSERT_EQUAL(CID_11, pdn_detach_cids[2]);
	TEST_ASSERT_EQUAL(CID_12, pdn_detach_cids[3]);
}

void test_lte_lc_pdn_id_get_eshutdown(void)
{
	int ret;

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_eshutdown;

	ret = lte_lc_pdn_id_get(CID_1);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
}

void test_lte_lc_pdn_id_get_ebadmsg(void)
{
	int ret;

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_cgetpdnid_bad_resp;

	ret = lte_lc_pdn_id_get(CID_1);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_pdn_id_get_error(void)
{
	int ret;

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_error;

	ret = lte_lc_pdn_id_get(CID_1);
	TEST_ASSERT_EQUAL(-ENOEXEC, ret);
}

void test_lte_lc_pdn_id_get(void)
{
	int ret;

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_xgetpdnid;

	ret = lte_lc_pdn_id_get(CID_1);
	TEST_ASSERT_EQUAL(1, ret);
}

void test_lte_lc_pdn_default_apn_get_eshutdown(void)
{
	int ret;
	char buf[64];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_eshutdown;

	ret = lte_lc_pdn_default_ctx_apn_get(buf, sizeof(buf));
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
}

void test_lte_lc_pdn_default_apn_get_e2big(void)
{
	int ret;
	char buf[1];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_cgdcont;

	ret = lte_lc_pdn_default_ctx_apn_get(buf, sizeof(buf));
	TEST_ASSERT_EQUAL(-E2BIG, ret);
}

void test_lte_lc_pdn_default_apn_get_no_match(void)
{
	int ret;
	char buf[64];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_no_match;

	ret = lte_lc_pdn_default_ctx_apn_get(buf, sizeof(buf));
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_lte_lc_pdn_default_apn_get(void)
{
	int ret;
	char buf[64];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_cgdcont;

	ret = lte_lc_pdn_default_ctx_apn_get(buf, sizeof(buf));
	TEST_ASSERT_EQUAL(0, ret);
}

void test_pdn_apn_rate_control_activate(void)
{
	on_cgev("+CGEV: APNRATECTRL STAT 2,1");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_APN_RATE_CONTROL_ON, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(2, pdn_evt.pdn.cid);

	on_cgev("+CGEV: APNRATECTRL STAT 1,1");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_APN_RATE_CONTROL_ON, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(1, pdn_evt.pdn.cid);
}

void test_pdn_apn_rate_control_deactivate(void)
{
	on_cgev("+CGEV: APNRATECTRL STAT 2,0");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_APN_RATE_CONTROL_OFF, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(2, pdn_evt.pdn.cid);

	on_cgev("+CGEV: APNRATECTRL STAT 1,0");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_APN_RATE_CONTROL_OFF, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(1, pdn_evt.pdn.cid);
}

void test_pdn_apn_rate_control_not_configured(void)
{
	on_cgev("+CGEV: APNRATECTRL STAT 2,2");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_APN_RATE_CONTROL_OFF, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(2, pdn_evt.pdn.cid);

	on_cgev("+CGEV: APNRATECTRL STAT 1,2");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN, pdn_evt.type);
	TEST_ASSERT_EQUAL(LTE_LC_EVT_PDN_APN_RATE_CONTROL_OFF, pdn_evt.pdn.type);
	TEST_ASSERT_EQUAL(1, pdn_evt.pdn.cid);
}

void test_lte_lc_pdp_context_destroy_einval(void)
{
	int ret;

	ret = lte_lc_pdn_ctx_destroy(0);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = lte_lc_pdn_ctx_destroy(3); /* does not exist */
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_pdp_context_destroy_enoexec_eshutdown(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_eshutdown;
	k_free_fake.custom_fake = k_free_PDN1;

	ret = lte_lc_pdn_ctx_destroy(CID_1);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);

}

void test_lte_lc_pdp_context_destroy_enoexec(void)
{
	int ret;

	/* Context is destroyed locally regardless of modem error response so we recreate it here */
	test_lte_lc_pdp_context_create_cid1();

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_error;
	k_free_fake.custom_fake = k_free_PDN1;

	ret = lte_lc_pdn_ctx_destroy(CID_1);
	TEST_ASSERT_EQUAL(-ENOEXEC, ret);

}

void test_lte_lc_pdp_context_destroy(void)
{
	int ret;

	/* Context is destroyed locally regardless of modem error response so we recreate it here */
	test_lte_lc_pdp_context_create_cid1();

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgdcont_cid1;
	k_free_fake.custom_fake = k_free_PDN1;

	ret = lte_lc_pdn_ctx_destroy(CID_1);
	TEST_ASSERT_EQUAL(0, ret);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgdcont_cid2;
	k_free_fake.custom_fake = k_free_PDN2;

	ret = lte_lc_pdn_ctx_destroy(CID_2);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_lte_lc_pdn_esm_strerror(void)
{
	const char *str;

	str = lte_lc_pdn_esm_strerror(0x26);
	TEST_ASSERT_EQUAL_STRING("Network failure", str);

	str = lte_lc_pdn_esm_strerror(0xbad);
	TEST_ASSERT_EQUAL_STRING("<unknown>", str);
}

void test_lte_lc_pdn_dynamic_info_get_einval(void)
{
	int ret;

	ret = lte_lc_pdn_dynamic_info_get(0, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_pdn_dynamic_info_get_ebadmsg(void)
{
	int ret;
	struct lte_lc_pdn_dynamic_info pdn_info = {};

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_cgcontrdp_no_match;

	ret = lte_lc_pdn_dynamic_info_get(0, &pdn_info);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_lte_lc_pdn_dynamic_info_get(void)
{
	int ret;
	struct lte_lc_pdn_dynamic_info pdn_info = {};

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_cgcontrdp;

	ret = lte_lc_pdn_dynamic_info_get(0, &pdn_info);
	TEST_ASSERT_EQUAL(0, ret);

	TEST_ASSERT_EQUAL(1500, pdn_info.ipv4_mtu);
	TEST_ASSERT_EQUAL(1600, pdn_info.ipv6_mtu);
	TEST_ASSERT_EQUAL_MEMORY(ipv4_dns_primary, &pdn_info.dns_addr4_primary,
				 sizeof(ipv4_dns_primary));
	TEST_ASSERT_EQUAL_MEMORY(ipv4_dns_secondary, &pdn_info.dns_addr4_secondary,
				 sizeof(ipv4_dns_secondary));
	TEST_ASSERT_EQUAL_MEMORY(ipv6_dns_primary, &pdn_info.dns_addr6_primary,
				 sizeof(ipv6_dns_primary));
	TEST_ASSERT_EQUAL_MEMORY(ipv6_dns_secondary, &pdn_info.dns_addr6_secondary,
				 sizeof(ipv6_dns_secondary));
}

void test_lte_lc_pdn_dynamic_info_get_match_ipv4(void)
{
	int ret;
	struct lte_lc_pdn_dynamic_info pdn_info = {};

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_cgcontrdp_match_ipv4;

	ret = lte_lc_pdn_dynamic_info_get(0, &pdn_info);
	TEST_ASSERT_EQUAL(0, ret);

	TEST_ASSERT_EQUAL(1500, pdn_info.ipv4_mtu);
	TEST_ASSERT_EQUAL(0, pdn_info.ipv6_mtu);

	TEST_ASSERT_EQUAL_MEMORY(ipv4_dns_primary, &pdn_info.dns_addr4_primary,
				 sizeof(ipv4_dns_primary));
	TEST_ASSERT_EQUAL_MEMORY(ipv4_dns_secondary, &pdn_info.dns_addr4_secondary,
				 sizeof(ipv4_dns_secondary));
}

void test_lte_lc_pdn_dynamic_info_get_match_ipv6(void)
{
	int ret;
	struct lte_lc_pdn_dynamic_info pdn_info = {};

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_cgcontrdp_match_ipv6;

	ret = lte_lc_pdn_dynamic_info_get(0, &pdn_info);
	TEST_ASSERT_EQUAL(0, ret);

	TEST_ASSERT_EQUAL(0, pdn_info.ipv4_mtu);
	TEST_ASSERT_EQUAL(1600, pdn_info.ipv6_mtu);

	TEST_ASSERT_EQUAL_MEMORY(ipv6_dns_primary, &pdn_info.dns_addr6_primary,
				 sizeof(ipv6_dns_primary));
	TEST_ASSERT_EQUAL_MEMORY(ipv6_dns_secondary, &pdn_info.dns_addr6_secondary,
				 sizeof(ipv6_dns_secondary));
}

void test_lte_lc_cellular_profile_configure_einval(void)
{
	int ret;

	ret = lte_lc_cellular_profile_configure(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_cellular_profile_configure(void)
{
	int ret;
	struct lte_lc_cellular_profile profile = {
		.id = 0,
		.act = LTE_LC_ACT_NBIOT,
		.uicc = LTE_LC_UICC_PHYSICAL,
	};

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cellularprfl;

	ret = lte_lc_cellular_profile_configure(&profile);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_lte_lc_cellular_profile_configure_softsim(void)
{
	int ret;
	struct lte_lc_cellular_profile profile = {
		.id = 0,
		.act = LTE_LC_ACT_NBIOT,
		.uicc = LTE_LC_UICC_SOFTSIM,
	};

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cellularprfl_softsim;

	ret = lte_lc_cellular_profile_configure(&profile);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_lte_lc_cellular_profile_multiple_act(void)
{
	int ret;
	struct lte_lc_cellular_profile profile = {
		.id = 0,
		.act = LTE_LC_ACT_NBIOT | LTE_LC_ACT_LTEM,
		.uicc = LTE_LC_UICC_PHYSICAL,
	};

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cellularprfl_multiple_act;

	ret = lte_lc_cellular_profile_configure(&profile);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_pdn_cellular_profile_notif(void)
{
	k_malloc_fake.custom_fake = k_malloc_PDN1;

	on_cellularprfl("%CELLULARPRFL: 1");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_CELLULAR_PROFILE_ACTIVE, pdn_evt.type);
	TEST_ASSERT_EQUAL(1, pdn_evt.cellular_profile.profile_id);

	on_cellularprfl("%CELLULARPRFL: 0");
	TEST_ASSERT_EQUAL(LTE_LC_EVT_CELLULAR_PROFILE_ACTIVE, pdn_evt.type);
	TEST_ASSERT_EQUAL(0, pdn_evt.cellular_profile.profile_id);

	memset(&pdn_evt, 0, sizeof(struct lte_lc_evt));

	on_cellularprfl("%CELLULARPRFL: 3");
	TEST_ASSERT_EQUAL(0, pdn_evt.type);
	TEST_ASSERT_EQUAL(0, pdn_evt.cellular_profile.profile_id);
}

void test_pdn_cellular_profile_notif_no_callback(void)
{
	k_malloc_fake.custom_fake = k_malloc_PDN1;

	lte_lc_deregister_handler(lte_lc_event_handler);

	on_cellularprfl("%CELLULARPRFL: 1");

	TEST_ASSERT_EQUAL(0, pdn_evt.cellular_profile.profile_id);
	TEST_ASSERT_EQUAL(0, pdn_evt.type);
}

void test_lte_lc_cellular_profile_remove(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cellularprfl_remove_cid0;

	ret = lte_lc_cellular_profile_remove(CELLULAR_PROFILE_ID_0);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_lte_lc_cellular_profile_remove_invalid_id(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cellularprfl_remove_cid2;

	ret = lte_lc_cellular_profile_remove(2U);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_lte_lc_pdn_default_ctx_events_enable(void)
{
	int ret;

	k_malloc_fake.custom_fake = k_malloc_PDN0;
	k_free_fake.custom_fake = k_free_PDN0;

	pdn0.context_id = -1;

	ret = lte_lc_pdn_default_ctx_events_enable();
	TEST_ASSERT_EQUAL(0, ret);

	TEST_ASSERT_EQUAL(1, k_malloc_fake.call_count);
	TEST_ASSERT_EQUAL(0, pdn0.context_id);

	ret = lte_lc_pdn_default_ctx_events_enable();
	TEST_ASSERT_EQUAL(0, ret);

	TEST_ASSERT_EQUAL(1, k_malloc_fake.call_count);
	TEST_ASSERT_EQUAL(0, pdn0.context_id);
}

void test_lte_lc_pdn_default_ctx_events_disable(void)
{
	int ret;

	k_malloc_fake.custom_fake = k_malloc_PDN0;
	k_free_fake.custom_fake = k_free_PDN0;

	ret = lte_lc_pdn_default_ctx_events_disable();
	TEST_ASSERT_EQUAL(0, ret);

	TEST_ASSERT_EQUAL(1, k_free_fake.call_count);

	ret = lte_lc_pdn_default_ctx_events_enable();
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(1, k_malloc_fake.call_count);

	ret = lte_lc_pdn_default_ctx_events_disable();
	TEST_ASSERT_EQUAL(0, ret);

	TEST_ASSERT_EQUAL(2, k_free_fake.call_count);

	ret = lte_lc_pdn_default_ctx_events_disable();
	TEST_ASSERT_EQUAL(-EINVAL, ret);
	TEST_ASSERT_EQUAL(2, k_free_fake.call_count);
}

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}
