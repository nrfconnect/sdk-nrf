/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICE_H_
#define ZEPHYR_INCLUDE_DEVICE_H_

/**
 * @brief Device Driver APIs
 * @defgroup io_interfaces Device Driver APIs
 * @{
 * @}
 */
/**
 * @brief Miscellaneous Drivers APIs
 * @defgroup misc_interfaces Miscellaneous Drivers APIs
 * @ingroup io_interfaces
 * @{
 * @}
 */

/**
 * @brief Device Model
 * @defgroup device_model Device Model
 * @{
 * @}
 */

/**
 * @brief Device Model APIs
 * @defgroup device_model_api Device Model APIs
 * @ingroup device_model
 * @{
 */

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type used to represent a "handle" for a device.
 *
 * Every struct device has an associated handle. You can get a pointer
 * to a device structure from its handle and vice versa, but the
 * handle uses less space than a pointer. The device.h API mainly uses
 * handles to store lists of multiple devices in a compact way.
 *
 * The extreme values and zero have special significance. Negative
 * values identify functionality that does not correspond to a Zephyr
 * device, such as the system clock or a SYS_INIT() function.
 *
 * @see device_handle_get()
 * @see device_from_handle()
 */
typedef int16_t device_handle_t;

/** @brief Flag value used in lists of device handles to separate
 * distinct groups.
 *
 * This is the minimum value for the device_handle_t type.
 */
#define DEVICE_HANDLE_SEP INT16_MIN

/** @brief Flag value used in lists of device handles to indicate the
 * end of the list.
 *
 * This is the maximum value for the device_handle_t type.
 */
#define DEVICE_HANDLE_ENDS INT16_MAX

/** @brief Flag value used to identify an unknown device. */
#define DEVICE_HANDLE_NULL 0

/**
 * @brief Expands to the name of a global device object.
 *
 * @details Return the full name of a device object symbol created by
 * DEVICE_DEFINE(), using the dev_id provided to DEVICE_DEFINE().
 * This is the name of the global variable storing the device
 * structure, not a pointer to the string in the device's @p name
 * field.
 *
 * It is meant to be used for declaring extern symbols pointing to device
 * objects before using the DEVICE_GET macro to get the device object.
 *
 * This macro is normally only useful within device driver source
 * code. In other situations, you are probably looking for
 * device_get_binding().
 *
 * @param name The same @p dev_id token given to DEVICE_DEFINE()
 *
 * @return The full name of the device object defined by DEVICE_DEFINE()
 */
#define DEVICE_NAME_GET(name) _CONCAT(__device_, name)

/* Node paths can exceed the maximum size supported by
 * device_get_binding() in user mode; this macro synthesizes a unique
 * dev_id from a devicetree node while staying within this maximum
 * size.
 *
 * The ordinal used in this name can be mapped to the path by
 * examining zephyr/include/generated/devicetree_generated.h.
 */
#define Z_DEVICE_DT_DEV_ID(node_id) _CONCAT(dts_ord_, DT_DEP_ORD(node_id))

/**
 * @brief Create a device object and set it up for boot time initialization.
 *
 * @details This macro defines a <tt>struct device</tt> that is
 * automatically configured by the kernel during system
 * initialization. This macro should only be used when the device is
 * not being allocated from a devicetree node. If you are allocating a
 * device from a devicetree node, use DEVICE_DT_DEFINE() or
 * DEVICE_DT_INST_DEFINE() instead.
 *
 * @param dev_id A unique token which is used in the name of the
 * global device structure as a C identifier.
 *
 * @param name A string name for the device, which will be stored
 * in the device structure's @p name field. This name can be used to
 * look up the device with device_get_binding(). This must be less
 * than Z_DEVICE_MAX_NAME_LEN characters (including terminating NUL)
 * in order to be looked up from user mode.
 *
 * @param init_fn Pointer to the device's initialization function,
 * which will be run by the kernel during system initialization.
 *
 * @param pm Pointer to the device's power management
 * resources, a <tt>struct pm_device</tt>, which will be stored in the
 * device structure's @p pm field. Use NULL if the device does not use
 * PM.
 *
 * @param data Pointer to the device's private mutable data, which
 * will be stored in the device structure's @p data field.
 *
 * @param config Pointer to the device's private constant data, which
 * will be stored in the device structure's @p config field.
 *
 * @param level The device's initialization level. See SYS_INIT() for
 * details.
 *
 * @param prio The device's priority within its initialization level.
 * See SYS_INIT() for details.
 *
 * @param api Pointer to the device's API structure. Can be NULL.
 */
