/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
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

#include "pdn.h"

DEFINE_FFF_GLOBALS;

#define CID_1 1
#define CID_2 2
#define CID_MAX 0x7f
#define CID_MIN -0x80

#define PDN_AUTH_DEFAULT PDN_AUTH_PAP
#define AUTH_USR_DEFAULT "usr"
#define AUTH_PWD_DEFAULT "pwd"

static int pdn_evt_reason;
uint8_t pdn_evt_cid;
enum pdn_event pdn_evt_event;

struct pdn pdn1;
struct pdn pdn2;

/* Defined in pdn.c, we redefine it here */
struct pdn {
	sys_snode_t node; /* list handling */
	pdn_event_handler_t callback;
	int8_t context_id;
};

FAKE_VALUE_FUNC(void *, k_malloc, size_t);
FAKE_VOID_FUNC(k_free, void *);
FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_scanf, const char *, const char *, ...);
FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_printf, const char *, ...);
FAKE_VALUE_FUNC_VARARG(int, nrf_modem_at_cmd, void *, size_t, const char *, ...);

extern void on_cgev(const char *notif);
extern void on_cnec_esm(const char *notif);

static void *k_malloc_NULL(size_t size)
{
	return NULL;
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

static void k_free_PDN1(void *data)
{
	TEST_ASSERT_EQUAL_PTR(&pdn1, data);
}

static void k_free_PDN2(void *data)
{
	TEST_ASSERT_EQUAL_PTR(&pdn2, data);
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

#define RESP_AT_CMD_ERROR "ERROR"

static int nrf_modem_at_cmd_error(void *buf, size_t len, const char *cmd, va_list args)
{
	ARG_UNUSED(cmd);
	ARG_UNUSED(args);

	TEST_ASSERT_TRUE(len >= sizeof(RESP_AT_CMD_ERROR));

	memcpy(buf, RESP_AT_CMD_ERROR, sizeof(RESP_AT_CMD_ERROR));

	return 65536; /* Modem respond "ERROR" */
}

#define CMD_CGDCONT_CID "AT+CGDCONT=%u"
#define CMD_CGDCONT_CID_FAM "AT+CGDCONT=%u,%s"
#define CMD_CGDCONT_CID_FAM_APN "AT+CGDCONT=%u,%s,%s"
#define CMD_CGDCONT_CID_FAM_APN_OPT "AT+CGDCONT=%u,%s,%s,,,,%u,,,,%u,%u"

#define FAM_IPV4_STR "IP"
#define FAM_IPV6_STR "IPV6"
#define FAM_IPV4V6_STR "IPV4V6"
#define FAM_NONIP_STR "Non-IP"
static const char apn_default[] = "apn0";

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
	TEST_ASSERT_EQUAL_STRING(apn_default, apn);

	return 0;
}

#define OPT_IPv4_ADDR_ALLOC_EXP 0xbe
#define OPT_NSLPI_EXP 0x1
#define OPT_SECURE_PCO_EXP 0xC0

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
	TEST_ASSERT_EQUAL_STRING(apn_default, apn);

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
	enum pdn_auth method;
	const char *usr;
	const char *pwd;

	TEST_ASSERT_EQUAL_STRING("AT+CGAUTH=%u,%d,%s,%s", cmd);

	cid = va_arg(args, int);
	TEST_ASSERT_EQUAL(CID_1, cid);

	method = va_arg(args, int);
	TEST_ASSERT_EQUAL(PDN_AUTH_PAP, method);

	usr = va_arg(args, char *);
	TEST_ASSERT_EQUAL_STRING(AUTH_USR_DEFAULT, usr);

	pwd = va_arg(args, char *);
	TEST_ASSERT_EQUAL_STRING(AUTH_PWD_DEFAULT, pwd);

	return 0;
}

