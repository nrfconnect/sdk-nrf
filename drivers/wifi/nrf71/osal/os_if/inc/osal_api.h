/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing declarations for the
 * OSAL Layer of the Wi-Fi driver.
 */

#ifndef __OSAL_API_H__
#define __OSAL_API_H__

#include "osal_structs.h"


/**
 * @brief Initialize the OSAL layer.
 * @param ops: Pointer to the OSAL operations structure.
 *
 * Initializes the OSAL layer and is expected to be called
 * before using the OSAL layer.
 */
void nrf_wifi_osal_init(const struct nrf_wifi_osal_ops *ops);

/**
 * @brief Deinitialize the OSAL layer.
 *
 * Deinitialize the OSAL layer and is expected to be called after done using
 * the OSAL layer.
 */
void nrf_wifi_osal_deinit(void);

/**
 * @brief Memory map IO memory into CPU space.
 * @param addr Address of the IO memory to be mapped.
 * @param size Size of the IO memory in bytes.
 *
 * Maps IO memory of @p size bytes pointed to by @p addr into CPU space.
 *
 * @return Pointer to the mapped IO memory on success, NULL on error.
 */
void *nrf_wifi_osal_iomem_mmap(unsigned long addr,
			       unsigned long size);

/**
 * @brief Unmap previously mapped IO memory from CPU space.
 * @param addr Pointer to mapped IO memory to be unmapped.
 *
 * Unmaps IO memory from CPU space that was mapped using nrf_wifi_osal_iomem_mmap.
 */
void nrf_wifi_osal_iomem_unmap(volatile void *addr);

/**
 * @brief Read value from a 32 bit IO memory mapped register.
 * @param addr Pointer to the IO memory mapped register address.
 *
 * @return 32 bit value read from register.
 */
unsigned int nrf_wifi_osal_iomem_read_reg32(const volatile void *addr);

/**
 * @brief Write a 32 bit value to a IO memory mapped register.
 * @param addr Pointer to the IO memory mapped register address.
 * @param val Value to be written to the register.
 *
 * Writes a 32 bit value (val) to a 32 bit device register using a memory
 * mapped address (addr).
 */
void nrf_wifi_osal_iomem_write_reg32(volatile void *addr,
				     unsigned int val);

/**
 * @brief Copy data from the memory of a memory mapped IO device to host memory.
 * @param dest Pointer to the host memory where data is to be copied.
 * @param src Pointer to the memory of the memory mapped IO device from where data is to be copied.
 * @param count The size of the data to be copied in bytes.
 *
 * Copies a block of data of size  count bytes from memory mapped device memory(src)
 * to host memory(dest).
 */
void nrf_wifi_osal_iomem_cpy_from(void *dest,
				  const volatile void *src,
				  size_t count);

/**
 * @brief Copy data to the memory of a memory mapped IO device from host memory.
 * @param dest: Pointer to the memory of the memory mapped IO device where data is to be copied.
 * @param src: Pointer to the host memory from where data is to be copied.
 * @param count: The size of the data to be copied in bytes.
 *
 * Copies a block of data of size  count bytes from host memory (src) to memory mapped
 * device memory(dest).
 */
void nrf_wifi_osal_iomem_cpy_to(volatile void *dest,
				const void *src,
				size_t count);


/**
 * @brief Get current system uptime in microseconds.
 *
 * Get the current system uptime in microseconds.
 *
 * @return System uptime in microseconds.
 */
unsigned long nrf_wifi_osal_time_get_curr_us(void);

/**
 * @brief Get elapsed time in microseconds.
 * @param start_time_us The timestamp in microseconds from which elapsed time is to be measured.
 *
 * Get the elapsed system uptime in microseconds.
 *
 * @return Elapsed time in microseconds.
 */
unsigned int nrf_wifi_osal_time_elapsed_us(unsigned long start_time_us);

/**
 * nrf_wifi_osal_time_get_curr_ms() - Get current system uptime in milliseconds.
 *
 * Gets the current system uptime in milliseconds.
 *
 * Return: System uptime in milliseconds.
 */
unsigned long nrf_wifi_osal_time_get_curr_ms(void);

/**
 * nrf_wifi_osal_time_elapsed_ms() - Get elapsed time in milliseconds
 * @param start_time_ms: The timestamp in milliseconds from which elapsed
 *			   time is to be measured.
 *
 * Returns the time elapsed in milliseconds since some
 * time instant (@p start_time_ms).
 *
 * Return: Elapsed time in milliseconds.
 */
