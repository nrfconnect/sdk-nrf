/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing OPs declarations for the
 * OSAL Layer of the Wi-Fi driver.
 */

#ifndef __OSAL_OPS_H__
#define __OSAL_OPS_H__

#include "osal_structs.h"


/**
 * @brief struct nrf_wifi_osal_ops - Ops to be provided by a specific OS implementation.
 *
 * This structure exposes Ops which need to be implemented by the underlying OS
 * in order for the WLAN driver to work. The Ops can be directly mapped to OS
 * primitives where a one-to-one mapping is available. In case a mapping is not
 * available, an equivalent function will need to be implemented and that
 * function will then need to be mapped to the corresponding Op.
 */
struct nrf_wifi_osal_ops {
	/**
	 * @brief Map IO memory into CPU space.
	 *
	 * @param addr The address of the IO memory.
	 * @param size The size of the IO memory.
	 * @return A pointer to the mapped IO memory.
	 */
	void *(*iomem_mmap)(unsigned long addr, unsigned long size);

	/**
	 * @brief Unmap IO memory from CPU space.
	 *
	 * @param addr A pointer to the mapped IO memory.
	 */
	void (*iomem_unmap)(volatile void *addr);

	/**
	 * @brief Read a 32-bit value from a device register using a memory mapped address.
	 *
	 * @param addr A pointer to the memory mapped address.
	 * @return The value read from the device register.
	 */
	unsigned int (*iomem_read_reg32)(const volatile void *addr);

	/**
	 * @brief Write a 32-bit value to a device register using a memory mapped address.
	 *
	 * @param addr A pointer to the memory mapped address.
	 * @param val The value to write to the device register.
	 */
	void (*iomem_write_reg32)(volatile void *addr, unsigned int val);

	/**
	 * @brief Copy data from memory mapped device memory to host memory.
	 *
	 * @param dest A pointer to the destination memory.
	 * @param src A pointer to the source memory.
	 * @param count The number of bytes to copy.
	 */
	void (*iomem_cpy_from)(void *dest, const volatile void *src, size_t count);

	/**
	 * @brief Copy data from host memory to memory mapped device memory.
	 *
	 * @param dest A pointer to the destination memory.
	 * @param src A pointer to the source memory.
	 * @param count The number of bytes to copy.
	 */
	void (*iomem_cpy_to)(volatile void *dest, const void *src, size_t count);

	/**
	 * @brief Read a 32-bit value from a QSPI device register.
	 *
	 * @param priv A pointer to the QSPI device private data.
	 * @param addr The address of the device register.
	 * @return The value read from the device register.
	 */
	unsigned int (*qspi_read_reg32)(void *priv, unsigned long addr);

	/**
	 * @brief Write a 32-bit value to a QSPI device register.
	 *
	 * @param priv A pointer to the QSPI device private data.
	 * @param addr The address of the device register.
	 * @param val The value to write to the device register.
	 */
	void (*qspi_write_reg32)(void *priv, unsigned long addr, unsigned int val);

	/**
	 * @brief Copy data from QSPI device memory to host memory.
	 *
	 * @param priv A pointer to the QSPI device private data.
	 * @param dest A pointer to the destination memory.
	 * @param addr The address of the device memory.
	 * @param count The number of bytes to copy.
	 */
	void (*qspi_cpy_from)(void *priv, void *dest, unsigned long addr, size_t count);

	/**
	 * @brief Copy data from host memory to QSPI device memory.
	 *
	 * @param priv A pointer to the QSPI device private data.
	 * @param addr The address of the device memory.
	 * @param src A pointer to the source memory.
	 * @param count The number of bytes to copy.
	 */
	void (*qspi_cpy_to)(void *priv, unsigned long addr, const void *src, size_t count);

	/**
	 * @brief Read a 32-bit value from a SPI device register.
	 *
	 * @param priv A pointer to the SPI device private data.
	 * @param addr The address of the device register.
	 * @return The value read from the device register.
	 */
	unsigned int (*spi_read_reg32)(void *priv, unsigned long addr);

	/**
	 * @brief Write a 32-bit value to a SPI device register.
	 *
	 * @param priv A pointer to the SPI device private data.
	 * @param addr The address of the device register.
	 * @param val The value to write to the device register.
	 */
	void (*spi_write_reg32)(void *priv, unsigned long addr, unsigned int val);

