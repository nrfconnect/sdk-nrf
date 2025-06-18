# Unit tests

## Dependencies
To run unit tests following dependencies are required.

### Tools
* ceedling v0.30.0

### Packages
None additional packages are required.

## Quick start
Call following command in the main directory of this repo:
```
$ ./scripts/unit_tests.sh
```
This command runs unit tests for all the supported variants.

## Coverage
Call following command in the main directory of this repo:
```
$ ./scripts/unit_tests.sh --coverage
```
The coverage report will be generated and can be found in the `build/artifacts/gcov/` directory.

## Useful features

### Fail fast
The following command will stop execution if any of the variants fails.
```
$ ./scripts/unit_tests.sh --fail-fast
```
