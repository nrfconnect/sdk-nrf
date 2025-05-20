/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef GNSS_SUPL_SUPPORT_H_
#define GNSS_SUPL_SUPPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

/** This function will open an socket to the SUPL server
 *
 * @brief This function will create and open a socket to the SUPL servers
 *         that can be used by the SUPL clients read and write functions.
 *         The socket handle is available in the implementation .c file scope
 *         and a reference is therefor not passed to the function.
 *
 * @retval 0  If opening socket was successful.
 * @retval -1 If opening socket failed
 */
int open_supl_socket(void);

/** This function will close the socket previously opened
 *
 * @brief This function will close the socket previously by the
 *        open_supl_socket function.
 *
 */
void close_supl_socket(void);

/** Function used by the SUPL client library for logging
 *
 * @brief This function is the implementation for logger function
 *        used by the SUPL client library. In this example will this
 *        lines be passed unchanged to the logger in Zephyr. If logging
 *        fails a value less than 0 should be returned to let the
 *        client know.
 *
 * @retval 0  If the logging information is processed correctly
 * @retval -1 If the function failed
 */
int supl_logger(int level, const char *fmt, ...);

/** Read function for SUPL socket used by the SUPL client
 *
 * @brief As the application owns the SUPL server socket the application
 *        can let the SUPL library client read data from the SUPL server
 *        calling this function.
 *
 *
 * @param[in] buf       Is the buffer that the SUPL client want to put
 *                      data that are received from the server into.
 * @param[in] nbytes    Is how many bytes that the supplied buffer can take
 * @param[in] user_data Is data previously specified by the application
 *                      in the context structure.
 *
 * @retval >0 Is the number of bytes received
 * @retval <0 If some error occurred
 */
ssize_t supl_read(void *buf, size_t nbytes, void *user_data);

/** Write function for SUPL socket used by the SUPL client
 *
 * @brief As the application owns the SUPL server socket the application
 *        can let the SUPL library client write data to the SUPL server
 *        calling this function.
 *
 * @param[out] buf       Is the buffer containing the data to be written to
 *                       SUPL server
 *             nbytes    Is the number of bytes in the buffer to be written
 *                       to the SUPL server.
 *             user_data Is data previously specified by the application
 *                       in the context structure.
 *
 * @retval >0 Is the number of bytes written
 * @retval <0 If some error occurred
 */
ssize_t supl_write(const void *buf, size_t nbytes, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* GNSS_SUPL_SUPPORT_H_ */
