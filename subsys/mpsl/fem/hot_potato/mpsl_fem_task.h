/*
 * Copyright (c) Nordic Semiconductor ASA. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor ASA.
 * The use, copying, transfer or disclosure of such information is prohibited except by
 * express written agreement with Nordic Semiconductor ASA.
 */

/**
 * @file
 *   This file defines an API that allows an arbitrary task of a peripheral (limited
 *   to FEM use-cases), to subscribe to any DPPI channel within (almost) any DPPIC domain.
 *   For nRF53 Series the implementation reduces to very simple code, because there is
 *   only one DPPIC controller.
 *   For nRF54L Series the implementation takes into account multiple DPPIC domains
 *   and PPIBs. The MCU DPPIC domain not supported.
 *
 *   The key object provided by this module is @ref mpsl_fem_task_t with a set of
 *   functions operating on it.
 */

#ifndef MPSL_FEM_TASK_H_
#define MPSL_FEM_TASK_H_

#include "nrfx.h"

#if defined(DPPI_PRESENT)

#include "hal/nrf_dppi.h"

#ifdef __STATIC_INLINE__
#undef __STATIC_INLINE__
#endif

#ifdef MPSL_FEM_TASK_DECLARE_ONLY
#define __STATIC_INLINE__
#else
#define __STATIC_INLINE__ __STATIC_INLINE
#endif

/** @brief Peripheral task proxy object to some target peripheral task.
 *
 *  For SoC series having multiple DPPIC controllers this object allows to
 *  subscribe the target peripheral task (possibly indirectly through PPIB)
 *  to a DPPI channel belonging to a DPPIC domain that the object was
 *  initialized for.
 *
 *  For SoC series having just one DPPIC controller this object allows to
 *  subscribe the target peripheral task to a DPPI channel belonging to the
 *  only one existing DPPIC controller.
 *
 *  The purpose of this object is to allow subscribe set/clear operations
 *  taking into account possible multiple DPPIC domains and PPIBs.
 *  By design the subscribe set/clear operation are intended to be performed
 *  in time critical code (focus on execution speed), while initialization
 *  occurs only at boot-time and it's role is to prepare the object and
 *  related PPIB/DPPI resources to possibly fastest subscribe set/clear.
 *
 *  Intended usage:
 *  During boot-time configuration:
 *  - Initialize the object with @ref mpsl_fem_task_init .
 *
 *  During run-time (time critical ptah):
 *  - Use @ref mpsl_fem_task_subscribe_set and @ref mpsl_fem_task_subscribe_clear .
 *
 *  Limitations:
 *  There can be multiple instances of @ref mpsl_fem_task_t initialized so that
 *  they allow to trigger the same @c target_task_addr (see @ref mpsl_fem_task_init),
 *  but at most one such object can subscribe to a DPPI channel at a time.
 *  An user is responsible for not violating this limitation. There is no
 *  protection against misuse (focus on speed, no additional memory overhead).
 */
typedef struct
{
    /** @brief Task address that allows some target task to be triggered through DPPI.
     *
     *  For nRF53 Series this is the @c target_task_addr passed to the @ref mpsl_fem_task_init.
     *
     *  For nRF54L Series this task address may refer either directly to the
     *  @c target_task_addr passed to the @ref mpsl_fem_task_init or or address of
     *  appropriate @c TASK_SEND task of a PPIB configured in such a way that
     *  triggering it causes the @c target_task_addr task to be triggered.
     */
    uint32_t task_addr;

#if defined(PPIB_PRESENT)
    /** @brief Saved SUBSCRIBE register for the @c task_addr. 
     *
     *  This field is necessary to restore the original subscription by a call to
     *  @ref mpsl_fem_task_subscribe_clear.
     */
    uint32_t saved_subscribe;
#endif
} mpsl_fem_task_t;

/** @brief Initializes a peripheral task proxy object object so that it can subscribe
 *         to DPPI channels in given DPPIC domain.
 *
 *  If @p target_task_addr is @c 0, then the object will be initialized to a handy
 *  "none" condition. In such case the @ref mpsl_fem_task_is_valid function will return
 *  @c false and other API calls on the @p p_obj are forbidden.
 *
 *  If @p target_task_addr is other then @c 0, then the object will be initialized
 *  so that the @ref mpsl_fem_task_subscribe_set function will be able to subscribe
 *  to a DPPI channel in the same domain as the peripheral containing @p source_domain_addr
 *  address.
 *
 *  @param[out] p_obj              Pointer to a peripheral task proxy object to initialize.
 *  @param[in]  target_task_addr   Address of a target peripheral task register that shall
 *                                 be able to subscribe to a DPPI channel within the DPPIC
 *                                 domain selected by @p source_domain_addr .
 *  @param[in]  source_addr        An address in source DPPIC domain from which given task
 *                                 will subs
 *
 *  @retval true  The object initialized successfully (also for "none" condition).
 *  @retval false Failure.
 */
