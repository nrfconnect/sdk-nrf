.. _nrf_desktop_hid_keymap:

HID keymap utility
##################

.. contents::
   :local:
   :depth: 2

The HID keymap utility can be used by an application module to map an application-specific key ID to a HID report ID and HID usage ID pair according to a statically defined user configuration.

Configuration
*************

Use the :option:`CONFIG_DESKTOP_HID_KEYMAP` Kconfig option to enable the utility.
You can use the utility only on HID peripherals (:option:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL`).

HID keymap
==========

You must define mapping between application-specific key IDs and HID report ID and HID usage ID pairs in utility's configuration.
For that purpose, you must create a configuration file with a ``hid_keymap`` array.
Every element of the array contains mapping from a single application-specific key ID to HID report ID and HID usage ID pair.
The location of the file is specified using the :option:`CONFIG_DESKTOP_HID_KEYMAP_DEF_PATH` Kconfig option.

Make sure that :c:struct:`hid_keymap` entries defined in the ``hid_keymap`` array are sorted ascending by the key ID (:c:member:`hid_keymap.key_id`).
This is required, because the utility uses binary search (:c:func:`bsearch`) to speed up searching through the array.

For example, the file contents should look like the following:

.. code-block:: c

	#include <caf/key_id.h>

	#include "hid_keymap.h"
	#inclue "fn_key_id.h"

	static const struct hid_keymap hid_keymap[] = {
		{ KEY_ID(0x00, 0x01), 0x0014, REPORT_ID_KEYBOARD_KEYS }, /* Q */
		{ KEY_ID(0x00, 0x02), 0x001A, REPORT_ID_KEYBOARD_KEYS }, /* W */
		{ KEY_ID(0x00, 0x03), 0x0008, REPORT_ID_KEYBOARD_KEYS }, /* E */
		{ KEY_ID(0x00, 0x04), 0x0015, REPORT_ID_KEYBOARD_KEYS }, /* R */
		{ KEY_ID(0x00, 0x05), 0x0018, REPORT_ID_KEYBOARD_KEYS }, /* U */

		...

		{ FN_KEY_ID(0x06, 0x02), 0x0082, REPORT_ID_SYSTEM_CTRL },   /* sleep */
		{ FN_KEY_ID(0x06, 0x03), 0x0196, REPORT_ID_CONSUMER_CTRL }, /* internet */
	};

.. note::
   The configuration file should be included only by the configured utility.
   Do not include the configuration file in other source files.

Caching
=======

By default, the utility caches the last returned key mapping to improve performance if mapping for the same key ID is requested multiple times in a row.
This happens, for example, if a button that was recently pressed is released.
You can disable the :option:`CONFIG_DESKTOP_HID_KEYMAP_CACHE` Kconfig option to turn off caching.

Using HID keymap
****************

The HID keymap utility cannot be instantiated, but the utility can be used by multiple application modules.

Initialization
==============

The utility must be initialized before use.
The initialization function (:c:func:`hid_keymap_init`) can be called multiple times.

Mapping key IDs
===============

You can use the :c:func:`hid_keymap_get` function to map key ID to the :c:struct:`hid_keymap` structure.
The structure contains the HID report ID (:c:member:`hid_keymap.report_id`) and HID usage ID (:c:member:`hid_keymap.hid_usage_id`).

API documentation
*****************

Application modules can use the following API of the HID keymap:

| Header file: :file:`applications/nrf_desktop/src/util/hid_keymap.h`
| Source file: :file:`applications/nrf_desktop/src/util/hid_keymap.c`

.. doxygengroup:: hid_keymap
