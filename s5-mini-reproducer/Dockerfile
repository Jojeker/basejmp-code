FROM ubuntu:jammy

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    git \
    build-essential \ 
    cmake \
    libfftw3-dev \
    libmbedtls-dev \
    libboost-program-options-dev \
    libconfig++-dev \
    libsctp-dev \
    libzmq3-dev \
    software-properties-common \
    g++ \
    gcc \
    clang \
    libncurses5-dev \
    libtecla1 \
    libtecla-dev \
    pkg-config \
    libyaml-cpp-dev \
    libgtest-dev \
    iproute2 \
    dnsutils \
    iputils-ping \
    libtool \
    vim \
    wget \
    bc \
    ccache \
    protobuf-compiler \
    libprotobuf-dev \
    libboost-all-dev \
    gnupg \
    dirmngr \
    libgsm1-dev \
    libspeex-dev

RUN apt update && \
    DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends \
    libusb-1.0-0 \
    libusb-1.0-0-dev \
    udev

RUN git clone https://github.com/Nuand/bladeRF.git /bladeRF && \ 
   cd /bladeRF && \
   git reset --hard 3a411c87c2416dc68030d5823d73ebf3f797a145 && \
   cd ./host && \
   mkdir build && \
   cd build && \
   cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DINSTALL_UDEV_RULES=ON ../ && \
   make && \
   make install && \
   ldconfig

RUN mkdir -p /usr/local/share/yate/nipc_web && \
   git clone https://github.com/yatevoip/ansql.git /usr/local/share/yate/nipc_web/ansql

RUN wget https://nuand.com/downloads/yate-rc-2.tar.gz && \
  tar xfz yate-rc-2.tar.gz && \
   cd yate && \
   ./autogen.sh && \
   ./configure && \
   make && \
   make install-noapi && \
   cd ../yatebts && \
   ./autogen.sh && \
   ./configure && \
   make && \
   make install

# Clean up (config is from outside)
RUN mkdir -p /var/log/yate && \
   ldconfig && \
   rm -rf /usr/local/etc/yate

# Make sure that the underflows do not happen
COPY ./configs/security/limits.conf /etc/security/limits.conf

RUN apt install telnet iptables -y

ENTRYPOINT [ "yate", "-v"]
