/* Copyright (c) 2024, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef MPSL_FEM_PSEMI_PINS_H_
#define MPSL_FEM_PSEMI_PINS_H_

#include <mpsl_fem_config_common.h>

#include <nrfx.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>
#ifdef NRF54L_SERIES
#include <hal/nrf_ppib.h>
#endif

#ifdef __STATIC_INLINE__
#undef __STATIC_INLINE__
#endif

#ifdef MPSL_FEM_PINS_COMMON_DECLARE_ONLY
#define __STATIC_INLINE__
#else
#define __STATIC_INLINE__ __STATIC_INLINE
#endif

/** @brief Struture representing a pin controlled by GPIOTE.
 *
 *  The purpose of this type is to store all necessary data precalculated
 *  without the need to perform any run-time calculation in time-critical code.
 */
typedef struct {
	/** @brief Pointer to a GPIOTE instance.
	 *
	 *  @note This pointer is NULL if the pin is disabled.
	 */
	NRF_GPIOTE_Type *p_gpiote;

	/** @brief GPIOTE channel number used for toggling the pin. */
	uint8_t gpiote_ch_id;

	/** @brief Tasks to toggle the pin aware of active state.
	 *
	 *  The tasks_active[0] is the task setting the pin to the inactive state.
	 *  The tasks_active[1] is the task setting the pin to the active state.
	 *  The meaning of active state is determined by @c
	 * mpsl_fem_gpiote_pin_config_t::active_high field.
	 */
	nrf_gpiote_task_t tasks_active[2];

#if defined(NRF54L_SERIES)
	/** @brief Tasks to toggle the pin aware of active state.
	 *
	 *  These tasks belong to the PPIB11, can be subscribed to DPPIs
	 *  within the Radio Power Domain.
	 */
	nrf_ppib_task_t ppib_tasks_active[2];
#endif
} mpsl_fem_gpiote_pin_t;

/** @brief Initializes the instance of mpsl_fem_gpiote_pin_t with given configuration.
 *
 *  @param[out] p_obj    Pointer to an instance of mpsl_fem_gpiote_pin_t.
 *  @param[in]  p_config Pointer to desired configuration.
 */
void mpsl_fem_gpiote_pin_init(mpsl_fem_gpiote_pin_t *p_obj,
			      const mpsl_fem_gpiote_pin_config_t *p_config);

/** @brief Checks if instance of mpsl_fem_gpiote_pin_t is enabled.
 *
 *  @retval false   The instance is disabled.
 *  @retval true    The instance is enabled.
 */
__STATIC_INLINE__ bool mpsl_fem_gpiote_pin_is_enabled(const mpsl_fem_gpiote_pin_t *p_obj);

/** @brief Sets given output level on a pin controlled an instance of mpsl_fem_gpiote_pin_t.
 *
 *  @param[in] p_obj     Pointer to an instance of mpsl_fem_gpiote_pin_t.
 *  @param[in] active    Output level to set. `false` for `inactive`, `true` for `active` level.
 */
__STATIC_INLINE__ void mpsl_fem_gpiote_pin_output_active_write(const mpsl_fem_gpiote_pin_t *p_obj,
							       bool active);

#ifdef DPPI_PRESENT
/** @brief Subscribes a task to control the pin drive into a desired state.
 *
 *  @sa Complementary to the function @ref mpsl_fem_gpiote_pin_task_subscribe_clear
 *
 *  @param[in] p_obj     Pointer to an instance of mpsl_fem_gpiote_pin_t.
 *  @param[in] active    Target state drive of the pin when given @c dppi_ch is triggered.
 *                       `false` is for driving into inactive state.
 *                       `true` is for driving into active state.
 *                       Note the electrical level (high/low) is determined by the
 *                       @c mpsl_fem_gpiote_pin_config_t::active_high field passed to
 *                       @ref mpsl_fem_gpiote_pin_init .
 *  @param[in] dppi_ch   The DPPI channel that given action is to be subscribed to.
 */
__STATIC_INLINE__ void mpsl_fem_gpiote_pin_task_subscribe_set(const mpsl_fem_gpiote_pin_t *p_obj,
							      bool active, uint8_t dppi_ch);

/** @brief Clears a subscription of a task controlling the pin drive.
 *
 *  @sa Complementary to the function @ref mpsl_fem_gpiote_pin_task_subscribe_set
 *
 *  @param[in] p_obj     Pointer to an instance of mpsl_fem_gpiote_pin_t.
 *  @param[in] active    The target state drive of the pin to clear.
 *                       See @c active parameter of @ref mpsl_fem_gpiote_pin_task_subscribe_set
 *                       for reference.
 */
__STATIC_INLINE__ void mpsl_fem_gpiote_pin_task_subscribe_clear(const mpsl_fem_gpiote_pin_t *p_obj,
								bool active);

#endif /* DPPI_PRESENT */

