FROM ubuntu:24.04

ARG VERSION
ENV HOME=/opt
RUN apt-get update && apt-get install -y wget ca-certificates qemu-system-arm git \
    libxcb-render-util0 libxcb-shape0 libxcb-icccm4  libxcb-keysyms1 libxcb-image0 libxkbcommon-x11-0 libsm6 libice6 udev \
    && rm -rf /var/lib/apt/lists/*
RUN wget https://files.nordicsemi.com/artifactory/swtools/external/nrfutil/executables/x86_64-unknown-linux-gnu/nrfutil -q -O /usr/local/bin/nrfutil  \
    && chmod +x /usr/local/bin/nrfutil \
    && nrfutil install toolchain-manager=0.15.0
RUN nrfutil toolchain-manager install --toolchain-bundle-id $VERSION \
    && nrfutil toolchain-manager env --as-script > /opt/toolchain-env.sh \
    && rm -rf /opt/ncs/downloads

# Prepare JLink to be installed when user accepts license agreement
COPY --chmod=0555 jlink/ /jlink
RUN wget --post-data="accept_license_agreement=accepted&submit=Download+software" -O /jlink/JLink_Linux.deb \
    https://www.segger.com/downloads/jlink/JLink_Linux_V876_x86_64.deb

# Add interactive-setup.sh which is called at the beginning of bash session, only if session is interactive
# interactive-setup.sh is saved as bashrc file in root's home folder (/opt)
COPY --chmod=0555 interactive-setup.sh /opt/.bashrc

# Add non-interactive-setup.sh which is called at the beginning of bash session, even is session is non-interactive
# It is called at the beginning of /etc/bash.bashrc, before /etc/bash.bashrc exits due to non-interactive session
COPY non-interactive-setup.sh /opt/non-interactive-setup.sh
ENV BASH_ENV=/opt/non-interactive-setup.sh

# Link to repo (https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-container-registry#labelling-container-images)
LABEL org.opencontainers.image.source=https://github.com/nrfconnect/sdk-nrf
# Set metadata for an image
LABEL org.opencontainers.image.description="This image contains toolchain for sdk-nrf."

ENTRYPOINT ["/bin/bash", "-c"]
SHELL ["/bin/bash", "-c"]
CMD ["/bin/bash"]
