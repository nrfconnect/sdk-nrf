.. _nrf_desktop:

.. ncs-sample::
   :title: nRF Desktop

   The nRF Desktop is a reference design of a :term:`Human Interface Device (HID)` that is connected to a host through Bluetooth® Low Energy or USB, or both.
   Depending on the configuration, this application can work as a desktop mouse, gaming mouse, keyboard, or connection dongle.
   See `nRF Desktop reference design page`_ for an overview of supported features.

   .. note::
      Future development of the nRF Desktop HID application reference design will move to a dedicated nRF Connect SDK Add-on (``HID Add-on``).
      Existing feature set will be maintained in the nRF Connect SDK 3.4 Long-term support (LTS) releases, but new features will be introduced only in the Add-on.
      The Add-on will support nRF54L Series SoC.

   .. tip::
      To get started with hardware programmed with pre-configured software, go to the :ref:`nrf_desktop_user_interface` section.

   See the subpages for detailed documentation on the application and its modules:

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   description
   application_kconfig
   board_configuration
   nRF21540ek_support
   memory_layout
   bluetooth
   bootloader_dfu
   fwupd
   integration
   modules
   utils
   configuration_options
   api
