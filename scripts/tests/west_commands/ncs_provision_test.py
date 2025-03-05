#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import pytest
from ncs_provision import KmuMetadata


@pytest.mark.parametrize(
    'rpolicy,expected_metadata',
    [
        (1, 0x10BA0030),
        (2, 0x113A0030),
        (3, 0x11BA0030)
    ],
    ids=['ROTATING', 'LOCKED', 'REVOKED']
)
def test_if_kmu_metadata_are_properly_generated_for_rpolicy(rpolicy, expected_metadata):
    metadata = KmuMetadata(
        metadata_version=0,
        key_usage_scheme=3,
        reserved=0,
        algorithm=10,
        size=3,
        rpolicy=rpolicy,
        usage_flags=8
    )
    assert metadata.value == expected_metadata
    assert str(metadata) == hex(expected_metadata)


@pytest.mark.parametrize(
    'metadata,rpolicy',
    [
        (0x10BA0030, 1),
        (0x113A0030, 2),
        (0x11bA0030, 3)
    ],
    ids=['ROTATING', 'LOCKED', 'REVOKED']
)
def test_if_kmu_metadata_are_properly_generated_from_value(rpolicy, metadata):
    actual_metadata = KmuMetadata.from_value(metadata)
    assert actual_metadata.rpolicy == rpolicy
