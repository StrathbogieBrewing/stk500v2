FROM debian:bullseye

ENV DEBIAN_FRONTEND=noninteractive
ENV PICO_SDK_PATH=/pico/pico-sdk

RUN : \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        cmake gcc-arm-none-eabi libnewlib-arm-none-eabi \
        libstdc++-arm-none-eabi-newlib git build-essential python3 \
        automake autoconf build-essential texinfo libtool pkg-config \
        libftdi-dev libusb-1.0-0-dev gdb-multiarch \
    && rm -rf /var/lib/apt/lists/* 

RUN : \   
    && mkdir pico \
    && cd pico \
    && git config --global http.sslverify false \
    && git clone --depth=1 --branch 1.4.0 https://github.com/raspberrypi/pico-sdk.git \
    && cd pico-sdk \
    && git submodule update --init --depth=1 

RUN : \   
    && cd .. \
    && git clone --depth=1 https://github.com/raspberrypi/picoprobe.git \
    && cd picoprobe \
    && git submodule update --init \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make -j4 \
    && cd .. 

RUN : \
    && cd .. \
    && git clone https://github.com/raspberrypi/openocd.git --branch rp2040 --depth=1 --no-single-branch \
    && cd openocd \
    && ./bootstrap \
    && ./configure \
    && make -j4 \
    && cd ..

ENTRYPOINT [ "/bin/bash" ]



