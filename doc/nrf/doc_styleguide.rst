.. _doc_styleguide:

Documentation style guide
#########################

NCS documentation is written in two formats:

* doxygen for API documentation
* RST for conceptual documentation


RST style guide
***************

See Zephyr's :ref:`zephyr:doc_guidelines` for a short introduction to RST and, most importantly, to the conventions used in Zephyr.
More information about RST is available in the `reStructuredText Primer`_.

The |NCS| documentation follows the Zephyr style guide, and adds a few more restrictive rules:

* Headings use sentence case, which means that the first word is capitalized, and the following words use normal capitalization.
* Do not use consecutive headings without intervening text.
* For readability reasons, start every sentence on a new line in the source files and do not add line breaks within the sentence.
  In the output, consecutive lines without blank lines in between are combined into one paragraph.

    .. note:: For the conceptual documentation written in RST, you can have more than 80 characters per line.
              The requirement for 80 characters per line applies only to the code documentation written in doxygen.

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

Diagrams
========

You can include Message Sequence Chart (MSC) diagrams in RST by using the ``.. msc::`` directive and including the MSC content, for example:

.. code-block:: python

    .. msc::
        hscale = "1.3";
        Module,Application;
        Module<<Application      [label="nrf_cloud_connect() returns successfully"];
        Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_CONNECTED"];
        Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST"];
        Module<<Application      [label="nrf_cloud_user_associate()"];
        Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATED"];
        Module>>Application      [label="NRF_CLOUD_EVT_READY"];
        Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED"];


This will generate the following output:

    .. msc::
        hscale = "1.3";
        Module,Application;
        Module<<Application      [label="nrf_cloud_connect() returns successfully"];
        Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_CONNECTED"];
        Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST"];
        Module<<Application      [label="nrf_cloud_user_associate()"];
        Module>>Application      [label="NRF_CLOUD_EVT_USER_ASSOCIATED"];
        Module>>Application      [label="NRF_CLOUD_EVT_READY"];
        Module>>Application      [label="NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED"];


Kconfig
=======

Kconfig options can be linked to from RST by using the ``:option:`` domain::

   :option:`CONFIG_DEBUG`

Breathe
=======

The Breathe Sphinx plugin provides a bridge between RST and doxygen.

The doxygen documentation is not automatically included in RST.
Therefore, every group must be explicitly added to an RST file.

.. code-block:: python
   :caption: Example of how to include a doxygen group

   .. doxygengroup:: bluetooth_gatt_throughput
      :project: nrf
      :members:


.. note::
    Including a group on a page does not include all its subgroups automatically.
    To include subgroups, list them on the page of the group they belong to.

The `Breathe documentation`_ contains information about what you can link to.

To link directly to a doxygen reference from RST, use the following Breathe domains:

* Function: ``:cpp:func:``
* Structure: ``:c:type:``
* Enum (i.e. the list): ``:cpp:enum:``
* Enumerator (i.e. an item): ``:cpp:enumerator:``
* Macro: ``:c:macro:``
* Structure member: ``:cpp:member:``

.. note::
   The ``:cpp:enum:`` and ``:cpp:enumerator:`` domains do not generate a link due to `Breathe issue #437`_.
   As a workaround, use the following command::

      :cpp:enumerator:`ENUM_VALUE <DOXYGEN_GROUP::ENUM_VALUE>`

Doxygen style guide
*******************

This style guide covers guidelines for the doxygen-based API documentation.

General documentation guidelines
================================

#. Always use full sentences, except for descriptions for variables, structs, and enums, where sentence fragments with no verb are accepted, and always end everything with period.
#. Everything that is documented must belong to a group (see below).
#. Use capitalization sparingly. When in doubt, use lowercase.
#. Line breaks: In doxygen, break after 80 characters (following the dev guidelines). In RST, break after each sentence.
#. **@note** and **@warning** should only be used in the details section, and only when really needed for emphasis.
   Use notes for emphasis and warnings if things will really really go wrong if you ignore the warning.

