.. _ncs_release_notes_latest:

Changes in |NCS| v1.5.99
########################

.. contents::
   :local:
   :depth: 2

The most relevant changes that are present on the master branch of the |NCS|, as compared to the latest release, are tracked in this file.

.. note::
   This file is a work in progress and might not cover all relevant changes.

Highlights
**********

There are no entries for this section yet.

Changelog
*********

The following sections provide detailed lists of changes by component.

nRF9160
=======

* Added:

  * :ref:`lib_modem_jwt` library, which provides an API to obtain a JSON Web Token (JWT) from the modem.  Functionality requires modem firmware v1.3.0 or higher.

  * :ref:`lib_modem_attest_token` library:

    * The library provides an API to get an attestation token from the modem.
    * Functionality requires modem firmware v1.3.0 or higher.

  * :ref:`mod_memfault` module and integration on |NCS| for nRF9160-based devices.

    * Integration of Memfault SDK into |NCS|.

  * :ref:`memfault_sample` sample implementing :ref:`mod_memfault` module and showing how to use Memfault SDK functionality in an application.


* Updated:

  * :ref:`lib_nrf_cloud` library:

    * Added cellular positioning support to :ref:`lib_nrf_cloud_cell_pos`.
    * Added Kconfig option :option:`CONFIG_NRF_CLOUD_CELL_POS` to obtain cell-based location from nRF Cloud instead of using the modem's GPS.
    * Added function :c:func:`nrf_cloud_modem_fota_completed` which is to be called by the application after it re-initializes the modem (instead of rebooting) after a modem FOTA update.
    * Updated to include the FOTA type value in the :c:enumerator:`NRF_CLOUD_EVT_FOTA_DONE` event.
    * Updated configuration options for setting the source of the MQTT client ID (nRF Cloud device ID).
    * Updated nRF Cloud FOTA to use type validated FOTA download.

  * :ref:`asset_tracker` application:

    * Updated to handle new Kconfig options:

      * :option:`CONFIG_NRF_CLOUD_CELL_POS`

  * :ref:`modem_info_readme` library:

    * Updated to prevent reinitialization of param list in :c:func:`modem_info_init`.

  * :ref:`lib_fota_download` library:

    * Added an API to retrieve the image type that is being downloaded.
    * Added an API to cancel current downloading.
    * Added an API to validate FOTA image type before starting installation.

  * :ref:`lib_ftp_client` library:

    * Support subset of RFC959 FTP commands only.
    * Added support of STOU and APPE (besides STOR) for "put".
    * Added detection of socket errors, report with proprietary reply message.
    * Increased FTP payload size from NET_IPV4_MTU(576) to MSS as defined on modem side (708).
    * Added polling "226 Transfer complete" after data channel TX/RX, with a configurable timeout of 60 seconds.
    * Ignored the reply code of "UTF8 ON" command as some FTP server returns abnormal reply.

  * :ref:`at_params_readme` library:

    * Added function :c:func:`at_params_int64_get` that allows for getting of AT param list entries containing signed 64 bit integers.

  * :ref:`lte_lc_readme` library:

    * Added support for %XT3412 AT command notifications, which allows the application to get prewarnings before Tracking Area Updates.
    * Added support for neighbor cell measurements.
    * Added support for %XMODEMSLEEP AT command notifications which allows the application to get notifications related to modem sleep.
    * Added support for %CONEVAL AT command that can be used to evaluate the LTE radio signal state in a cell prior to data transmission.

  * :ref:`serial_lte_modem` application:

    * Fixed TCP/UDP port range issue (0~65535).
    * Added AT#XSLEEP=2 to power off UART interface.
    * Added support for the ``verbose``, ``uput``, ``mput`` commands and data mode to the FTP service.
    * Added URC (unsolicited response code) to the FOTA service.
    * Enabled all SLM services by default.
    * Updated the HTTP client service code to handle chunked HTTP responses.
    * Added data mode to the MQTT Publish service to support JSON-type payload.
    * Added SMS support, to send/receive SMS in plain text.

  * :ref:`at_cmd_parser_readme`:

    * Added support for parsing parameters of type unsigned int or unsigned short.

  * :ref:`lib_spm` library:

    * Added support for the nRF9160 pulse-density modulation (PDM) and inter-IC sound (I2S) peripherals in non-secure applications.

  * :ref:`at_host_sample` sample:

    * Renamed nRF9160: AT Client sample to :ref:`at_host_sample`.

  * :ref:`gps_api`:

    * Renamed gps_agps_request() to gps_agps_request_send().

  * :ref:`aws_fota_sample` sample:

    * Removed nRF Connect for Cloud support code, because ``fota_v1`` is no longer supported in nRF Connect for Cloud.
    * Removed provisioning using :ref:`modem_key_mgmt` and :file:`certificates.h`, because this is not the recommended way of provisioning private certificates.
    * Renamed the following Kconfig options:

      * ``CONFIG_CLOUD_CERT_SEC_TAG`` renamed to :option:`CONFIG_CERT_SEC_TAG`.
      * ``CONFIG_USE_CLOUD_CLIENT_ID`` renamed to :option:`CONFIG_USE_CUSTOM_CLIENT_ID`.
      * ``CONFIG_CLOUD_CLIENT_ID`` renamed to ``CONFIG_CLIENT_ID``.
      * ``CONFIG_NRF_CLOUD_CLIENT_ID_PREFIX`` renamed to ``CONFIG_CLIENT_ID_PREFIX``.

