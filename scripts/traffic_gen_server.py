#!/usr/bin/env python3

# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import socket
import struct
import time
import argparse
import logging

# Define constants for client roles
UPLINK = 1
DOWNLINK = 2

# Define constants for traffic types
TCP = 1

# Define constants for traffic modes
CONTINUOUS = 1
DELAYED = 2

EXTRA_TIMEOUT_S = 5

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Define the message structure using a Python dictionary
config_data = {
    "role": 0,
    "type": 0,
    "mode": 0,
    "duration": 0,
    "payload_len": 0,
}


def tcp_server(ctrl_socket, config_data):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ("0.0.0.0", 7777)  # Listen on all available network interfaces
    server_socket.bind(server_address)
    server_socket.listen(1)
    logger.debug(
        f"TCP server is listening on {server_address[0]}:{server_address[1]}..."
    )

    client_socket, client_address = server_socket.accept()
    logger.debug(f"Connected to client: {client_address}")

    packet_count = 0
    total_bytes_received = 0
    start_time = time.time()
    remaining_timeout = config_data["duration"] + EXTRA_TIMEOUT_S

    try:
        while True:
            client_socket.settimeout(
                remaining_timeout
            )  # Set the current remaining timeout
            try:
                data = client_socket.recv(1024)
                if not data:
                    logger.debug("Received empty message. Closing the connection.")
                    break
                packet_count += 1
                total_bytes_received += len(data)
            except socket.timeout:
                logger.warning(
                    f"No data received for {remaining_timeout:.2f} seconds. Closing connection."
                )
                break
            except BlockingIOError:
                logger.debug("BlockingIOError caught. Setting socket to non-blocking.")
                client_socket.setblocking(False)  # Set the socket to non-blocking
                break
            finally:
                elapsed_time = time.time() - start_time
                remaining_timeout = max(
                    0, (config_data["duration"] + EXTRA_TIMEOUT_S) - elapsed_time
                )

    finally:
        # Calculate statistics
        end_time = time.time()
        total_seconds = int(end_time - start_time)
        throughput_kbps = int(
            (total_bytes_received * 8) / (total_seconds * 1024)
        )  # Kbps calculation
        average_jitter = 0  # Calculate jitter as needed

        # Prepare the report data
        report_data = {
            "bytes_received": total_bytes_received,
            "packets_received": packet_count,
            "elapsed_time": total_seconds,
            "throughput": throughput_kbps,
            "average_jitter": average_jitter,
        }

        # Print the report
        print_report(report_data)
        time.sleep(5)
        # Send the report back to the client using the 6666 server socket
        send_server_report(ctrl_socket, report_data)
        client_socket.close()
        server_socket.close()
        logger.info("TCP server is closed.")


def send_server_report(client_socket, report_data):
    report_format = "!iiiii"
    report_packed = struct.pack(
        report_format,
        report_data["bytes_received"],
        report_data["packets_received"],
        report_data["elapsed_time"],
        report_data["throughput"],
        report_data["average_jitter"],
    )
    client_socket.sendall(report_packed)
    logger.debug("Report sent to client.")


def print_report(report_data):
    logger.info("TCP Server Report:")
    logger.info(f"   Bytes Received: {report_data['bytes_received']} bytes")
    logger.info(f"   Packets Received: {report_data['packets_received']}")
    logger.info(f"   Elapsed Time: {report_data['elapsed_time']} Seconds")
    logger.info(f"   Throughput: {report_data['throughput']} Kbps")
    logger.info(
        f"   Average Jitter: {report_data['average_jitter']}"
    )  # Calculate and provide actual value


def tcp_client(config_data, client_address):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = (
        client_address[0],
        7777, # TODO: Get this from the client
    )
    client_socket.connect(server_address)

    try:
        start_time = time.time()
        while time.time() - start_time < config_data["duration"]:
            message = (
                b"x" * config_data["payload_len"]
            )  # Creating a message with specified frame length
            client_socket.sendall(message)
            time.sleep(1 / 1000)  # Sending messages every 1 second
        logger.debug("Data transmission finished.")
        # Send an empty message to signal termination
        client_socket.sendall(b"")
    finally:
        client_socket.close()


def process_client_config_data(ctrl_socket, client_address):
    global config_data

    logger.info("Waiting for client config info\n")
    buffer = ctrl_socket.recv(1024)
    cf = buffer[:24]  # Assuming sizeof(struct config_data) is 24 bytes
    config_data = {
        "role": int.from_bytes(cf[0:4], byteorder="big"),
        "type": int.from_bytes(cf[4:8], byteorder="big"),
        "mode": int.from_bytes(cf[8:12], byteorder="big"),
        "duration": int.from_bytes(cf[12:16], byteorder="big"),
        "payload_len": int.from_bytes(cf[16:20], byteorder="big"),
    }

    logger.info("Client config data received")
    logger.info("\tClient Role: %d", config_data["role"])
    logger.info("\tTraffic type: %d", config_data["type"])
    logger.info("\tTraffic Mode: %d", config_data["mode"])
    logger.info("\tDuration: %d", config_data["duration"])
    logger.info("\tFrame len: %d", config_data["payload_len"])

    # Implement logic for starting servers/clients and processing traffic
    if config_data["role"] == UPLINK:
        logger.debug("UPLINK:")
        if config_data["type"] == TCP:
            logger.debug("TCP SERVER STARTED")
            tcp_server(ctrl_socket, config_data)
        else:
            logger.info("Invalid Traffic type")
    elif config_data["role"] == DOWNLINK:
        logger.debug("DOWNLINK:")
        time.sleep(10)
        if config_data["type"] == TCP:
            logger.debug("TCP CLIENT STARTED")
            tcp_client(config_data, client_address)
            logger.debug("TCP CLIENT FINISHED")
        else:
            logger.error("Invalid Traffic Type")

def parse_args():
    parser = argparse.ArgumentParser(
        allow_abbrev=False,
        description="Traffic Generator server for testing nRF70 Wi-Fi chipsets"
    )
    parser.add_argument(
        "--control-port",
        type=int,
        default=6666,
        help="Port for control connection with client",
    )
    parser.add_argument("-d", "--debug", action="store_true", help="Enable debug logs")

    return parser.parse_args()


def main():
    args = parse_args()  # Parse command-line arguments
    if args.debug:
        logger.setLevel(logging.DEBUG)
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = (
        "0.0.0.0",
        args.control_port,
    )  # Listen on all available network interfaces
    server_socket.bind(server_address)
    server_socket.listen(1)
    logger.info(f"Server is listening on {server_address[0]}:{server_address[1]}...")

    client_socket, client_address = server_socket.accept()
    logger.info(f"Connected to client: {client_address}")

    try:
        process_client_config_data(client_socket, client_address)
    finally:
        time.sleep(2)
        client_socket.close()
        server_socket.close()
        logger.debug("Server is closed.")

if __name__ == "__main__":
    main()
