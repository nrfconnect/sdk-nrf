/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SECDOM_INCLUDE_ARBITER_H__
#define SECDOM_INCLUDE_ARBITER_H__

/** The arbiter subsystem is an access control layer used to enforce rules
 * for actions done by SDFW on behalf of local domains.
 *
 * The arbiter memory API is used to validate both memory access grants
 * (e.g. via ADAC) and runtime memory accesses (e.g. IPC services and ADAC).
 * It is needed to protect both local domain and SecDom/SysCtrl
 * memory from oracle attacks via the privileged Secure Core.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Status code used to indicate success by the arbiter API. */
#define ARBITER_STATUS_OK 0xaa55ffcc

/* Flags used to define memory permissions.
 *
 * Individual flags are bitwise OR-ed together to compose a condition.
 * Note that the permission flags are negated, meaning that a value of
 * zero for a permission part of the bitfield means that the permission
 * is required.
 *
 * The ARBITER_MEM_PERM helper macro can be used to simplify construction
 * of the permission field.
 */

/** Operation does not require read permission. */
#define ARBITER_PERM_NREAD  (0x76UL << 0)
/** Operation does not require write permission. */
#define ARBITER_PERM_NWRITE (0x1cUL << 8)
/** Operation does not require execute permission. */
#define ARBITER_PERM_NEXEC  (0xa8UL << 16)
/** Operation can access both secure and nonsecure address ranges. */
#define ARBITER_PERM_SECURE (0xd9UL << 24)

/**
 * @brief Shorthand macro for specifying "no permissions".
 */
#define ARBITER_MEM_PERM_NONE (ARBITER_PERM_NREAD | ARBITER_PERM_NWRITE | ARBITER_PERM_NEXEC)

/**
 * @brief Macro for constructing memory permissions.
 *
 * The macro takes one or more of the following shortened permission names:
 * - READ
 * - WRITE
 * - EXEC
 * - SECURE
 *
 * Example usage:
 *
 * @code{.c}
 * uint32_t r_perms = ARBITER_MEM_PERM(READ);
 * uint32_t rxs_perms = ARBITER_MEM_PERM(READ, EXEC, SECURE);
 * @endcode
 */
#define ARBITER_MEM_PERM(...) _ARBITER_MEM_PERM(__VA_ARGS__)

/* clang-format off */
#define _ARBITER_MEM_PERM(...)                                                                     \
	((FOR_EACH(_ARBITER_MEM_PERM_AND, (&), __VA_ARGS__)) |                                     \
	 (FOR_EACH(_ARBITER_MEM_PERM_OR, (|), __VA_ARGS__)))
/* clang-format on */

#define _ARBITER_MEM_PERM_AND(_perm) UTIL_CAT(_ARBITER_MEM_PERM_AND_, _perm)
#define _ARBITER_MEM_PERM_AND_READ   (ARBITER_PERM_NWRITE | ARBITER_PERM_NEXEC)
#define _ARBITER_MEM_PERM_AND_WRITE  (ARBITER_PERM_NREAD | ARBITER_PERM_NEXEC)
#define _ARBITER_MEM_PERM_AND_EXEC   (ARBITER_PERM_NREAD | ARBITER_PERM_NWRITE)
#define _ARBITER_MEM_PERM_AND_SECURE (ARBITER_PERM_NREAD | ARBITER_PERM_NWRITE | ARBITER_PERM_NEXEC)

#define _ARBITER_MEM_PERM_OR(_perm) UTIL_CAT(_ARBITER_MEM_PERM_OR_, _perm)
#define _ARBITER_MEM_PERM_OR_READ   (0)
#define _ARBITER_MEM_PERM_OR_WRITE  (0)
#define _ARBITER_MEM_PERM_OR_EXEC   (0)
#define _ARBITER_MEM_PERM_OR_SECURE (ARBITER_PERM_SECURE)

/* Flags used to define a set of allowed memory types.
 *
 * Individual flags are bitwise OR-ed together to compose a condition.
 *
 * The ARBITER_MEM_TYPE helper macro can be used to simplify construction
 * of the permission field.
 */

