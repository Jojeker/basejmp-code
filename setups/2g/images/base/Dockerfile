FROM ubuntu:jammy

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y  \
    git-core \
    build-essential \ 
    libtool \
    libtalloc-dev \
    shtool \
    autoconf \
    automake \
    pkg-config \
    make \
    gcc \
    libpcsclite-dev \
    libortp-dev \
    libsctp-dev \
    libmnl-dev \
    libdbi-dev \
    libdbd-sqlite3 \
    libsqlite3-dev \
    libc-ares-dev \
    libgnutls28-dev \
    libconfig++-dev \
    bc \
    vim \
    software-properties-common

RUN add-apt-repository -y ppa:ettusresearch/uhd

RUN apt update && \
    DEBIAN_FRONTEND=noninteractive apt install -y libusb-1.0-0 \
    libusb-1.0-0-dev \
    udev \
    uhd-host \
    libuhd-dev

# General Deps - uring, osmocore, abis, netif, libsmpp34

RUN git clone https://github.com/axboe/liburing.git && \
  cd liburing/ && \
  ./configure && \
  make -j $(nproc) && \
  make install && \
  ldconfig -i && \
  cd ..

RUN git clone https://gitea.osmocom.org/osmocom/libosmocore.git && \
  cd libosmocore && \
  autoreconf -i && \
  ./configure && \
  make -j $(nproc) && \
  make install && \
  ldconfig -i && \
  cd ..

RUN apt install dahdi-source -y
RUN git clone https://gitea.osmocom.org/osmocom/libosmo-abis.git && \
  cd libosmo-abis && \
  autoreconf -i && \
  ./configure && \
  make -j $(nproc) && \
  make install && \
  ldconfig -i && \
  cd .. 

RUN git clone https://gitea.osmocom.org/osmocom/libosmo-netif.git && \
  cd libosmo-netif && \
  autoreconf -i && \
  ./configure && \
  make -j $(nproc) && \
  make install && \
  ldconfig -i && \
  cd ..


RUN git clone https://gitea.osmocom.org/cellular-infrastructure/libsmpp34.git && \
  cd libsmpp34 && \
  autoreconf -i && \
  ./configure && \
  make -j $(nproc) && \
  make install && \
  ldconfig -i && \
  cd ..


