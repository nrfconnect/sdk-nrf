.. |gl| replace:: guidelines

.. _doc_styleguide:

Documentation |gl|
##################

.. contents::
   :local:
   :depth: 2

The |NCS| documentation is written in two formats:

* reStructuredText (RST) for conceptual documentation
* doxygen for API documentation

RST |gl|
********

See Zephyr's :ref:`zephyr:doc_guidelines` for a short introduction to RST and Zephyr documentation conventions.
More information about RST is available in the `reStructuredText Primer`_.

The |NCS| documentation follows the Zephyr style guide, with the addition of the following rules.

Title and headings
===================

* Keep titles and headings short and to the point.
* Add a reference label above the title.

  For example, this page has the reference label ``.. _doc_styleguide:``.
  To see it, click :guilabel:`View page source` in the upper corner of the page.

* Do not repeat the section name in the titles of subpages, such as *sample* when adding a sample.

Table of contents
=================

If your page uses sections, add the ``.. contents::`` directive just under the page title.
This will add a linked table of contents at the top of the page.

For easy navigation, do not include a table of contents if the page has a list of subpages.

Subpages
========

Use the ``.. toctree::`` directive at the bottom of a page to list pages that are located further down in the hierarchy.
For example, the :ref:`documentation` page has a list of subpages, which includes this page you are currently reading.

For a clean structure, pages with the subpages section must not contain heading-based sections or a table of contents.

Linking
=======

You can use different linking and inclusion methods, depending on the content you want to link to.

Hyperlinks
----------

External links
^^^^^^^^^^^^^^

All external links must be defined in the :file:`links.txt` file.
Do not define them directly in the RST file.
This creates one location for validating links, making it easier to update broken links.

Define each link only once in :file:`links.txt`.
If you are adding a new link, make sure it has not already been added.

To use the link, use ```link text`_``.
If you want to change the link text, you can override it with the following syntax: ```new link text <original link text_>`_``.

If you need to use several different link texts for the same link, you can add more default link texts in the :file:`links.txt` file, for example::

   .. _`nRF Programmer`:
   .. _`nRF Programmer mobile app`: https://www.nordicsemi.com/Products/Development-tools/nrf-programmer

Internal links
^^^^^^^^^^^^^^

For internal links, use ``:ref:`` followed by the label of the target document or section in backticks::

   :ref:`target_label`

If the link is targeted to another documentation set, for example Zephyr::

   :ref:`zephyr:coding_style`

If you want to change the link text::

   :ref:`Zephyr coding style <zephyr:coding_style>`

Replacements
------------

If you need to repeat some information, do not duplicate the text.
Use the ``.. |tag| replace:: replacement`` command to reuse the text.
Whenever you use the tag in an RST document, it will be replaced with the text specified for the tag.

You can reuse the content with the tag either on one page or on multiple pages:

* To reuse the text on one page, define the ``|tag|`` and the replacement text before the reference label and the page title.
* To reuse the text on multiple pages, define the ``|tag|`` and the replacement text in :file:`nrf/doc/nrf/shortcuts.txt`.

For example, on this page, the ``|gl|`` tag is defined for local usage and will be replaced with |gl|.
This tag is not available on other pages.
The page is also using the ``|NCS|`` tag that is defined in :file:`shortcuts.txt` and can be used on all documentation pages in the |NCS| project.

Breathe
-------

The Breathe Sphinx plugin provides a bridge between RST and doxygen.

The doxygen documentation is not automatically included in RST.
Therefore, every group must be explicitly added to an RST file.
For example, the code below adds the ``bluetooth_throughput`` group to the RST document, and includes the public members of any classes in the group.
The `Breathe documentation`_ contains information about what you can link to and how to do it.

.. code-block:: none

   .. doxygengroup:: bluetooth_throughput
      :project: nrf
      :members:


.. note::
   Including a group on a page does not include all its subgroups automatically.
   To include subgroups, add the ``:inner:`` option.

   However, if subgroups are defined in separate files, you should rather list them manually on the page of the group they belong to, so that you can include information on where they are defined.

To link directly to a doxygen reference from RST, use the following Breathe domains:

* Function: ``:c:func:``
* Structure: ``:c:struct:``
* Type: ``:c:type:``
* Enum (the list): ``:c:enum:``
* Enumerator (an item): ``:c:enumerator:``
* Macro or define: ``:c:macro:``
* Structure member: ``:c:member:``

Kconfig
-------

To link to the Kconfig options from RST, use the ``:kconfig:option:`` domain::

   :kconfig:option:`CONFIG_DEBUG`

Doxygen |gl|
************

These are the |gl| for the doxygen-based API documentation.

General documentation |gl|
==========================

* Always write full sentences, and end them with a period.

  * *Exception:* Sentence fragments are acceptable for descriptions of variables, structs, and enums.
* Ensure that all documented items belong to a correct group (see the section below).
* Use capitalization sparingly.
  When in doubt, use lowercase.
* Break the line after 100 characters.
* Use ``@note`` only in the details section, and only when really needed for emphasis.
* Use ``@warning`` only if an operating procedure or practice, which, if not correctly followed, could result in personal injury or loss of life.

File headers and groups
=======================

* ``@file`` element is always required at the start of a file.
* ``@brief`` is not needed for ``@file``.
* ``@defgroup`` or ``@addgroup`` usually follows ``@file``.
  You can divide a file into several groups.

  * ``@{`` must open the group, ``@}`` must close it.
* Add ``@brief`` for every defgroup.

  * ``@details`` is optional within the defgroup.

