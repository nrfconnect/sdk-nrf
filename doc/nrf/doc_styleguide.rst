.. _doc_styleguide:

Documentation style guide
#########################

.. note::

   These style guides are not finalized. Details may change.

NCS documentation is written in two formats:

* Doxygen for API documentation
* RST for conceptual documentation


RST style guide
***************

See :ref:`zephyr:doc_guidelines` for a short introduction to RST and, most importantly, to the conventions used in Zephyr.
More information about RST is available in the `reStructuredText Primer`_.

The |NCS| documentation follows the Zephyr style guide, and adds a few more restrictive rules:

* Headings use sentence case, which means that the first word is capitalized, and the following words use normal capitalization.
* Do not use consecutive headings without intervening text.
* For readability reasons, start every sentence on a new line in the source files.
  In the output, consecutive lines without blank lines in between are combined into one paragraph.

Hyperlinks
==========

All external links must be defined in the ``links.txt`` file.
Do not define them directly in the RST file.
The reason for this is to allow for validating all links in one central location and to make it easy to update breaking links.

Each link should be defined only once in ``links.txt``.

If the link text that is defined in ``links.txt`` does not fit in the context where you use the link, you can override it by using the following syntax::

   `new link text <original link text_>`_

It is also possible to define more than one default link text for a link, which can be useful if you frequently need a different link text::

   .. _`Link text one`:
   .. _`Link text two`: http://..

Kconfig
=======

Kconfig options can be linked to from RST by using the ``:option:`` domain::

   :option:`CONFIG_DEBUG`

Breathe
=======

The Breathe Sphinx plugin provides a bridge between RST and Doxygen.

The Doxygen documentation is not automatically included in RST.
Therefore, every group must be explicitly added to an RST file.

See the following example for including a doxygen group:

.. code-block:: python

   .. doxygengroup:: bluetooth_gatt_throughput
      :project: nrf
      :members:

The `Breathe documentation`_ contains information about what you can link to.

To link directly to a Doxygen reference from RST, use the following breathe
domains:

* Function: ``:cpp:func:``
* Structure: ``:c:type:``
* Enum (i.e. the list): ``:cpp:enum:``
* Enumerator (i.e. an item): ``:cpp:enumerator:``
* Macro: ``:c:macro:``
* Structure member: ``:cpp:member:``

.. note::
   The ``:cpp:enum:`` and ``:cpp:enumerator:`` domains do not generate a link
   due to an issue with Sphinx and breathe.

Doxygen style guide
*******************

This style guide covers guidelines for the Doxygen-based API documentation.

General documentation guidelines
================================

#. Always use full sentences, except for descriptions for variables, structs, and enums, where sentence fragments with no verb are accepted, and always end everything with period.
#. Everything that is documented must belong to a group (see below).
#. Use capitalization sparingly; when in doubt, use lowercase. *We'll create a list of terms that should be capitalized.*
#. Line breaks: In Doxygen, break after 80 characters (following the dev guidelines). In RST, break after each sentence.
#. **@note** and **@warning** should only be used in the details section, and only when really needed for emphasis.
   Use notes for emphasis and warnings if things will really really go wrong if you ignore the warning.

File headers and groups
=======================

#. **@file** element is always required at the start of a file.
#. **There is no need to use @brief** for **@file**. *Coding Style page on Vestavind needs to be updated.*
#. **@defgroup** or **@addgroup** usually follows **@file**.
   You can divide a file into several groups as well.
#. **@{** must open the group, **@}** must close it.
#. **@brief** must be added for every defgroup.
#. **@details** is optional to be used within the defgroup.

