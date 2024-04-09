.. _pca63565:

PCA63565 shield
###############

.. contents::
   :local:
   :depth: 2

Overview
********

PCA63565 is a testing shield with following sensors:

* ADXL362 - Accelerometer (SPI interface)
* BMI270 - Accelerometer and gyroscope (SPI interface)
* BME688 - Temperature, pressure, humidity and gas sensor (I2C interface with on-board pull-up resistors 4.7 kohm).
           It is targeted for testing I2C, and SPI protocols drivers.

Supply voltage - 1.8V

SPI interface:

* SCK (P2.6)
* MISO (P2.9)
* MOSI (P2.8)
* CS ADXL362 (P2.10)
* CS BMI270 (P1.13)
* IRQ from ADXL or BMI270 (selected with a jumper) (P1.14)

I2C interface:

* SCL (P1.11)
* SDA (P1.12)

Requirements
************

This shield is pin compatible with the nRF54L15 PDK (tested with PCA10156 rev.0.2.1).

Usage
*****

The shield can be used in any application by setting ``SHIELD`` to ``pca63565``.