.. code-block:: c

   /**
    * @file
    * @defgroup bt_gatt_pool BLE GATT attribute pool API
    * @{
    * @brief BLE GATT attribute pools.
    */

   #ifdef __cplusplus
   extern "C" {
   #endif

   #include <zephyr/bluetooth/gatt.h>
   #include <zephyr/bluetooth/uuid.h>

   /**
    *  @brief Register a primary service descriptor.
    *
    *  @param _svc GATT service descriptor.
    *  @param _svc_uuid_init Service UUID.
    */
   #define BT_GATT_POOL_SVC_GET(_svc, _svc_uuid_init)   \
   {                                                    \
      struct bt_uuid *_svc_uuid = _svc_uuid_init;       \
      bt_gatt_pool_svc_get(_svc, _svc_uuid);            \
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

* Do not use ``@fn``. Instead, document each function where it is defined.
* ``@brief`` is mandatory.

  * Start the brief with the imperative form (for example "do something").

    .. code-block:: none

       /** @brief Request a read operation to be executed from Secure Firmware.

       /** @brief Send Boot Keyboard Input Report.

* ``@details`` is optional.
  You can introduce the additional information using ``@details`` or a blank line after ``@brief``.
* Use ``@param`` for every parameter.

  * Always add a parameter description.
    Use a sentence fragment (no verb) with a period at the end.
  * Make sure the parameter documentation within the function is consistently using the parameter type: ``[in]``, ``[out]``, or ``[in,out]``.

    .. code-block:: none

       * @param[out] destination Pointer to destination array where the content is
       *                         to be copied.
       * @param[in]  addr        Address to be copied from.
       * @param[in]  len         Number of bytes to copy.

* If you include more than one ``@sa`` ("see also", optional), add them like this::

      @sa first_function
      @sa second_function

* Use ``@return`` or ``@retval`` instead of ``@returns``.

  * Use ``@return`` to describe a generic return value without a specific value (for example, ``@return The length of ...``, ``@return The handle``).
    Usually, there is only one return value.

    .. code-block:: none

       *  @return  Initializer that sets up the pipe, length, and byte array for
       *           content of the TX data.

  * Use ``@retval`` for specific return values (for example, ``@retval true``, ``@retval CONN_ERROR``).
    Describe the condition for each of the return values (for example, "If the function completes successfully", "If the connection cannot be established").

    .. code-block:: none

       *  @retval 0 If the operation was successful.
       *            Otherwise, a (negative) error code is returned.
       *  @retval (-ENOTSUP) Special error code used when the UUID
       *            of the service does not match the expected UUID.

Here is an example of a fully defined function:

.. code-block:: c

   /** @brief Request a random number from the Secure Firmware.
    *
    * This function provides a True Random Number from the on-board random number generator.
    *
    * @note Currently, the RNG hardware runs each time this function is called. This
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
    int spm_request_random_number(uint8_t *output, size_t len, size_t *olen);

Enums
=====

Place the documentation block above the enum.
The documentation for the elements inside the enum can be above them or inline.

An example with documentation preceding the documented element:

.. code-block:: c

   /** HID Service Protocol Mode events. */
   enum hids_pm_evt {

      /** Boot mode entered. */
      HIDS_PM_EVT_BOOT_MODE_ENTERED,

      /** Report mode entered. */
      HIDS_PM_EVT_REPORT_MODE_ENTERED,
    };

An example with the documentation inline (note the less than sign `<` after the asterisks `**`):

.. code-block:: c

   /** @brief PDN library event. */
   enum pdn_event {
           PDN_EVENT_CNEC_ESM,		/**< +CNEC ESM error code. */
           PDN_EVENT_ACTIVATED,		/**< PDN connection activated. */
           PDN_EVENT_DEACTIVATED,	/**< PDN connection deactivated. */
           PDN_EVENT_IPV6_UP,		/**< PDN has IPv6 connectivity. */
           PDN_EVENT_IPV6_DOWN,		/**< PDN has lost IPv6 connectivity. */

Structs
=======

The documentation block must precede the documented element.
Make sure to add ``:members:`` when you include the API documentation in RST; otherwise, the member documentation will not show up.

In the RST file:

.. code-block:: console

   API documentation
   *****************

   | Header file: :file:`include/bluetooth/gatt_dm.h`
   | Source file: :file:`subsys/bluetooth/gatt_dm.c`

   .. doxygengroup:: bt_gatt_dm
      :project: nrf
      :members:

In the header file:

.. code-block:: c

   /** @brief Event header structure.
    *
    * @warning When an event structure is defined, application event header must be placed
    *          as the first field.
    */
   struct app_event_header {

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

To link to functions, enums, or structs from within doxygen itself, use the ``@ref`` keyword.

.. code-block:: none

   /** @brief Event header structure.
    *  Use this structure with the function @ref function_name and
    *  this structure is related to another structure, @ref structure_name.
    */

.. note::
   Linking to functions does not currently work due to `Breathe issue #438`_.


Typedefs
========

The documentation block must precede the documented element.

.. code-block:: c

   /**
    * @brief Download client asynchronous event handler.
    *
    * Through this callback, the application receives events, such as
    * download of a fragment, download completion, or errors.
    *
    * If the callback returns a non-zero value, the download stops.
    * To resume the download, use @ref download_client_start().
    *
    * @param[in] event   The event.
    *
    * @retval 0 The download continues.
    * @retval non-zero The download stops.
    */
    typedef int (*download_client_callback_t)(const struct download_client_evt *event);
