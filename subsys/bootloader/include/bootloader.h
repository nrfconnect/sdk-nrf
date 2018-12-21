#ifndef BOOTLOADER_H__
#define BOOTLOADER_H__

#include <autoconf.h>
#include <fw_metadata.h>

/*TODO: Remove DT_ defines when multi-image has been merged*/
#define ROM_ADDR_SIZE (CONFIG_FLASH_SIZE * 1024)
#define DT_FLASH_AREA_PROVISION_SIZE		0x1000
#ifdef CONFIG_SOC_NRF9160
#define DT_FLASH_AREA_PROVISION_OFFSET	(NRF_UICR_S->OTP)
#define DT_FLASH_AREA_PROVISION_ROM_SIZE	0 /* OTP is not in the regular ROM region, so it takes no code space from the app. */
#else
#define DT_FLASH_AREA_PROVISION_OFFSET	(ROM_ADDR_SIZE - DT_FLASH_AREA_PROVISION_SIZE)
#define DT_FLASH_AREA_PROVISION_ROM_SIZE	DT_FLASH_AREA_PROVISION_SIZE
#endif

#if CONFIG_SOC_NRF52840  ||\
	CONFIG_SOC_NRF52832  ||\
	CONFIG_SOC_NRF52810  ||\
	CONFIG_SOC_NRF51822_QFAA ||\
	CONFIG_SOC_NRF51822_QFAB ||\
	CONFIG_SOC_NRF51822_QFAC ||\
	CONFIG_SOC_NRF9160
/* partition@0 */
#define DT_FLASH_AREA_SECURE_BOOT_OFFSET	0x0
#define DT_FLASH_AREA_SECURE_BOOT_SIZE		0x8000

/* partition@8000 */
#define DT_FLASH_AREA_S0_OFFSET		0x8000
#define DT_FLASH_AREA_S0_SIZE		0x4000

/* partition@C000 */
#define DT_FLASH_AREA_S1_OFFSET		0xc000
#define DT_FLASH_AREA_S1_SIZE		0x4000

/* partition@10000 */
#define DT_FLASH_AREA_APP_OFFSET	0x10000
#define DT_FLASH_AREA_APP_SIZE		(ROM_ADDR_SIZE - DT_FLASH_AREA_APP_OFFSET - DT_FLASH_AREA_PROVISION_ROM_SIZE)

#else
#error "Board not supported for SECURE BOOT"
#endif /* CONFIG_SOC_NRF52840 */


struct __packed fw_validation_info {
	/* Magic value to verify that the struct has the correct type. */
	u32_t magic[MAGIC_LEN_WORDS];

	/* The address of the start (vector table) of the firmware. */
	u32_t firmware_address;

	/* The hash of the firmware.*/
	u8_t  firmware_hash[CONFIG_SB_HASH_LEN];

	/* Public key to be used for signature verification. This must be
	 * checked against a trusted hash.
	 */
	u8_t  public_key[CONFIG_SB_PUBLIC_KEY_LEN];

	/* Signature over the firmware as represented by the firmware_address
	 * and firmware_size in the firmware_info.
	 */
	u8_t  signature[CONFIG_SB_SIGNATURE_LEN];
};


/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_validation_info, magic, 0);
OFFSET_CHECK(struct fw_validation_info, firmware_address, CONFIG_FW_MAGIC_LEN);
OFFSET_CHECK(struct fw_validation_info, firmware_hash,
	(CONFIG_FW_MAGIC_LEN + 4));
OFFSET_CHECK(struct fw_validation_info, public_key,
	(CONFIG_FW_MAGIC_LEN + 4 + CONFIG_SB_HASH_LEN));
OFFSET_CHECK(struct fw_validation_info, signature,
	(CONFIG_FW_MAGIC_LEN + 4 + CONFIG_SB_HASH_LEN
	+ CONFIG_SB_SIGNATURE_LEN));

/* Can be used to make the firmware discoverable in other locations, e.g. when
 * searching backwards. This struct would typically be constructed locally, so
 * it needs no version.
 */
struct __packed fw_validation_pointer {
	u32_t magic[MAGIC_LEN_WORDS];
	const struct fw_validation_info *validation_info;
};

/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_validation_pointer, magic, 0);
OFFSET_CHECK(struct fw_validation_pointer, validation_info,
	CONFIG_FW_MAGIC_LEN);


/* Find the validation_info at the end of the firmware. */
static inline const struct fw_validation_info *
validation_info_find(const struct fw_firmware_info *finfo,
			u32_t search_distance)
{
	u32_t vinfo_addr = finfo->firmware_address + finfo->firmware_size;
	const struct fw_validation_info *vinfo;
	const u32_t validation_info_magic[] = {VALIDATION_INFO_MAGIC};

	for (int i = 0; i <= search_distance; i++) {
		vinfo = (const struct fw_validation_info *)(vinfo_addr + i);
		if (memeq(vinfo->magic, validation_info_magic,
					CONFIG_FW_MAGIC_LEN)) {
			return vinfo;
		}
	}
	return NULL;
}

#endif /* BOOTLOADER_H__ */

