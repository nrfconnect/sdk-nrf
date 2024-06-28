.. _ug_nrf54h20_suit_fetch:

How to fetch payloads
#####################

.. contents::
   :local:
   :depth: 2

In the Software Updates for Internet of Things (SUIT), it is possible for a device to obtain a new firmware in two ways:

1. The push model - where all necessary candidate images are integrated into the SUIT envelope, and uploaded to a device as a single unit.

#. The fetch model - where the envelope contains 0 or not all necessary candidate images.
   The manifest contains logic that instructs the device to fetch other required candidate images through a user-defined mechanism during the envelope processing.

This guide explains how to reconfigure an application that uses the push model to a fetch model-based upgrade.

Differences and similarities between the push and fetch models
**************************************************************

In both the push and fetch models, the candidate images are uploaded to the device in the background, which means that the firmware continues normal operation while the data is being transferred.

The push model does not require the SUIT processor to be built into the application firmware.
The fetch model, on the other hand, requires the SUIT processor to be built into the application firmware in order to execute the ``suit-payload-fetch`` sequence.
You can enable this by using the :kconfig:option:`CONFIG_SUIT_DFU_CANDIDATE_PROCESSING_FULL` Kconfig option.

In the push model, the envelope along with all necessary candidate images must be stored in a continuous memory region in the MCU's non-volatile storage.
In the fetch model, the envelope must still be stored in a continuous memory region in the MCU, but it does not occupy as much space because not all candidate images are integrated into it.
Some or all candidate images can be stored in separate memory regions from the envelope, or on a completely different storage device altogether.

Reasons to use the fetch model
******************************

The fetch model has greater flexibility compared to the push model in the following areas:

* Candidate images can be fetched conditionally.
  For example, fetching up-to-date firmware can be completely skipped, which saves battery life and transfer costs.

* Custom fetch source can be implemented.
  Nordic Semiconductor provides a reference fetch source implementation which uses the SMP protocol over serial or Bluetooth® LE.
  You have the option to implement any fetching mechanism needed for the application, such as fetching from an HTTP resource.

* Candidate images can be stored in a different memory partition than the envelope itself.
  The SUIT envelope itself must always be stored in non-volatile storage that is integrated into the MCU.
  The fetched candidate images do not have restrictions on where they are placed and can be stored on an external memory chip.

Migrating from push-based to fetch-based firmware upgrade
*********************************************************

The :ref:`nrf54h_suit_sample` sample uses the SMP protocol for uploading new envelopes and is by default configured to use the push model for firmware upgrades.
To reconfigure the sample to use the fetch model, complete the following steps:

1. Enable the following three Kconfig options:

   * :kconfig:option:`CONFIG_SUIT_DFU_CANDIDATE_PROCESSING_FULL` - This enables SUIT envelope processing in the application firmware.
     The application firmware will execute the ``suit-payload-fetch`` sequence, if it is defined in the root manifest.
   * :kconfig:option:`CONFIG_MGMT_SUITFU_GRP_SUIT_CAND_ENV_UPLOAD` and :kconfig:option:`CONFIG_MGMT_SUITFU_GRP_SUIT_IMAGE_FETCH`.
     These options enable Nordic Semiconductor's reference implementation of the SMP-based fetch source.
     In this implementation, the SMP server (the device being updated) uses the SMP client (a computer or a mobile phone) as a proxy to fetch the required candidate images.

#. Add the ``suit-payload-fetch`` sequence to the root manifest :file:`root_with_binary_nordic_top.yaml.jinja2`:

   .. code-block:: yaml

      suit-payload-fetch:
        - suit-directive-set-component-index: 0
        - suit-directive-override-parameters:
            suit-parameter-uri: '#{{ app['name'] }}'
        - suit-directive-fetch:
          - suit-send-record-failure
        - suit-condition-dependency-integrity:
          - suit-send-record-success
          - suit-send-record-failure
          - suit-send-sysinfo-success
          - suit-send-sysinfo-failure
        - suit-directive-process-dependency:
          - suit-send-record-success
          - suit-send-record-failure
          - suit-send-sysinfo-success
          - suit-send-sysinfo-failure

   This instructs the SUIT processor to execute the ``suit-payload-fetch`` sequence in the application manifest, which will be added in the next step.

#. Modify the application manifest :file:`app_envelope.yaml.jinja2` by completing the following:

   a. Append the ``CACHE_POOL`` component:

      .. code-block:: yaml

         suit-components:
             ...
         - - CACHE_POOL
           - 0

      The ``CACHE_POOL`` component with identifier ``0`` is significant, as it is always available and occupies the free space in the DFU partition after the envelope.
      It is possible to define additional ``CACHE_POOL`` partitions using devicetree.

      In this example, the ``CACHE_POOL`` component index is ``2``.
      In the following steps the cache pool component is selected with ``suit-directive-set-component-index: 2``.

   #. Add the ``suit-payload-fetch`` sequence to the application manifest:

      .. code-block:: yaml

         suit-payload-fetch:
         - suit-directive-set-component-index: 2
         - suit-directive-override-parameters:
             suit-parameter-uri: 'file://{{ app['binary'] }}'
         - suit-directive-fetch:
           - suit-send-record-failure

      The SUIT procedure attempts to use all fetch sources registered with :c:func:`suit_dfu_fetch_source_register` until one of them fetches the payload.
      If no sources are able to fetch the payload, the update process ends with an error.

      The reference SMP fetch source implementation only recognizes URIs that start with ``file://``.

   #. Modify the ``suit-install`` sequence to use an identical URI, as in the ``suit-payload-fetch``, instead of the integrated one.

      .. code-block:: diff

           suit-install:
             ...
           - suit-directive-set-component-index: 1
           - suit-directive-override-parameters:
         -     suit-parameter-uri: '#{{ app['name'] }}'
         +     suit-parameter-uri: 'file://{{ app['binary'] }}'
           - suit-directive-fetch:
             - suit-send-record-failure

      When the secure domain firmware processes the ``suit-install`` sequence, this sequence of directives instructs the secure domain to search for a payload with a given URI in all cache partitions.
      If no such payload is found, the update process ends with an error.

   #. Remove the application binary from the integrated payloads:

      .. code-block:: diff

         - suit-integrated-payloads:
         -   '#{{ app['name'] }}': {{ app['binary'] }}
         + suit-integrated-payloads: {}

      In the fetch model-based firmware upgrade, it is not necessary to integrate the payload into the envelope.
      However, you may still choose to integrate certain payloads.

Creating a custom fetch source
******************************

The reference fetch source (provided by Nordic Semiconductor's implementation) can be found in the :file:`subsys/mgmt/suitfu/src/suitfu_mgmt_suit_image_fetch.c` file.
This serves as a base for implementing custom fetch sources, such as fetching from an HTTP server.
The fetch source API can be found in the :file:`include/dfu/suit_dfu_fetch_source.h` file.
