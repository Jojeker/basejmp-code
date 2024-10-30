ARG OSMOBTS_VERSION="v1"

FROM osmo-ggsn:${OSMOBTS_VERSION}

RUN git clone https://gitea.osmocom.org/cellular-infrastructure/osmo-hlr.git && \
  cd osmo-hlr && \
  autoreconf -i && \
  ./configure && \
  make -j $(nproc) && \
  make install && \
  ldconfig && \
  cd ..

RUN git clone https://gitea.osmocom.org/cellular-infrastructure/osmo-sgsn.git && \
  cd osmo-sgsn && \
  autoreconf -i && \
  ./configure && \
  make -j $(nproc) && \
  make install && \
  ldconfig && \
  cd ..