/** @brief Configures GPIO pin.
 *
 *  @param[in] p_fem_pin  Specifies the pin to be configured.
 *  @param[in] dir        Pin direction.
 *  @param[in] input      Connect or disconnect the input buffer.
 *  @param[in] pull       Pull configuration.
 *  @param[in] drive      Drive configuration.
 *  @param[in] sense      Pin sensing mechanism.
 */
void mpsl_fem_pin_configure(const mpsl_fem_pin_t *p_fem_pin, nrf_gpio_pin_dir_t dir,
			    nrf_gpio_pin_input_t input, nrf_gpio_pin_pull_t pull,
			    nrf_gpio_pin_drive_t drive, nrf_gpio_pin_sense_t sense);

/** @brief Sets given output level to a GPIO pin.
 *
 *  @param[in] p_fem_pin  Specifies the pin to be driven.
 *  @param[in] level      Output level to write. `false` for low level, `true` for high.
 */
__STATIC_INLINE__ void mpsl_fem_pin_output_write(const mpsl_fem_pin_t *p_fem_pin, bool level);

/** @brief Configures GPIO pin and it's GPIOTE channel according to configuration structure
 *
 *  @param[in] p_config     Configuration structure of GPIO pin and GPIOTE channel.
 */
void mpsl_fem_gpiote_pin_config_configure(const mpsl_fem_gpiote_pin_config_t *p_config);

/** @brief Returns a pointer to the GPIOTE instance stored in a gpiote pin configuration.
 *
 *  @param[in] p_config     Configuration of a pin.
 *
 *  @return Pointer to the GPIOTE instance.
 */
__STATIC_INLINE__ NRF_GPIOTE_Type *
mpsl_fem_gpiote_pin_config_gpiote_instance_get(const mpsl_fem_gpiote_pin_config_t *p_config);

/** @brief Sets given output level to a pin controlled by GPIOTE.
 *
 *  @param[in] p_config  Configuration of a pin. mpsl_fem_gpiote_pin_config_t::active_high
 *                       determines which electrical level is considered 'active'.
 *  @param[in] active    Output level to set. `false` for `inactive`, `true` for `active` level.
 */
__STATIC_INLINE__ void
mpsl_fem_gpiote_pin_config_output_active_write(const mpsl_fem_gpiote_pin_config_t *p_config,
					       bool active);

/** @brief Returns task to achieve given activity level on a pin.
 *
 *  @param[in] p_config  Configuration of a pin. mpsl_fem_gpiote_pin_config_t::active_high
 *                       determines which electrical level is considered 'active'.
 *  @param[in] activate  Action to perform. `false` for setting inactive level. `true` is
 *                       for active level.
 *
 *  @return Task to be triggered on GPIOTE to achieve desired level on the pin.
 */
__STATIC_INLINE__ nrf_gpiote_task_t mpsl_fem_gpiote_pin_config_action_task_get(
	const mpsl_fem_gpiote_pin_config_t *p_config, bool activate);

/** @brief Equivalent to @ref mpsl_fem_gpiote_pin_config_action_task_get but returns address of task
 * rather than the task itself.
 *
 *  @note This function is more suitable for PPI as PPIs have TEP registers which are direct
 *        address of a task.
 *
 *  @param[in] p_config  Configuration of a pin. mpsl_fem_gpiote_pin_config_t::active_high
 *                       determines which electrical level is considered 'active'.
 *  @param[in] activate  Action to perform. `false` for setting inactive level. `true` is
 *                       for active level.
 *
 *  @return Address of a task to be triggered on GPIOTE to achieve desired level on the pin.
 */
__STATIC_INLINE__ uint32_t mpsl_fem_gpiote_pin_config_action_task_address_get(
	const mpsl_fem_gpiote_pin_config_t *p_config, bool activate);

/** @brief Configures GPIO pin according to configuration structure
 *
 *  @param[in] p_config     Configuration structure of GPIO pin.
 */
void mpsl_fem_gpio_pin_config_configure(const mpsl_fem_gpio_pin_config_t *p_config);

/** @brief Writes given output level to GPIO pin.
 *
 *  @note This function ignores mpsl_fem_gpiote_pin_config_t::active_high setting.
 *
 *  @param[in] p_config  Configuration of the GPIO pin.
 *  @param[in] level     Output level to write. `false` for low level, `true` for high.
 */
__STATIC_INLINE__ void
mpsl_fem_gpio_pin_config_pin_output_write(const mpsl_fem_gpio_pin_config_t *p_config, bool level);

#ifndef MPSL_FEM_PINS_COMMON_DECLARE_ONLY

__STATIC_INLINE__ bool mpsl_fem_gpiote_pin_is_enabled(const mpsl_fem_gpiote_pin_t *p_obj)
{
	return (p_obj->p_gpiote != NULL);
}

