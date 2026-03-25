Key Verification Messages
=========================

Each file in this directory is a pre-computed Ed25519 signature that the
manufacturing application uses to verify key material stored in the KMU.

The verification process (Step 4 of the manufacturing flow):
  1. A known message is defined at build time (message content is TBD - see
     open question in README.rst).
  2. The message was signed offline using the PRIVATE key counterpart of each
     public key provisioned to the KMU.
  3. At runtime, psa_verify_message() is called with the KMU-stored public key
     and the signature from this file. A successful verification confirms the
     KMU slot holds the correct key material.

Naming convention:
  <key_name>_signed.msg   — Ed25519 signature (64 bytes, raw binary)

To generate a verification message (replace with your own key and message):
  # Define a known fixed message (must match MFG_KEY_VERIFY_MESSAGE in
  # src/key_provisioning.c):
  echo -n "manufacturing_key_verification_v1" > verify_msg.bin

  # Sign with the private key:
  openssl pkeyutl -sign -inkey urot_privkey_gen0.pem \
                  -rawin -in verify_msg.bin \
                  -out urot_pubkey_gen0_signed.msg

Files:
  urot_pubkey_gen0_signed.msg   — signature for urot_pubkey_gen0
  urot_pubkey_gen1_signed.msg   — signature for urot_pubkey_gen1

IMPORTANT: These are PLACEHOLDER files. Replace them after generating your own
key pairs, by re-signing the known verification message with each private key.