unsigned int nrf_wifi_osal_time_elapsed_ms(unsigned long start_time_ms);

/**
 * @brief Initialize a PCIe driver.
 * @param dev_name Name of the PCIe device.
 * @param vendor_id Vendor ID of the PCIe device.
 * @param sub_vendor_id Sub-vendor ID of the PCIE device.
 * @param device_id Device ID of the PCIe device.
 * @param sub_device_id Sub-device ID of the PCIe device.
 *
 * Initializes a PCIe device.
 *
 * @return OS specific PCIe device context.
 */
void *nrf_wifi_osal_bus_pcie_init(const char *dev_name,
				  unsigned int vendor_id,
				  unsigned int sub_vendor_id,
				  unsigned int device_id,
				  unsigned int sub_device_id);


/**
 * @brief Deinitialize a PCIe device driver.
 * @param os_pcie_priv OS specific PCIe context.
 *
 * This API should be called when the PCIe device driver is to be unregistered from
 * the OS's PCIe core.
 */
void nrf_wifi_osal_bus_pcie_deinit(void *os_pcie_priv);


/**
 * brief Add a PCIe device instance.
 * @param os_pcie_priv OS specific PCIe context.
 * @param osal_pcie_dev_ctx: Pointer to the OSAL PCIe device context.
 */
void *nrf_wifi_osal_bus_pcie_dev_add(void *os_pcie_priv,
				     void *osal_pcie_dev_ctx);


/**
 * @brief Remove a PCIe device instance.
 * @param os_pcie_dev_ctx Pointer to the OS specific PCIe device context which was
 *	returned by  nrf_wifi_osal_bus_pcie_dev_add.
 *
 * Function to be invoked when a matching PCIe device is removed from the system.
 */
void nrf_wifi_osal_bus_pcie_dev_rem(void *os_pcie_dev_ctx);


/**
 * @brief Initialize a PCIe device instance.
 * @param os_pcie_dev_ctx Pointer to the OS specific PCIe device context which was
 *                        returned by  nrf_wifi_osal_bus_pcie_dev_add.
 *
 * Function to be invoked when a PCIe device is to be initialized.
 *
 * @return NRF_WIFI_STATUS_SUCCESS if successful, NRF_WIFI_STATUS_FAIL otherwise.
 */
enum nrf_wifi_status nrf_wifi_osal_bus_pcie_dev_init(void *os_pcie_dev_ctx);


/**
 * @brief Deinitialize a PCIe device instance.
 * @param os_pcie_dev_ctx Pointer to the OS specific PCIe device context which was
 *                        returned by  nrf_wifi_osal_bus_pcie_dev_add.
 *
 * Function to be invoked when a PCIe device is to be deinitialized.
 */
void nrf_wifi_osal_bus_pcie_dev_deinit(void *os_pcie_dev_ctx);


/**
 * @brief Register an interrupt handler for a PCIe device.
 * @param os_pcie_dev_ctx OS specific PCIe device context.
 * @param callbk_data Data to be passed to the ISR.
 * @param callbk_fn ISR to be invoked on receiving an interrupt.
 *
 * Registers an interrupt handler to the OS. This API also passes the callback
 * data to be passed to the ISR when an interrupt is received.
 *
 * @return NRF_WIFI_STATUS_SUCCESS if successful, NRF_WIFI_STATUS_FAIL otherwise.
 */
enum nrf_wifi_status nrf_wifi_osal_bus_pcie_dev_intr_reg(void *os_pcie_dev_ctx,
							 void *callbk_data,
							 int (*callbk_fn)(void *callbk_data));


/**
 * @brief Unregister an interrupt handler for a PCIe device.
 * @param os_pcie_dev_ctx OS specific PCIe device context.
 *
 * Unregisters the interrupt handler that was registered using
 */
void nrf_wifi_osal_bus_pcie_dev_intr_unreg(void *os_pcie_dev_ctx);


/**
 * @brief Map host memory for DMA access.
 * @param os_pcie_dev_ctx Pointer to a OS specific PCIe device handle.
 * @param virt_addr Virtual host address to be DMA mapped.
 * @param size Size in bytes of the host memory to be DMA mapped.
 * @param dir DMA direction.
 *
 * Maps host memory of @p size bytes pointed to by the virtual address
 * @p virt_addr to be used by the device(@p dma_dev) for DMAing contents.
 * The contents are available for DMAing to the device if @p dir has a
 * value of NRF_WIFI_OSAL_DMA_DIR_TO_DEV. Conversely the device can DMA
 * contents to the host memory if @p dir has a value of
 * NRF_WIFI_OSAL_DMA_DIR_FROM_DEV. The function returns the DMA address
 * of the mapped memory.
 */
