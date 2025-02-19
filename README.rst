nRF Connect SDK –  Diagnostic Headphones 🎧🔊
########################

.. contents::
   :local:
   :depth: 2

This repository contains the core of nRF Connect SDK, including subsystems,
libraries, samples, and applications. Files in applications/nrf5340_audio will be modified to integrate new features into the nRF5340 Audio application. 
These enhancements are tailored for a diagnostic headphone system that measures ambient noise and generates hearing test signals.
It is also the SDK's west manifest repository, containing the nRF Connect SDK
manifest (west.yml).


Documentation
*************
🔹 About This Fork


This fork builds upon the original nRF5340 Audio application with the following modifications:

✅ Ambient Noise Measurement and Active Noise Cancellation – Integrating noise level detection to adapt hearing tests based on the environment.

✅ Modified Unicast Server & Client – Extending the existing audio streaming features to support diagnostic functionalities.

📌 Key Modifications


📡 Ambient Noise Detection: Measures surrounding noise levels and transmits a signal indicating noise conditions.

🎵 Enhanced Audio Processing: Adapts hearing test signals based on ambient noise to improve accuracy.

🔧 Codec Evaluation & Implementation: Investigating additional codec support for improved audio signal processing.


🚀 Getting Started
To use this modified version, clone the repo and follow the standard nRF Connect SDK setup:

          git clone https://github.com/mighri-manar/BLE_HEADPHONES_FOR_AUDIOMETRY.git

          cd sdk-nrf

          west init -l

          west update

📜 License
This project is based on the original nRF Connect SDK and follows its licensing terms.



Official latest documentation at https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/index.html

For earlier versions, open the latest version and use the drop-down under the title header.
