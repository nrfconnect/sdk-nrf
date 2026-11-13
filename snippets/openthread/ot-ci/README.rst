.. _snippet_ot_ci:
.. _snippet-ot-ci:

OpenThread snippet for CI purpose (ot-ci)
#########################################

.. contents::
   :local:
   :depth: 2

The ``ot-ci`` snippet is used in the OpenThread samples to ensure that the samples are tested without the boot banner and shell prompt.

Overview
********

The snippet applies the following configuration:

* Disables the boot banner (:kconfig:option:`CONFIG_BOOT_BANNER`).
* Clears the shell prompt (:kconfig:option:`CONFIG_SHELL_PROMPT_UART`) and disables the VT100 colors in the shell (:kconfig:option:`CONFIG_SHELL_VT100_COLORS`).
* Sets the default shell terminal width to ``618`` (:kconfig:option:`CONFIG_SHELL_DEFAULT_TERMINAL_WIDTH`).
* Enables assertions (:kconfig:option:`CONFIG_ASSERT`) without the condition information (:kconfig:option:`CONFIG_ASSERT_NO_COND_INFO`).
* Disables the reset on a fatal error (:kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR`).

Usage
*****

To build with this snippet, follow the instructions in the :ref:`zephyr:using-snippets` page.
When building with west, run the following command:

.. code-block:: console

   west build -- -D<project_name>_SNIPPET=ot-ci