void *nrf_wifi_osal_bus_pcie_dev_dma_map(void *os_pcie_dev_ctx,
					 void *virt_addr,
					 size_t size,
					 enum nrf_wifi_osal_dma_dir dir);


/**
 * @brief Unmap DMA mapped host memory.
 * @param os_pcie_dev_ctx Pointer to a OS specific PCIe device handle.
 * @param dma_addr DMA mapped physical host memory address.
 * @param size Size in bytes of the DMA mapped host memory.
 * @param dir DMA direction.
 *
 * Unmaps the host memory which was mapped for DMA using nrf_wifi_osal_dma_map.
 */
void nrf_wifi_osal_bus_pcie_dev_dma_unmap(void *os_pcie_dev_ctx,
					  void *dma_addr,
					  size_t size,
					  enum nrf_wifi_osal_dma_dir dir);


/**
 * @brief Get host mapped address for a PCIe device.
 * @param os_pcie_dev_ctx OS specific PCIe device context.
 * @param host_map Host map address information.
 *
 * Get host mapped address for a PCIe device.
 */
void nrf_wifi_osal_bus_pcie_dev_host_map_get(void *os_pcie_dev_ctx,
					     struct nrf_wifi_osal_host_map *host_map);




/**
 * @brief Initialize a qspi driver.
 *
 * Registers a qspi device driver to the OS's qspi core.
 *
 * @return OS specific qspi device context.
 */
void *nrf_wifi_osal_bus_qspi_init(void);

/**
 * @brief Deinitialize a qspi device driver.
 * @param os_qspi_priv OS specific qspi context.
 *
 * This API should be called when the qspi device driver is
 * to be unregistered from the OS's qspi core.
 */
void nrf_wifi_osal_bus_qspi_deinit(void *os_qspi_priv);


/**
 * brief Add a qspi device instance.
 * @param os_qspi_priv OS specific qspi context.
 * @param osal_qspi_dev_ctx: Pointer to the OSAL qspi device context.
 *
 * Function to be invoked when a matching qspi device is added to the system.
 * It expects the pointer to an OS specific qspi device context to be returned.
 *
 * @return OS specific qspi device context.
 */
void *nrf_wifi_osal_bus_qspi_dev_add(void *os_qspi_priv,
				     void *osal_qspi_dev_ctx);


/**
 * brief Remove a qspi device instance.
 * @param os_qspi_dev_ctx: Pointer to the OS specific qspi device context which was
 *                         returned by nrf_wifi_osal_bus_qspi_dev_add.
 *
 * Function to be invoked when a matching qspi device is removed from the system.
 */
void nrf_wifi_osal_bus_qspi_dev_rem(void *os_qspi_dev_ctx);


/**
 * @brief Initialize a qspi device instance.
 * @param os_qspi_dev_ctx: Pointer to the OS specific qspi device context which was
 *                         returned by nrf_wifi_osal_bus_qspi_dev_add.
 *
 * Function to be invoked when a qspi device is to be initialized.
 *
 * @return
 *     - Pass: NRF_WIFI_STATUS_SUCCESS.
 *     - Fail: NRF_WIFI_STATUS_FAIL.
 */
enum nrf_wifi_status nrf_wifi_osal_bus_qspi_dev_init(void *os_qspi_dev_ctx);


/**
 * brief Deinitialize a qspi device instance.
 * @param os_qspi_dev_ctx: Pointer to the OS specific qspi device context which was
 *                         returned by nrf_wifi_osal_bus_qspi_dev_add.
 *
 * Function to be invoked when a qspi device is to be deinitialized.
 */
void nrf_wifi_osal_bus_qspi_dev_deinit(void *os_qspi_dev_ctx);


/**
 * brief Register a interrupt handler for a qspi device.
 * @param os_qspi_dev_ctx OS specific qspi device context.
 * @param callbk_data Data to be passed to the ISR.
 * @param callbk_fn ISR to be invoked on receiving an interrupt.
 *
 * Registers an interrupt handler to the OS. This API also passes the callback
 * data to be passed to the ISR when an interrupt is received.
 *
 * @return NRF_WIFI_STATUS_SUCCESS if successful, NRF_WIFI_STATUS_FAIL otherwise.
 */