#define DEVICE_DEFINE(dev_id, name, init_fn, pm, data, config, level, prio,    \
		      api)                                                     \
	Z_DEVICE_STATE_DEFINE(dev_id);                                         \
	Z_DEVICE_DEFINE(DT_INVALID_NODE, dev_id, name, init_fn, pm, data,      \
			config, level, prio, api,                              \
			&Z_DEVICE_STATE_NAME(dev_id))

/**
 * @brief Return a string name for a devicetree node.
 *
 * @details This macro returns a string literal usable as a device's
 * @p name field from a devicetree node identifier.
 *
 * @param node_id The devicetree node identifier.
 *
 * @return The value of the node's "label" property, if it has one.
 * Otherwise, the node's full name in "node-name@@unit-address" form.
 */
#define DEVICE_DT_NAME(node_id) \
	DT_PROP_OR(node_id, label, DT_NODE_FULL_NAME(node_id))

/**
 * @brief Create a device object from a devicetree node identifier and
 * set it up for boot time initialization.
 *
 * @details This macro defines a <tt>struct device</tt> that is
 * automatically configured by the kernel during system
 * initialization. The global device object's name as a C identifier
 * is derived from the node's dependency ordinal. The device
 * structure's @p name field is set to
 * <tt>DEVICE_DT_NAME(node_id)</tt>.
 *
 * The device is declared with extern visibility, so a pointer to a
 * global device object can be obtained with
 * <tt>DEVICE_DT_GET(node_id)</tt> from any source file that includes
 * device.h. Before using the pointer, the referenced object should be
 * checked using device_is_ready().
 *
 * @param node_id The devicetree node identifier.
 *
 * @param init_fn Pointer to the device's initialization function,
 * which will be run by the kernel during system initialization.
 *
 * @param pm Pointer to the device's power management
 * resources, a <tt>struct pm_device</tt>, which will be stored in the
 * device structure's @p pm field. Use NULL if the device does not use
 * PM.
 *
 * @param data Pointer to the device's private mutable data, which
 * will be stored in the device structure's @p data field.
 *
 * @param config Pointer to the device's private constant data, which
 * will be stored in the device structure's @p config field.
 *
 * @param level The device's initialization level. See SYS_INIT() for
 * details.
 *
 * @param prio The device's priority within its initialization level.
 * See SYS_INIT() for details.
 *
 * @param api Pointer to the device's API structure. Can be NULL.
 */
#define DEVICE_DT_DEFINE(node_id, init_fn, pm, data, config, level, prio, api, \
			 ...)                                                  \
	Z_DEVICE_STATE_DEFINE(Z_DEVICE_DT_DEV_ID(node_id));                    \
	Z_DEVICE_DEFINE(node_id, Z_DEVICE_DT_DEV_ID(node_id),                  \
			DEVICE_DT_NAME(node_id), init_fn, pm, data, config,    \
			level, prio, api,                                      \
			&Z_DEVICE_STATE_NAME(Z_DEVICE_DT_DEV_ID(node_id)),     \
			__VA_ARGS__)

/**
 * @brief Like DEVICE_DT_DEFINE(), but uses an instance of a
 * DT_DRV_COMPAT compatible instead of a node identifier.
 *
 * @param inst instance number. The @p node_id argument to
 * DEVICE_DT_DEFINE is set to <tt>DT_DRV_INST(inst)</tt>.
 *
 * @param ... other parameters as expected by DEVICE_DT_DEFINE.
 */
#define DEVICE_DT_INST_DEFINE(inst, ...) \
	DEVICE_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief The name of the global device object for @p node_id
 *
 * @details Returns the name of the global device structure as a C
 * identifier. The device must be allocated using DEVICE_DT_DEFINE()
 * or DEVICE_DT_INST_DEFINE() for this to work.
 *
 * This macro is normally only useful within device driver source
 * code. In other situations, you are probably looking for
 * DEVICE_DT_GET().
 *
 * @param node_id Devicetree node identifier
 *
 * @return The name of the device object as a C identifier
 */
#define DEVICE_DT_NAME_GET(node_id) DEVICE_NAME_GET(Z_DEVICE_DT_DEV_ID(node_id))

