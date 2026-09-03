.. _dult_readme:

Detecting Unwanted Location Trackers (DULT)
###########################################

.. contents::
   :local:
   :depth: 2

The Detecting Unwanted Location Trackers (DULT) library implements a set of functionalities required for :ref:`ug_dult` with the |NCS|.
The implementation is based on the official `DULT`_ specification, which lists a set of best practices and protocols for products with built-in location tracking capabilities.
Following the specification improves the privacy and safety of individuals by preventing the location tracking products from tracking users without their knowledge or consent.

Accessory non-owner service (ANOS)
**********************************

The DULT library implements the accessory non-owner service (ANOS), which is a GATT service that uses the accessory non-owner characteristic to communicate with other devices.
The ANOS uses UUID of ``15190001-12F4-C226-88ED-2AC5579F2A85``.
The accessory non-owner characteristic manages the `DULT Accessory Information`_ and the `DULT Non-owner controls`_ defined in the `DULT`_ specification.

By default, the ANOS accepts operations only in the separated near-owner state, as required by the DULT specification.
An accessory-locating network layered on top of DULT can override this gate for the Accessory Information operations by registering the :c:struct:`dult_bt_anos_cb` structure with the :c:func:`dult_bt_anos_cb_register` function.
The Non-owner controls operations remain gated on the separated near-owner state regardless of the registered callback, and operations that are not defined by the DULT specification are always rejected.

Location-enabled advertising
****************************

The DULT specification defines a location-enabled advertising payload with a fixed part that is common to every accessory-locating network.
Integrating this payload is up to the accessory-locating network, and not every network uses it.
Some networks define their own advertising payload structure instead.

If your accessory-locating network does integrate the DULT location-enabled advertising payload, use the :c:func:`dult_bt_adv_data_fill` function to serialize its fixed part into a caller-provided buffer, followed by the optional network-specific proprietary data.
This way the network does not have to assemble the common part itself.
Only the currently associated DULT user can call this function, as the payload is defined only for an associated accessory.

Describe the payload with the :c:struct:`dult_bt_adv_data` structure and initialize it with one of the :c:macro:`DULT_BT_ADV_DATA_INIT`, :c:macro:`DULT_BT_ADV_DATA_PROPRIETARY_INIT`, or :c:macro:`DULT_BT_ADV_DATA_NO_PROPRIETARY_INIT` macros.
The :c:member:`dult_bt_adv_data.flags_present` field selects the proprietary data length limit: :c:macro:`DULT_BT_ADV_PROPRIETARY_DATA_MAX_LEN_WITH_FLAGS` when the surrounding advertising payload includes the optional Flags AD type, and :c:macro:`DULT_BT_ADV_PROPRIETARY_DATA_MAX_LEN_NO_FLAGS` when it does not.

.. _dult_configuration:

Configuration
*************

Set the :kconfig:option:`CONFIG_DULT` Kconfig option to enable the module.
This Kconfig option depends on the :kconfig:option:`CONFIG_BT` option that enables the Bluetooth® stack.

The following Kconfig options are also available for this module:

* :kconfig:option:`CONFIG_DULT_API_VARIANT_V1` and :kconfig:option:`CONFIG_DULT_API_VARIANT_V2` - These options select the DULT API contract.
  The :kconfig:option:`CONFIG_DULT_API_VARIANT_V1` option is chosen by default and is deprecated.
  For details on the differences between the variants, see the :ref:`DULT API variant <ug_dult_api_variant>` section of the DULT integration guide.

* :kconfig:option:`CONFIG_DULT_USER_MAX` - This option sets the maximum number of DULT users that can be registered at the same time.
  The default value is set to ``1``.
  Values greater than ``1`` require the :kconfig:option:`CONFIG_DULT_API_VARIANT_V2` option, as the :kconfig:option:`CONFIG_DULT_API_VARIANT_V1` option supports only a single DULT user.

* :kconfig:option:`CONFIG_DULT_ACCESSORY_TYPE_SMALL` and :kconfig:option:`CONFIG_DULT_ACCESSORY_TYPE_LARGE` - These options declare the size and the discoverability of your accessory.
  The :kconfig:option:`CONFIG_DULT_ACCESSORY_TYPE_SMALL` option is chosen by default.
  The DULT specification best practices are required for small and not easily discoverable accessories, and only recommended for large and easily discoverable ones.
  With the :kconfig:option:`CONFIG_DULT_ACCESSORY_TYPE_SMALL` option, the module validates the mandatory accessory capabilities during the DULT user registration.