enum nrf_wifi_status nrf_wifi_osal_bus_qspi_dev_intr_reg(void *os_qspi_dev_ctx,
							 void *callbk_data,
							 int (*callbk_fn)(void *callbk_data));


/**
 * brief Unregister an interrupt handler for a qspi device.
 * @param os_qspi_dev_ctx OS specific qspi device context.
 *
 * Unregisters the interrupt handler that was registered using
 * nrf_wifi_osal_bus_qspi_dev_intr_reg.
 */
void nrf_wifi_osal_bus_qspi_dev_intr_unreg(void *os_qspi_dev_ctx);


/**
 * @brief Get host mapped address for a qspi device.
 * @param os_qspi_dev_ctx OS specific qspi device context.
 * @param host_map Host map address information.
 *
 * Gets the host map address for a qspi device.
 */
void nrf_wifi_osal_bus_qspi_dev_host_map_get(void *os_qspi_dev_ctx,
					     struct nrf_wifi_osal_host_map *host_map);

/**
 * @brief Read value from a 32 bit register on a QSPI slave device.
 * @param priv
 * @param addr Address of the register to read from.
 *
 * @return 32 bit value read from register.
 */
unsigned int nrf_wifi_osal_qspi_read_reg32(void *priv,
					   unsigned long addr);

/**
 * @brief Writes a 32 bit value to a 32 bit device register on a QSPI slave device.
 * @param priv
 * @param addr Address of the register to write to.
 * @param val Value to be written to the register.
 */
void nrf_wifi_osal_qspi_write_reg32(void *priv,
				    unsigned long addr,
				    unsigned int val);

/**
 * @brief Copies data from a QSPI slave device to a destination buffer.
 * @param priv
 * @param dest Destination buffer.
 * @param addr Address of the data to be copied.
 * @param count Number of bytes to be copied.
 */
void nrf_wifi_osal_qspi_cpy_from(void *priv,
				 void *dest,
				 unsigned long addr,
				 size_t count);

/**
 * @brief Copies data from a source buffer to a QSPI slave device.
 * @param priv
 * @param addr Address of the data to be written.
 * @param src Source buffer.
 * @param count Number of bytes to be copied.
 */
void nrf_wifi_osal_qspi_cpy_to(void *priv,
			       unsigned long addr,
			       const void *src,
			       size_t count);

/**
 * @brief Initialize a spi driver.
 *
 * Registers a spi device driver to the OS's spi core.
 *
 * @return OS specific spi device context.
 */
void *nrf_wifi_osal_bus_spi_init(void);

/**
 * @brief Deinitialize a spi device driver.
 * @param os_spi_priv OS specific spi context.
 *
 * This API should be called when the spi device driver is
 * to be unregistered from the OS's spi core.
 */
void nrf_wifi_osal_bus_spi_deinit(void *os_spi_priv);


/**
 * brief Add a spi device instance.
 * @param os_spi_priv OS specific spi context.
 * @param osal_spi_dev_ctx Pointer to the OSAL spi device context.
 *
 * Function to be invoked when a matching spi device is added to the system.
 * It expects the pointer to a OS specific spi device context to be returned.
 *
 * @return OS specific spi device context.
 */
void *nrf_wifi_osal_bus_spi_dev_add(void *os_spi_priv,
				    void *osal_spi_dev_ctx);


/**
 * @brief Remove a spi device instance.
 * @param os_spi_dev_ctx Pointer to the OS specific spi device context which was
 *                       returned by nrf_wifi_osal_bus_spi_dev_add.
 *
 * Function to be invoked when a matching spi device is removed from the system.
 */
void nrf_wifi_osal_bus_spi_dev_rem(void *os_spi_dev_ctx);


/**
 * @brief Initialize a spi device instance.
 * @param os_spi_dev_ctx Pointer to the OS specific spi device context which was
 *                       returned by nrf_wifi_osal_bus_spi_dev_add.
 *
 * Function to be invoked when a spi device is to be initialized.
 *
 * @return
 *      - Pass: nrf_wifi_STATUS_SUCCESS.
 *      - Fail: nrf_wifi_STATUS_FAIL.
 */
enum nrf_wifi_status nrf_wifi_osal_bus_spi_dev_init(void *os_spi_dev_ctx);


/**
 * @brief Deinitialize a spi device instance.
 * @param os_spi_dev_ctx Pointer to the OS specific spi device context which was
 *                       returned by nrf_wifi_osal_bus_spi_dev_add.
 *
 * Function to be invoked when a spi device is to be deinitialized.
 */
