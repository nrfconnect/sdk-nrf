.. _nrf9160_gs_testing_cellular:

Testing the cellular connection on nRF91 Series DK
##################################################

The :ref:`at_client_sample` sample enables you to send AT commands to the modem of your nRF91 Series DK to test and monitor the cellular connection.
You can use it to troubleshoot and debug any connection problems.

Complete the following steps to test the cellular connection using the AT Client sample:

1. Follow the steps in :ref:`nrf9160_gs_updating_fw_application` to program the sample to the DK.
   When selecting the HEX file, select the following file instead of the one for Asset Tracker v2:

   * nRF9161 DK: :file:`nrf9161dk_at_client_<version-number>.hex`
   * nRF9160 DK: :file:`nrf9160dk_at_client_<version-number>.hex`

#. Test the AT Client sample as described in the Testing section of the :ref:`at_client_sample` documentation.
