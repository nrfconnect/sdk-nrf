# Makefile for generating Python protobuf definitions
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

.PHONY: all generate clean install-deps help test

# Default target
all: generate

# Generate Python protobuf definitions
generate:
	@echo "Generating Python protobuf definitions..."
	protoc --python_out=. request.proto common.proto response.proto result.proto version.proto
	@echo "Generated files:"
	@ls -la *_pb2.py

# Clean generated files
clean:
	@echo "Cleaning generated files..."
	rm -f *_pb2.py
	@echo "Cleaned!"

# Install dependencies (if needed)
install-deps:
	@echo "Installing protobuf dependencies..."
	pip install protobuf

# Run tests
test: generate
	@echo "Running auth mode validation tests..."
	python3 test_auth_modes.py

# Show help
help:
	@echo "Available targets:"
	@echo "  generate     - Generate Python protobuf definitions"
	@echo "  clean        - Remove generated files"
	@echo "  install-deps - Install protobuf dependencies"
	@echo "  test         - Run validation tests"
	@echo "  help         - Show this help message"
