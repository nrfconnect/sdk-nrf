#!/usr/bin/env python3
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import argparse
import datetime
import os
from cryptography import x509
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.x509.oid import NameOID


def argument_parser():
    parser = argparse.ArgumentParser(
        description="Generate private key, CSR Root CA certificate, Subordinate CA certificate, or sign a certificate",
        allow_abbrev=False
    )
    subparsers = parser.add_subparsers(
        dest="action",
        help="Action to perform"
    )

    client_key_parser = subparsers.add_parser(
        "client_key",
        help="Generate EC private key"
    )
    client_key_parser.add_argument(
        "--client-key",
        default="certs/private-key.pem",
        help="Filename to save the private key"
    )

    root_ca_parser = subparsers.add_parser(
        "root_ca",
        help="Generate Root CA certificate for testing purposes"
    )
    root_ca_parser.add_argument(
        "--root-key",
        default="ca/root-ca-key.pem",
        help="Filename to save the private key"
    )
    root_ca_parser.add_argument(
        "--root-cert",
        default="ca/root-ca-cert.pem",
        help="Filename to save the root CA certificate"
    )
    root_ca_parser.add_argument(
        "--common-name",
        default="Test Root CA",
        help="Common Name for the Root CA certificate"
    )
    sub_ca_parser = subparsers.add_parser(
        "sub_ca",
        help="Generate Subordinate CA certificate for testing purposes"
    )
    sub_ca_parser.add_argument(
        "--sub-key",
        default="ca/sub-ca-key.pem",
        help="Filename to save the private key"
    )
    sub_ca_parser.add_argument(
        "--sub-cert",
        default="ca/sub-ca-cert.pem",
        help="Filename to save the sub CA certificate"
    )
    sub_ca_parser.add_argument(
        "--root-key",
        default="ca/root-ca-key.pem",
        help="Filename to load the root CA private key"
    )
    sub_ca_parser.add_argument(
        "--root-cert",
        default="ca/root-ca-cert.pem",
        help="Filename to load the root CA certificate"
    )

    csr_parser = subparsers.add_parser(
        "csr",
        help="Generate a CSR for a client certificate"
    )
    csr_parser.add_argument(
        "--client-key",
        default="certs/private-key.pem",
        help="Filename to load the client key"
    )
    csr_parser.add_argument(
        "--client-csr",
        default="certs/client-csr.pem",
        help="Filename to save the CSR"
    )
    csr_parser.add_argument(
        "--common-name", required=True,
        help="Common Name for the client certificate"
    )

    sign_cert_parser = subparsers.add_parser(
        "sign",
        help="Sign a certificate using the sub CA certificate"
    )
    sign_cert_parser.add_argument(
        "--client-csr",
        default="certs/client-csr.pem",
        help="Filename to load the client CSR"
    )
    sign_cert_parser.add_argument(
        "--client-cert",
        default="certs/client-cert.pem",
        help="Filename to save the signed certificate"
    )
    sign_cert_parser.add_argument(
        "--sub-key",
        default="ca/sub-ca-key.pem",
        help="Filename to load the sub CA private key"
    )
    sign_cert_parser.add_argument(
        "--sub-cert",
        default="ca/sub-ca-cert.pem",
        help="Filename to load the sub CA certificate"
    )

    sign_root_cert_parser = subparsers.add_parser(
        "sign_root",
        help="Sign using the root CA certificate"
    )
    sign_root_cert_parser.add_argument(
        "--client-csr",
        default="certs/client-csr.pem",
        help="Filename to load the client CSR"
    )
    sign_root_cert_parser.add_argument(
        "--client-cert",
        default="certs/client-cert.pem",
        help="Filename to save the signed certificate"
    )
    sign_root_cert_parser.add_argument(
        "--root-key",
        default="ca/root-ca-key.pem",
        help="Filename to load the root CA private key"
    )
    sign_root_cert_parser.add_argument(
        "--root-cert",
        default="ca/root-ca-cert.pem",
        help="Filename to load the root CA certificate"
    )

    return parser


def generate_ec_private_key():
    return ec.generate_private_key(ec.SECP256R1())


def private_key_to_pem(private_key):
    return private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption()
    )


def ensure_directory(filename):
    directory = os.path.dirname(filename)
    if not os.path.exists(directory):
        os.makedirs(directory)


def save_key_to_file(private_key, filename):
    ensure_directory(filename)
    key_pem = private_key_to_pem(private_key)

    with open(filename, "wb") as f:
        f.write(key_pem)
    print(f"{filename} generated successfully!")


def generate_signed_certificate(subject_name, issuer_key, issuer_cert, public_key, cert_filename):
    cert = (
        x509.CertificateBuilder()
        .subject_name(subject_name)
        .issuer_name(issuer_cert.subject if issuer_cert else subject_name)
        .public_key(public_key)
        .serial_number(x509.random_serial_number())
        .not_valid_before(datetime.datetime.utcnow())
        .not_valid_after(datetime.datetime.utcnow() + datetime.timedelta(days=365))
        .add_extension(x509.BasicConstraints(ca=True, path_length=None), critical=True)
        .sign(issuer_key, hashes.SHA256())
    )

    ensure_directory(cert_filename)
    with open(cert_filename, "wb") as f:
        f.write(cert.public_bytes(serialization.Encoding.PEM))

    print(
        "\033[93mWARNING: "
        "This certificate is for testing purposes only and should not be used in production."
        "\033[0m"
    )
    print(f"Certificate saved as {cert_filename}")


