FROM ubuntu:22.04

ARG VERSION
ENV HOME=/opt
RUN apt-get update && apt-get install -y wget ca-certificates qemu-system-arm libffi7 git \
    && rm -rf /var/lib/apt/lists/*
RUN wget https://developer.nordicsemi.com/.pc-tools/nrfutil/x64-linux/nrfutil -q -O /usr/local/bin/nrfutil  \
    && chmod +x /usr/local/bin/nrfutil \
    && nrfutil install device=2.4.0 \
    && nrfutil install toolchain-manager=0.15.0
RUN nrfutil toolchain-manager install --toolchain-bundle-id $VERSION \
    && nrfutil toolchain-manager env --as-script > /opt/toolchain-env.sh \
    && rm -rf /opt/ncs/downloads

# Link to repo (https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-container-registry#labelling-container-images)
LABEL org.opencontainers.image.source=https://github.com/nrfconnect/sdk-nrf
# Set metadata for an image
LABEL org.opencontainers.image.description="This image contains toolchain for sdk-nrf."

## Add entrypoint which calls toolchain-env.sh
COPY entrypoint.sh /opt/entrypoint.sh
ENTRYPOINT ["/opt/entrypoint.sh"]

RUN cat /opt/entrypoint.sh >> /tmp.sh
RUN cat /etc/bash.bashrc >> /tmp.sh
RUN cat /tmp.sh > /etc/bash.bashrc
ENV BASH_ENV=/etc/bash.bashrc

SHELL ["/bin/bash", "-c"]
CMD ["bin/bash"]
