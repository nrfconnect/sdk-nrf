# sdk-nrf-toolchain docker image

## Content

The image is based on ubuntu:22.04 docker image with additional tools:

* git
* nrfutil with following commands:
    * toolchain-manager
    * device
* ncs toolchain-bundle
* JLink (installation requires accepting SEGGER License)

## Building image

To build docker image you have to provide `VERSION` argument containing ID of Linux toolchain bundle you want to
install.
You can use [print_toolchain_checksum.sh](../print_toolchain_checksum.sh) to get ID proper for your sdk-nrf revision.

```shell
TOOLCHAIN_ID=$(../print_toolchain_checksum.sh)
docker build --build-arg VERSION=$TOOLCHAIN_ID .
```

## Accepting JLink License

To install and use JLink software you have to accept [JLink License Agreement](./jlink/license.txt).

```
By accessing and using Software and Materials provided by SEGGER as free download, you acknowledge and agree to the following Terms of Use. If you do not agree to these Terms, do not download or use any Software or Material.

1) You agree that you will not use the Software or Material for any purpose that is unlawful or illegal.
2) You agree to use the Software only in accordance with the license regulations included in the Software.
3) You acknowledge that the Software and Material is provided by SEGGER on "as is" basis without any express or implied warranty of any kind.
4) You confirm that you are not a person, entity or organization designated by the European Community as a terrorist, terror organization or entity pursuant to the applicable European Council Regulations.
5) You confirm that you are not located in a prohibited or embargoed country and confirm that you will not ship, distribute, transfer and/or export our Software or Material to any prohibited or embargoed country as mentioned in any such European Union law or regulation.

Further information with regard to the listed persons, entities and organizations can be obtained from the official EU website. If there is any doubt if you are on this list it is strongly recommended to review such lists or get in touch with SEGGER prior download of any Software or Material.

Copyright (c) SEGGER Microcontroller GmbH
```

If container is run in interactive mode, you will be asked for accepting license on the start of container.

You can also start container with `ACCEPT_JLINK_LICENSE=1` environment variable which and JLink will be installed
without prompting.

During post installation steps, JLink configures `udev` roles. That operation requires running container with
`--privileged` flag to grant container needed permissions.

## Running container

Toolchain environment variables are set only in bash session. Outside bash session, these variables are not set and
tools delivered in toolchain bundle cannot be found on path.
`sdk-nrf-toolchain` image uses `bash` command as entrypoint. If entrypoint is not overwritten, you can start your docker
container in both interactive and non-interactive mode.

### Locally

To run `sdk-nrf-toolchain` container to build and flash hexes, please run following command.

```docker run -ti ghcr.io/nrfconnect/sdk-nrf-toolchain:<TAG> -v /dev:/dev --privileged -e ACCEPT_JLINK_LICENSE=1 bash```

> [!TIP]
> You can also add `-v <PATH-TO-NCS-PROJECT-ON-DOCKER-HOST>:/ncs` argument to `docker run` command to mount NCS project
> to container.
> In such case, you might encounter issues with repositories ownership and west project configuration. To resolve them,
> please:
> * Run `git config --global --add safe.directory '*'` command to mark mounted git repositories as trusted.
> * Run `west update` to import west commands delivered in NCS project (for example `west build`)
> * Add `--pristine` argument to `west build` command - to avoid path mismatch issues.

### In CI

CI systems usually run docker container in detached mode and can override default entrypoint and command.
However, it is still possible to use `sdk-nrf-toolchain` image in pipelines.

#### GitHub Actions

GitHub action starts container with overwritten default entrypoint. That means you have to set `bash` as a default shell
to load all required environment variables.

If container is started with `ACCEPT_JLINK_LICENSE` env variable set to 1, JLink will be installed just before first
bash command is executed.

Example job configuration:

```yaml
jobs:
  build-in-docker:
    runs-on: ubuntu-22.04
    container:
      image: ghcr.io/nrfconnect/sdk-nrf-toolchain:<TAG>  # steps in this job are executed inside sdk-nrf-toolchain container
      env:
        ACCEPT_JLINK_LICENSE: 1 # set if you want to install JLink
    defaults:
      run:
        shell: bash  # It is required to set `bash` as default shell
    steps:
      - name: Run command in NCS toolchain environment
        run: "west --version"  # This command is executed in bash shell `docker exec <container> bash -c west --version`)
        # It will also install JLink if ACCEPT_JLINK_LICENSE is set to 1
```