def handle_client_key_action(args):
    private_key = generate_ec_private_key()
    save_key_to_file(private_key, args.client_key)


def handle_root_ca_action(args):
    private_key = generate_ec_private_key()
    save_key_to_file(private_key, args.root_key)
    subject_name = x509.Name([
        x509.NameAttribute(NameOID.COUNTRY_NAME, "US"),
        x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, "Test"),
        x509.NameAttribute(NameOID.LOCALITY_NAME, "Test"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Test Organization"),
        x509.NameAttribute(NameOID.COMMON_NAME, args.common_name),
    ])
    generate_signed_certificate(
        subject_name,
        private_key,
        None,
        private_key.public_key(),
        args.root_cert
    )


def handle_sub_ca_action(args):
    private_key = generate_ec_private_key()
    save_key_to_file(private_key, args.sub_key)

    with open(args.root_key, "rb") as f:
        root_private_key_pem = f.read()
    root_private_key = serialization.load_pem_private_key(root_private_key_pem, password=None)

    with open(args.root_cert, "rb") as f:
        root_ca_cert_pem = f.read()
    root_ca_cert = x509.load_pem_x509_certificate(root_ca_cert_pem)

    subject_name = x509.Name([
        x509.NameAttribute(NameOID.COUNTRY_NAME, "US"),
        x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, "Test"),
        x509.NameAttribute(NameOID.LOCALITY_NAME, "Test"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Test Organization"),
        x509.NameAttribute(NameOID.COMMON_NAME, "Subordinate CA"),
    ])
    generate_signed_certificate(
        subject_name,
        root_private_key,
        root_ca_cert,
        private_key.public_key(),
        args.sub_cert
    )


def read_private_key(filename):
    with open(filename, "rb") as f:
        private_key_pem = f.read()
    return serialization.load_pem_private_key(private_key_pem, password=None)


def read_certificate(filename):
    with open(filename, "rb") as f:
        cert_pem = f.read()
    return x509.load_pem_x509_certificate(cert_pem)


def handle_csr_action(args):
    private_key = read_private_key(args.client_key)

    subject_name = x509.Name([
        x509.NameAttribute(NameOID.COUNTRY_NAME, "US"),
        x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, "Test"),
        x509.NameAttribute(NameOID.LOCALITY_NAME, "Test"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Test Organization"),
        x509.NameAttribute(NameOID.COMMON_NAME, args.common_name),
    ])
    csr = (
        x509.CertificateSigningRequestBuilder()
        .subject_name(subject_name)
        .sign(private_key, hashes.SHA256())
    )

    with open(args.client_csr, "wb") as f:
        f.write(csr.public_bytes(serialization.Encoding.PEM))
    print(f"CSR saved as {args.client_csr}")


def handle_sign_action(args):
    if args.action == "sign":
        sign_cert = read_certificate(args.sub_cert)
        sign_key = read_private_key(args.sub_key)
    elif args.action == "sign_root":
        sign_cert = read_certificate(args.root_cert)
        sign_key = read_private_key(args.root_key)
    else:
        raise ValueError("Invalid action")

    with open(args.client_csr, "rb") as f:
        csr_pem = f.read()
    csr = x509.load_pem_x509_csr(csr_pem)

    key_usage = x509.KeyUsage(
        digital_signature=True,
        content_commitment=False,
        key_encipherment=True,
        data_encipherment=False,
        key_agreement=False,
        key_cert_sign=False,
        crl_sign=False,
        encipher_only=False,
        decipher_only=False
    )

    for attribute in csr.subject:
        if attribute.oid == NameOID.COMMON_NAME:
            print(f"Signing certificate with CN: {attribute.value}")

    signed_cert = (
        x509.CertificateBuilder()
        .subject_name(csr.subject)
        .issuer_name(sign_cert.subject)
        .public_key(csr.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(datetime.datetime.utcnow())
        .not_valid_after(datetime.datetime.utcnow() + datetime.timedelta(days=365))
        .add_extension(x509.BasicConstraints(ca=False, path_length=None), critical=True)
        .add_extension(x509.SubjectKeyIdentifier.from_public_key(csr.public_key()), critical=False)
        .add_extension(x509.AuthorityKeyIdentifier.from_issuer_public_key(sign_cert.public_key()), critical=False)
        .add_extension(x509.ExtendedKeyUsage([x509.oid.ExtendedKeyUsageOID.CLIENT_AUTH]), critical=False)
        .add_extension(key_usage, critical=True)
        .sign(sign_key, sign_cert.signature_hash_algorithm)
    )

    with open(args.client_cert, "wb") as f:
        f.write(signed_cert.public_bytes(serialization.Encoding.PEM))
    print(f"Signed client certificate saved as {args.client_cert}")


def main():
    parser = argument_parser()
    args = parser.parse_args()

    if args.action == "client_key":
        handle_client_key_action(args)
    elif args.action == "root_ca":
        handle_root_ca_action(args)
    elif args.action == "sub_ca":
        handle_sub_ca_action(args)
    elif args.action == "sign":
        handle_sign_action(args)
    elif args.action == "sign_root":
        handle_sign_action(args)
    elif args.action == "csr":
        handle_csr_action(args)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
