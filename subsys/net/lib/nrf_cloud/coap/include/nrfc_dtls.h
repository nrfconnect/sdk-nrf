/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRFC_DTLS_H_
#define NRFC_DTLS_H_

/** @brief Set up DTLS on socket.
 *
 *  @param sock - an open network socket.
 *  @return 0 if the operation succeeded, otherwise, a negative error code.
 */
int nrfc_dtls_setup(int sock);

/** @brief Determine if CID is in use.
 *
 *  If connection ID (CID) is available, an open LTE network socket with an active
 *  or sleeping (PSM, eDRX) LTE connection will be usuable for a long time,
 *  even if there is no network traffic for long periods. Only one full
 *  DTLS handshake will be required. Without CID, NAT translation changes
 *  between the device and the server will occur frequently (~30 seconds),
 *  resulting in no response from the server to a device's request.
 *
 *  @param sock - the socket previously setup for DTLS using dtls_setup().
 *  @retval true - CID is in use.
 *  @retval false - CID is not in use.
 */
bool nrfc_dtls_cid_is_active(int sock);

/** @brief Save DTLS CID session.
 *
 *  This function temporarily pauses a DTLS CID connection in the modem so that
 *  another socket can be opened and used. Once the new socket is no longer needed,
 *  close that one, then use dtls_session_load() to resume using the specified
 *  socket, sock. The socket sock must not be closed, and the LTE connection must
 *  not be shut down, or else the requisite data for the socket will be discarded,
 *  and the DTLS CID connection cannot be reused. In that case, close the socket,
 *  reopen it, and set it up again with dtls_setup(), resulting in a full
 *  DTLS handshake.
 *
 *  @param sock - the socket to save.
 *  @return 0 if successful, a negative error code otherwise.
 */
int nrfc_dtls_session_save(int sock);

/** @brief Load DTLS CID session.
 *
 *  This resumes a previously saved session.
 *
 *  @param sock - the socket to load.
 *  @return 0 if successful, a negative error code otherwise.
 */
int nrfc_dtls_session_load(int sock);

/** @brief Check if you can use SO_KEEPOPEN.
 *
 * Modem firmware < 2.0.1 does not support this.
 *
 * @retval true - SO_KEEPOPEN can be used.
 * @retval false - it cannot be used.
 */
bool nrfc_keepopen_is_supported(void);

#endif /* NRFC_DTLS_H_ */
