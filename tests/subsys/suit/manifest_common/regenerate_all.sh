#!/bin/bash
#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

declare -A envelopes=(
  ["../orchestrator/manifest/sample_unsupported_component_54.yaml"]="
    ../orchestrator/manifest/src/manifest_unsupported_component_54.c"
  ["../orchestrator/manifest/sample_unsupported_component.yaml"]="
    ../orchestrator/manifest/src/manifest_unsupported_component.c"
  ["../orchestrator/manifest/sample_zero_54.yaml"]="
    ../orchestrator/manifest/src/manifest_zero_size_54.c"
  ["../orchestrator/manifest/sample_zero.yaml"]="
    ../orchestrator/manifest/src/manifest_zero_size.c"
  ["../fetch_integrated_payload_flash/manifest/manifest_52.yaml"]="
    ../fetch_integrated_payload_flash/src/manifest_52.c"
  ["../fetch_integrated_payload_flash/manifest/manifest_54.yaml"]="
    ../fetch_integrated_payload_flash/src/manifest_54.c"
  ["../orchestrator/manifest/sample_valid_root_payload_fetch.yaml"]="
    ../orchestrator/manifest/src/manifest_valid_payload_fetch.c"
  ["../orchestrator/manifest/sample_valid_app_payload_fetch.yaml"]="
    ../orchestrator/manifest/src/manifest_valid_payload_fetch_app.c"
  ["../orchestrator/manifest/root_no_validate.yaml"]="
    ../orchestrator/manifest/src/manifest_root_no_validate.c"
  ["../orchestrator/manifest/root_validate_fail.yaml"]="
    ../orchestrator/manifest/src/manifest_root_validate_fail.c"
  ["../orchestrator/manifest/root_validate_load_fail.yaml"]="
    ../orchestrator/manifest/src/manifest_root_validate_load_fail.c"
  ["../orchestrator/manifest/root_validate_load_no_invoke.yaml"]="
    ../orchestrator/manifest/src/manifest_root_validate_load_no_invoke.c"
  ["../orchestrator/manifest/root_validate_load_invoke_fail.yaml"]="
    ../orchestrator/manifest/src/manifest_root_validate_load_invoke_fail.c"
  ["../orchestrator/manifest/root_validate_load_invoke.yaml"]="
    ../orchestrator/manifest/src/manifest_root_validate_load_invoke.c"
  ["../orchestrator/manifest/sample_valid_recovery.yaml"]="
    ../orchestrator/manifest/src/manifest_recovery.c"
  ["../orchestrator/manifest/sample_valid_recovery_54.yaml"]="
    ../orchestrator/manifest/src/manifest_recovery_54.c"
  ["../orchestrator/manifest/recovery_no_validate.yaml"]="
    ../orchestrator/manifest/src/manifest_recovery_no_validate.c"
  ["../orchestrator/manifest/recovery_validate_fail.yaml"]="
    ../orchestrator/manifest/src/manifest_recovery_validate_fail.c"
  ["../orchestrator/manifest/recovery_validate_load_fail.yaml"]="
    ../orchestrator/manifest/src/manifest_recovery_validate_load_fail.c"
  ["../orchestrator/manifest/recovery_validate_load_no_invoke.yaml"]="
    ../orchestrator/manifest/src/manifest_recovery_validate_load_no_invoke.c"
  ["../orchestrator/manifest/recovery_validate_load_invoke_fail.yaml"]="
    ../orchestrator/manifest/src/manifest_recovery_validate_load_invoke_fail.c"
  ["../orchestrator/manifest/recovery_validate_load_invoke.yaml"]="
    ../orchestrator/manifest/src/manifest_recovery_validate_load_invoke.c"
  ["../orchestrator/manifest/root_unsupported_command.yaml"]="
    ../orchestrator/manifest/src/manifest_root_unsupported_command.c"
  ["../orchestrator/manifest/root_candidate_verification_fail.yaml"]="
    ../orchestrator/manifest/src/manifest_root_candidate_verification_fail.c"
  ["../orchestrator/manifest/root_candidate_verification_no_install.yaml"]="
    ../orchestrator/manifest/src/manifest_root_candidate_verification_no_install.c"
  ["../orchestrator/manifest/root_candidate_verification_install_fail.yaml"]="
    ../orchestrator/manifest/src/manifest_root_candidate_verification_install_fail.c"
  ["../orchestrator/manifest/root_candidate_verification_install.yaml"]="
    ../orchestrator/manifest/src/manifest_root_candidate_verification_install.c"
)

declare -A envelopes_v2=(
  ["../orchestrator/manifest/sample_wrong_manifest_version_54.yaml"]="
    ../orchestrator/manifest/src/manifest_wrong_version_54.c"
  ["../orchestrator/manifest/sample_wrong_manifest_version.yaml"]="
    ../orchestrator/manifest/src/manifest_wrong_version.c"
)