/**
 * @brief Get a <tt>const struct device*</tt> from a devicetree node
 * identifier
 *
 * @details Returns a pointer to a device object created from a
 * devicetree node, if any device was allocated by a driver.
 *
 * If no such device was allocated, this will fail at linker time. If
 * you get an error that looks like <tt>undefined reference to
 * __device_dts_ord_<N></tt>, that is what happened. Check to make
 * sure your device driver is being compiled, usually by enabling the
 * Kconfig options it requires.
 *
 * @param node_id A devicetree node identifier
 * @return A pointer to the device object created for that node
 */
#define DEVICE_DT_GET(node_id) (&DEVICE_DT_NAME_GET(node_id))

/**
 * @brief Get a <tt>const struct device*</tt> for an instance of a
 *        DT_DRV_COMPAT compatible
 *
 * @details This is equivalent to <tt>DEVICE_DT_GET(DT_DRV_INST(inst))</tt>.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @return A pointer to the device object created for that instance
 */
#define DEVICE_DT_INST_GET(inst) DEVICE_DT_GET(DT_DRV_INST(inst))

/**
 * @brief Get a <tt>const struct device*</tt> from a devicetree compatible
 *
 * If an enabled devicetree node has the given compatible and a device
 * object was created from it, this returns a pointer to that device.
 *
 * If there no such devices, this returns NULL.
 *
 * If there are multiple, this returns an arbitrary one.
 *
 * If this returns non-NULL, the device must be checked for readiness
 * before use, e.g. with device_is_ready().
 *
 * @param compat lowercase-and-underscores devicetree compatible
 * @return a pointer to a device, or NULL
 */
#define DEVICE_DT_GET_ANY(compat)					    \
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(compat),			    \
		    (DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(compat))), \
		    (NULL))

/**
 * @brief Get a <tt>const struct device*</tt> from a devicetree compatible
 *
 * @details If an enabled devicetree node has the given compatible and
 * a device object was created from it, this returns a pointer to that
 * device.
 *
 * If there no such devices, this will fail at compile time.
 *
 * If there are multiple, this returns an arbitrary one.
 *
 * If this returns non-NULL, the device must be checked for readiness
 * before use, e.g. with device_is_ready().
 *
 * @param compat lowercase-and-underscores devicetree compatible
 * @return a pointer to a device
 */
#define DEVICE_DT_GET_ONE(compat)					    \
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(compat),			    \
		    (DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(compat))), \
		    (ZERO_OR_COMPILE_ERROR(0)))

/**
 * @brief Utility macro to obtain an optional reference to a device.
 *
 * @details If the node identifier refers to a node with status
 * "okay", this returns <tt>DEVICE_DT_GET(node_id)</tt>. Otherwise, it
 * returns NULL.
 *
 * @param node_id devicetree node identifier
 *
 * @return a <tt>const struct device*</tt> for the node identifier,
 * which may be NULL.
 */
#define DEVICE_DT_GET_OR_NULL(node_id)					\
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),			\
		    (DEVICE_DT_GET(node_id)), (NULL))

/**
 * @brief Obtain a pointer to a device object by name
 *
 * @details Return the address of a device object created by
 * DEVICE_DEFINE(), using the dev_id provided to DEVICE_DEFINE().
 *
 * @param name The same as dev_id provided to DEVICE_DEFINE()
 *
 * @return A pointer to the device object created by DEVICE_DEFINE()
 */
#define DEVICE_GET(name) (&DEVICE_NAME_GET(name))

/**
 * @brief Declare a static device object
 *
 * This macro can be used at the top-level to declare a device, such
 * that DEVICE_GET() may be used before the full declaration in
 * DEVICE_DEFINE().
 *
 * This is often useful when configuring interrupts statically in a
 * device's init or per-instance config function, as the init function
 * itself is required by DEVICE_DEFINE() and use of DEVICE_GET()
 * inside it creates a circular dependency.
 *
 * @param name Device name
 */
#define DEVICE_DECLARE(name) static const struct device DEVICE_NAME_GET(name)

/**
 * @brief Get a <tt>const struct init_entry*</tt> from a devicetree node
 *
 * @param node_id A devicetree node identifier
 *
 * @return A pointer to the init_entry object created for that node
 */
#define DEVICE_INIT_DT_GET(node_id) (&Z_INIT_ENTRY_NAME(DEVICE_DT_NAME_GET(node_id)))