	/**
	 * @brief Copy data from SPI device memory to host memory.
	 *
	 * @param priv A pointer to the SPI device private data.
	 * @param dest A pointer to the destination memory.
	 * @param addr The address of the device memory.
	 * @param count The number of bytes to copy.
	 */
	void (*spi_cpy_from)(void *priv, void *dest, unsigned long addr, size_t count);

	/**
	 * @brief Copy data from host memory to SPI device memory.
	 *
	 * @param priv A pointer to the SPI device private data.
	 * @param addr The address of the device memory.
	 * @param src A pointer to the source memory.
	 * @param count The number of bytes to copy.
	 */
	void (*spi_cpy_to)(void *priv, unsigned long addr, const void *src, size_t count);


	/**
	 * @brief Log a debug message.
	 *
	 * @param fmt The format string of the message.
	 * @param args The arguments for the format string.
	 * @return The number of characters written.
	 */
	int (*log_dbg)(const char *fmt, va_list args);

	/**
	 * @brief Log an informational message.
	 *
	 * @param fmt The format string of the message.
	 * @param args The arguments for the format string.
	 * @return The number of characters written.
	 */
	int (*log_info)(const char *fmt, va_list args);

	/**
	 * @brief Log an error message.
	 *
	 * @param fmt The format string of the message.
	 * @param args The arguments for the format string.
	 * @return The number of characters written.
	 */
	int (*log_err)(const char *fmt, va_list args);

	/**
	 * @brief Delay for a specified number of microseconds.
	 *
	 * @param usecs The number of microseconds to delay.
	 * @return 0 on success, a negative value on failure.
	 */
	int (*delay_us)(int usecs);

	/**
	 * @brief Get the current time of the day in microseconds.
	 *
	 * @return The current time of the day in microseconds.
	 */
	unsigned long (*time_get_curr_us)(void);

	/**
	 * @brief Return the time elapsed in microseconds since a specified time instant.
	 *
	 * @param start_time The time instant to measure the elapsed time from.
	 * @return The time elapsed in microseconds.
	 */
	unsigned int (*time_elapsed_us)(unsigned long start_time);

	/** @brief Get the current time of the day in milliseconds.
	 *
	 * @return The current time of the day in milliseconds.
	 */
	unsigned long (*time_get_curr_ms)(void);

	/**
	 * @brief Return the time elapsed in milliseconds since a specified time instant.
	 *
	 * @param start_time The time instant to measure the elapsed time from.
	 * @return The time elapsed in milliseconds.
	 */
	unsigned int (*time_elapsed_ms)(unsigned long start_time_us);

	/**
	 * @brief Initialize the PCIe bus.
	 *
	 * @param dev_name The name of the PCIe device.
	 * @param vendor_id The vendor ID of the PCIe device.
	 * @param sub_vendor_id The sub-vendor ID of the PCIe device.
	 * @param device_id The device ID of the PCIe device.
	 * @param sub_device_id The sub-device ID of the PCIe device.
	 * @return A pointer to the initialized PCIe bus.
	 */
	void *(*bus_pcie_init)(const char *dev_name,
						   unsigned int vendor_id,
						   unsigned int sub_vendor_id,
						   unsigned int device_id,
						   unsigned int sub_device_id);

	/**
	 * @brief Deinitialize the PCIe bus.
	 *
	 * @param os_pcie_priv A pointer to the PCIe bus.
	 */
	void (*bus_pcie_deinit)(void *os_pcie_priv);

	/**
	 * @brief Add a PCIe device to the bus.
	 *
	 * @param pcie_priv A pointer to the PCIe bus.
	 * @param osal_pcie_dev_ctx A pointer to the PCIe device context.
	 * @return A pointer to the added PCIe device.
	 */
	void *(*bus_pcie_dev_add)(void *pcie_priv,
							  void *osal_pcie_dev_ctx);

	/**
	 * @brief Remove a PCIe device from the bus.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 */
	void (*bus_pcie_dev_rem)(void *os_pcie_dev_ctx);

	/**
	 * @brief Initialize a PCIe device.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 * @return The status of the initialization.
	 */
	enum nrf_wifi_status (*bus_pcie_dev_init)(void *os_pcie_dev_ctx);

	/**
	 * @brief Deinitialize a PCIe device.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 */
	void (*bus_pcie_dev_deinit)(void *os_pcie_dev_ctx);

