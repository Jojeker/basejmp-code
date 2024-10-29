ARG OSMOBTS_VERSION="v1"

FROM osmo-base:${OSMOBTS_VERSION}

RUN git clone https://gitea.osmocom.org/cellular-infrastructure/osmo-bsc.git && \
  cd osmo-bsc && \
  autoreconf -i && \
  ./configure && \
  make -j $(nproc) && \
  make install && \
  ldconfig -i && \
  cd ..