/**
 * @brief Get a <tt>const struct init_entry*</tt> from a device by name
 *
 * @param name The same as dev_id provided to DEVICE_DEFINE()
 *
 * @return A pointer to the init_entry object created for that device
 */
#define DEVICE_INIT_GET(name) (&Z_INIT_ENTRY_NAME(DEVICE_NAME_GET(name)))

/**
 * @brief Runtime device dynamic structure (in RAM) per driver instance
 *
 * Fields in this are expected to be default-initialized to zero. The
 * kernel driver infrastructure and driver access functions are
 * responsible for ensuring that any non-zero initialization is done
 * before they are accessed.
 */
struct device_state {
	/** Non-negative result of initializing the device.
	 *
	 * The absolute value returned when the device initialization
	 * function was invoked, or `UINT8_MAX` if the value exceeds
	 * an 8-bit integer. If initialized is also set, a zero value
	 * indicates initialization succeeded.
	 */
	unsigned int init_res : 8;

	/** Indicates the device initialization function has been
	 * invoked.
	 */
	bool initialized : 1;
};

struct pm_device;

#ifdef CONFIG_HAS_DYNAMIC_DEVICE_HANDLES
#define Z_DEVICE_HANDLES_CONST
#else
#define Z_DEVICE_HANDLES_CONST const
#endif

/**
 * @brief Runtime device structure (in ROM) per driver instance
 */
struct device {
	/** Name of the device instance */
	const char *name;
	/** Address of device instance config information */
	const void *config;
	/** Address of the API structure exposed by the device instance */
	const void *api;
	/** Address of the common device state */
	struct device_state *state;
	/** Address of the device instance private data */
	void *data;
	/** optional pointer to handles associated with the device.
	 *
	 * This encodes a sequence of sets of device handles that have
	 * some relationship to this node. The individual sets are
	 * extracted with dedicated API, such as
	 * device_required_handles_get().
	 */
	Z_DEVICE_HANDLES_CONST device_handle_t *handles;

#ifdef CONFIG_PM_DEVICE
	/** Reference to the device PM resources. */
	struct pm_device *pm;
#endif
};

/**
 * @brief Get the handle for a given device
 *
 * @param dev the device for which a handle is desired.
 *
 * @return the handle for the device, or DEVICE_HANDLE_NULL if the
 * device does not have an associated handle.
 */
static inline device_handle_t
device_handle_get(const struct device *dev)
{
	device_handle_t ret = DEVICE_HANDLE_NULL;
	extern const struct device __device_start[];

	/* TODO: If/when devices can be constructed that are not part of the
	 * fixed sequence we'll need another solution.
	 */
	if (dev != NULL) {
		ret = 1 + (device_handle_t)(dev - __device_start);
	}

	return ret;
}

/**
 * @brief Get the device corresponding to a handle.
 *
 * @param dev_handle the device handle
 *
 * @return the device that has that handle, or a null pointer if @p
 * dev_handle does not identify a device.
 */
static inline const struct device *
device_from_handle(device_handle_t dev_handle)
{
	extern const struct device __device_start[];
	extern const struct device __device_end[];
	const struct device *dev = NULL;
	size_t numdev = __device_end - __device_start;

	if ((dev_handle > 0) && ((size_t)dev_handle <= numdev)) {
		dev = &__device_start[dev_handle - 1];
	}

	return dev;
}

/**
 * @brief Prototype for functions used when iterating over a set of devices.
 *
 * Such a function may be used in API that identifies a set of devices and
 * provides a visitor API supporting caller-specific interaction with each
 * device in the set.
 *
 * The visit is said to succeed if the visitor returns a non-negative value.
 *
 * @param dev a device in the set being iterated
 *
 * @param context state used to support the visitor function
 *
 * @return A non-negative number to allow walking to continue, and a negative
 * error code to case the iteration to stop.
 *
 * @see device_required_foreach()
 * @see device_supported_foreach()
 */
typedef int (*device_visitor_callback_t)(const struct device *dev, void *context);

/**
 * @brief Get the device handles for devicetree dependencies of this device.
 *
 * This function returns a pointer to an array of device handles. The
 * length of the array is stored in the @p count parameter.
 *
 * The array contains a handle for each device that @p dev requires
 * directly, as determined from the devicetree. This does not include
 * transitive dependencies; you must recursively determine those.
 *
 * @param dev the device for which dependencies are desired.
 *
 * @param count pointer to where this function should store the length
 * of the returned array. No value is stored if the call returns a
 * null pointer. The value may be set to zero if the device has no
 * devicetree dependencies.
 *
 * @return a pointer to a sequence of @p *count device handles, or a null
 * pointer if @p dev does not have any dependency data.
 */
static inline const device_handle_t *
device_required_handles_get(const struct device *dev,
			    size_t *count)
{
	const device_handle_t *rv = dev->handles;

	if (rv != NULL) {
		size_t i = 0;

		while ((rv[i] != DEVICE_HANDLE_ENDS)
		       && (rv[i] != DEVICE_HANDLE_SEP)) {
			++i;
		}
		*count = i;
	}

	return rv;
}

/**
 * @brief Get the device handles for injected dependencies of this device.
 *
 * This function returns a pointer to an array of device handles. The
 * length of the array is stored in the @p count parameter.
 *
 * The array contains a handle for each device that @p dev manually injected
 * as a dependency, via providing extra arguments to Z_DEVICE_DEFINE. This does
 * not include transitive dependencies; you must recursively determine those.
 *
 * @param dev the device for which injected dependencies are desired.
 *
 * @param count pointer to where this function should store the length
 * of the returned array. No value is stored if the call returns a
 * null pointer. The value may be set to zero if the device has no
 * devicetree dependencies.
 *
 * @return a pointer to a sequence of @p *count device handles, or a null
 * pointer if @p dev does not have any dependency data.
 */
static inline const device_handle_t *
device_injected_handles_get(const struct device *dev,
			    size_t *count)
{
	const device_handle_t *rv = dev->handles;
	size_t region = 0;
	size_t i = 0;

	if (rv != NULL) {
		/* Fast forward to injected devices */
		while (region != 1) {
			if (*rv == DEVICE_HANDLE_SEP) {
				region++;
			}
			rv++;
		}
		while ((rv[i] != DEVICE_HANDLE_ENDS)
		       && (rv[i] != DEVICE_HANDLE_SEP)) {
			++i;
		}
		*count = i;
	}

	return rv;
}

/**
 * @brief Get the set of handles that this device supports.
 *
 * This function returns a pointer to an array of device handles. The
 * length of the array is stored in the @p count parameter.
 *
 * The array contains a handle for each device that @p dev "supports"
 * -- that is, devices that require @p dev directly -- as determined
 * from the devicetree. This does not include transitive dependencies;
 * you must recursively determine those.
 *
 * @param dev the device for which supports are desired.
 *
 * @param count pointer to where this function should store the length
 * of the returned array. No value is stored if the call returns a
 * null pointer. The value may be set to zero if nothing in the
 * devicetree depends on @p dev.
 *
 * @return a pointer to a sequence of @p *count device handles, or a null
 * pointer if @p dev does not have any dependency data.
 */
static inline const device_handle_t *
device_supported_handles_get(const struct device *dev,
			     size_t *count)
{
	const device_handle_t *rv = dev->handles;
	size_t region = 0;
	size_t i = 0;

	if (rv != NULL) {
		/* Fast forward to supporting devices */
		while (region != 2) {
			if (*rv == DEVICE_HANDLE_SEP) {
				region++;
			}
			rv++;
		}
		/* Count supporting devices */
		while (rv[i] != DEVICE_HANDLE_ENDS) {
			++i;
		}
		*count = i;
	}

	return rv;
}

/**
 * @brief Visit every device that @p dev directly requires.
 *
 * Zephyr maintains information about which devices are directly required by
 * another device; for example an I2C-based sensor driver will require an I2C
 * controller for communication. Required devices can derive from
 * statically-defined devicetree relationships or dependencies registered
 * at runtime.
 *
 * This API supports operating on the set of required devices. Example uses
 * include making sure required devices are ready before the requiring device
 * is used, and releasing them when the requiring device is no longer needed.
 *
 * There is no guarantee on the order in which required devices are visited.
 *
 * If the @p visitor function returns a negative value iteration is halted,
 * and the returned value from the visitor is returned from this function.
 *
 * @note This API is not available to unprivileged threads.
 *
 * @param dev a device of interest. The devices that this device depends on
 * will be used as the set of devices to visit. This parameter must not be
 * null.
 *
 * @param visitor_cb the function that should be invoked on each device in the
 * dependency set. This parameter must not be null.
 *
 * @param context state that is passed through to the visitor function. This
 * parameter may be null if @p visitor tolerates a null @p context.
 *
 * @return The number of devices that were visited if all visits succeed, or
 * the negative value returned from the first visit that did not succeed.
 */
