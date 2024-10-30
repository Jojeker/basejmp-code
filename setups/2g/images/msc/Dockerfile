ARG OSMOBTS_VERSION="v1"

FROM osmo-stp:${OSMOBTS_VERSION} 

# Depends on both hlr, mgw and msc
RUN git clone https://gitea.osmocom.org/cellular-infrastructure/osmo-hlr.git && \
  cd osmo-hlr && \
  autoreconf -i && \
  ./configure && \
  make -j $(nproc) && \
  make install && \
  ldconfig -i && \
  cd ..

RUN git clone https://gitea.osmocom.org/cellular-infrastructure/osmo-mgw.git && \
  cd osmo-mgw && \
  autoreconf -i && \
  ./configure && \
  make -j $(nproc) && \
  make install && \
  ldconfig -i && \
  cd ..

RUN git clone https://gitea.osmocom.org/cellular-infrastructure/osmo-msc.git && \
  cd osmo-msc && \
  autoreconf -i && \
  ./configure && \
  make -j $(nproc) && \
  make install && \
  ldconfig -i && \
  cd ..

