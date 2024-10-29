ARG OSMOBTS_VERSION="v1"

FROM osmo-base:${OSMOBTS_VERSION}

RUN git clone https://gitea.osmocom.org/cellular-infrastructure/osmo-trx.git && \
  cd osmo-trx && \
  autoreconf -i && \
  ./configure --with-uhd && \
  make -j $(nproc) && \
  make install && \
  ldconfig -i && \
  cd ..