/** Operation is allowed for free (unallocated) memory. */
#define ARBITER_TYPE_FREE     (0xe3UL << 0)
/** Operation is allowed for memory reserved or leased by the given owner. */
#define ARBITER_TYPE_RESERVED (0x45UL << 8)
/** Operation is allowed for memory with ownership that is hardcoded to the given owner. */
#define ARBITER_TYPE_FIXED    (0x97UL << 16)
/** Operation is allowed for any unlocked UICRs. */
#define ARBITER_TYPE_UICR     (0xbUL << 24)
/** Operation is allowed for configuration parameters. */
#define ARBITER_TYPE_CFG      (0x3UL << 28)

/**
 * @brief Shorthand macro for specifying "any memory type".
 */
#define ARBITER_MEM_TYPE_ALL (ARBITER_MEM_TYPE(FREE, RESERVED, FIXED, UICR, CFG))

/**
 * @brief Macro for constructing allowed memory types.
 *
 * The macro takes one or more of the following permission names:
 * - FREE
 * - RESERVED
 * - FIXED
 * - UICR
 * - CFG
 *
 * Example usage:
 *
 * @code{.c}
 * uint32_t free_or_reserved = ARBITER_MEM_TYPE(FREE, RESERVED);
 * uint32_t reserved_or_owned = ARBITER_MEM_TYPE(RESERVED, FIXED);
 * @endcode
 */
#define ARBITER_MEM_TYPE(...) _ARBITER_MEM_TYPE(__VA_ARGS__)

/* clang-format off */
#define _ARBITER_MEM_TYPE(...) (FOR_EACH(_ARBITER_MEM_TYPE_CAT, (|), __VA_ARGS__))
/* clang-format on */

#define _ARBITER_MEM_TYPE_CAT(_cat) UTIL_CAT(ARBITER_TYPE_, _cat)

typedef enum {
	NRF_OWNER_NONE = 0,	   /*!< Used to denote that ownership is not enforced */
	NRF_OWNER_GLOBAL = 0,	   /*!< Used to denote that ownership is not enforced */
	NRF_OWNER_APPLICATION = 2, /*!< Application Core                              */
	NRF_OWNER_RADIOCORE = 3,   /*!< Radio Core                                    */
} nrf_owner_t;

/**
 * @brief Common description of a memory access/reservation.
 */
struct arbiter_mem_params_common {
	nrf_owner_t owner; /**< Owner that is accessing the address range. */
	uintptr_t address; /**< Start address of the address range. */
	size_t size;	   /**< Size of the address range. */
	/** Permissions required for the access/reservation - see the ARBITER_MEM_PERM macro. */
	uint32_t permissions;
};

/**
 * @brief Memory access validation check parameters.
 */
struct arbiter_mem_params_access {
	/** Types of memory allowed for the access/reservation - see the ARBITER_MEM_TYPE macro. */
	uint32_t allowed_types;
	struct arbiter_mem_params_common access; /**< Access area parameters. */
};

/**
 * @brief Check memory permissions to an address range for a given owner.
 *
 * A return value of ARBITER_STATUS_OK indicates that the memory access is permitted.
 * Any other return value means that access is not permitted.
 *
 * @note When given, the ARBITER_PERM_SECURE permission flag means that the access
 *       is allowed to both secure and non-secure memory regions. If omitted, the
 *       access is only allowed to non-secure regions.
 *
 * @retval ARBITER_STATUS_OK if owner has the requested permission.
 * @retval -EINVAL if given invalid parameters.
 * @retval -SDFW_EMEMLOCK if reservations have not yet been marked as complete.
 * @retval -SDFW_EMEMOOB if address range is not contained within known memory.
 * @retval -SDFW_EMEMPERM if the access is not allowed based on permissions.
 * @retval -SDFW_EMEMTYPE if the access overlaps with memory with a type not explicitly allowed.
 * @retval -SDFW_ELCS if the access overlaps with a resource that is protected based on LCS.
 */
int arbiter_mem_access_check(const struct arbiter_mem_params_access *const params);

#ifdef __cplusplus
}
#endif

#endif /* SECDOM_INCLUDE_SDFW_MEMACCESS_H__ */