bool mpsl_fem_task_init(mpsl_fem_task_t     * p_obj,
                        uint32_t              target_task_addr,
                        uint32_t              source_domain_addr);

/** @brief Checks if a peripheral task proxy object contains a valid task.
 *
 *  See @ref mpsl_fem_task_init for details.
 *
 *  @param[in] p_obj  Pointer to a peripheral task proxy object to check
 *
 *  @retval true   The object is initialized to a valid task.
 *  @retval false  The object is initialized to the "none" task condition.
 */
__STATIC_INLINE__ bool mpsl_fem_task_is_valid(const mpsl_fem_task_t * p_obj);

/** @brief Subscribes a peripheral task proxy object to a DPPI channel within
 *         the peripheral task proxy object's DPPIC domain.
 *
 *  This function may be called only on an initialized (see @ref mpsl_fem_task_init) and
 *  valid (see @ref mpsl_fem_task_is_valid) object.
 *
 *  There might exist multiple @ref mpsl_fem_task_t objects allowing to trigger
 *  the same @c taget_task (see parameter for @ref mpsl_fem_task_init), but at most
 *  one such object may be subscribed to a DPPI channel at a time. Attempt to subscribe
 *  more than one of such objects results in an undefined behavior.
 *
 *  @param[in]  p_obj   Peripheral task proxy object.
 *  @param[in]  dppi_ch DPPI channel within a DPPIC domain that the @p p_obj was initialized for.
 */
__STATIC_INLINE__ void mpsl_fem_task_subscribe_set(mpsl_fem_task_t * p_obj, uint8_t dppi_ch);

/** @brief Clears a subscription of a peripheral task proxy object to any DPPI channel.
 *
 *  This function may be called only on an initialized (see @ref mpsl_fem_task_init) and
 *  valid (see @ref mpsl_fem_task_is_valid) object.
 *
 *  @param[in]  p_obj   Peripheral task proxy object.
 */
__STATIC_INLINE__ void mpsl_fem_task_subscribe_clear(mpsl_fem_task_t * p_obj);

/** @brief Returns a DPPIC instance applicable to connect to given peripheral via publish/subscribe).
 *
 * This function may be limited to these domains/DPPICs/peripheral which can be used for FEM purposes.
 *
 * @param[in] addr      Address of a peripheral or a register within a peripheral.
 *
 * @return Pointer to a DPPIC controller instance that the peripheral can publish/subscribe to the DPPI
 *         channels belonging to the DPPIC controller.
 *         @c NULL if no DPPIC controller could me matched.
 */
NRF_DPPIC_Type * mpsl_fem_peripheral_dppic_get(uint32_t addr);

#ifndef MPSL_FEM_TASK_DECLARE_ONLY

__STATIC_INLINE__ bool mpsl_fem_task_is_valid(const mpsl_fem_task_t * p_obj)
{
    return p_obj->task_addr != 0U;
}

#ifndef NRF_DPPI_ENDPOINT_GET
#define NRF_DPPI_ENDPOINT_GET(task_or_event) \
    (*((volatile uint32_t *)(task_or_event + NRF_SUBSCRIBE_PUBLISH_OFFSET(task_or_event))))
#endif

__STATIC_INLINE__ void mpsl_fem_task_subscribe_set(mpsl_fem_task_t * p_obj, uint8_t dppi_ch)
{
    uint32_t task_addr = p_obj->task_addr;
#if defined(PPIB_PRESENT)
    p_obj->saved_subscribe = NRF_DPPI_ENDPOINT_GET(task_addr);
#endif
    NRF_DPPI_ENDPOINT_SETUP(task_addr, dppi_ch);
}

__STATIC_INLINE__ void mpsl_fem_task_subscribe_clear(mpsl_fem_task_t * p_obj)
{
    uint32_t task_addr = p_obj->task_addr;

#if defined(PPIB_PRESENT)
    if ((p_obj->saved_subscribe & NRF_SUBSCRIBE_PUBLISH_ENABLE) != 0U)
    {
        /* NOTE: task_addr was on path form some other domain, because we saved that
         * it already subscribed to some DPPI ch. Due to re-using of resources we
         * need to restore original DPPI subscription rather than clearing it.
         */
        uint8_t orig_dppi_ch = (uint8_t)(p_obj->saved_subscribe);
        NRF_DPPI_ENDPOINT_SETUP(task_addr, orig_dppi_ch);
        return;
    }
#endif

    NRF_DPPI_ENDPOINT_CLEAR(task_addr);
}

#endif // MPSL_FEM_TASK_DECLARE_ONLY

#undef __STATIC_INLINE__

#endif /* defined(DPPI_PRESENT) */

#endif // MPSL_FEM_TASK_H_
