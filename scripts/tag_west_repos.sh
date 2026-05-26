#!/bin/bash

# ----------------------------------------------------------------------
# Helper script for creating and pushing release tags in west.yml
# repositories.
#
# IMPORTANT:
#
#     ONLY USE THIS IF YOU ARE AUTHORIZED TO MAKE SIGNED RELEASE TAGS.
#     Contact the NCS release team if you need more information.
#
# To use:
#
#     1. Fill in the release tag data below.
#
#     2. Go to your nrf repository clone, and update your workspace
#        to match the manifest you want to tag:
#
#          cd ~/ncs/nrf
#          git checkout $SDK_NRF_REVISION_YOU_WANT
#          west update
#
#     3. Create the configured tags:
#
#          ./scripts/tag_west_repos.sh tag-all
#
#     4. Push those tags to GitHub:
#
#          ./scripts/tag_west_repos.sh push-all
#
# If you make a mistake and want to delete your local tags before
# pushing them and starting over, run 'tag_west_repos.sh remove-all'.

# ----------------------------------------------------------------------
# Configure the tags by editing this section.

declare -A UPSTREAM_REMOTES

# Set the release tag once for all the projects below.
#
#     - RELEASE_TAG_BASE should include the v prefix, e.g. v3.2.0
#     - RELEASE_TAG_SUFFIX should be empty or something like -rc1
#
RELEASE_TAG_BASE=""
RELEASE_TAG_SUFFIX=""

CORE_REPOSITORIES=(
    nrfxlib
    find-my
    matter
    nrf-802154
)

OSS_REPOSITORIES=(
    zephyr
    mcuboot
    trusted-firmware-m
    mbedtls
    oberon-psa-crypto
)

# Upstream OSS remotes
UPSTREAM_REMOTES[zephyr]="https://github.com/zephyrproject-rtos/zephyr"
UPSTREAM_REMOTES[mcuboot]="https://github.com/mcu-tools/mcuboot"
UPSTREAM_REMOTES[trusted-firmware-m]="https://github.com/TrustedFirmware-M/trusted-firmware-m"
UPSTREAM_REMOTES[mbedtls]="https://github.com/Mbed-TLS/mbedtls"
UPSTREAM_REMOTES[oberon-psa-crypto]="" # Upstream skipped because it's private


# ----------------------------------------------------------------------
# Everything below this line is implementation details.

SCRIPT=$(basename "$0")

hline() {
    # Helper function for printing a horizontal line
    printf '%*s\n' "$(tput cols)" '' | tr ' ' -
}

has_remote() {
    project="$1"

    if [ -z "${UPSTREAM_REMOTES[$project]}" ]; then
        return 1
    else
        return 0
    fi
}

release_tag_names() {
    if [ -z "$RELEASE_TAG_BASE" ]; then
        echo "RELEASE_TAG_BASE is empty" 1>&2
        return 1
    fi

    core_tag="${RELEASE_TAG_BASE}${RELEASE_TAG_SUFFIX}"
    oss_tag="ncs-${RELEASE_TAG_BASE}${RELEASE_TAG_SUFFIX}"
}

tag() {
    # Synopsis:
    #
    #    tag <project-name> <tag-name>
    #
    # Create a tag named <tag-name> in the west.yml project named
    # <project-name>, at the current manifest-rev.
    #
    # The tag message is '<repository-path> <tag-name>', where
    # <repository-path> is something like 'sdk-foo' for project 'foo'.

    project="$1"
    tagname="$2"

    if [ -z "$project" ] || [ -z "$tagname" ]; then
	    echo "empty project or tagname" 1>&2
	    return
    fi

    hline

    sha=$(west list -f '{sha}' "$project")
    remote_basename=$(west list -f '{url}' "$project" | xargs basename)
    local_path=$(west list -f '{abspath}' "$project")
    message="$remote_basename $tagname"

    if has_remote "$project"; then
        describe=$(git -C "$local_path" describe --exclude 'ncs-*')
        message="$message"$'\n'"$project $describe"
    fi

    echo "$project": creating tag "$tagname" for "$sha" in "$local_path"
    git -C "$local_path" tag -s -a -m "$message" "$tagname" "$sha" || exit 1

    echo "$project": verifying tag
    git -C "$local_path" tag -v "$tagname" || exit 1
}

push_tag() {
    # Synopsis:
    #
    #   push_tag <project-name> <tag-name>
    #
    # Push the tag named <tag-name> into the remote URL
    # for the west.yml project named <project-name>.

    project="$1"
    tagname="$2"

    if [ -z "$project" ] || [ -z "$tagname" ]; then
	    echo "empty project or tagname" 1>&2
	    return
    fi

    hline

    local_path=$(west list -f '{abspath}' "$project")
    url=$(west list -f '{url}' "$project")

    echo "$project": pushing tag "$tagname" to "$url"
    git -C "$local_path" push "$url" "$tagname"
    echo "$project": pushed tag "$url/releases/tag/$tagname"
}

remove_tag() {
    # Synopsis:
    #
    #   remove_tag <project-name> <tag-name>
    #
    # Locally delete the tag named <tag-name> in the
    # west.yml project named <project-name>

    project="$1"
    tagname="$2"

    if [ -z "$project" ] || [ -z "$tagname" ]; then
	    echo "empty project or tagname" 1>&2
	    return
    fi

    hline

    local_path=$(west list -f '{abspath}' "$project")

    echo "$project": removing tag "$tagname" in "$local_path"

    git -C "$local_path" tag -d "$tagname"
}

fetch_oss() {
    # Fetches all OSS repositories remotes

    hline
    echo Fetching OSS repositories remotes

    for project in "${OSS_REPOSITORIES[@]}"; do
        if ! has_remote "$project"; then
            echo "Remote not set for $project"
            continue
        fi

        local_path=$(west list -f '{abspath}' "$project")
        echo "$project": Fetching remote  in "$local_path"
        git -C "$local_path" fetch --tags "${UPSTREAM_REMOTES[$project]}" || exit 1
    done
}

tag_all() {
    # Creates all configured release tags.

    release_tag_names || exit 1

    fetch_oss

    hline
    echo Tagging all repositories

    for project in "${CORE_REPOSITORIES[@]}"; do
        tag "$project" "$core_tag"
    done

    for project in "${OSS_REPOSITORIES[@]}"; do
        tag "$project" "$oss_tag"
    done
}

push_all() {
    # Pushes all configured release tags to the main
    # nrfconnect repositories on GitHub.

    release_tag_names || exit 1

    for project in "${CORE_REPOSITORIES[@]}"; do
        push_tag "$project" "$core_tag"
    done

    for project in "${OSS_REPOSITORIES[@]}"; do
        push_tag "$project" "$oss_tag"
    done
}

remove_all() {
    # Removes all configured release tags.

    release_tag_names || exit 1

    for project in "${CORE_REPOSITORIES[@]}"; do
        remove_tag "$project" "$core_tag"
    done

    for project in "${OSS_REPOSITORIES[@]}"; do
        remove_tag "$project" "$oss_tag"
    done
}

command="$1"
shift

case "$command" in
    tag-all)
	# Create tags in all the repositories in the local working trees.
	# Verify the GPG signatures on each tag. Failed verification is a fatal error.
	tag_all
	;;
    remove-all)
	# Remove any tags created by 'tag-all' from local repositories (for debugging)
	remove_all
	;;
    push-all)
	# Push tags created by 'tag-all' to the remote.
	push_all
	;;
    *)
	echo unknown or missing command
	echo example: \'"$SCRIPT" tag-all\'
	echo all commands: tag-all remove-all push-all
	;;
esac
