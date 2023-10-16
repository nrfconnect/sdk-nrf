FROM ubuntu:22.04 as nrfutil-builder

ARG VERSION
RUN apt-get update && apt-get install -y \
    wget \
    && rm -rf /var/lib/apt/lists/*
RUN wget https://developer.nordicsemi.com/.pc-tools/nrfutil/x64-linux/nrfutil && chmod +x nrfutil \
    && ./nrfutil install toolchain-manager && ./nrfutil toolchain-manager install --toolchain-bundle-id $VERSION
RUN  ./nrfutil toolchain-manager env --as-script > /root/toolchain-env.sh


FROM ubuntu:22.04

# Link to repo (https://docs.github.com/en/packages/working-with-a-github-packages-registry/working-with-the-container-registry#labelling-container-images)
LABEL org.opencontainers.image.source=https://github.com/nrfconnect/sdk-nrf
# Set metadata for an image
LABEL org.opencontainers.image.description="This image contains toolchain for sdk-nrf."

RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy toolchain
COPY --from=nrfutil-builder /root/ncs/toolchains /root/ncs/toolchains
COPY --from=nrfutil-builder /root/toolchain-env.sh /root/toolchain-env.sh

# Add entrypoint which calls toolchain-env.sh
ADD entrypoint.sh /root/entrypoint.sh
ENTRYPOINT ["/root/entrypoint.sh"]
CMD ["bin/bash"]
