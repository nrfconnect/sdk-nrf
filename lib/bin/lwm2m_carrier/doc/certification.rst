.. _lwm2m_certification:

Certification and version dependencies
######################################

.. contents::
   :local:
   :depth: 2

Every released version of the LwM2M carrier library is considered for certification with applicable carriers.
The LwM2M carrier library is certified together with specific versions of the modem firmware and the |NCS|.
Refer to the :ref:`liblwm2m_carrier_changelog` or the :ref:`versiondep_table` to check the certification status of a particular version of the library, and to see the version of the |NCS| it was released with.

For a list of all the carrier certifications (including those certifications with no dependency on the LwM2M carrier library), see the `Mobile network operator certifications`_.

.. note::

   Your final product will need certification from the carrier.
   Please contact the carrier for more information on their respective device certification program.

.. _versiondep_table:

Version dependency table
************************

See the following table for the certification status of the library and the related version dependencies:

+-----------------+---------------+---------------+---------------+
| LwM2M carrier   | Modem version | |NCS| release | Certification |
| library version |               |               |               |
+=================+===============+===============+===============+
| 0.10.0          | 1.1.4         | 1.4.0         |               |
|                 |               |               |               |
|                 | 1.2.2         |               |               |
+-----------------+---------------+---------------+---------------+
| 0.9.1           | 1.2.1         | 1.3.1         | AT&T          |
+-----------------+---------------+---------------+---------------+
| 0.9.0           | 1.2.1         | 1.3.0         |               |
+-----------------+---------------+---------------+---------------+
| 0.8.2           | 1.1.2         | 1.2.1         | Verizon       |
+-----------------+---------------+---------------+---------------+
| 0.8.1+build1    | 1.1.0         | 1.2.0         | Verizon       |
+-----------------+---------------+---------------+---------------+
| 0.8.0           | 1.1.0         | 1.1.0         |               |
+-----------------+---------------+---------------+---------------+
| 0.6.0           | 1.0.1         | 1.0.0         |               |
+-----------------+---------------+---------------+---------------+