int device_required_foreach(const struct device *dev,
			  device_visitor_callback_t visitor_cb,
			  void *context);

/**
 * @brief Visit every device that @p dev directly supports.
 *
 * Zephyr maintains information about which devices are directly supported by
 * another device; for example an I2C controller will support an I2C-based
 * sensor driver. Supported devices can derive from statically-defined
 * devicetree relationships.
 *
 * This API supports operating on the set of supported devices. Example uses
 * include iterating over the devices connected to a regulator when it is
 * powered on.
 *
 * There is no guarantee on the order in which required devices are visited.
 *
 * If the @p visitor function returns a negative value iteration is halted,
 * and the returned value from the visitor is returned from this function.
 *
 * @note This API is not available to unprivileged threads.
 *
 * @param dev a device of interest. The devices that this device supports
 * will be used as the set of devices to visit. This parameter must not be
 * null.
 *
 * @param visitor_cb the function that should be invoked on each device in the
 * support set. This parameter must not be null.
 *
 * @param context state that is passed through to the visitor function. This
 * parameter may be null if @p visitor tolerates a null @p context.
 *
 * @return The number of devices that were visited if all visits succeed, or
 * the negative value returned from the first visit that did not succeed.
 */
int device_supported_foreach(const struct device *dev,
			     device_visitor_callback_t visitor_cb,
			     void *context);

/**
 * @brief Get a <tt>const struct device*</tt> from its @p name field
 *
 * @details This function iterates through the devices on the system.
 * If a device with the given @p name field is found, and that device
 * initialized successfully at boot time, this function returns a
 * pointer to the device.
 *
 * If no device has the given name, this function returns NULL.
 *
 * This function also returns NULL when a device is found, but it
 * failed to initialize successfully at boot time. (To troubleshoot
 * this case, set a breakpoint on your device driver's initialization
 * function.)
 *
 * @param name device name to search for. A null pointer, or a pointer
 * to an empty string, will cause NULL to be returned.
 *
 * @return pointer to device structure with the given name; NULL if
 * the device is not found or if the device with that name's
 * initialization function failed.
 */
__syscall const struct device *device_get_binding(const char *name);

/** @brief Get access to the static array of static devices.
 *
 * @param devices where to store the pointer to the array of
 * statically allocated devices. The array must not be mutated
 * through this pointer.
 *
 * @return the number of statically allocated devices.
 */
size_t z_device_get_all_static(const struct device * *devices);

/**
 * @brief Verify that a device is ready for use.
 *
 * This is the implementation underlying device_is_ready(), without the overhead
 * of a syscall wrapper.
 *
 * @param dev pointer to the device in question.
 *
 * @retval true If the device is ready for use.
 * @retval false If the device is not ready for use or if a NULL device pointer
 * is passed as argument.
 *
 * @see device_is_ready()
 */
bool z_device_is_ready(const struct device *dev);

/** @brief Verify that a device is ready for use.
 *
 * Indicates whether the provided device pointer is for a device known to be
 * in a state where it can be used with its standard API.
 *
 * This can be used with device pointers captured from DEVICE_DT_GET(), which
 * does not include the readiness checks of device_get_binding(). At minimum
 * this means that the device has been successfully initialized.
 *
 * @param dev pointer to the device in question.
 *
 * @retval true If the device is ready for use.
 * @retval false If the device is not ready for use or if a NULL device pointer
 * is passed as argument.
 */
__syscall bool device_is_ready(const struct device *dev);

static inline bool z_impl_device_is_ready(const struct device *dev)
{
	return z_device_is_ready(dev);
}

/**
 * @}
 */

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Synthesize a unique name for the device state associated with
 * dev_id.
 */
#define Z_DEVICE_STATE_NAME(dev_id) _CONCAT(__devstate_, dev_id)

/**
 * @brief Utility macro to define and initialize the device state.
 *
 * @param dev_id Device identifier.
 */
#define Z_DEVICE_STATE_DEFINE(dev_id)				\
	static struct device_state Z_DEVICE_STATE_NAME(dev_id)	\
	__attribute__((__section__(".z_devstate")))

/**
 * @brief Synthesize the name of the object that holds device ordinal and
 * dependency data.
 *
 * @param dev_id Device identifier.
 */
