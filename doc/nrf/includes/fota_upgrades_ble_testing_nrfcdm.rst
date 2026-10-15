.. fota_upgrades_outro_start

To perform a FOTA update, complete the following steps:

.. fota_upgrades_over_ble_nrfcdm_common_dfu_steps_start

1. Generate the DFU package by building your application with the FOTA support over Bluetooth Low Energy.
   You can find the generated :file:`dfu_application.zip` archive in the build directory.

   .. note::
      For each image included in the DFU-generated package, use a higher version number than your currently active firmware.
      Otherwise, the DFU target may reject the FOTA process due to a downgrade prevention mechanism.

#. Download the :file:`dfu_application.zip` archive to your device.
   See :ref:`app_build_output_files` for more information about the contents of update archive.

   .. note::
      nRF Connect for Desktop does not currently support the FOTA process.

#. Use the `nRF Connect Device Manager`_ mobile app to update your device with the new firmware.

   a. Ensure that you can access the :file:`dfu_application.zip` archive from your phone or tablet.
   #. In the mobile app, scan and select the device to update.
   #. Switch to the :guilabel:`Image` tab.
   #. Tap the :guilabel:`SELECT FILE` button and select the :file:`dfu_application.zip` archive.
   #. Tap the :guilabel:`START` button.

      .. note::
         When performing a FOTA update with the iOS app for samples using random HCI identities, ensure that the :guilabel:`Erase application settings` option is deselected before starting the procedure.
         Otherwise, the new image will boot with random IDs, causing communication issues between the app and the device.

   #. Initiate the DFU process of transferring the image to the device:

      * If you are using an Android device, select a mode in the dialog window, and tap the :guilabel:`START` button.
      * If you are using an iOS device, tap the selected mode in the pop-up window.

      .. note::
         For samples using random HCI identities, the Test and Confirm mode should not be used.

   #. Wait for the DFU to finish and then verify that the application works properly.

.. fota_upgrades_over_ble_nrfcdm_common_dfu_steps_end

.. fota_upgrades_outro_end
