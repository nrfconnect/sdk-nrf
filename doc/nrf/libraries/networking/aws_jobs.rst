.. _lib_aws_jobs:

AWS jobs
########

.. contents::
   :local:
   :depth: 2

The Amazon Web Services (AWS) jobs library provides functions for working with the `AWS IoT jobs`_ service.

You can use the library to report the status of AWS IoT jobs and to subscribe to job topics.

The module also contains the following elements:

* String templates that can be used for generating MQTT topics
* Defines for lengths of topics, status, and job IDs
* Defines for subscribe message IDs

This library assumes that all strings can be formatted in UTF-8.

Configuration
*************

Configure the following parameters when using this library:

* :kconfig:option:`CONFIG_UPDATE_JOB_PAYLOAD_LEN`


API documentation
*****************

| Header file: :file:`include/net/aws_jobs.h`
| Source files: :file:`subsys/net/lib/aws_jobs/`

.. doxygengroup:: aws_jobs
   :project: nrf
   :members:
