.. _nrf_desktop_app_internal_utils:

nRF Desktop: Application internal utilities
###########################################

The nRF Desktop application uses its own set of internal utilities.
The utilities are used by :ref:`application modules <nrf_desktop_app_internal_modules>`.

A utility exposes an API through a dedicated header that is included by an application module.
The utility does not subscribe for application events, but it might submit an application event.
More information about each utility and its configuration details is available on the subpages.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   doc/config_channel.rst
   doc/dfu_lock.rst
   doc/hid_eventq.rst
   doc/hid_keymap.rst
   doc/hid_reportq.rst
   doc/keys_state.rst