File headers and groups
=======================

#. **@file** element is always required at the start of a file.
#. There is no need to use **@brief** for **@file**.
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

#. Do not use **@fn**. Instead, document each function where it is defined.
#. **@brief** is mandatory.

   * Start the brief with the "do sth" form.

    .. code-block:: none
        :caption: Brief documentation examples

        /** @brief Request a read operation to be executed from Secure Firmware.

        /** @brief Send Boot Keyboard Input Report.

#. **@details** is optional.
   It can be introduced either by using **@details** or by leaving a blank line after **@brief**.
#. **@param** should be used for every parameter.

   * Always add parameter description.
     Use a sentence fragment (no verb) with period at the end.
   * Make sure the parameter documentation within the function is consistently using the parameter type: ``[in]``, ``[out]``, or ``[in,out]``.

    .. code-block:: none
        :caption: Parameter documentation example

        * @param[out] destination Pointer to destination array where the content is
        *                         to be copied.
        * @param[in]  addr        Address to be copied from.
        * @param[in]  len         Number of bytes to copy.

#. If you include more than one **@sa** ("see also", optional), add them this way::

      @sa first_function
      @sa second_function

#. **@return** should be used to describe a generic return value without a specific value (for example, "@return The length of ...", "@return The handle").
   There is usually only one return value.

   .. code-block:: none
      :caption: Return documentation example

      *  @return  Initializer that sets up the pipe, length, and byte array for
      *           content of the TX data.

#. **@retval** should be used for specific return values (for example, "@retval true", "@retval CONN_ERROR").
   Describe the condition for each of the return values (for example, "If the function completes successfully", "If the connection cannot be established").

    .. code-block:: none
       :caption: Retval documentation example

       *  @retval 0 If the operation was successful.
       *            Otherwise, a (negative) error code is returned.
       *  @retval (-ENOTSUP) Special error code used when the UUID
       *            of the service does not match the expected UUID.

#. Do not use **@returns**.
   Use **@return** instead.

.. code-block:: c
   :caption: Complete function documentation example

    /** @brief Request a random number from the Secure Firmware.
     *
     * This function provides a True Random Number from the on-board random number generator.
     *
     * @note Currently, the RNG hardware is run each time this function is called. This
     *       consumes significant time and power.
     *
     * @param[out] output  The random number. Must be at least @p len long.
     * @param[in]  len     The length of the output array. Currently, @p len must be
     *                     144.
     * @param[out] olen    The length of the random number provided.
     *
     * @retval 0        If the operation was successful.
     * @retval -EINVAL  If @p len is invalid. Currently, @p len must be 144.
     */
     int spm_request_random_number(u8_t *output, size_t len, size_t *olen);

Enums
=====

The documentation block should precede the documented element.
This is in accordance with the `Zephyr coding style`_.


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
This is in accordance with the `Zephyr coding style`_.
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


.. note::
   Always add a name for the struct.
   Avoid using unnamed structs due to `Sphinx parser issue`_.


References
==========

To link to functions, enums, or structs from within doxygen itself, use the
``@ref`` keyword.

.. code-block:: none
   :caption: Reference documentation example

    /** @brief Event header structure.
     *  Use this structure with the function @ref function_name and
     *  this structure is related to another structure, @ref structure_name.
     */

.. note::
   Linking to functions does not currently work due to `Breathe issue #438`_.


Typedefs
========

The documentation block should precede the documented element.
This is in accordance with the `Zephyr coding style`_.

.. code-block:: c
   :caption: Typedef documentation example

   /**
    * @brief Download client asynchronous event handler.
    *
    * Through this callback, the application receives events, such as
    * download of a fragment, download completion, or errors.
    *
    * If the callback returns a non-zero value, the download stops.
    * To resume the download, use @ref download_client_start().
    *
    * @param[in] event	The event.
    *
    * @retval 0 The download continues.
    * @retval non-zero The download stops.
    */
    typedef int (*download_client_callback_t)(const struct download_client_evt *event);

