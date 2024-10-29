ARG OSMOBTS_VERSION="v1"

FROM osmo-base:${OSMOBTS_VERSION}

RUN git clone https://gitea.osmocom.org/cellular-infrastructure/osmo-bts.git && \
  cd osmo-bts && \
  autoreconf -i && \
  ./configure --enable-trx && \
  make -j $(nproc) && \
  make install && \
  ldconfig -i && \
  cd ..

