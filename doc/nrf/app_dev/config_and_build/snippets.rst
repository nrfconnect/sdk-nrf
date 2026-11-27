.. _ug_snippets:

Snippets
########

:ref:`Snippets <app_build_snippets>` are a Zephyr mechanism for defining portable build system overrides that you can apply to any application.
This page lists the snippets that the |NCS| provides in the :file:`snippets` directory of the |NCS| main directory, grouped by the devices and the features they target.
Zephyr provides additional snippets, which are listed on the :ref:`zephyr:snippets` page.

Each snippet has its own page that describes what the snippet configures, which board targets it supports, and how to apply it.
For general information about how to apply a snippet to a build, see :ref:`cmake_options`.

The Wi-Fi snippets apply to both the nRF70 Series and the nRF71 Series devices, and each of them provides a separate configuration for the two series.
The VPR core snippets let the application core boot a VPR core, such as the FLPR (Fast Lightweight Peripheral Processor) or the PPR (Peripheral Processor).
You can use the OpenThread snippets only with the OpenThread samples, and you must add the sample name as a prefix before the ``SNIPPET`` CMake argument.

.. note::
   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>`.

.. toctree::
   :maxdepth: 1
   :caption: General-purpose snippets:

   ../../../snippets/ci-shell/README
   ../../../snippets/coverage_support/README
   ../../../snippets/hw-flow-control/README
   ../../../snippets/memfault-coredump/README
   ../../../snippets/nordic-bt-rpc/README
   ../../../snippets/nordic-vregmain-ldo/README
   ../../../snippets/secondary-app-partition/README
   ../../../snippets/zperf/README

.. toctree::
   :maxdepth: 1
   :caption: Snippets for the nRF70 Series:

   ../../../snippets/nrf70-driver-debug/README
   ../../../snippets/nrf70-driver-verbose-debug/README
   ../../../snippets/nrf70-fw-patch-ext-flash/README
   ../../../snippets/nrf70-wifi/README

.. toctree::
   :maxdepth: 1
   :caption: Snippets for the nRF71 Series:

   ../../../snippets/nrf71-driver-debug/README
   ../../../snippets/nrf71-driver-verbose-debug/README
   ../../../snippets/nrf71-idle-power/README
   ../../../snippets/nrf71-idle-power-diag/README
   ../../../snippets/nrf71-idle-power-quiet/README

.. toctree::
   :maxdepth: 1
   :caption: Snippets for the nRF91 Series:

   ../../../snippets/nrf91-modem-trace-ext-flash/README
   ../../../snippets/nrf91-modem-trace-uart/README
   ../../../snippets/nrf91-modem-trace-rtt/README
   ../../../snippets/nrf91-modem-trace-ram/README
   ../../../snippets/tfm-enable-share-uart/README

.. toctree::
   :maxdepth: 1
   :caption: Wi-Fi snippets:

   ../../../snippets/nordic-wifi-enterprise/README
   ../../../snippets/wifi-monitor-mode/README
   ../../../snippets/wifi-p2p/README
   ../../../snippets/wifi-promiscuous-mode/README
   ../../../snippets/wifi-raw-tx/README
   ../../../snippets/wifi-sap/README
   ../../../snippets/wpa-supplicant-debug/README

.. toctree::
   :maxdepth: 1
   :caption: VPR core snippets:

   ../../../snippets/nordic-flpr/README
   ../../../snippets/nordic-flpr-xip/README
   ../../../snippets/nordic-ppr/README
   ../../../snippets/nordic-ppr-xip/README

.. toctree::
   :maxdepth: 1
   :caption: High-Performance Framework snippets:

   ../../../snippets/hpf/gpio/icbmsg/README
   ../../../snippets/hpf/gpio/icmsg/README
   ../../../snippets/hpf/gpio/mbox/README
   ../../../snippets/hpf/mspi/README

.. toctree::
   :maxdepth: 1
   :caption: OpenThread snippets:

   ../../../snippets/openthread/ot-ci/README
   ../../../snippets/openthread/ot-debug/README
   ../../../snippets/openthread/ot-diag-gpio/README
   ../../../snippets/openthread/ot-logging/README
   ../../../snippets/openthread/ot-usb/README
   ../../../snippets/openthread/ot-zephyr-l2/README
