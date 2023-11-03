.. _nrf_802154_limitations:

Limitations
###########

When working with the nRF 802.15.4 Radio Driver, you should be aware of the following limitations.
In addition, see :ref:`known_issues` for temporary issues that will be fixed in future releases.

IRQ priority limitations
  Application and device drivers (excluding those compliant with :ref:`mpsl`) must not use IRQ priority higher than :c:macro:`NRF_802154_SWI_PRIORITY` and :c:macro:`NRF_802154_SL_RTC_IRQ_PRIORITY`.

KRKNWK-11204: Transmitting an 802.15.4 frame with improperly populated Auxiliary Security Header field will result in assert
  **Workaround:** Make sure that you populate the Auxiliary Security Header field according to the IEEE Std 802.15.4-2015 specification, section 9.4.

KRKNWK-12482: Reception of correct frames will occasionally end in failure with error ``NRF_802154_RX_ERROR_RUNTIME``
  This issue can occur for the ``nrf5340dk_nrf5340_cpunet`` target if a custom application (other than :ref:`multiprotocol-rpmsg-sample` sample or :ref:`zephyr:nrf-ieee802154-rpmsg-sample` sample) is used.
