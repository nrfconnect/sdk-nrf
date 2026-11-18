.. _nrf_desktop:

.. ncs-sample::
   :title: nRF Desktop

   The nRF Desktop is a reference design of a :term:`Human Interface Device (HID)` that is connected to a host through Bluetooth® Low Energy or USB, or both.
   Depending on the configuration, this application can work as a desktop mouse, gaming mouse, keyboard, or connection dongle.
   See `nRF Desktop reference design page`_ for an overview of supported features.

   .. note::
      The nRF Desktop application in the |NCS| is deprecated and will be removed in the future.
      Development of the HID application reference design continues in the dedicated `HID Add-on`_.
      The in-tree application remains available in the |NCS| v3.4.x long-term support (LTS) release branch until its removal.
      New features are introduced only in the Add-on.

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
