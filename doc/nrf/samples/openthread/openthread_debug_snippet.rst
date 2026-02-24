.. _snippet_ot_debug:

OpenThread snippet for debugging purpose (ot-debug)
###################################################

.. contents::
   :local:
   :depth: 2

To build with this snippet, follow the instructions in the :ref:`using-snippets` page.
When building with west, run the following command:

.. code-block:: console

   west build -- -D<project_name>_SNIPPET=ot-debug

Overview
********

This snippet enables the :c:func:`__ASSERT` function for debugging OpenThread samples.
This snippet also enables multithread support in GNU Debugger (GDB) and thread monitoring.
