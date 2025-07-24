.. _migration_uicr_nRF54H20:

Migrating nRF54H20 SoC UICR HEX Files from SUIT to IronSide SE
##############################################################

.. contents::
   :local:
   :depth: 2

On the nRF54H20 SoC, any application running on a local domain must configure the User Information Configuration Registers (UICRs) registers for the domain its running onto.
As the nRF54H20 SoC migrated from SUIT to IronSide SE, the UICR configuration format changed.


Migrating an existing application
*********************************

The following steps show how to migrate the configuration of the UICRs from being SUIT-compatible to IronSide-SE-compatible.
The Peripheral RSRC sample is used as an example of an existing application:

1. Ensure that the correct version of IronSide SE is programmed on the nRF54H20 SoC::

        nrfutil device x-read --direct --address 0x2f88fd04 --bytes 4 \
        --traits jlink

   The expected output is: ``0x2F88FD04: 15010107``.

#. Install ``nrf-regtool`` v9.2.0::

        pip3 install --upgrade nrf-regtool==9.2.0

#. Perform the CMake configuration stage of the build process for the application you want to port to IronSide SE, using the non-IronSide-SE variant of the application::

        west build -d ~/ncs/build_app -b nrf54h20dk/nrf54h20/cpuapp --cmake-only \
        ~/ncs/nrf/samples/bluetooth/peripheral_rscs --pristine

#. Input all generated UICR HEX files to the ``nrf-regtool uicr-migrate`` command.
   The output file is stored in the source directory of the target application::

        export PYTHONPATH="$(west topdir)/zephyr/scripts/dts/python-devicetree/src"\
                    nrf-regtool uicr-migrate \
                    --uicr-hex-file ~/ncs/build_app/peripheral_rscs/zephyr/uicr.hex \
                    --uicr-hex-file ~/ncs/build_app/ipc_radio/zephyr/uicr.hex \
                    --edt-pickle-file ~/ncs/build_app/peripheral_rscs/zephyr/edt.pickle
                    --output-periphconf-file ~/ncs/nrf/samples/bluetooth/peripheral_rscs/src/my_periphconf.c

   .. note::
      This output file must be regenerated whenever changes are made to the project's devicetree source (DTS) files.


#. Update the application ``CMakeLists.txt`` to include the :file:`my_periphconf.c` source file:

        cat ~/ncs/nrf/samples/bluetooth/peripheral_rscs/CMakeLists.txt

        (...)

        target_sources(app PRIVATE
        src/main.c
        src/my_periphconf.c # <-- add the my_periphconf.c source file
        )

        (...)

#. Build the target application for the IronSide SE variant::

        west build -d ~/ncs/build_app --pristine -b nrf54h20dk/nrf54h20/cpuapp/iron \
        ~/ncs/nrf/samples/bluetooth/peripheral_rscs \
        -- -DCONFIG_SOC_NRF54H20_CPURAD_ENABLE=y

   .. note::
      When building Bluetooth Low Energy (BLE) applications that require the radio core to boot, you must enable the :kconfig:option:`CONFIG_SOC_NRF54H20_CPURAD_ENABLE` Kconfig option.

#. Program the application, invoking west flash::

       west flash -d ~/ncs/build_app