#define Z_DEVICE_HANDLES_NAME(dev_id)                                          \
	_CONCAT(__devicehdl_, dev_id)

/**
 * @brief Expand extra handles with a comma in between.
 *
 * @param ... Extra handles
 */
#define Z_DEVICE_EXTRA_HANDLES(...)				\
	FOR_EACH_NONEMPTY_TERM(IDENTITY, (,), __VA_ARGS__)

/** @brief Linker section were device handles are placed. */
#define Z_DEVICE_HANDLES_SECTION                                              \
	__attribute__((__section__(".__device_handles_pass1")))

/**
 * @brief Define device handles.
 *
 * Initial build provides a record that associates the device object
 * with its devicetree ordinal, and provides the dependency ordinals.
 * These are provided as weak definitions (to prevent the reference
 * from being captured when the original object file is compiled), and
 * in a distinct pass1 section (which will be replaced by
 * postprocessing).
 *
 * Before processing in gen_handles.py, the array format is:
 * {
 *     DEVICE_ORDINAL (or DEVICE_HANDLE_NULL if not a devicetree node),
 *     List of devicetree dependency ordinals (if any),
 *     DEVICE_HANDLE_SEP,
 *     List of injected dependency ordinals (if any),
 *     DEVICE_HANDLE_SEP,
 *     List of devicetree supporting ordinals (if any),
 * }
 *
 * After processing in gen_handles.py, the format is updated to:
 * {
 *     List of existing devicetree dependency handles (if any),
 *     DEVICE_HANDLE_SEP,
 *     List of injected devicetree dependency handles (if any),
 *     DEVICE_HANDLE_SEP,
 *     List of existing devicetree support handles (if any),
 *     DEVICE_HANDLE_NULL
 * }
 *
 * It is also (experimentally) necessary to provide explicit alignment
 * on each object. Otherwise x86-64 builds will introduce padding
 * between objects in the same input section in individual object
 * files, which will be retained in subsequent links both wasting
 * space and resulting in aggregate size changes relative to pass2
 * when all objects will be in the same input section.
 */
#define Z_DEVICE_HANDLES_DEFINE(node_id, dev_id, ...)                          \
	extern Z_DEVICE_HANDLES_CONST device_handle_t                          \
		Z_DEVICE_HANDLES_NAME(dev_id)[];                               \
	Z_DEVICE_HANDLES_CONST Z_DECL_ALIGN(device_handle_t)                   \
	Z_DEVICE_HANDLES_SECTION __weak Z_DEVICE_HANDLES_NAME(dev_id)[] = {    \
		COND_CODE_1(DT_NODE_EXISTS(node_id),                           \
			    (DT_DEP_ORD(node_id),                              \
			     DT_REQUIRES_DEP_ORDS(node_id)),                   \
			    (DEVICE_HANDLE_NULL,))                             \
		DEVICE_HANDLE_SEP,                                             \
		Z_DEVICE_EXTRA_HANDLES(__VA_ARGS__)                            \
		DEVICE_HANDLE_SEP,                                             \
		COND_CODE_1(DT_NODE_EXISTS(node_id),                           \
			    (DT_SUPPORTS_DEP_ORDS(node_id)), ())               \
	}

/**
 * @brief Maximum device name length.
 *
 * The maximum length is set so that device_get_binding() can be used from
 * userspace.
 */
#define Z_DEVICE_MAX_NAME_LEN	48

/**
 * @brief Compile time check for device name length
 *
 * @param name Device name.
 */
#define Z_DEVICE_NAME_CHECK(name)                                              \
	BUILD_ASSERT(sizeof(Z_STRINGIFY(name)) <= Z_DEVICE_MAX_NAME_LEN,       \
		     Z_STRINGIFY(DEVICE_NAME_GET(name)) " too long")

/**
 * @brief Initializer for struct device.
 *
 * @param name_ Name of the device.
 * @param pm_ Reference to struct pm_device (optional).
 * @param data_ Reference to device data.
 * @param config_ Reference to device config.
 * @param api_ Reference to device API ops.
 * @param state_ Reference to device state.
 * @param handles_ Reference to device handles.
 */
#define Z_DEVICE_INIT(name_, pm_, data_, config_, api_, state_, handles_)      \
	{                                                                      \
		.name = name_,                                                 \
		.data = (data_),                                               \
		.config = (config_),                                           \
		.api = (api_),                                                 \
		.state = (state_),                                             \
		.handles = (handles_),                                         \
		IF_ENABLED(CONFIG_PM_DEVICE, (.pm = (pm_),))                   \
	}