declare -A envelopes_wrong_key=(
  ["../orchestrator/manifest/sample_valid_54.yaml"]="
    ../orchestrator/manifest/src/manifest_different_key_54.c"
  ["../orchestrator/manifest/sample_valid.yaml"]="
    ../orchestrator/manifest/src/manifest_different_key.c"
)

declare -A dependency_envelopes=(
  ["../storage/manifest/manifest_app.yaml"]="
    ../storage/src/manifest_app.c"
  ["../storage/manifest/manifest_app_posix.yaml"]="
    ../storage/src/manifest_app_posix.c"
  ["../storage/manifest/manifest_app_posix_v2.yaml"]="
    ../storage/src/manifest_app_posix_v2.c"
  ["../storage/manifest/manifest_rad.yaml"]="
    ../storage/src/manifest_rad.c"
  ["../orchestrator/manifest/sample_valid_54.yaml"]="
    ../orchestrator/manifest/src/manifest_valid_app_54.c"
  ["../orchestrator/manifest/sample_valid.yaml"]="
    ../orchestrator/manifest/src/manifest_valid_app.c"
)
declare -A root_envelopes=(
  ["../storage/manifest/manifest_root.yaml"]="
    ../storage/src/manifest_root.c"
  ["../storage/manifest/manifest_root_posix.yaml"]="
    ../storage/src/manifest_root_posix.c"
  ["../storage/manifest/manifest_root_posix_v2.yaml"]="
    ../storage/src/manifest_root_posix_v2.c"
  ["../orchestrator/manifest/sample_valid_root_54.yaml"]="
    ../orchestrator/manifest/src/manifest_valid_54.c"
  ["../orchestrator/manifest/sample_valid_root.yaml"]="
    ../orchestrator/manifest/src/manifest_valid.c"
)
declare -A envelope_dependency_names=(
  ["../storage/manifest/manifest_app.yaml"]="app.suit"
  ["../storage/manifest/manifest_app_posix.yaml"]="app_posix.suit"
  ["../storage/manifest/manifest_app_posix_v2.yaml"]="app_posix_v2.suit"
  ["../storage/manifest/manifest_rad.yaml"]="rad.suit"
  ["../storage/manifest/manifest_root.yaml"]="root.suit"
  ["../storage/manifest/manifest_root_posix.yaml"]="root_posix.suit"
  ["../storage/manifest/manifest_root_posix_v2.yaml"]="root_posix_v2.suit"
  ["../orchestrator/manifest/sample_valid_54.yaml"]="sample_app.suit"
  ["../orchestrator/manifest/sample_valid.yaml"]="sample_app_posix.suit"
  ["../orchestrator/manifest/sample_valid_root_54.yaml"]="sample_root.suit"
  ["../orchestrator/manifest/sample_valid_root.yaml"]="sample_root_posix.suit"
)


# Regenerate regular envelopes
for envelope_yaml in "${!envelopes[@]}"; do
  envelope_c=${envelopes[$envelope_yaml]}
  echo "### Regenerate: ${envelope_yaml} ###"
  ./regenerate.sh ${envelope_yaml} || exit 1
  python3 replace_manifest.py ${envelope_c} sample_signed.suit || exit 2
done


# Regenerate hierarchical manifests for the SUIT storage test
for envelope_yaml in "${!dependency_envelopes[@]}"; do
  envelope_c=${dependency_envelopes[$envelope_yaml]}
  envelope_name=${envelope_dependency_names[$envelope_yaml]}
  echo "### Regenerate: ${envelope_yaml} ###"
  ./regenerate.sh ${envelope_yaml} || exit 1
  if [ -e ${envelope_c} ]; then
    python3 replace_manifest.py ${envelope_c} sample_signed.suit || exit 2
  fi
  mv sample_signed.suit ${envelope_name}
done
for envelope_yaml in "${!root_envelopes[@]}"; do
  envelope_c=${root_envelopes[$envelope_yaml]}
  envelope_name=${envelope_dependency_names[$envelope_yaml]}
  echo "### Regenerate: ${envelope_yaml} ###"
  ./regenerate.sh ${envelope_yaml} || exit 1
  python3 replace_manifest.py ${envelope_c} sample_signed.suit || exit 2
  mv sample_signed.suit ${envelope_name}
done


# Manually manipulate some of the generated envelopes to match test description
MANIPULATION=" * @details The manipulation is done on byte of index 11, from value 0x58 to 0xFF"
cp "../orchestrator/manifest/src/manifest_valid_54.c" \
  "../orchestrator/manifest/src/manifest_manipulated_54.c"
sed -i 's/0xD8, 0x6B, \(.*\)0x58/0xD8, 0x6B, \10xFF/g' \
  "../orchestrator/manifest/src/manifest_manipulated_54.c"
sed -i 's/Valid SUIT envelope/Manipulated SUIT envelope/g' \
  "../orchestrator/manifest/src/manifest_manipulated_54.c"
sed -i "s/.yaml/.yaml\n *\n$MANIPULATION/g" \
  "../orchestrator/manifest/src/manifest_manipulated_54.c"