	/**
	 * @brief Register an interrupt handler for a PCIe device.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 * @param callbk_data The data to be passed to the callback function.
	 * @param callback_fn The callback function to be invoked when an interrupt occurs.
	 * @return The status of the registration.
	 */
	enum nrf_wifi_status (*bus_pcie_dev_intr_reg)(void *os_pcie_dev_ctx,
			void *callbk_data,
			int (*callback_fn)(void *callbk_data));

	/**
	 * @brief Unregister the interrupt handler for a PCIe device.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 */
	void (*bus_pcie_dev_intr_unreg)(void *os_pcie_dev_ctx);

	/**
	 * @brief Map a DMA buffer for a PCIe device.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 * @param virt_addr The virtual address of the buffer.
	 * @param size The size of the buffer.
	 * @param dir The direction of the DMA transfer.
	 * @return A pointer to the mapped DMA buffer.
	 */
	void *(*bus_pcie_dev_dma_map)(void *os_pcie_dev_ctx,
								  void *virt_addr,
								  size_t size,
								  enum nrf_wifi_osal_dma_dir dir);

	/**
	 * @brief Unmap a DMA buffer for a PCIe device.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 * @param dma_addr The DMA address of the buffer.
	 * @param size The size of the buffer.
	 * @param dir The direction of the DMA transfer.
	 */
	void (*bus_pcie_dev_dma_unmap)(void *os_pcie_dev_ctx,
								   void *dma_addr,
								   size_t size,
								   enum nrf_wifi_osal_dma_dir dir);

	/**
	 * @brief Get the host mapping of a PCIe device.
	 *
	 * @param os_pcie_dev_ctx A pointer to the PCIe device.
	 * @param host_map A pointer to the host mapping structure.
	 */
	void (*bus_pcie_dev_host_map_get)(void *os_pcie_dev_ctx,
			struct nrf_wifi_osal_host_map *host_map);

	/**
	 * @brief Initialize the QSPI bus.
	 *
	 * @return A pointer to the initialized QSPI bus.
	 */
	void *(*bus_qspi_init)(void);

	/**
	 * @brief Deinitialize the QSPI bus.
	 *
	 * @param os_qspi_priv A pointer to the QSPI bus.
	 */
	void (*bus_qspi_deinit)(void *os_qspi_priv);

	/**
	 * @brief Add a QSPI device to the bus.
	 *
	 * @param qspi_priv A pointer to the QSPI bus.
	 * @param osal_qspi_dev_ctx A pointer to the QSPI device context.
	 * @return A pointer to the added QSPI device.
	 */
	void *(*bus_qspi_dev_add)(void *qspi_priv,
							  void *osal_qspi_dev_ctx);

	/**
	 * @brief Remove a QSPI device from the bus.
	 *
	 * @param os_qspi_dev_ctx A pointer to the QSPI device.
	 */
	void (*bus_qspi_dev_rem)(void *os_qspi_dev_ctx);

	/**
	 * @brief Initialize a QSPI device.
	 *
	 * @param os_qspi_dev_ctx A pointer to the QSPI device.
	 * @return The status of the initialization.
	 */
	enum nrf_wifi_status (*bus_qspi_dev_init)(void *os_qspi_dev_ctx);

	/**
	 * @brief Deinitialize a QSPI device.
	 *
	 * @param os_qspi_dev_ctx A pointer to the QSPI device.
	 */
	void (*bus_qspi_dev_deinit)(void *os_qspi_dev_ctx);

	/**
	 * @brief Register an interrupt handler for a QSPI device.
	 *
	 * @param os_qspi_dev_ctx A pointer to the QSPI device.
	 * @param callbk_data The data to be passed to the callback function.
	 * @param callback_fn The callback function to be invoked when an interrupt occurs.
	 * @return The status of the registration.
	 */
	enum nrf_wifi_status (*bus_qspi_dev_intr_reg)(void *os_qspi_dev_ctx,
			void *callbk_data,
			int (*callback_fn)(void *callbk_data));

	/**
	 * @brief Unregister the interrupt handler for a QSPI device.
	 *
	 * @param os_qspi_dev_ctx A pointer to the QSPI device.
	 */
	void (*bus_qspi_dev_intr_unreg)(void *os_qspi_dev_ctx);

	/**
	 * @brief Get the host mapping of a QSPI device.
	 *
	 * @param os_qspi_dev_ctx A pointer to the QSPI device.
	 * @param host_map A pointer to the host mapping structure.
	 */
	void (*bus_qspi_dev_host_map_get)(void *os_qspi_dev_ctx,
			struct nrf_wifi_osal_host_map *host_map);

