.. _snippet_ot_logging:

OpenThread snippet for logging (ot-logging)
===========================================

.. contents::
   :local:
   :depth: 2

To build with this snippet, follow the instructions in the :ref:`using-snippets` page.
When building with west, run the following command:

.. code-block:: console

   west build -- -D<project_name>_SNIPPET=ot-logging

Overview
********

This snippet enables logging for the OpenThread stack.

Development kits support
************************

For nRF52840 Dongle board, this snippet enables UART logging for debugging Thread samples.
For other boards, it enables RTT logging for debugging Thread samples.