#define CMD_CGACT "AT+CGACT=%u,%u"

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

	/* Bad format */
	on_cgev("+CGEV: ME PDN ACT1\r\n");

	/* different CID*/
	on_cgev("+CGEV: ME PDN ACT 0\r\n");

	on_cgev("+CGEV: ME PDN ACT 1,0\r\n");

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

	/* Bad format */
	on_cgev("+CGEV: ME PDN ACT1\r\n");

	/* different CID*/
	on_cgev("+CGEV: ME PDN ACT 0\r\n");

	on_cgev("+CGEV: ME PDN ACT 1,1\r\n");

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

	on_cnec_esm("+CNEC_ESM: 2,1");
	on_cnec_esm("+CNEC_ESM: 2,2");

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

	on_cnec_esm("+CNEC_ESM: 2,2");

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

static int nrf_modem_at_printf_xepco(const char *cmd, va_list args)
{
	TEST_ASSERT_EQUAL_STRING("AT%%XEPCO=1", cmd);

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
	TEST_ASSERT_EQUAL_STRING("+CGDCONT: 0,\"%*[^\"]\",\"%64[^\"]\"", fmt);

	apn = va_arg(args, int *);
	*apn = 1;

	return 1;
}

#define CMD_XGETPDNID "AT%%XGETPDNID=%u"
#define RESP_XGETPDNID "%XGETPDNID: 1"

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

static int nrf_modem_at_cmd_cgetpdnid_bad_resp(void *buf, size_t len, const char *cmd, va_list args)
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

static void pdn_event_handler(uint8_t cid, enum pdn_event event, int reason)
{
	pdn_evt_cid = cid;
	pdn_evt_reason = reason;
	pdn_evt_event = event;
}

void setUp(void)
{
	RESET_FAKE(k_malloc);
	RESET_FAKE(k_free);
	RESET_FAKE(k_free);
	RESET_FAKE(nrf_modem_at_printf);
	RESET_FAKE(nrf_modem_at_scanf);
	RESET_FAKE(nrf_modem_at_cmd);

	pdn_evt_cid = 0;
	pdn_evt_event = PDN_EVENT_CNEC_ESM;
	pdn_evt_reason = 0;
}

void tearDown(void)
{
}

