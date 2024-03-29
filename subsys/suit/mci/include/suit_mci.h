/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_MCI_H__
#define SUIT_MCI_H__

/*
 * SUIT (Software Updates for Internet of Things) installation and booting processes are described
 * in manifests as a sequences of instructions. That enables customization of update and booting
 * process at level of manifest. Aim of this API is to represent set of device constrains which
 * shall be applied to manifest processing.
 *
 * Manifests topology: Device images are described by one or more SUIT manifests, grouped in a tree
 * structures. Each of those manifests supported by the device is identified by UUID based
 * suit_manifest_class_id_t
 *
 * Boot order: SUIT manifest contains booting instructions (suit-invoke command sequence),
 * effectively controlling the booting order for components it covers. As manifests are grouped in
 * tree structures, parent manifest controls the order of invocation of its children suit-invoke
 * command sequences. Having multiple manifests in the device - there should be a mechanism which
 * would allow bootloader to chose a specific manifest or list of manifests to process its
 * suit-invoke in desired order.
 *
 * Downgrade prevention policy: Product requirements may enforce different downgrade prevention
 * policies attached to different components (i.e. cellular and application firmware).
 * Each manifest supported by the device has its downgrade prevention policy attached.
 * When update candidate is delivered to the device, update may be proceeded or be refused based
 * on sequence number in candidate manifest, sequence number of relevant manifest installed
 * in the device and respective downgrade prevention policy
 *
 * Key ID: Key id value can be utilized to encode purpose/signing authority associated with the key.
 * This may be utilized to detect the situation that manifest is signed with valid key, but
 * associated to authority which does not hold rights to sign this manifest. Definition of key id
 * semantic is out of scope of MCI, MCI just enables validation of value of selected bits of key id.
 *
 * Manifest access rights: malicious or unintentionally defective manifests may operate on
 * resources (i.e. memory ranges, cpu and others) they are not entitled to. There should be a
 * mechanism which would allow to check whether manifest operates on resources it is entitled to.
 * Such checking shall be applied before actual manifest processing starts, as detection of access
 * violation in the middle of update procedure may lead to unbootable state of the device. Memory
 * range checks do not differentiate between read and write access, as distinct manifest is
 * typically responsible both for installation and verification and eventual invocation of specific
 * suit component.
 *
 */
#include <zephyr/types.h>
#include <suit_plat_err.h>
#include <suit_metadata.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int mci_err_t;

/**< Invalid or unsupported manifest class id */
#define MCI_ERR_MANIFESTCLASSID 1
/**< Manifest is not entitled to operate on resource */
#define MCI_ERR_NOACCESS	2
/**< Provided key ID is invalid for desired operation */
#define MCI_ERR_WRONGKEYID	3

/**
 * @brief Gets Nordic vendor id
 *
 * @param[out]  vendor_id     Nordic vendor id
 *
 * @retval SUIT_PLAT_SUCCESS    on success
 * @retval SUIT_PLAT_ERR_INVAL  invalid parameter, i.e. null pointer
 */
mci_err_t suit_mci_nordic_vendor_id_get(const suit_uuid_t **vendor_id);

/**
 * @brief Gets an array of supported manifest class_info struct containing class_id.
 *
 * @param[out]     class_id	Array of manifest class ids and roles
 * @param[in,out]  size		as input - maximal amount of elements an array can hold,
 *				as output - amount of stored elements
 *
 * @retval SUIT_PLAT_SUCCESS    on success
 * @retval SUIT_PLAT_ERR_INVAL  invalid parameter, i.e. null pointer
 * @retval SUIT_PLAT_ERR_SIZE   too small to store all information
 */
mci_err_t suit_mci_supported_manifest_class_ids_get(suit_manifest_class_info_t *class_info,
						    size_t *size);

/**
 * @brief Gets an array of manifest class ids in invocation order.
 *
 * @param[out]     class_id	Array of manifest class ids
 * @param[in,out]  size		as input - maximal amount of elements an array can hold,
 *				as output - amount of stored elements
 *
 * @retval SUIT_PLAT_SUCCESS              on success
 * @retval SUIT_PLAT_ERR_INVAL            invalid parameter, i.e. null pointer
 * @retval SUIT_PLAT_ERR_SIZE             too small to store all information
 * @retval SUIT_PLAT_ERR_NOT_FOUND        no class_id with given role
 * @retval SUIT_PLAT_ERR_INCORRECT_STATE  unsupported execution mode
 */
mci_err_t suit_mci_invoke_order_get(const suit_manifest_class_id_t **class_id, size_t *size);

/**
 * @brief Gets downgrade prevention policy for manifest class id
 *
 * @param[in]   class_id	Manifest class id
 * @param[out]  policy		Downgrade prevention policy value
 *
 * @retval SUIT_PLAT_SUCCESS            on success
 * @retval SUIT_PLAT_ERR_INVAL          invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID      manifest class id unsupported
 * @retval SUIT_PLAT_ERR_OUT_OF_BOUNDS  data provisioned for downgrade prevention policy is invalid
 */