nRF5
====

Matter (Project CHIP)
---------------------

* Project CHIP has been officially renamed to `Matter`_.
* Added:

  * New user guide about :ref:`ug_matter_configuring`.

* Updated:

  * Renamed occurrences of Project CHIP to Matter.

Bluetooth mesh
--------------

* Added:

  * Support for vendor-specific mesh model :ref:`bt_mesh_silvair_enocean_srv_readme`.
  * A new API function ``bt_mesh_rpl_pending_store`` to manually store pending RPL entries in the persistent storage without waiting for the timeout.
  * A ``bt_mesh_scene_entry::recall_complete`` callback that is called for each model that has a scene entry when recalling a scene data is done.

* Updated:

  * Updated the :ref:`bt_mesh_light_xyl_srv_readme` to publish its state values that were loaded from flash after powering up.
  * Replaced the linked list of scene entries in the model contexts, with a lookup in ROM-allocated scene entries.
  * Updated so the transition pointer can be NULL, if no transition time parameters are provided in APIs.
  * Renamed Kconfig option ``CONFIG_BT_MESH_LIGHT_CTRL_STORE_TIMEOUT`` to :option:`CONFIG_BT_MESH_MODEL_SRV_STORE_TIMEOUT`, and default value is set to 0.
  * Updated the :ref:`bt_mesh_light_ctrl_srv_readme` with a timer, allowing it to resume operation after a certain delay.
  * Updated the proportional-integral (PI) feedback regulator to use instant transition time to relieve the application from overhead.
  * Fixed an issue where an undefined state for some sensor properties is a valid state, and should be handled without giving errors.
  * Fixed an issue with storing and recalling the Light OnOff state in :ref:`bt_mesh_light_ctrl_srv_readme`.
  * Fixed an issue where :ref:`bt_mesh_lightness_srv_readme` publishes twice if extended by two models.
  * Updated :ref:`bt_mesh_light_hue_srv_readme` and :ref:`bt_mesh_light_sat_srv_readme` to store their states upon a change.
  * Fixed an issue where :ref:`bt_mesh_light_hue_srv_readme` and :ref:`bt_mesh_light_sat_srv_readme` did not erase model data on reset.
  * Fixed an issue where :ref:`bt_mesh_scene_srv_readme` called scene recall at startup.
  * Fixed an issue by publishing a new value during scene recall in :ref:`bt_mesh_onoff_srv_readme` and :ref:`bt_mesh_lvl_srv_readme`.
  * Fixed issues where extended models stored or recalled instead of the extending model.
  * Updated the extending models by adding the extension API by default through Kconfig.
  * Forced the extension of :ref:`bt_mesh_lightness_srv_readme` to be initialized before :ref:`bt_mesh_light_ctrl_srv_readme`.
  * Fixed an issue where :ref:`bt_mesh_light_ctrl_srv_readme` should disable control when the lightness is set by receiving a message.
  * Added persistent storage to the :ref:`bt_mesh_scheduler_readme` to restore previously configured entries on power-up.
  * Fixed an issue where CTL temperature bindings should use rounding operation for division in the binding formula.
  * Samples are using :option:`CONFIG_NVS` instead of :option:`CONFIG_FCB` as the default storage backend.
  * Fixed an issue in :ref:`bt_mesh_light_ctrl_srv_readme` by always setting the transition time to a Fade Standby Manual state time when a Light Off event is received.
  * Fixed an issue by reporting maximum remaining time for all components for CTL state transition time when GET is processed.
  * Fixed an issue where a deleted :ref:`bt_mesh_scene_srv_readme` did not delete all its pages from the file system.
  * Fixed an issue where Sensor Threshold was trimmed and an invalid value was calculated.
  * Updated :ref:`bt_mesh_scheduler_srv_readme` to no longer extend :ref:`bt_mesh_scene_srv_readme`, but they must be present on the same element.
  * Moved the configuration settings for acknowledged messages into Kconfig to make them public.
  * Fixed an issue where an Occupancy On event did not transition to a Fade On state even if Occupancy Mode is disabled.
  * Added a flag to :ref:`bt_mesh_onoff_srv_readme` to skip Default Transition Time on Generic OnOff Set (Unack) messages.
  * Fixed an issue by correcting the bindings between the Generic OnOff state and the Light OnOff state.
  * Fixed an issue by clearing the internal sum in the proportional-integral (PI) feedback regulator when entering the OFF state of the state machine.
  * Fixed an issue where :ref:`bt_mesh_lightness_srv_readme` could accidentally disable :ref:`bt_mesh_light_ctrl_srv_readme` before it was started.
  * Fixed an issue by publishing the Light Lightness Status message on startup even if the OnPowerUp state is OFF.
  * Fixed issues by publishing the Light OnOff Status when disabling and restoring the Light LC state.
  * Fixed an issue where temperature and range should be within a valid default range for the :ref:`bt_mesh_light_temp_srv_readme`.
  * Removed sensor type ``BT_MESH_SENSOR_DELTA_DISABLED`` as it is removed from specification.
  * Replaced ``struct bt_mesh_model_ack_ctx`` with ``struct bt_mesh_msg_ack_ctx`` from :ref:`zephyr:bluetooth_mesh_msg`.