void test_pdn_pdn_default_ctx_cb_reg_efault(void)
{
	int ret;

	ret = pdn_default_ctx_cb_reg(NULL);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_pdn_pdn_default_ctx_cb_reg_enomem(void)
{
	int ret;

	k_malloc_fake.custom_fake = k_malloc_NULL;

	ret = pdn_default_ctx_cb_reg(pdn_event_handler);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_pdn_pdn_default_ctx_cb_reg(void)
{
	int ret;

	k_malloc_fake.custom_fake = k_malloc_PDN1;

	ret = pdn_default_ctx_cb_reg(pdn_event_handler);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_pdn_default_ctx_cb_dereg_einval(void)
{
	int ret;

	ret = pdn_default_ctx_cb_dereg(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_pdn_default_ctx_cb_dereg(void)
{
	int ret;

	k_free_fake.custom_fake = k_free_PDN1;

	ret = pdn_default_ctx_cb_dereg(pdn_event_handler);
	TEST_ASSERT_EQUAL(0, ret);

	/* Context is removed */
	ret = pdn_default_ctx_cb_dereg(pdn_event_handler);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_pdn_ctx_create_eshutdown(void)
{
	int ret;
	uint8_t cid = 0;

	k_malloc_fake.custom_fake = k_malloc_PDN1;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_eshutdown;

	ret = pdn_ctx_create(&cid, pdn_event_handler);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
}

void test_pdn_ctx_create_efault(void)
{
	int ret;
	uint8_t cid = 0;

	ret = pdn_ctx_create(NULL, pdn_event_handler);
	TEST_ASSERT_EQUAL(-EFAULT, ret);

	k_malloc_fake.custom_fake = k_malloc_PDN1;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_no_match;

	ret = pdn_ctx_create(&cid, pdn_event_handler);
	TEST_ASSERT_EQUAL(-EFAULT, ret);

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_xnewcid_too_big;

	ret = pdn_ctx_create(&cid, pdn_event_handler);
	TEST_ASSERT_EQUAL(-EFAULT, ret);

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_xnewcid_too_small;

	ret = pdn_ctx_create(&cid, pdn_event_handler);
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_pdn_ctx_create_enomem(void)
{
	int ret;
	uint8_t cid = 1;

	k_malloc_fake.custom_fake = k_malloc_NULL;

	ret = pdn_ctx_create(&cid, pdn_event_handler);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_pdn_ctx_create_at_cmd_scanf_eshutdown(void)
{
	int ret;
	uint8_t cid = 0;

	k_malloc_fake.custom_fake = k_malloc_PDN1;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_eshutdown;

	ret = pdn_ctx_create(&cid, pdn_event_handler);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
}

void test_pdn_ctx_create_cid1(void)
{
	int ret;
	uint8_t cid = CID_1;

	k_malloc_fake.custom_fake = k_malloc_PDN1;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_xnewcid_1;

	ret = pdn_ctx_create(&cid, pdn_event_handler);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_pdn_ctx_create_cid2(void)
{
	int ret;
	uint8_t cid = CID_2;

	k_malloc_fake.custom_fake = k_malloc_PDN2;
	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_xnewcid_2;

	ret = pdn_ctx_create(&cid, NULL);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_pdn_ctx_configure_einval(void)
{
	int ret;
	/* APN_STR_MAX_LEN + 1 */
	static const char apn_toobig[] = "loremipsumdolorsitametconsectet "
					 "uradipiscingelitnamaceleifendaug";
	enum pdn_fam fam = PDN_FAM_IPV4;
	struct pdn_pdp_opt opt;

	ret = pdn_ctx_configure(CID_1, apn_toobig, fam, &opt);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	fam = 4; /* Invalid */
	ret = pdn_ctx_configure(CID_1, apn_default, fam, &opt);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_pdn_ctx_configure_eshutdown(void)
{
	int ret;
	enum pdn_fam fam = PDN_FAM_IPV4;
	struct pdn_pdp_opt opt;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_eshutdown;

	ret = pdn_ctx_configure(CID_1, apn_default, fam, &opt);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
}

void test_pdn_ctx_configure_enoexec(void)
{
	int ret;
	enum pdn_fam fam = PDN_FAM_IPV4;
	struct pdn_pdp_opt opt;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_error;

	ret = pdn_ctx_configure(CID_1, apn_default, fam, &opt);
	TEST_ASSERT_EQUAL(-ENOEXEC, ret);
}

void test_pdn_ctx_configure(void)
{
	int ret;
	struct pdn_pdp_opt opt = {
		.ip4_addr_alloc =  OPT_IPv4_ADDR_ALLOC_EXP,
		.nslpi = OPT_NSLPI_EXP,
		.secure_pco = OPT_SECURE_PCO_EXP
	};

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv4;

	ret = pdn_ctx_configure(CID_1, NULL, PDN_FAM_IPV4, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv6;

	ret = pdn_ctx_configure(CID_1, NULL, PDN_FAM_IPV6, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv4v6;

	ret = pdn_ctx_configure(CID_1, NULL, PDN_FAM_IPV4V6, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cmd_cgdcont_cid_fam_nonip;

	ret = pdn_ctx_configure(CID_1, NULL, PDN_FAM_NONIP, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv4_apn;

	ret = pdn_ctx_configure(CID_1, apn_default, PDN_FAM_IPV4, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cmd_cgdcont_cid_fam_ipv4_apn_opt;

	ret = pdn_ctx_configure(CID_1, apn_default, PDN_FAM_IPV4, &opt);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_pdn_ctx_auth_set_einval(void)
{
	int ret;

	static const char user[] = "usr";
	static const char password[] = "pwd";

	ret = pdn_ctx_auth_set(CID_1, PDN_AUTH_PAP, NULL, password);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = pdn_ctx_auth_set(CID_1, PDN_AUTH_PAP, user, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = pdn_ctx_auth_set(CID_1, PDN_AUTH_CHAP, NULL, password);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = pdn_ctx_auth_set(CID_1, PDN_AUTH_CHAP, user, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}


void test_pdn_ctx_auth_set_enoexec(void)
{
	int ret;

	const char user[] = AUTH_USR_DEFAULT;
	const char password[] = AUTH_PWD_DEFAULT;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_error;

	ret = pdn_ctx_auth_set(CID_1, PDN_AUTH_DEFAULT, user, password);
	TEST_ASSERT_EQUAL(-ENOEXEC, ret);
}

void test_pdn_ctx_auth_set_eshutdown(void)
{
	int ret;

	const char user[] = AUTH_USR_DEFAULT;
	const char password[] = AUTH_PWD_DEFAULT;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_eshutdown;

	ret = pdn_ctx_auth_set(CID_1, PDN_AUTH_DEFAULT, user, password);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
}

void test_pdn_ctx_auth_set(void)
{
	int ret;

	const char user[] = AUTH_USR_DEFAULT;
	const char password[] = AUTH_PWD_DEFAULT;

	ret = pdn_ctx_auth_set(CID_1, PDN_AUTH_NONE, NULL, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgauth;

	ret = pdn_ctx_auth_set(CID_1, PDN_AUTH_DEFAULT, user, password);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_pdn_activate_esm_timeout(void)
{
	int ret;
	int esm = 1;
	enum pdn_fam fam;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_ipv4;

	ret = pdn_activate(CID_1, &esm, &fam);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(0, esm);
}

void test_pdn_activate_eshutdown(void)
{
	int ret;
	int esm = 1;
	enum pdn_fam fam;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_eshutdown;

	ret = pdn_activate(CID_1, &esm, &fam);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
	TEST_ASSERT_EQUAL(0, esm);
}

void test_pdn_activate_error(void)
{
	int ret;
	int esm = 1;
	enum pdn_fam fam;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_error;

	ret = pdn_activate(CID_1, &esm, &fam);
	TEST_ASSERT_EQUAL(-ENOEXEC, ret);
	TEST_ASSERT_EQUAL(0, esm);
}

void test_pdn_activate_esm_bad_format(void)
{
	int ret;
	int esm = 1;
	enum pdn_fam fam;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_ipv4;

	ret = pdn_activate(CID_1, NULL, &fam);
	TEST_ASSERT_EQUAL(0, ret);
	/* esm unchanged */
	TEST_ASSERT_EQUAL(1, esm);

	TEST_ASSERT_EQUAL(CID_1, pdn_evt_cid);
	TEST_ASSERT_EQUAL(PDN_EVENT_ACTIVATED, pdn_evt_event); /* Activated*/

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_esm_bad_format;

	ret = pdn_activate(CID_1, &esm, &fam);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(0, esm);
	TEST_ASSERT_EQUAL(0, pdn_evt_reason);

	TEST_ASSERT_EQUAL(CID_1, pdn_evt_cid);
	TEST_ASSERT_EQUAL(PDN_EVENT_CNEC_ESM, pdn_evt_event); /* Activated*/
}

void test_pdn_activate(void)
{
	int ret;
	int esm = 1;
	enum pdn_fam fam;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_fam_unknown;

	ret = pdn_activate(CID_1, NULL, NULL);
	TEST_ASSERT_EQUAL(0, ret);
	/* esm unchanged */
	TEST_ASSERT_EQUAL(1, esm);

	TEST_ASSERT_EQUAL(CID_1, pdn_evt_cid);
	TEST_ASSERT_EQUAL(PDN_EVENT_ACTIVATED, pdn_evt_event);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_ipv4;

	ret = pdn_activate(CID_1, NULL, &fam);
	TEST_ASSERT_EQUAL(0, ret);
	/* esm unchanged */
	TEST_ASSERT_EQUAL(1, esm);
	TEST_ASSERT_EQUAL(PDN_FAM_IPV4, fam);

	TEST_ASSERT_EQUAL(CID_1, pdn_evt_cid);
	TEST_ASSERT_EQUAL(PDN_EVENT_ACTIVATED, pdn_evt_event);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_ipv6;

	ret = pdn_activate(CID_1, NULL, &fam);
	TEST_ASSERT_EQUAL(0, ret);
	/* esm unchanged */
	TEST_ASSERT_EQUAL(1, esm);
	TEST_ASSERT_EQUAL(PDN_FAM_IPV6, fam);

	TEST_ASSERT_EQUAL(CID_1, pdn_evt_cid);
	TEST_ASSERT_EQUAL(PDN_EVENT_ACTIVATED, pdn_evt_event);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_esm_cid1;

	ret = pdn_activate(CID_1, &esm, &fam);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(2, esm);
	TEST_ASSERT_EQUAL(2, pdn_evt_reason);

	TEST_ASSERT_EQUAL(CID_1, pdn_evt_cid);
	TEST_ASSERT_EQUAL(PDN_EVENT_CNEC_ESM, pdn_evt_event);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_activate_esm_cid2;
	pdn_evt_cid = 0;
	pdn_evt_event = 0;

	ret = pdn_activate(CID_2, &esm, &fam);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(2, esm);
	TEST_ASSERT_EQUAL(2, pdn_evt_reason);

	/* no callback */
	TEST_ASSERT_EQUAL(0, pdn_evt_cid);
	TEST_ASSERT_EQUAL(0, pdn_evt_event);
}


void test_pdn_deactivate_eshutdown(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_eshutdown;

	ret = pdn_deactivate(CID_1);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
}

void test_pdn_deactivate_error(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_error;

	ret = pdn_deactivate(CID_1);
	TEST_ASSERT_EQUAL(-ENOEXEC, ret);
}

void test_pdn_deactivate(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgact_deactivate;

	ret = pdn_deactivate(CID_1);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_EQUAL(PDN_EVENT_DEACTIVATED, pdn_evt_event);
}


void test_pdn_id_get_eshutdown(void)
{
	int ret;

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_eshutdown;

	ret = pdn_id_get(CID_1);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
}

void test_pdn_id_get_ebadmsg(void)
{
	int ret;

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_cgetpdnid_bad_resp;

	ret = pdn_id_get(CID_1);
	TEST_ASSERT_EQUAL(-EBADMSG, ret);
}

void test_pdn_id_get_error(void)
{
	int ret;

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_error;

	ret = pdn_id_get(CID_1);
	TEST_ASSERT_EQUAL(-ENOEXEC, ret);
}

void test_pdn_id_get(void)
{
	int ret;

	nrf_modem_at_cmd_fake.custom_fake = nrf_modem_at_cmd_xgetpdnid;

	ret = pdn_id_get(CID_1);
	TEST_ASSERT_EQUAL(1, ret);
}

void test_pdn_default_apn_get_eshutdown(void)
{
	int ret;
	char buf[64];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_eshutdown;

	ret = pdn_default_apn_get(buf, sizeof(buf));
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);
}

void test_pdn_default_apn_get_e2big(void)
{
	int ret;
	char buf[1];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_cgdcont;

	ret = pdn_default_apn_get(buf, sizeof(buf));
	TEST_ASSERT_EQUAL(-E2BIG, ret);
}

void test_pdn_default_apn_get_no_match(void)
{
	int ret;
	char buf[64];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_no_match;

	ret = pdn_default_apn_get(buf, sizeof(buf));
	TEST_ASSERT_EQUAL(-EFAULT, ret);
}

void test_pdn_default_apn_get(void)
{
	int ret;
	char buf[64];

	nrf_modem_at_scanf_fake.custom_fake = nrf_modem_at_scanf_custom_cgdcont;

	ret = pdn_default_apn_get(buf, sizeof(buf));
	TEST_ASSERT_EQUAL(0, ret);
}

void test_pdn_apn_rate_control_activate(void)
{
	on_cgev("+CGEV: APNRATECTRL STAT 2,1");
	/* no callback */
	TEST_ASSERT_EQUAL(0, pdn_evt_cid);
	TEST_ASSERT_EQUAL(0, pdn_evt_event);

	on_cgev("+CGEV: APNRATECTRL STAT 1,1");

	TEST_ASSERT_EQUAL(CID_1, pdn_evt_cid);
	TEST_ASSERT_EQUAL(PDN_EVENT_APN_RATE_CONTROL_ON, pdn_evt_event);
}

void test_pdn_apn_rate_control_deactivate(void)
{
	on_cgev("+CGEV: APNRATECTRL STAT 2,0");
	/* no callback */
	TEST_ASSERT_EQUAL(0, pdn_evt_cid);
	TEST_ASSERT_EQUAL(0, pdn_evt_event);

	on_cgev("+CGEV: APNRATECTRL STAT 1,0");

	TEST_ASSERT_EQUAL(CID_1, pdn_evt_cid);
	TEST_ASSERT_EQUAL(PDN_EVENT_APN_RATE_CONTROL_OFF, pdn_evt_event);
}

void test_pdn_ctx_destroy_einval(void)
{
	int ret;

	ret = pdn_ctx_destroy(0);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = pdn_ctx_destroy(3); /* does not exist */
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_pdn_ctx_destroy_enoexec_eshutdown(void)
{
	int ret;

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_eshutdown;
	k_free_fake.custom_fake = k_free_PDN1;

	ret = pdn_ctx_destroy(CID_1);
	TEST_ASSERT_EQUAL(-ESHUTDOWN, ret);

}

void test_pdn_ctx_destroy_enoexec(void)
{
	int ret;

	/* Context is destroyed locally regardless of modem error response so we recreate it here */
	test_pdn_ctx_create_cid1();

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_error;
	k_free_fake.custom_fake = k_free_PDN1;

	ret = pdn_ctx_destroy(CID_1);
	TEST_ASSERT_EQUAL(-ENOEXEC, ret);

}

void test_pdn_ctx_destroy(void)
{
	int ret;

	/* Context is destroyed locally regardless of modem error response so we recreate it here */
	test_pdn_ctx_create_cid1();

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgdcont_cid1;
	k_free_fake.custom_fake = k_free_PDN1;

	ret = pdn_ctx_destroy(CID_1);
	TEST_ASSERT_EQUAL(0, ret);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_cgdcont_cid2;
	k_free_fake.custom_fake = k_free_PDN2;

	ret = pdn_ctx_destroy(CID_2);
	TEST_ASSERT_EQUAL(0, ret);
}

/* We dont add the PDN and PDN_ESM_STRERROR Kconfigs so we add this as extern. */
extern const char *pdn_esm_strerror(int reason);

void test_pdn_esm_strerror(void)
{
	const char *str;

	str = pdn_esm_strerror(0x26);
	TEST_ASSERT_EQUAL_STRING("Network failure", str);

	str = pdn_esm_strerror(0xbad);
	TEST_ASSERT_EQUAL_STRING("<unknown>", str);
}

extern void on_modem_init(int ret, void *ctx);
void test_on_modem_init(void)
{
	/* we don't expect anything to happen in this case. */
	on_modem_init(-ESHUTDOWN, NULL);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_eshutdown;

	on_modem_init(0, NULL);

	nrf_modem_at_printf_fake.custom_fake = nrf_modem_at_printf_xepco;

	on_modem_init(0, NULL);
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