	/**
	 * @brief Initialize the SPI bus.
	 *
	 * @return A pointer to the initialized SPI bus.
	 */
	void *(*bus_spi_init)(void);

	/**
	 * @brief Deinitialize the SPI bus.
	 *
	 * @param os_spi_priv A pointer to the SPI bus.
	 */
	void (*bus_spi_deinit)(void *os_spi_priv);

	/**
	 * @brief Add a SPI device to the bus.
	 *
	 * @param spi_priv A pointer to the SPI bus.
	 * @param osal_spi_dev_ctx A pointer to the SPI device context.
	 * @return A pointer to the added SPI device.
	 */
	void *(*bus_spi_dev_add)(void *spi_priv,
							 void *osal_spi_dev_ctx);

	/**
	 * @brief Remove a SPI device from the bus.
	 *
	 * @param os_spi_dev_ctx A pointer to the SPI device.
	 */
	void (*bus_spi_dev_rem)(void *os_spi_dev_ctx);

	/**
	 * @brief Initialize a SPI device.
	 *
	 * @param os_spi_dev_ctx A pointer to the SPI device.
	 * @return The status of the initialization.
	 */
	enum nrf_wifi_status (*bus_spi_dev_init)(void *os_spi_dev_ctx);

	/**
	 * @brief Deinitialize a SPI device.
	 *
	 * @param os_spi_dev_ctx A pointer to the SPI device.
	 */
	void (*bus_spi_dev_deinit)(void *os_spi_dev_ctx);

	/**
	 * @brief Register an interrupt handler for a SPI device.
	 *
	 * @param os_spi_dev_ctx A pointer to the SPI device.
	 * @param callbk_data The data to be passed to the callback function.
	 * @param callback_fn The callback function to be invoked when an interrupt occurs.
	 * @return The status of the registration.
	 */
	enum nrf_wifi_status (*bus_spi_dev_intr_reg)(void *os_spi_dev_ctx,
			void *callbk_data,
			int (*callback_fn)(void *callbk_data));

	/**
	 * @brief Unregister the interrupt handler for a SPI device.
	 *
	 * @param os_spi_dev_ctx A pointer to the SPI device.
	 */
	void (*bus_spi_dev_intr_unreg)(void *os_spi_dev_ctx);

	/**
	 * @brief Get the host mapping of a SPI device.
	 *
	 * @param os_spi_dev_ctx A pointer to the SPI device.
	 * @param host_map A pointer to the host mapping structure.
	 */
	void (*bus_spi_dev_host_map_get)(void *os_spi_dev_ctx,
			struct nrf_wifi_osal_host_map *host_map);

	#if defined(NRF_WIFI_LOW_POWER) || defined(__DOXYGEN__)
	/**
	 * @brief Put the QSPI bus to sleep.
	 *
	 * @param os_qspi_priv A pointer to the QSPI bus.
	 * @return 0 on success, a negative value on failure.
	 */
	int (*bus_qspi_ps_sleep)(void *os_qspi_priv);

	/**
	 * @brief Wake up the QSPI bus from sleep.
	 *
	 * @param os_qspi_priv A pointer to the QSPI bus.
	 * @return 0 on success, a negative value on failure.
	 */
	int (*bus_qspi_ps_wake)(void *os_qspi_priv);

	/**
	 * @brief Get the power state of the QSPI bus.
	 *
	 * @param os_qspi_priv A pointer to the QSPI bus.
	 * @return The power state of the QSPI bus.
	 */
	int (*bus_qspi_ps_status)(void *os_qspi_priv);
	#endif /* NRF_WIFI_LOW_POWER */

	/**
	 * @brief Assert a condition and display an error message if the condition is false.
	 *
	 * @param test_val The value to test.
	 * @param val The value to compare against.
	 * @param op The comparison operator.
	 * @param assert_msg The error message to display.
	 */
	void (*assert)(int test_val,
				   int val,
				   enum nrf_wifi_assert_op_type op,
				   char *assert_msg);

	/**
	 * @brief Get the length of a string.
	 *
	 * @param str A pointer to the string.
	 * @return The length of the string.
	 */
	unsigned int (*strlen)(const void *str);

	/**
	 * @brief Get a random 8-bit value.
	 *
	 * @return A random 8-bit value.
	 */
	unsigned char (*rand8_get)(void);
	int (*ipc_send_msg)(unsigned int msg_type, void *msg, unsigned int msg_len);
};
#endif /* __OSAL_OPS_H__ */