void nrf_wifi_osal_bus_spi_dev_deinit(void *os_spi_dev_ctx);


/**
 * @brief Register a interrupt handler for a spi device.
 * @param os_spi_dev_ctx OS specific spi device context.
 * @param callbk_data Data to be passed to the ISR.
 * @param callbk_fn ISR to be invoked on receiving an interrupt.
 *
 * Registers an interrupt handler to the OS. This API also passes the callback
 * data to be passed to the ISR when an interrupt is received.
 *
 * @return
 *     Pass: nrf_wifi_STATUS_SUCCESS.
 *     Fail: nrf_wifi_STATUS_FAIL.
 */
enum nrf_wifi_status nrf_wifi_osal_bus_spi_dev_intr_reg(void *os_spi_dev_ctx,
							void *callbk_data,
							int (*callbk_fn)(void *callbk_data));


/**
 * @brief Unregister an interrupt handler for a spi device.
 * @param os_spi_dev_ctx OS specific spi device context.
 *
 * Unregisters the interrupt handler that was registered using
 *  nrf_wifi_osal_bus_spi_dev_intr_reg.
 */
void nrf_wifi_osal_bus_spi_dev_intr_unreg(void *os_spi_dev_ctx);


/**
 * @brief Get host mapped address for a spi device.
 * @param os_spi_dev_ctx OS specific spi device context.
 * @param host_map Host map address information.
 *
 * Get the host map address for a spi device.
 */
void nrf_wifi_osal_bus_spi_dev_host_map_get(void *os_spi_dev_ctx,
					    struct nrf_wifi_osal_host_map *host_map);

/**
 * @brief Read value from a 32 bit register on a SPI slave device.
 * @param priv
 * @param addr Address of the register to read from.
 *
 * @return 32 bit value read from register.
 */
unsigned int nrf_wifi_osal_spi_read_reg32(void *priv,
					  unsigned long addr);

/**
 * @brief Writes a 32 bit value to a 32 bit device register on a SPI slave device.
 * @param priv
 * @param addr Address of the register to write to.
 * @param val Value to be written to the register.
 */
void nrf_wifi_osal_spi_write_reg32(void *priv,
				   unsigned long addr,
				   unsigned int val);

/**
 * @brief Copies data from a SPI slave device to a destination buffer.
 * @param priv
 * @param dest Destination buffer.
 * @param addr Address of the register to read from.
 * @param count Number of bytes to copy.
 */
void nrf_wifi_osal_spi_cpy_from(void *priv,
				void *dest,
				unsigned long addr,
				size_t count);

/**
 * @brief Copies data from a source buffer to a SPI slave device.
 * @param priv
 * @param addr Address of the register to write to.
 * @param src Source buffer.
 * @param count Number of bytes to copy.
 */
void nrf_wifi_osal_spi_cpy_to(void *priv,
			      unsigned long addr,
			      const void *src,
			      size_t count);


#if defined(NRF_WIFI_LOW_POWER) || defined(__DOXYGEN__)
/**
 * @brief Puts the QSPI interface to sleep.
 * @param os_qspi_priv Pointer to the QSPI private data.
 *
 * @return 0 on success, negative error code on failure.
 */
int nrf_wifi_osal_bus_qspi_ps_sleep(void *os_qspi_priv);

/**
 * @brief Wakes up the QSPI interface from sleep.
 * @param os_qspi_priv Pointer to the QSPI private data.
 *
 * @return 0 on success, negative error code on failure.
 */
int nrf_wifi_osal_bus_qspi_ps_wake(void *os_qspi_priv);

/**
 * @brief Get the power status of a QSPI interface.
 * @param os_qspi_priv Pointer to the QSPI private data.
 *
 * @return 0 if the QSPI interface is in sleep mode,
 *         1 if it is awake,
 *         Negative error code on failure.
 */
int nrf_wifi_osal_bus_qspi_ps_status(void *os_qspi_priv);
#endif /* NRF_WIFI_LOW_POWER */

/**
 * nrf_wifi_osal_rand8_get() - Get a random 8 bit number.
 *
 * Generates an 8 bit random number.
 *
 * Return: an 8 bit random number.
 */
unsigned char nrf_wifi_osal_rand8_get(void);

int nrf_wifi_osal_ipc_send_msg(unsigned int msg_type,
	void *msg,
	unsigned int msg_len);

#endif /* __OSAL_API_H__ */
