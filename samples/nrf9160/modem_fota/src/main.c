/*$$$LICENCE_NORDIC_STANDARD<2016>$$$*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <zephyr.h>

#include "bsd.h"
#include "nrf_socket.h"

#define APP_DFU_FRAGMENT_SIZE    1024


static int      m_modem_dfu_fd;          /**< Socket fd used for modem DFU. */
static bool     m_skip_upgrade = false;  /**< State variable indicating if a upgrade is needed for not. */
static uint32_t m_offset  = 0;           /**< Offset for firmware upgrade. */

const static uint8_t * m_modem_fw_patch = (uint8_t  *)0xC0004;
static uint32_t        m_modem_fw_size;

static void app_modem_firmware_revision_validate(nrf_dfu_fw_version_t * p_revision)
{
    // Validate firmware revision or use it to trigger updates.
}


/**@brief Initialize modem DFU. */
static void app_modem_dfu_init(void)
{
    m_modem_fw_size  = *((uint32_t *)0xC0000);
    printk("Delta image size = %lu\n", m_modem_fw_size);

    if ((m_modem_fw_size == 0) || (m_modem_fw_size == 0xFFFFFFFF))
    {
        // Nothing to upgrade.
        m_skip_upgrade = true;
    }

    // Request a socket for firmware upgrade.
    m_modem_dfu_fd = nrf_socket(NRF_AF_LOCAL, NRF_SOCK_STREAM, NRF_PROTO_DFU);
    __ASSERT(m_modem_dfu_fd != -1, "Failed to open Modem DFU socket.");

    // Bind the socket to Modem.


    uint32_t optlen;
    // Get the current revision of modem.
    nrf_dfu_fw_version_t revision;
    optlen = sizeof(nrf_dfu_fw_version_t);
    uint32_t retval = nrf_getsockopt(m_modem_dfu_fd,
                                     NRF_SOL_DFU,
                                     NRF_SO_DFU_FW_VERSION,
                                     &revision,
                                     &optlen);
    __ASSERT(retval != -1, "Firmware revision request failed.");
    app_modem_firmware_revision_validate(&revision);
    char * r = revision;
    printk("Revision = \n");
    printk("    %x%x%x%x%x%x-%x%x%x%x%x%x-%x%x%x%x%x%x\n", r[0],r[1],r[2],r[3],r[4],r[5], r[6],r[7],r[8],r[9],r[10],r[11],r[12],r[13],r[14],r[15],r[16],r[17]);
    printk("    %x%x%x%x%x%x-%x%x%x%x%x%x-%x%x%x%x%x%x\n", r[18],r[19],r[20],r[21],r[22],r[23], r[24],r[25],r[26],r[27],r[28],r[29],r[30],r[31],r[32],r[33],r[34],r[35]);

    // Get available resources to improve chances of a successful upgrade.
    nrf_dfu_fw_resource_t resource;
    optlen = sizeof(nrf_dfu_fw_resource_t);
    retval = nrf_getsockopt(m_modem_dfu_fd,
                            NRF_SOL_DFU,
                            NRF_SO_DFU_RESOURCE,
                            &resource,
                            &optlen);
    __ASSERT(retval != -1, "Resource request failed.");
    __ASSERT(resource.flash_size >= m_modem_fw_size, "Delta image too big!");

    // Get offset of existing download in the event that the download was interrupted.
    nrf_dfu_fw_offset_t offset;
    optlen = sizeof(nrf_dfu_fw_offset_t);
    retval = nrf_getsockopt(m_modem_dfu_fd,
                            NRF_SOL_DFU,
                            NRF_SO_DFU_OFFSET,
                            &offset,
                            &optlen);
    __ASSERT(retval != -1, "Offset request failed.");
    __ASSERT(offset <= m_modem_fw_size, "Offset bigger than firmware size.");
    m_offset = offset;

    if (offset == m_modem_fw_size)
    {
        printk("Nothing to transfer!\n");
        m_skip_upgrade = true;
    }
}


/**@brief Start transfer of the new firmware patch. */
static void app_modem_dfu_transfer_start(void)
{
    if (m_skip_upgrade == true)
    {
        return;
    }

    // Start transfer of firmware.
    uint32_t to_send = m_modem_fw_size - m_offset;

    while (to_send)
    {
        int sent = nrf_send(m_modem_dfu_fd,
                            &m_modem_fw_patch[m_offset],
                            to_send > APP_DFU_FRAGMENT_SIZE ? APP_DFU_FRAGMENT_SIZE : to_send,
                            0);
        printk("Offset = %lu, Sent = %d, remaining = %lu\n", m_offset, sent, to_send);
        if (sent == -1)
        {
            // Read errno to decide if the procedure may continue later or must be aborted.
            __ASSERT (errno == EAGAIN, "DFU send failed. errno = %d", errno);
        }
        else
        {
            m_offset += sent;
            to_send  -= sent;
        }
    };
}


/**
 * @brief Apply the firmware transferred firmware.
 *
 * @note In all normal operations, including the LTE link are disrupted on
 *       application of new firmware. The communication with the modem should be
 *       reinitialized using the bsd_init. The LTE link establishment procedures
 *       must re-initiated as well. This reinitialization is needed regardless of
 *       success or failure of the procedure. The success or failure of the procedure
 *       indicates if firmware upgrade was successful or not. The success indicates that
 *       new firmware was applied and failure indicates that firmware upgrade was not
 *       successful and the older revision of firmware will be used.
 */
static void app_modem_dfu_transfer_apply(void)
{
    if (m_skip_upgrade == true) {
        uint32_t retval = nrf_setsockopt(m_modem_dfu_fd,
                                     NRF_SOL_DFU,
                                     NRF_SO_DFU_REVERT,
                                     NULL,
                                     0);
        __ASSERT(retval != -1, "Failed to revert to the old firmware.\n");
        printk("Requested reverting to the old firmware.\n");

        m_offset = 0;

        retval = nrf_setsockopt(m_modem_dfu_fd,
                             NRF_SOL_DFU,
                             NRF_SO_DFU_OFFSET,
                             &m_offset,
                             sizeof(m_offset));
        __ASSERT(retval != -1, "Failed to reset offset.\n");
        printk("Reset offset.\n");

        // Delete the backup
        retval = nrf_setsockopt(m_modem_dfu_fd,
                             NRF_SOL_DFU,
                             NRF_SO_DFU_BACKUP_DELETE,
                             NULL,
                             0);
        __ASSERT(retval != -1, "Failed to reset offset.\n");
        printk("Reset offset.\n");

    } else {
        uint32_t retval = nrf_setsockopt(m_modem_dfu_fd,
                                         NRF_SOL_DFU,
                                         NRF_SO_DFU_APPLY,
                                         NULL,
                                         0);
        __ASSERT(retval != -1, "Failed to apply the new firmware.\n");
        printk("Requested apply of new firmware.\n");
    }


    nrf_close(m_modem_dfu_fd);

    // Reinitialize.
    printk("Reinitializing bsd.\n");
    bsd_init();

}


int main(void)
{
    bsd_init();

    app_modem_dfu_init();

    app_modem_dfu_transfer_start();

    app_modem_dfu_transfer_apply();

    printk("Idleing..\n");
    k_cpu_idle();

    return 0;
}
