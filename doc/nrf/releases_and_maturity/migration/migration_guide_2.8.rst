.. _migration_2.8:

Migration guide for |NCS| v2.8.0 (Working draft)
################################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v2.7.0 to |NCS| v2.8.0.

.. HOWTO

   Add changes in the following format:

   Component (for example, application, sample or libraries)
   *********************************************************

   .. toggle::

      * Change1 and description
      * Change2 and description

.. _migration_2.8_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

Samples and applications
========================

This section describes the changes related to samples and applications.

Libraries
=========

This section describes the changes related to libraries.

AT command parser
-----------------

.. toggle::

  * The :c:func:`at_parser_cmd_type_get` has been renamed to :c:func:`at_parser_at_cmd_type_get`.

.. _migration_2.8_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Libraries
=========

This section describes the changes related to libraries.

AT command parser
-----------------

.. toggle::

  * The :ref:`at_cmd_parser_readme` library has been deprecated in favor of the :ref:`at_parser_readme` library and will be removed in a future version.

    You can follow this guide to migrate your application to use the :ref:`at_parser_readme` library.
    This will reduce the footprint of the application and will alleviate memory pressure on the heap.

    To replace :ref:`at_cmd_parser_readme` with the ref:`at_parser_readme`, complete the following steps:

    1. Rename the :kconfig:option:`CONFIG_AT_CMD_PARSER` to :kconfig:option:`CONFIG_AT_PARSER`.

    #. Replace header files:

       * Remove:

         .. code-block:: C

          #include <modem/at_cmd_parser.h>
          #include <modem/at_params.h>

       * Add:

         .. code-block:: C

          #include <modem/at_parser.h>

    #. Replace AT parameter list:

       * Remove:

         .. code-block:: C

          struct at_param_list param_list;

       * Add:

         .. code-block:: C

          struct at_parser parser;

    #. Replace AT parameter list initialization:

       * Remove:

         .. code-block:: C

          /* `param_list` is a pointer to the AT parameter list.
           * `AT_PARAMS_COUNT` is the maximum number of parameters of the list.
           */
          at_params_list_init(&param_list, AT_PARAMS_COUNT);

          /* Other code. */

          /* `at_string` is the AT command string to be parsed.
           * `&remainder` is a pointer to the returned remainder after parsing.
           * `&param_list` is a pointer to the AT parameter list.
           */
          at_parser_params_from_str(at_string, &remainder, &param_list);

       * Add:

         .. code-block:: C

          /* `&at_parser` is a pointer to the AT parser.
           * `at_string` is the AT command string to be parsed.
           */
          at_parser_init(&at_parser, at_string);

    #. Replace integer parameter retrieval:

       * Remove:

         .. code-block:: C

          int value;

          /* `&param_list` is a pointer to the AT parameter list.
           * `index` is the index of the parameter to retrieve.
           * `&value` is a pointer to the output integer variable.
           */
          at_params_int_get(&param_list, index, &value);

          uint16_t value;
          at_params_unsigned_short_get(&param_list, index, &value);

          /* Other variants: */
          at_params_short_get(&param_list, index, &value);
          at_params_unsigned_int_get(&param_list, index, &value);
          at_params_int64_get(&param_list, index, &value);

       * Add:

         .. code-block:: C

          int value;

          /* `&at_parser` is a pointer to the AT parser.
           * `index` is the index of the parameter to retrieve.
           * `&value` is a pointer to the output integer variable.
           *
           * Note: this function is type-generic on the type of the output integer variable.
           */
          err = at_parser_num_get(&at_parser, index, &value);

          uint16_t value;
          /* Note: this function is type-generic on the type of the output integer variable. */
          err = at_parser_num_get(&at_parser, index, &value);

    #. Replace string parameter retrieval:

       * Remove:

         .. code-block:: C

          /* `&param_list` is a pointer to the AT parameter list.
           * `index` is the index of the parameter to retrieve.
           * `value` is the output buffer where the string is copied into.
           * `&len` is a pointer to the length of the copied string.
           *
           * Note: the copied string is not null-terminated.
           */
          at_params_string_get(&param_list, index, value, &len);

          /* Null-terminate the string. */
          value[len] = '\0';

       * Add:

         .. code-block:: C

          /* `&at_parser` is a pointer to the AT parser.
           * `index` is the index of the parameter to retrieve.
           * `value` is the output buffer where the string is copied into.
           * `&len` is a pointer to the length of the copied string.
           *
           * Note: the copied string is null-terminated.
           */
          at_parser_string_get(&at_parser, index, value, &len);

    #. Replace parameter count retrieval:

       * Remove:

         .. code-block:: C

          /* `&param_list` is a pointer to the AT parameter list.
           * `count` is the returned parameter count.
           */
          uint32_t count = at_params_valid_count_get(&param_list);

       * Add:

         .. code-block:: C

          size_t count;

          /* `&at_parser` is a pointer to the AT parser.
           * `&count` is a pointer to the returned parameter count.
           */
          at_parser_cmd_count_get(&at_parser, &count);

    #. Replace command type retrieval:

       * Remove:

         .. code-block:: C

          /* `at_string` is the AT string that we want to retrieve the command type of.
           */
          enum at_cmd_type type = at_parser_at_cmd_type_get(at_string);

       * Add:

         .. code-block:: C

          enum at_parser_cmd_type type;

          /* `&at_parser` is a pointer to the AT parser.
           * `&type` pointer to the returned command type.
           */
          at_parser_cmd_type_get(&at_parser, &type);
