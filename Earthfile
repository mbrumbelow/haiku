# Earthfile for https://earthly.dev
VERSION 0.8

LOCALLY
ARG --global BRANCH=$(git branch --show-current)
ARG --global REV=$(git describe --dirty --tags --match=hrev* --abbrev=1 | tr '-' '_')
ARG --global REVCOUNT=$(git describe --dirty --tags --match=hrev* | cut -d'-' -f2)

FROM ghcr.io/haiku/toolchain-worker-$BRANCH:latest

WORKDIR /code

code:
  COPY . .

# Generates an archive of system-packages for a chroot or haikuporter buildmaster
#
# usage:
#   gh auth token | docker login ghcr.io --username (username) --password-stdin
#   example: earthly +system-packages --ARCH=x86_gcc2h
#
system-packages:
  FROM +code
  ARG ARCH=x86_64
  RUN mkdir -p /generated; \
      mkdir -p /output/${BRANCH}_${REVCOUNT}; \
      PRI_TRIPLET=$(/code/build/scripts/find_triplet $ARCH); \
      TOOLCHAIN_PRIMARY="--cross-tools-prefix /toolchains/cross-tools-$ARCH/bin/${PRI_TRIPLET}-"; \
      TOOLCHAIN_SECONDARY=""; \
      case "$ARCH" in \
          "x86_gcc2h") \
              PRI_TRIPLET=$(/code/build/scripts/find_triplet x86_gcc2); \
              SEC_TRIPLET=$(/code/build/scripts/find_triplet x86); \
              TOOLCHAIN_PRIMARY="--cross-tools-prefix /toolchains/cross-tools-x86_gcc2/bin/${PRI_TRIPLET}-"; \
              TOOLCHAIN_SECONDARY="--cross-tools-prefix /toolchains/cross-tools-x86/bin/${SEC_TRIPLET}-"; \
              ;; \
          "x86_64h") \
              PRI_TRIPLET=$(/code/build/scripts/find_triplet x86_64); \
              SEC_TRIPLET=$(/code/build/scripts/find_triplet x86); \
              TOOLCHAIN_PRIMARY="--cross-tools-prefix /toolchains/cross-tools-x86_64/bin/${PRI_TRIPLET}-"; \
              TOOLCHAIN_SECONDARY="--cross-tools-prefix /toolchains/cross-tools-x86/bin/${SEC_TRIPLET}-"; \
              ;; \
      esac; \
      cd /generated; \
      echo "/code/configure --distro-compatibility official ${TOOLCHAIN_PRIMARY} ${TOOLCHAIN_SECONDARY}"; \
      /code/configure --distro-compatibility official ${TOOLCHAIN_PRIMARY} ${TOOLCHAIN_SECONDARY}; \
      jam -q -j$(nproc) @nightly-raw; \
      cd /output/${BRANCH}_${REVCOUNT}; \
      for PACKAGE in /generated/objects/haiku/*/packaging/packages/*.hpkg; do \
          cp "$PACKAGE" $(/generated/objects/linux/$(uname -m)/release/tools/package/package info -f "%fileName%" "$PACKAGE"); \
      done; \
      cp /generated/download/*.hpkg . ; \
      echo "Packaging /output/haiku-${BRANCH}_${REVCOUNT}-${ARCH}.tar.gz"; \
      tar -C /output -cvzf /output/haiku-${BRANCH}_${REVCOUNT}-${ARCH}.tar.gz ${BRANCH}_${REVCOUNT}
  SAVE ARTIFACT /output/haiku-${BRANCH}_${REVCOUNT}-${ARCH}.tar.gz AS LOCAL haiku-${BRANCH}_${REVCOUNT}-${ARCH}.tar.gz