Thread
------

* Added support for the following Thread 1.2 features:

  * Link Metrics
  * Coordinated Sampled Listening (CSL)

  These features are supported for nRF52 Series devices.

Zigbee
------

* Added the v0.9.5 version of the `ZBOSS NCP Host`_ package that includes a simple gateway application.
* Updated:

  * Reworked the :ref:`NCP sample <zigbee_ncp_sample>` to work with the simple gateway application.
  * Moved the `NCP Host documentation`_ from the `ZBOSS NCP Host`_ package to the same location as the `external ZBOSS development guide and API documentation`_.

Common
======

There are no entries for this section yet.

MCUboot
=======

The MCUboot fork in |NCS| (``sdk-mcuboot``) contains all commits from the upstream MCUboot repository up to and including ``2fce9769b1``, plus some |NCS| specific additions.

The code for integrating MCUboot into |NCS| is located in :file:`ncs/nrf/modules/mcuboot`.

The following list summarizes the most important changes inherited from upstream MCUboot:

* Added support for indicating serial recovery through LED.
* Made the debounce delay of the serial detect pin state configurable.
* Added support for mbed TLS ECDSA for signatures.
* Added an option to use GPIO PIN to enter to USB DFU class recovery.
* Added an optional check that prevents attempting to boot an image built for a different ROM address than the slot it currently resides in.
  The check is enabled if the image was signed with the ``IMAGE_F_ROM_FIXED`` flag.

Mcumgr
======

The mcumgr library contains all commits from the upstream mcumgr repository up to and including snapshot ``74e77ad08``.

The following list summarizes the most important changes inherited from upstream mcumgr:

* No changes yet

Zephyr
======

.. NOTE TO MAINTAINERS: The latest Zephyr commit appears in multiple places; make sure you update them all.

The Zephyr fork in |NCS| (``sdk-zephyr``) contains all commits from the upstream Zephyr repository up to and including ``730acbd6ed`` (``v2.6.0-rc1``), plus some |NCS| specific additions.

For a complete list of upstream Zephyr commits incorporated into |NCS| since the most recent release, run the following command from the :file:`ncs/zephyr` repository (after running ``west update``):

.. code-block:: none

   git log --oneline v2.6.0-rc1 ^v2.4.99-ncs1

For a complete list of |NCS| specific commits, run:

.. code-block:: none

   git log --oneline manifest-rev ^v2.6.0-rc1

The current |NCS| release is based on Zephyr v2.6.0-rc1.
See the :ref:`zephyr:zephyr_2.6` Release Notes for an overview of the most important changes inherited from upstream Zephyr.

Matter (Project CHIP)
=====================

The Matter fork in the |NCS| (``sdk-connectedhomeip``) contains all commits from the upstream Matter repository up to and including ``aa96ea0365``.

The following list summarizes the most important changes inherited from the upstream Matter:

* Added:

  * Completed the persistent storage feature, which allows Matter devices to successfully communicate with each other even after reboot.
  * Added support for OpenThread's Service Registration Protocol (SRP) to enable the discovery of Matter nodes using the DNS-SD protocol.
  * Added support for Network Commissioning Cluster, used when provisioning a Matter node.
  * Enabled Message Reliability Protocol (MRP) for the User Datagram Protocol (UDP) traffic within a Matter network.
  * Added support for Operational Credentials Cluster, used to equip a Matter node with an operational certificate.

Documentation
=============

There are no entries for this section yet.

Known issues
************

Known issues are only tracked for the latest official release.
See `known issues for nRF Connect SDK v1.5.0`_ for the list of issues valid for this release.
