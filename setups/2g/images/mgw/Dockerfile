ARG OSMOBTS_VERSION="v1"

FROM osmo-base:${OSMOBTS_VERSION}

RUN git clone https://gitea.osmocom.org/cellular-infrastructure/osmo-msc.git && \
  cd osmo-msc && \
  autoreconf -i && \
  ./configure && \
  make -j $(nproc) && \
  make install && \
  ldconfig -i && \
  cd ..