.. code-block:: c
   :caption: File header and group documentation example

	/**
	 * @file
	 * @defgroup bt_gatt_pool BLE GATT attribute pool API
	 * @{
	 * @brief BLE GATT attribute pools.
	 */

	#ifdef __cplusplus
	extern "C" {
	#endif

	#include <bluetooth/gatt.h>
	#include <bluetooth/uuid.h>

	/**
	 *  @brief Register a primary service descriptor.
	 *
	 *  @param _svc GATT service descriptor.
	 *  @param _svc_uuid_init Service UUID.
	 */
	#define BT_GATT_POOL_SVC_GET(_svc, _svc_uuid_init)	\
	{							\
		struct bt_uuid *_svc_uuid = _svc_uuid_init;	\
		bt_gatt_pool_svc_get(_svc, _svc_uuid);		\
	}

	[...]
	/** @brief Return a CCC descriptor to the pool.
	 *
	 *  @param attr Attribute describing the CCC descriptor to be returned.
	 */
	void bt_gatt_pool_ccc_put(struct bt_gatt_attr const *attr);

	#if CONFIG_BT_GATT_POOL_STATS != 0
	/** @brief Print basic module statistics (containing pool size usage).
	*/
	void bt_gatt_pool_stats_print(void);
	#endif

	#ifdef __cplusplus
	}
	#endif

	/**
	 * @}
	 */


Functions
=========

#. **Do not use @fn**. Instead, document each function where it is defined.
#. **@brief** is mandatory.

   * Start the brief with the "do sth" form (for example, "Initialize the module", "Send Boot Keyboard Input Report").

#. **@details** is optional.
   It can be introduced either by using **@details** or by leaving a blank line after **@brief**.
#. **@param** should be used for every parameter.

   * Always add parameter description.
     Use a sentence fragment (no verb) with period at the end.
   * Specify for all parameters whether they are ``[in]``, ``[out]``, or ``[in,out]``. *- TBD*

#. If you include more than one **@sa** ("see also", optional), add them this way::

      @sa first_function
      @sa second_function

#. **@return** should be used to describe a return value (for example, "@return The length of ...", "@return The handle").
   There is usually only one return value.
#. **@retval** should be used for specific return values (for example, "@retval true", "@retval CONN_ERROR").
   Describe the condition for each of the return values (for example, "If the function completes successfully", "If the connection cannot be established").
   If there is only one retval, add what happens otherwise. Example: "Otherwise, an error code is returned".
#. **Do not use @returns**.
   Use **@return** instead.

.. code-block:: c
   :caption: Function documentation example

	/** @brief Send Boot Keyboard Input Report.
	 *
	 *  @param hids_obj  	HIDS instance.
	 *  @param rep 		Pointer to the report data.
	 *  @param len 		Length of report data.
	 *
	 *  @retval 0 		If the operation was successful.
         *                      Otherwise, a (negative) error code is returned.
	 */
	int hids_boot_kb_inp_rep_send(struct hids *hids_obj, u8_t const *rep,
					  u16_t len);

Enums
=====

The documentation block should precede the documented element.


.. code-block:: c
   :caption: Enum documentation example

        /** HID Service Protocol Mode events. */
        enum hids_pm_evt {

        	/** Boot mode entered. */
	        HIDS_PM_EVT_BOOT_MODE_ENTERED,

	        /** Report mode entered. */
	        HIDS_PM_EVT_REPORT_MODE_ENTERED,
         };

Structs
=======

The documentation block should precede the documented element.
Make sure to add ``:members:`` when you include the API documentation in RST; otherwise, the member documentation will not show up.

.. code-block:: c
   :caption: Struct documentation example

	/** @brief Event header structure.
	 *
	 * @warning When event structure is defined event header must be placed
	 *          as the first field.
	 */
	struct event_header {

        	/** Linked list node used to chain events. */
		sys_dlist_t node;

        	/** Pointer to the event type object. */
		const struct event_type *type_id;
	};

Typedefs - WIP
==============

#. The documentation block should follow, not precede, the documented element.
#. The C99-style single line comment, ``//``, is not allowed, as per `Zephyr coding style`_.

.. code-block:: c
   :caption: Typedef documentation example -- PH

   TBD

TBD
==============

@def, @fn should not be used for defines or functions; Zephyr seems to require this but we should be ok without this.
Just use a @brief and let doxygen figure out what exactly you are documenting.

For parameters, it is recommended to specify whether they are [in], [out], or [in,out].
If you specify this for one parameter in a function, all others must have it as well, for consistency. *To be discussed if this should be a requirement.*

What about @warning, @pre, and other rare doxygen tags?
Should we have a rule for these?
