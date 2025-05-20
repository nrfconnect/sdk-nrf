.. _at_parser_readme:

AT parser
#########

.. contents::
   :local:
   :depth: 2

The AT parser is a library that parses AT command responses, notifications, and events.

Overview
========

The library exports an API that lets the user iterate through an AT command string and retrieve any of its elements based on their indices, efficiently and without heap allocations.
Under the hood, this library uses regular expressions converted to deterministic finite automata by the free and open-source lexer generator ``re2c``.

Configuration
=============

To begin using the AT parser, you first need to initialize it.
This is done by invoking the :c:func:`at_parser_init` function, where you provide a pointer to the AT parser and the AT command string that needs to be parsed.

The following code snippet shows how to initialize the AT parser for an AT command response:

.. code-block:: c

   int err;
   struct at_parser parser;
   const char *at_response = "+CFUN: 1";

   err = at_parser_init(&parser, at_response);
   if (err) {
      return err;
   }

Usage
=====

Based on the type of the element at a given index, you can obtain its value by calling the type-generic macro :c:macro:`at_parser_num_get` for integer values, or the function :c:func:`at_params_string_get` for string values.

The following code snippet shows how to retrieve the prefix, a ``uint16_t`` value, and a string value from an AT command response using the AT parser:

.. code-block:: c

   int err;
   struct at_parser parser;
   const char *at_response = "+CGCONTRDP: 0,,\"internet\",\"\",\"\",\"10.0.0.1\",\"10.0.0.2\",,,,,1028";
   uint16_t num;
   char buffer[16] = { 0 };
   size_t len = sizeof(buffer);

   err = at_parser_init(&parser, at_response);
   if (err) {
      return err;
   }

   /* Retrieve the AT command response prefix, at index 0 of the AT command string. */
   err = at_parser_string_get(&parser, 0, buffer, &len);
   if (err) {
      return err;
   }

   /* "Prefix: `+CGCONTRDP`" */
   printk("Prefix: `%s`\n", buffer);

   /* Retrieve the first subparameter, at index 1 of the AT command string.
    * `at_parser_num_get` is a type-generic macro that retrieves an integer based on the type
    * of the passed variable. In this example, the preprocessor will expand the macro to the
    * function corresponding to the `uint16_t` type, which is the function `at_parser_uint16_get`.
    */
   err = at_parser_num_get(&parser, 1, &num);
   if (err) {
      return err;
   }

   /* "First subparameter: 0" */
   printk("First subparameter: %u\n", num);

   /* Reset the buffer length. */
   len = sizeof(buffer);

   /* Retrieve the third subparameter, at index 3 of the AT command string. */
   err = at_parser_string_get(&parser, 3, buffer, &len);
   if (err) {
      return err;
   }

   /* "Third subparameter: `internet`" */
   printk("Third subparameter: `%s`\n", buffer);

API documentation
*****************

| Header file: :file:`include/modem/at_parser.h`
| Source file: :file:`lib/at_parser/src/at_parser.c`

.. doxygengroup:: at_parser