mci_err_t suit_mci_downgrade_prevention_policy_get(const suit_manifest_class_id_t *class_id,
						   suit_downgrade_prevention_policy_t *policy);

/**
 * @brief Gets independent updateability policy for manifest class id
 *
 * @param[in]   class_id	Manifest class id
 * @param[out]  policy		Independent updateability policy value
 *
 * @retval SUIT_PLAT_SUCCESS            on success
 * @retval SUIT_PLAT_ERR_INVAL          invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID      manifest class id unsupported
 * @retval SUIT_PLAT_ERR_OUT_OF_BOUNDS  data provisioned for independent updateability policy
 *                                      is invalid
 */
mci_err_t suit_mci_independent_update_policy_get(const suit_manifest_class_id_t *class_id,
						 suit_independent_updateability_policy_t *policy);

/**
 * @brief Validates if manifest class id is supported in the device.
 *
 * @param[in]  class_id	Manifest class id
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 */
mci_err_t suit_mci_manifest_class_id_validate(const suit_manifest_class_id_t *class_id);

/**
 * @brief Verifying whether specific key_id is valid for signing/checking signature of specific
 *manifest class
 *
 * @param[in]   class_id	Manifest class id
 * @param[in]   key_id		Identifier of key utilized for manifest signing. key_id may be equal
 *				to 0. In that case function returns success if manifest class id
 *				does not require signing.
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 * @retval MCI_ERR_WRONGKEYID       provided key ID is invalid for signing
 *                                  for provided manifest class
 */
mci_err_t suit_mci_signing_key_id_validate(const suit_manifest_class_id_t *class_id,
					   uint32_t key_id);

/**
 * @brief Verifies if manifest with specific class id is entitled to start (invoke) code on specific
 * processor
 *
 * @param[in]   class_id	    Manifest class id
 * @param[in]   processor_id	Processor id. Refer to Product Specification
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 * @retval MCI_ERR_NOACCESS         manifest is not entitled to operate on cpu
 */
mci_err_t suit_mci_processor_start_rights_validate(const suit_manifest_class_id_t *class_id,
						   int processor_id);

/**
 * @brief Verifies if manifest with specific class id is entitled to operate on memory range
 *
 * @param[in]   class_id	Manifest class id
 * @param[in]   address
 * @param[in]   mem_size	size of access area
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 * @retval MCI_ERR_NOACCESS         manifest is not entitled to operate on
 *                                  memory range
 */
mci_err_t suit_mci_memory_access_rights_validate(const suit_manifest_class_id_t *class_id,
						 void *address, size_t mem_size);

/**
 * @brief Verifies if manifest with specific class id is entitled to operate on non-memory platform
 * component
 *
 * @param[in]   class_id	Manifest class id
 * @param[in]   platform_specific_component_number
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 * @retval MCI_ERR_NOACCESS         manifest is not entitled to operate on
 *                                  non-memory platform component
 */
mci_err_t
suit_mci_platform_specific_component_rights_validate(const suit_manifest_class_id_t *class_id,
						     int platform_specific_component_number);

/**
 * @brief Gets vendor id for provided manifest class id
 *
 * @param[in]   class_id      Component/manifest class id
 * @param[out]  vendor_id     Vendor id for the class id
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 */
mci_err_t suit_mci_vendor_id_for_manifest_class_id_get(const suit_manifest_class_id_t *class_id,
						       const suit_uuid_t **vendor_id);

/**
 * @brief Verifies whether parent-child relationship for selected manifests is valid
 *
 * @param[in]   parent_class_id	Parent manifest class id
 * @param[in]   child_class_id	Child manifest class id
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 * @retval MCI_ERR_NOACCESS         parent-child relation for selected
 *                                  manifests is invalid
 */
mci_err_t
suit_mci_manifest_parent_child_declaration_validate(const suit_manifest_class_id_t *parent_class_id,
						    const suit_manifest_class_id_t *child_class_id);

/**
 * @brief Verifies whether parent-child relationship for selected manifests is valid in the context
 * of the current execution_mode
 *
 * @param[in]   parent_class_id	Parent manifest class id
 * @param[in]   child_class_id	Child manifest class id
 *
 * @retval SUIT_PLAT_SUCCESS        on success
 * @retval SUIT_PLAT_ERR_INVAL      invalid parameter, i.e. null pointer
 * @retval MCI_ERR_MANIFESTCLASSID  manifest class id unsupported
 * @retval MCI_ERR_NOACCESS         parent-child relation for selected
 *                                  manifests is invalid
 */
mci_err_t
suit_mci_manifest_process_dependency_validate(const suit_manifest_class_id_t *parent_class_id,
					      const suit_manifest_class_id_t *child_class_id);

/**
 * @brief Initializes MCI
 *
 * @return SUIT_PLAT_SUCCESS
 *
 */
mci_err_t suit_mci_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_MCI_H__ */
