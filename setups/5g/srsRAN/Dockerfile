FROM ubuntu:jammy

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y  \
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
    libncurses5-dev \
    libtecla1 \
    libtecla-dev \
    pkg-config \
    libyaml-cpp-dev \
    libgtest-dev \
    iproute2 \
    dnsutils \
    libtool \
    nano \
    bc \
    ccache

RUN add-apt-repository -y ppa:ettusresearch/uhd

RUN apt update && \
    DEBIAN_FRONTEND=noninteractive apt install -y libusb-1.0-0 \
                udev \
 	  					  uhd-host \
                libuhd-dev \
                libbladerf-dev \
                libbladerf2 

RUN DEBIAN_FRONTEND=noninteractive apt install -y libsoapysdr0.8 \
    libsoapysdr-dev \
    uhd-soapysdr \
    soapysdr-tools


RUN	git clone https://github.com/srsRAN/srsRAN_Project.git /opt/srsRAN_Project && \
    cd /opt/srsRAN_Project && \ 
    mkdir build && \
    cd build && \
    cmake ../ && \
    make -j $(nproc) && \
    make install 

COPY ./entrypoint.sh /usr/local/bin/entrypoint.sh
COPY ./configs/gnb.yaml /gnb.yaml
RUN  chmod 644 /gnb.yaml
RUN mkdir -p /opt/srsran && chmod 644 /opt/srsran

ENTRYPOINT [ "/bin/bash" ]
#CMD [ "-c", "/gnb.yaml"]
