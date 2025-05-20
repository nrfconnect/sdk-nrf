/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * This file defines the network configuration when CONFIG_SLM_AUTO_CONNECT is enabled.
 */

/* Network-specific default system mode configured by %XSYSTEMMODE (refer to AT command manual) */
1,        /* LTE support */
1,        /* NB-IoT support */
1,        /* GNSS support, also define CONFIG_MODEM_ANTENNA if not Nordic DK */
0,        /* LTE preference */
/* Network-specific default PDN configured by +CGDCONT and +CGAUTH (refer to AT command manual) */
false,    /* PDP context definition required or not */
"",       /* PDP type: "IP", "IPV6", "IPV4V6", "Non-IP" */
"",       /* Access point name */
0,        /* PDP authentication protocol: 0(None), 1(PAP), 2(CHAP) */
"",       /* PDN connection authentication username */
""        /* PDN connection authentication password */
