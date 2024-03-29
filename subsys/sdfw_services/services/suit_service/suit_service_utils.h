/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_SERVICE_UTILS_H__
#define SUIT_SERVICE_UTILS_H__

/**
 * @brief Macro to generate SUIT SSF request choice field name.
 *
 * @details It is expected to be used inside test vector definition, so filling an SSF
 *          request structure will fit within the maximum line length limits.
 */
#define SSF_SUIT_REQ_CHOICE(name) suit_req_msg_suit_##name##_req_m_c

/**
 * @brief Macro to generate SUIT SSF request field name.
 *
 * @details It is expected to be used inside test vector definition, so filling an SSF
 *          request structure will fit within the maximum line length limits.
 */
#define SSF_SUIT_REQ(name) suit_req_msg_suit_##name##_req_m

/**
 * @brief Macro to generate SUIT SSF request argument field name.
 *
 * @details It is expected to be used inside test vector definition, so filling an SSF
 *          request structure will fit within the maximum line length limits.
 */
#define SSF_SUIT_REQ_ARG(name, arg) suit_##name##_req_##arg

/**
 * @brief Macro to generate SUIT SSF response choice field name.
 *
 * @details It is expected to be used inside test vector definition, so filling an SSF
 *          response structure will fit within the maximum line length limits.
 */
#define SSF_SUIT_RSP_CHOICE(name) suit_rsp_msg_suit_##name##_rsp_m_c

/**
 * @brief Macro to generate SUIT SSF response field name.
 *
 * @details It is expected to be used inside test vector definition, so filling an SSF
 *          response structure will fit within the maximum line length limits.
 */
#define SSF_SUIT_RSP(name) suit_rsp_msg_suit_##name##_rsp_m

/**
 * @brief Macro to generate SUIT SSF response argument field name.
 *
 * @details It is expected to be used inside test vector definition, so filling an SSF
 *          response structure will fit within the maximum line length limits.
 */
#define SSF_SUIT_RSP_ARG(name, arg) suit_##name##_rsp_##arg

#endif /* SUIT_SERVICE_UTILS_H__ */
