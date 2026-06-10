.. _snippet-ci-shell:

CI shell snippet (ci-shell)
###########################

.. contents::
   :local:
   :depth: 2

This snippet configures the Zephyr shell for automated testing and continuous integration (CI) environments by making the console output clean, consistent, and easy to parse.
It removes interactive elements and visual formatting that can interfere with automated analysis.

The snippet applies the following configuration:

* Disables the boot banner (:kconfig:option:`CONFIG_BOOT_BANNER`) to keep the startup output clean.
* Sets an empty shell prompt (:kconfig:option:`CONFIG_SHELL_PROMPT_UART`) so that the prompt string does not appear in the captured output.
* Disables VT100 color escape sequences (:kconfig:option:`CONFIG_SHELL_VT100_COLORS`) to avoid control characters in the output.
* Sets a wide default terminal width (:kconfig:option:`CONFIG_SHELL_DEFAULT_TERMINAL_WIDTH`) to prevent the shell from wrapping long lines.
* Keeps the device running after a fatal error by disabling the :kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR` Kconfig option so that the failure state can be inspected.
* Enables thread information for debugging (:kconfig:option:`CONFIG_DEBUG_THREAD_INFO`).

Usage
*****

Apply the snippet when building, for example:

.. code-block:: console

   west build -S ci-shell [...]
