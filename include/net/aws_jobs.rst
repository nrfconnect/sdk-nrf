.. _lib_aws_jobs:

AWS Jobs
########

This library provides functions for working with the `AWS IoT jobs
<https://docs.aws.amazon.com/iot/latest/developerguide/iot-jobs.html>`_
service.

The AWS Jobs library provides APIs that can be used to:

- report status,
- subscribe to job topics.

The module also contains the following elements:

- string templates for topics,
- defines for lengths of topics, status, and job IDs,
- defines for Subscribe message IDs.

This library assumes that all strings can be UTF-8 formatted.

Configuration
*************

Use Kconfig to configure the MQTT message buffer size.


API documentation
*****************

| Header file: :file:`include/aws_jobs.h`
| Source files: :file:`subsys/net/lib/aws_jobs/`

.. doxygengroup:: aws_jobs
   :project: nrf
   :members:
