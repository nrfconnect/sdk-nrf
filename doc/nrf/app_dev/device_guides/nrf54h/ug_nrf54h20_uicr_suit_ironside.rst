:orphan:

.. _ug_54h_uicr_suit_ironside:

Converting UICR HEX Files from SUIT-SDFW to IRONside SE
=========================================================

To use IRONside SE properly, the local domain must configure the UICR registers. Currently, this configuration must be performed within the application.

The following steps show how to configure the UICR for the Peripheral RSRC sample:


1. Ensure that the IRONside SE version programmed on the nRF54H20 SoC is correct::

        nrfutil device x-read --direct --address 0x2f88fd04 --bytes 4 \
        --traits jlink

   The expected output is: ``0x2F88FD04: 15010107``.

#. Install `nrf-regtool` v9.2.0rc1::

        pip3 install --upgrade nrf-regtool==9.2.0rc1

#. Perform the CMake configuration stage of the build process for the sample you want to port to IRONside, using the non-IRONside variant of the sample::

        west build -d ~/ncs/build_app -b nrf54h20dk/nrf54h20/cpuapp --cmake-only \
        ~/ncs/nrf/samples/bluetooth/peripheral_rscs --pristine

#. Input all generated UICR HEX files to the `nrf-regtool uicr-migrate` command.
   The output file is stored in the source directory of the target sample::

        export PYTHONPATH="$(west topdir)/zephyr/scripts/dts/python-devicetree/src"\
                    nrf-regtool uicr-migrate \
                    --uicr-hex-file ~/ncs/build_app/peripheral_rscs/zephyr/uicr.hex \
                    --uicr-hex-file ~/ncs/build_app/ipc_radio/zephyr/uicr.hex \
                    --edt-pickle-file ~/ncs/build_app/peripheral_rscs/zephyr/edt.pickle
                    --output-periphconf-file ~/ncs/nrf/samples/bluetooth/peripheral_rscs/src/my_periphconf.c

   .. note::
      This output file must be regenerated whenever changes are made to the project's Devicetree source (DTS).


#. Update the sample ``CMakeLists.txt`` to include the ``my_periphconf.c`` source file:

        cat ~/ncs/nrf/samples/bluetooth/peripheral_rscs/CMakeLists.txt

        (...)

        target_sources(app PRIVATE
        src/main.c
        src/my_periphconf.c # <-- add something like this
        )

        (...)

#. Build the target sample for the iron variant::

        west build -d ~/ncs/build_app --pristine -b nrf54h20dk/nrf54h20/cpuapp/iron \
        ~ncs/nrf/samples/bluetooth/peripheral_rscs \
        -- -DCONFIG_SOC_NRF54H20_CPURAD_ENABLE=y

   .. note::
      When building Bluetooth Low Energy (BLE) samples that require the radio core to boot,
      you must enable the CONFIG_SOC_NRF54H20_CPURAD_ENABLE Kconfig option.

#. Program the sample, invoking west flash::

       west flash -d ~/ncs/build_app
