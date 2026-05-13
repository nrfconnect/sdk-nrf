import os

import pytest


def pytest_addoption(parser):
    parser.addoption(
        "--limits-path",
        action="store",
        default=None,
    )


@pytest.fixture(scope="session")
def limits_path(request):
    raw_path = request.config.getoption("--limits-path")
    if not raw_path:
        return None

    expanded_path = os.path.expandvars(raw_path)
    final_path = os.path.abspath(expanded_path)
    if not os.path.exists(final_path):
        pytest.fail(f"Limit file not found at path: {final_path}")

    return final_path