* :kconfig:option:`CONFIG_DULT_BATTERY` - This option enables support for battery information such as battery type and battery level.
  The battery information is an optional feature in the DULT specification.
  By default, this option is disabled.

  * :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_CRITICAL_THR`, :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_LOW_THR`, and :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_MEDIUM_THR` - These options allow to configure the mapping between a battery level expressed as a percentage value and battery levels defined in the `DULT`_ specification.
    The default values are set to ``10``, ``40``, and ``80`` respectively.
  * :kconfig:option:`CONFIG_DULT_BATTERY_TYPE_POWERED`, :kconfig:option:`CONFIG_DULT_BATTERY_TYPE_NON_RECHARGEABLE`, and :kconfig:option:`CONFIG_DULT_BATTERY_TYPE_RECHARGEABLE` - These options allow to choose the device's declared battery type.
    By default, the :kconfig:option:`CONFIG_DULT_BATTERY_TYPE_POWERED` is chosen.

* There are following ANOS configuration options for the DULT module:

  * :kconfig:option:`CONFIG_DULT_BT_ANOS_ID_PAYLOAD_LEN_MAX` - This option allows to configure the maximum length of the accessory-locating network identifier.
    The default value is set to ``18``. The identifier is defined by the accessory-locating network that the accessory belongs to.
  * :kconfig:option:`CONFIG_DULT_BT_ANOS_INDICATION_COUNT` - This option allows to configure the number of simultaneously processed GATT indications by the ANOS.
    The default value is set to ``2``.

* :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR` - This option enables support for motion detector.
  The motion detector is an optional feature in the DULT specification.
  For more details see the `DULT motion detector`_ section of the DULT specification.
  This option is disabled by default.

  * :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_TEST_MODE` - This option allows to configure motion detector parameters for testing purposes.
    These values are defined in the DULT specification and should not be changed in the production code.
    This option is disabled by default.
    When this option is enabled, the separated unwanted tracking timing parameters can also be overridden at runtime with the :c:func:`dult_test_motion_detector_separated_ut_period_set` function.
    The runtime values are expressed in seconds and bounded by the :c:macro:`DULT_TEST_MOTION_DETECTOR_PERIOD_MAX` macro, while the Kconfig options below are expressed in minutes.
    New values take effect on the next timer arm and do not restart a timer that is already running.

    * :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_BACKOFF_PERIOD` - This option allows to configure the period in minutes to disable the motion detector if the accessory is in the separated state.
      If this option is configurable, its default value is set to ``2``.
      Otherwise, its default value is set to ``360`` according to the DULT specification.
    * :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MIN` - This option allows to configure the minimum time span in minutes in separated state before enabling motion detector.
      If this option is configurable, its default value is set to ``3``.
      Otherwise, its default value is set to ``480`` according to the DULT specification.
    * :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_SEPARATED_UT_TIMEOUT_PERIOD_MAX` - This option allows to configure the maximum time span in minutes in separated state before enabling motion detector.
      If this option is configurable, its default value is set to ``3``.
      Otherwise, its default value is set to ``1440`` according to the DULT specification.

See the Kconfig help for details.

.. caution::
   The :kconfig:option:`CONFIG_DULT_MOTION_DETECTOR_TEST_MODE` option selects the :kconfig:option:`CONFIG_DULT_TEST` option, which enables the test-only layer of the DULT module.
   Never enable it in a production build.

Implementation details
**********************

The implementation uses :c:macro:`BT_GATT_SERVICE_DEFINE` to statically define and register the ANOS.
Because of that, the ANOS is still present in the GATT database after the DULT subsystem is disabled.
In the DULT subsystem disabled state, GATT operations on the ANOS are rejected.

The ANOS handles all requests received from the outer world.
In case of an application input needed to handle a GATT operation, the DULT subsystem calls the appropriate registered application callback.
For more details, see the :ref:`Integration steps <ug_integrating_dult>` section of the DULT integration guide.

With the :kconfig:option:`CONFIG_DULT_API_VARIANT_V2` Kconfig option, the module keeps a slot table sized by the :kconfig:option:`CONFIG_DULT_USER_MAX` Kconfig option.
Each slot holds the per-user state, which is why callbacks, the battery level, and the near-owner state survive the :c:func:`dult_reset` function call and are only cleared by the :c:func:`dult_user_unregister` function call.

API documentation
*****************

| Header files: :file:`include/dult/dult.h`, :file:`include/dult/bt.h`, :file:`include/dult/multi_user.h`, :file:`include/dult/test.h`
| Source files: :file:`subsys/dult`

.. note::
   The :file:`include/dult.h` header is deprecated and only includes :file:`include/dult/dult.h`.
   Include :file:`include/dult/dult.h` directly instead.

.. doxygengroup:: dult

.. doxygengroup:: dult_bt

.. doxygengroup:: dult_multi_user

.. doxygengroup:: dult_test