sed -i 's/manifest_valid_buf/manifest_manipulated_buf/g' \
  "../orchestrator/manifest/src/manifest_manipulated_54.c"
sed -i 's/manifest_valid_len/manifest_manipulated_len/g' \
  "../orchestrator/manifest/src/manifest_manipulated_54.c"

cp "../orchestrator/manifest/src/manifest_valid.c" \
  "../orchestrator/manifest/src/manifest_manipulated.c"
sed -i 's/0xD8, 0x6B, \(.*\)0x58/0xD8, 0x6B, \10xFF/g' \
  "../orchestrator/manifest/src/manifest_manipulated.c"
sed -i 's/Valid SUIT envelope/Manipulated SUIT envelope/g' \
  "../orchestrator/manifest/src/manifest_manipulated.c"
sed -i "s/.yaml/.yaml\n *\n$MANIPULATION/g" \
  "../orchestrator/manifest/src/manifest_manipulated.c"
sed -i 's/manifest_valid_buf/manifest_manipulated_buf/g' \
  "../orchestrator/manifest/src/manifest_manipulated.c"
sed -i 's/manifest_valid_len/manifest_manipulated_len/g' \
  "../orchestrator/manifest/src/manifest_manipulated.c"

cp "../orchestrator/manifest/src/manifest_recovery.c" \
  "../orchestrator/manifest/src/manifest_recovery_manipulated.c"
sed -i 's/0xD8, 0x6B, \(.*\)0x58/0xD8, 0x6B, \10xFF/g' \
  "../orchestrator/manifest/src/manifest_recovery_manipulated.c"
sed -i 's/Valid SUIT envelope/Manipulated SUIT envelope/g' \
  "../orchestrator/manifest/src/manifest_recovery_manipulated.c"
sed -i "s/.yaml/.yaml\n *\n$MANIPULATION/g" \
  "../orchestrator/manifest/src/manifest_recovery_manipulated.c"
sed -i 's/manifest_valid_recovery_buf/manifest_manipulated_recovery_buf/g' \
  "../orchestrator/manifest/src/manifest_recovery_manipulated.c"
sed -i 's/manifest_valid_recovery_len/manifest_manipulated_recovery_len/g' \
  "../orchestrator/manifest/src/manifest_recovery_manipulated.c"

cp "../orchestrator/manifest/src/manifest_recovery_54.c" \
  "../orchestrator/manifest/src/manifest_recovery_manipulated_54.c"
sed -i 's/0xD8, 0x6B, \(.*\)0x58/0xD8, 0x6B, \10xFF/g' \
  "../orchestrator/manifest/src/manifest_recovery_manipulated_54.c"
sed -i 's/Valid SUIT envelope/Manipulated SUIT envelope/g' \
  "../orchestrator/manifest/src/manifest_recovery_manipulated_54.c"
sed -i "s/.yaml/.yaml\n *\n$MANIPULATION/g" \
  "../orchestrator/manifest/src/manifest_recovery_manipulated_54.c"
sed -i 's/manifest_valid_recovery_buf/manifest_manipulated_recovery_buf/g' \
  "../orchestrator/manifest/src/manifest_recovery_manipulated_54.c"
sed -i 's/manifest_valid_recovery_len/manifest_manipulated_recovery_len/g' \
  "../orchestrator/manifest/src/manifest_recovery_manipulated_54.c"

# Generate envelopes with modified manifest version number
sed -i 's/suit-manifest-version         => 1,/suit-manifest-version         => 2,/g' \
  "../../../../../modules/lib/suit-processor/cddl/manifest.cddl"
for envelope_yaml in "${!envelopes_v2[@]}"; do
  envelope_c=${envelopes_v2[$envelope_yaml]}
  echo "### Regenerate: ${envelope_yaml} ###"
  ./regenerate.sh ${envelope_yaml} || exit 1
  python3 replace_manifest.py ${envelope_c} sample_signed.suit || exit 2
done
sed -i 's/suit-manifest-version         => 2,/suit-manifest-version         => 1,/g' \
  "../../../../../modules/lib/suit-processor/cddl/manifest.cddl"

SUIT_GENERATOR_DIR="../../../../../modules/lib/suit-generator"
KEYS_DIR=${SUIT_GENERATOR_DIR}/ncs

# Generate envelopes and sign them with new, temporary key
suit-generator keys --output-file key
mv ${KEYS_DIR}/key_private.pem ${KEYS_DIR}/key_private.pem.bak
mv key_priv.pem ${KEYS_DIR}/key_private.pem

for envelope_yaml in "${!envelopes_wrong_key[@]}"; do
  envelope_c=${envelopes_wrong_key[$envelope_yaml]}
  echo "### Regenerate: ${envelope_yaml} ###"
  ./regenerate.sh ${envelope_yaml} || exit 1
  python3 replace_manifest.py ${envelope_c} sample_signed.suit || exit 2
done

mv ${KEYS_DIR}/key_private.pem.bak ${KEYS_DIR}/key_private.pem