__STATIC_INLINE__ NRF_GPIOTE_Type *
mpsl_fem_gpiote_pin_config_gpiote_instance_get(const mpsl_fem_gpiote_pin_config_t *p_config)
{
#if defined(NRF54L_SERIES)
	return p_config->p_gpiote;
#else
	(void)p_config;
	return NRF_GPIOTE;
#endif
}

__STATIC_INLINE__ void mpsl_fem_gpiote_pin_output_active_write(const mpsl_fem_gpiote_pin_t *p_obj,
							       bool active)
{
	nrf_gpiote_task_trigger(p_obj->p_gpiote, p_obj->tasks_active[active]);
}

__STATIC_INLINE__ void
mpsl_fem_gpiote_pin_config_output_active_write(const mpsl_fem_gpiote_pin_config_t *p_config,
					       bool active)
{
	NRF_GPIOTE_Type *p_gpiote = mpsl_fem_gpiote_pin_config_gpiote_instance_get(p_config);

	if (p_config->active_high ^ active) {
		nrf_gpiote_task_force(p_gpiote, p_config->gpiote_ch_id,
				      NRF_GPIOTE_INITIAL_VALUE_LOW);
	} else {
		nrf_gpiote_task_force(p_gpiote, p_config->gpiote_ch_id,
				      NRF_GPIOTE_INITIAL_VALUE_HIGH);
	}
}

#ifdef DPPI_PRESENT
__STATIC_INLINE__ void mpsl_fem_gpiote_pin_task_subscribe_set(const mpsl_fem_gpiote_pin_t *p_obj,
							      bool active, uint8_t dppi_ch)
{
#if defined(NRF54L_SERIES)
	nrf_ppib_subscribe_set(NRF_PPIB11, p_obj->ppib_tasks_active[active], dppi_ch);
#else
	nrf_gpiote_subscribe_set(p_obj->p_gpiote, p_obj->tasks_active[active], dppi_ch);
#endif
}

__STATIC_INLINE__ void mpsl_fem_gpiote_pin_task_subscribe_clear(const mpsl_fem_gpiote_pin_t *p_obj,
								bool active)
{
#if defined(NRF54L_SERIES)
	nrf_ppib_subscribe_clear(NRF_PPIB11, p_obj->ppib_tasks_active[active]);
#else
	nrf_gpiote_subscribe_clear(p_obj->p_gpiote, p_obj->tasks_active[active]);
#endif
}

#endif /* DPPI_PRESENT */

__STATIC_INLINE__ nrf_gpiote_task_t mpsl_fem_gpiote_pin_config_action_task_get(
	const mpsl_fem_gpiote_pin_config_t *p_config, bool activate)
{
	// Note: nrf_gpiote_clr_task_get and nrf_gpiote_set_task_get contain
	// an assert which has run-time cost.
	nrf_gpiote_task_t result;

	if (p_config->active_high ^ activate) {
		result = (nrf_gpiote_task_t)NRFX_OFFSETOF(
			NRF_GPIOTE_Type, TASKS_CLR[p_config->gpiote_ch_id]); // lint !e413
	} else {
		result = (nrf_gpiote_task_t)NRFX_OFFSETOF(
			NRF_GPIOTE_Type, TASKS_SET[p_config->gpiote_ch_id]); // lint !e413
	}

	return result;
}

__STATIC_INLINE__ uint32_t mpsl_fem_gpiote_pin_config_action_task_address_get(
	const mpsl_fem_gpiote_pin_config_t *p_config, bool activate)
{
	uint32_t result;
	NRF_GPIOTE_Type *p_gpiote = mpsl_fem_gpiote_pin_config_gpiote_instance_get(p_config);

	if (p_config->active_high ^ activate) {
		result = (uint32_t)((uintptr_t)(&p_gpiote->TASKS_CLR[p_config->gpiote_ch_id]));
	} else {
		result = (uint32_t)((uintptr_t)(&p_gpiote->TASKS_SET[p_config->gpiote_ch_id]));
	}

	return result;
}

__STATIC_INLINE__ void
mpsl_fem_gpio_pin_config_pin_output_write(const mpsl_fem_gpio_pin_config_t *p_config, bool level)
{
	if (level) {
		nrf_gpio_port_out_set(p_config->gpio_pin.p_port,
				      1UL << p_config->gpio_pin.port_pin);
	} else {
		nrf_gpio_port_out_clear(p_config->gpio_pin.p_port,
					1UL << p_config->gpio_pin.port_pin);
	}
}

__STATIC_INLINE__ void mpsl_fem_pin_output_write(const mpsl_fem_pin_t *p_fem_pin, bool level)
{
	if (level) {
		nrf_gpio_port_out_set(p_fem_pin->p_port, 1UL << p_fem_pin->port_pin);
	} else {
		nrf_gpio_port_out_clear(p_fem_pin->p_port, 1UL << p_fem_pin->port_pin);
	}
}

#endif // MPSL_FEM_PINS_COMMON_DECLARE_ONLY

#undef __STATIC_INLINE__

#endif // MPSL_FEM_PSEMI_PINS_H_