/**
 * @brief Device section
 *
 * Each device is placed in a section with a name crafted so that it allows
 * linker scripts to sort them according to the specified level/priority.
 *
 * @param level Initialization level
 * @param prio Initialization priority
 */
#define Z_DEVICE_SECTION(level, prio)					       \
	__attribute__((__section__(".z_device_" #level STRINGIFY(prio) "_")))

/**
 * @brief Define a struct device
 *
 * @param node_id Devicetree node id for the device (DT_INVALID_NODE if a
 *                software device).
 * @param dev_id Device identifier (used to name the defined struct device).
 * @param name Name of the device.
 * @param pm Reference to struct pm_device associated with the device.
 *           (optional).
 * @param data Reference to device data.
 * @param config Reference to device config.
 * @param level Initialization level.
 * @param prio Initialization priority.
 * @param api Reference to device API.
 * @param ... Optional dependencies, manually specified.
 */
#define Z_DEVICE_BASE_DEFINE(node_id, dev_id, name, pm, data, config, level,   \
			     prio, api, state, handles)                        \
	COND_CODE_1(DT_NODE_EXISTS(node_id), (), (static))                     \
	const Z_DECL_ALIGN(struct device) DEVICE_NAME_GET(dev_id)              \
		Z_DEVICE_SECTION(level, prio) __used =                         \
			Z_DEVICE_INIT(name, pm, data, config, api, state,      \
				      handles)

/**
 * @brief Define the init entry for a device.
 *
 * @param dev_id Device identifier.
 * @param init_fn Device init function.
 * @param level Initialization level.
 * @param prio Initialization priority.
 */
#define Z_DEVICE_INIT_ENTRY_DEFINE(dev_id, init_fn, level, prio)               \
	Z_INIT_ENTRY_DEFINE(DEVICE_NAME_GET(dev_id), init_fn,                  \
			    (&DEVICE_NAME_GET(dev_id)), level, prio)

/**
 * @brief Define a struct device and all other required objects.
 *
 * This is the common macro used to define struct device objects. It can be
 * used to define both Devicetree and software devices.
 *
 * @param node_id Devicetree node id for the device (DT_INVALID_NODE if a
 *                software device).
 * @param dev_id Device identifier (used to name the defined struct device).
 * @param name Name of the device.
 * @param init_fn Device init function.
 * @param pm Reference to struct pm_device associated with the device.
 *           (optional).
 * @param data Reference to device data.
 * @param config Reference to device config.
 * @param level Initialization level.
 * @param prio Initialization priority.
 * @param api Reference to device API.
 * @param state Reference to device state.
 * @param ... Optional dependencies, manually specified.
 */
#define Z_DEVICE_DEFINE(node_id, dev_id, name, init_fn, pm, data, config,      \
			level, prio, api, state, ...)                          \
	Z_DEVICE_NAME_CHECK(name);                                             \
                                                                               \
	Z_DEVICE_HANDLES_DEFINE(node_id, dev_id, __VA_ARGS__);                 \
                                                                               \
	Z_DEVICE_BASE_DEFINE(node_id, dev_id, name, pm, data, config, level,   \
			     prio, api, state, Z_DEVICE_HANDLES_NAME(dev_id)); \
			                                                       \
	Z_DEVICE_INIT_ENTRY_DEFINE(dev_id, init_fn, level, prio)

#if defined(CONFIG_HAS_DTS) || defined(__DOXYGEN__)
/**
 * @brief Declare a device for each status "okay" devicetree node.
 *
 * @note Disabled nodes should not result in devices, so not predeclaring these
 * keeps drivers honest.
 *
 * This is only "maybe" a device because some nodes have status "okay", but
 * don't have a corresponding struct device allocated. There's no way to figure
 * that out until after we've built the zephyr image, though.
 */
#define Z_MAYBE_DEVICE_DECLARE_INTERNAL(node_id) \
	extern const struct device DEVICE_DT_NAME_GET(node_id);

DT_FOREACH_STATUS_OKAY_NODE(Z_MAYBE_DEVICE_DECLARE_INTERNAL)
#endif /* CONFIG_HAS_DTS */

/** @endcond */

#ifdef __cplusplus
}
#endif


#include <syscalls/device.h>

#endif /* ZEPHYR_INCLUDE_DEVICE_H_ */
