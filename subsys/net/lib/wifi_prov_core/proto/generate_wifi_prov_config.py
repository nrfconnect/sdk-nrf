#!/usr/bin/env python3
"""
Generate EAP-TLS configuration protobuf message from certificate files.

This script generates WiFi configuration protobuf messages for EAP-TLS or Personal
authentication modes. It reads certificate files from a specified directory and
creates a protobuf message that can be used for WiFi provisioning.

Usage: python3 generate_eap_tls_config.py <ssid> <bssid> [options]

Copyright (c) 2025 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import argparse
import base64
import os
import sys

# Import generated protobuf modules
try:
    from common_pb2 import AuthMode, Band, OpCode, WifiInfo
    from request_pb2 import EnterpriseCertConfig, Request, WifiConfig
except ImportError:
    print("Error: Python protobuf defigdnitions not found.")
    print("Please ensure the CMake build has generated the protobuf files.")
    print("The files should be generated automatically during the build process.")
    sys.exit(1)

def read_cert_file(file_path):
    """Read certificate file and return as bytes.

    Args:
        file_path: Path to the certificate file

    Returns:
        bytes: Certificate file contents

    Raises:
        FileNotFoundError: If certificate file does not exist
        IOError: If file cannot be read
    """
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"Certificate file not found: {file_path}")

    try:
        with open(file_path, 'rb') as f:
            return f.read()
    except OSError as e:
        raise OSError(f"Failed to read certificate file {file_path}: {e}")

def check_required_files(cert_dir):
    """Check if all required certificate files exist.

    Args:
        cert_dir: Directory containing certificate files

    Returns:
        bool: True if all required files exist, False otherwise
    """
    required_files = [
        'ca.pem',
        'client.pem',
        'client-key.pem',
        'ca2.pem',
        'client2.pem',
        'client-key2.pem'
    ]

    missing_files = []
    for file in required_files:
        file_path = os.path.join(cert_dir, file)
        if not os.path.exists(file_path):
            missing_files.append(file)

    if missing_files:
        print(f"Error: Missing required certificate files: {missing_files}")
        return False

    print(f"[OK] All required certificate files found in {cert_dir}")
    return True

def generate_wifi_config(ssid, bssid, channel=6, band=1, auth_mode=5,
                        cert_dir=None, passphrase=None, identity="user@example.com", password="user_password",
                        private_key_passwd=None, private_key_passwd2=None):
    """Generate WiFi configuration protobuf message (EAP-TLS or Personal)."""

    try:
        # Validate auth mode
        valid_auth_modes = list(range(24))  # 0-23 based on AuthMode enum
        if auth_mode not in valid_auth_modes:
            raise ValueError(f"invalid enumerator {auth_mode}")

        # Create WifiInfo
        wifi_info = WifiInfo()
        wifi_info.ssid = ssid.encode('utf-8')
        wifi_info.bssid = bytes.fromhex(bssid.replace(':', ''))
        wifi_info.band = Band.BAND_2_4_GH if band == 1 else Band.BAND_5_GH
        wifi_info.channel = channel

        # Set auth mode using enum value
        try:
            wifi_info.auth = auth_mode
        except ValueError:
            # Try using the enum directly
            auth_enum_map = {
                0: AuthMode.OPEN,
                1: AuthMode.WEP,
                2: AuthMode.WPA_PSK,
                3: AuthMode.WPA2_PSK,
                4: AuthMode.WPA_WPA2_PSK,
                5: AuthMode.WPA2_ENTERPRISE,
                6: AuthMode.WPA3_PSK,
                7: AuthMode.NONE,
                8: AuthMode.PSK,
                9: AuthMode.PSK_SHA256,
                10: AuthMode.SAE,
                11: AuthMode.WAPI,
                12: AuthMode.EAP,
                13: AuthMode.WPA_AUTO_PERSONAL,
                14: AuthMode.DPP,
                15: AuthMode.EAP_PEAP_MSCHAPV2,
                16: AuthMode.EAP_PEAP_GTC,
                17: AuthMode.EAP_TTLS_MSCHAPV2,
                18: AuthMode.EAP_PEAP_TLS,
                19: AuthMode.FT_PSK,
                20: AuthMode.FT_SAE,
                21: AuthMode.FT_EAP,
                22: AuthMode.FT_EAP_SHA384,
                23: AuthMode.SAE_EXT_KEY
            }
            if auth_mode in auth_enum_map:
                wifi_info.auth = auth_enum_map[auth_mode]
            else:
                raise ValueError(f"invalid enumerator {auth_mode}")

        # Create WifiConfig
        wifi_config = WifiConfig()
        wifi_config.wifi.CopyFrom(wifi_info)
        wifi_config.volatileMemory = False

        # Handle EAP-TLS mode
        if cert_dir is not None:
            # Check if all required files exist
            if not check_required_files(cert_dir):
                return None

            # Read certificate files from directory
            ca_cert_data = read_cert_file(os.path.join(cert_dir, 'ca.pem'))
            client_cert_data = read_cert_file(os.path.join(cert_dir, 'client.pem'))
            private_key_data = read_cert_file(os.path.join(cert_dir, 'client-key.pem'))

            # Read secondary certificates (same as primary for EAP-TLS)
            ca_cert2_data = read_cert_file(os.path.join(cert_dir, 'ca2.pem'))
            client_cert2_data = read_cert_file(os.path.join(cert_dir, 'client2.pem'))
            private_key2_data = read_cert_file(os.path.join(cert_dir, 'client-key2.pem'))

            print("[OK] Successfully read all certificate files")

            # Create EnterpriseCertConfig
            # MbedTLS uses null-terminator to distinguish b/w PEM and DER formats
            def add_null_terminator(data):
                if data and not data.endswith(b'\0'):
                    return data + b'\0'
                return data

            cert_config = EnterpriseCertConfig()
            cert_config.ca_cert = add_null_terminator(ca_cert_data)
            cert_config.client_cert = add_null_terminator(client_cert_data)
            cert_config.private_key = add_null_terminator(private_key_data)
            cert_config.private_key_passwd = private_key_passwd or password
            cert_config.ca_cert2 = add_null_terminator(ca_cert2_data)
            cert_config.client_cert2 = add_null_terminator(client_cert2_data)
            cert_config.private_key2 = add_null_terminator(private_key2_data)
            cert_config.private_key_passwd2 = private_key_passwd2 or password
            cert_config.identity = identity
            cert_config.password = password

            wifi_config.certs.CopyFrom(cert_config)
            print("[OK] EAP-TLS mode configured")

        # Handle Personal mode
        elif passphrase is not None:
            wifi_config.passphrase = passphrase.encode('utf-8')
            print("[OK] Personal mode configured")

        else:
            print("[ERROR] Error: Must specify either --cert-dir (EAP-TLS) or --passphrase (Personal)")
            return None

        # Create Request
        request = Request()
        request.op_code = OpCode.SET_CONFIG
        request.config.CopyFrom(wifi_config)

        print("[OK] Successfully created protobuf message")
        return request

    except ValueError as e:
        print(f"[ERROR] Error creating configuration: {e}")
        raise  # Re-raise ValueError to be caught by caller
    except Exception as e:
        print(f"[ERROR] Error creating configuration: {e}")
        return None

def main():
    parser = argparse.ArgumentParser(
        description='Generate WiFi configuration protobuf message (EAP-TLS or Personal)',
        epilog='''
Examples:
  # Generate EAP-TLS configuration (shows only encoded protobuf)
  %(prog)s "MyWiFi" "AA:BB:CC:DD:EE:FF" -d /path/to/certs

  # Generate EAP-TLS with JSON output
  %(prog)s "MyWiFi" "AA:BB:CC:DD:EE:FF" -d /path/to/certs -j

  # Generate EAP-TLS and save as JSON file
  %(prog)s "MyWiFi" "AA:BB:CC:DD:EE:FF" -d /path/to/certs -j -o config.json

  # Generate EAP-TLS and save as binary protobuf
  %(prog)s "MyWiFi" "AA:BB:CC:DD:EE:FF" -d /path/to/certs -o config.bin

  # Generate Personal mode (WPA2-PSK) configuration
  %(prog)s "MyWiFi" "AA:BB:CC:DD:EE:FF" -w "mypassword" -a 3

  # Generate WPA3-SAE configuration
  %(prog)s "MyWiFi" "AA:BB:CC:DD:EE:FF" -w "mypassword" -a 10

  # Generate EAP-TLS with custom private key passwords
  %(prog)s "MyWiFi" "AA:BB:CC:DD:EE:FF" -d /path/to/certs -k "keypass" -k2 "keypass2"
        ''',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False
    )
    parser.add_argument('ssid', help='WiFi SSID')
    parser.add_argument('bssid', help='WiFi BSSID (MAC address)')
    parser.add_argument('-d', '--cert-dir', help='Directory containing certificate files (for EAP-TLS)')
    parser.add_argument('-w', '--passphrase', help='WiFi passphrase (for Personal mode)')
    parser.add_argument('-a', '--auth-mode', type=int, default=5,
                       help='Auth mode: 0=OPEN, 1=WEP, 2=WPA_PSK, 3=WPA2_PSK, 4=WPA_WPA2_PSK, 5=WPA2_ENTERPRISE, 6=WPA3_PSK, 7=NONE, 8=PSK, 9=PSK_SHA256, 10=SAE, 11=WAPI, 12=EAP, 13=WPA_AUTO_PERSONAL, 14=DPP, 15=EAP_PEAP_MSCHAPV2, 16=EAP_PEAP_GTC, 17=EAP_TTLS_MSCHAPV2, 18=EAP_PEAP_TLS, 19=FT_PSK, 20=FT_SAE, 21=FT_EAP, 22=FT_EAP_SHA384, 23=SAE_EXT_KEY (default: 5=WPA2_ENTERPRISE)')
    parser.add_argument('-c', '--channel', type=int, default=0, help='WiFi channel (default: 0)')
    parser.add_argument('-b', '--band', type=int, default=0, choices=[0, 1, 2],
                       help='WiFi band: 0=AUTO, 1=2.4GHz, 2=5GHz (default: 0)')

    # Private key password arguments
    parser.add_argument('-k', '--private-key-passwd', help='Primary private key password')
    parser.add_argument('-k2', '--private-key-passwd2', help='Secondary private key password')

    # EAP identity and password
    parser.add_argument('-i', '--identity', default='user@example.com', help='EAP identity')
    parser.add_argument('-p', '--password', default='user_password', help='EAP password')

    parser.add_argument('-o', '--output', help='Output file (optional)')
    parser.add_argument('-j', '--json', action='store_true', help='Save as JSON file instead of binary protobuf')

    args = parser.parse_args()

    # Validate inputs
    if args.cert_dir is not None and not os.path.isdir(args.cert_dir):
        print(f"Error: Certificate directory does not exist: {args.cert_dir}")
        sys.exit(1)

    if args.cert_dir is None and args.passphrase is None:
        print("Error: Must specify either --cert-dir (EAP-TLS) or --passphrase (Personal)")
        sys.exit(1)

    # Validate auth mode for EAP-TLS
    if args.cert_dir is not None:
        if args.auth_mode not in [5, 12, 15, 16, 17, 18]:  # Valid EAP modes
            print(f"Error: Auth mode {args.auth_mode} is not valid for EAP-TLS")
            print("Valid EAP auth modes: 5(WPA2_ENTERPRISE), 12(EAP), 15(EAP_PEAP_MSCHAPV2),")
            print("16(EAP_PEAP_GTC), 17(EAP_TTLS_MSCHAPV2), 18(EAP_PEAP_TLS)")
            sys.exit(1)

    # Generate configuration
    request = generate_wifi_config(
        args.ssid, args.bssid, args.channel, args.band, args.auth_mode,
        args.cert_dir, args.passphrase, args.identity, args.password,
        args.private_key_passwd, args.private_key_passwd2
    )

    if request is None:
        sys.exit(1)

        # Output results
    serialized = request.SerializeToString()

    # Show JSON only if requested
    if args.json:
        import json

        from google.protobuf.json_format import MessageToDict

        json_data = MessageToDict(request, preserving_proto_field_name=True)
        print("[JSON] JSON Configuration:")
        print(json.dumps(json_data, indent=2))
        print()

    # Always show encoded protobuf string
    print("[PROTO] Encoded Protobuf (Base64):")
    print(base64.b64encode(serialized).decode('utf-8'))
    print()

    # Save files if output specified
    if args.output:
        if args.json:
            # Save as JSON
            import json

            from google.protobuf.json_format import MessageToDict
            json_data = MessageToDict(request, preserving_proto_field_name=True)
            with open(args.output, 'w') as f:
                json.dump(json_data, f, indent=2)
            print(f"[OK] JSON configuration written to: {args.output}")
        else:
            # Save as binary protobuf
            with open(args.output, 'wb') as f:
                f.write(serialized)
            print(f"[OK] Binary protobuf written to: {args.output}")

    print(f"[INFO] Protobuf size: {len(serialized)} bytes")

        # Get auth mode name based on proto AuthMode enum (backward compatible)
    auth_names = {
        # Original values (backward compatible)
        0: "OPEN",
        1: "WEP",
        2: "WPA_PSK",
        3: "WPA2_PSK",
        4: "WPA_WPA2_PSK",
        5: "WPA2_ENTERPRISE",
        6: "WPA3_PSK",
        # New Zephyr WiFi security types
        7: "NONE",
        8: "PSK",
        9: "PSK_SHA256",
        10: "SAE",
        11: "WAPI",
        12: "EAP",
        13: "WPA_AUTO_PERSONAL",
        14: "DPP",
        15: "EAP_PEAP_MSCHAPV2",
        16: "EAP_PEAP_GTC",
        17: "EAP_TTLS_MSCHAPV2",
        18: "EAP_PEAP_TLS",
        19: "FT_PSK",
        20: "FT_SAE",
        21: "FT_EAP",
        22: "FT_EAP_SHA384",
        23: "SAE_EXT_KEY"
    }
    auth_name = auth_names.get(args.auth_mode, f"Unknown({args.auth_mode})")

    print("\nConfiguration details:")
    print(f"  SSID: {args.ssid}")
    print(f"  BSSID: {args.bssid}")
    print(f"  Channel: {args.channel}")
    print(f"  Band: {'2.4GHz' if args.band == 1 else '5GHz'}")
    print(f"  Auth: {auth_name}")
    if args.cert_dir:
        print("  Mode: EAP-TLS")
        print(f"  Identity: {args.identity}")
    elif args.passphrase:
        print("  Mode: Personal")
        print(f"  Passphrase: {'*' * len(args.passphrase)}")

if __name__ == "__main__":
    main()
