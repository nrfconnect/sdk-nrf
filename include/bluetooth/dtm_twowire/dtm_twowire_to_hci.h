/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DTM_TWOWIRE_TO_HCI_H_
#define DTM_TWOWIRE_TO_HCI_H_

#include <stdint.h>
#include <zephyr/net_buf.h>

/**
 * @file
 * @defgroup dtm_twowire_to_hci Bluetooth DTM 2-wire UART to HCI Conversion API
 * @{
 * @brief API for the Bluetooth DTM 2-wire UART to HCI conversion library.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	/* Returned when a 2-wire event was generated */
	DTM_TW_TO_HCI_STATUS_TW_EVENT  = 0,
	/* Returned when an HCI command was generated */
	DTM_TW_TO_HCI_STATUS_HCI_CMD   = 1,
	/* Returned when the provided HCI event was irrelevant to DTM */
	DTM_TW_TO_HCI_STATUS_UNHANDLED = 2,
	/* Returned when an error was encountered while processing the command or event */
	DTM_TW_TO_HCI_STATUS_ERROR     = 3
} dtm_tw_to_hci_status_t;

/** @brief Process a DTM 2-wire command and generate either a 2-wire event or an HCI command.
 *
 *  @note CTE and antenna switching features are not currently supported by this library.
 *
 *  @param[in] tw_cmd     2-wire command to process.
 *  @param[out] hci_cmd   Buffer for the generated HCI command (allocated by caller), if any.
 *  @param[out] tw_event  Pointer to the generated 2-wire event, if any.
 *
 *  @return Status of the processing result.
 *  @retval DTM_TW_TO_HCI_STATUS_TW_EVENT A 2-wire event was generated and stored in @p tw_event .
 *  @retval DTM_TW_TO_HCI_STATUS_HCI_CMD  An HCI command was generated and stored in @p hci_cmd .
 *  @retval DTM_TW_TO_HCI_STATUS_ERROR    An error was encountered during processing.
 *                                        No output was generated.
 */
dtm_tw_to_hci_status_t dtm_tw_to_hci_process_tw_cmd(const uint16_t tw_cmd, struct net_buf *hci_cmd,
						    uint16_t *tw_event);

/** @brief Process an HCI event and generate a DTM 2-wire event.
 *
 *  @param[in] tw_cmd       2-wire command that the caller is expecting a response to.
 *  @param[in] hci_event    HCI event to process.
 *  @param[out] tw_event    Pointer to the generated 2-wire event, if any.
 *
 *  @return Status of the processing result.
 *  @retval DTM_TW_TO_HCI_STATUS_TW_EVENT   A 2-wire event was generated and stored in @p tw_event .
 *  @retval DTM_TW_TO_HCI_STATUS_UNHANDLED  The HCI event was irrelevant to DTM.
 *                                          No output was generated.
 *  @retval DTM_TW_TO_HCI_STATUS_ERROR      An error was encountered during processing.
 *                                          No output was generated.
 */
dtm_tw_to_hci_status_t dtm_tw_to_hci_process_hci_event(const uint16_t tw_cmd,
						       const struct net_buf *hci_event,
						       uint16_t *tw_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* DTM_TWOWIRE_TO_HCI_H_ */
