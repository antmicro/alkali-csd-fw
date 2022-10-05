# Copyright 2021-2022 Western Digital Corporation or its affiliates
# Copyright 2021-2022 Antmicro
#
# SPDX-License-Identifier: Apache-2.0

FROM debian:bullseye
ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies
RUN apt update -y && apt install -y \
  bc \
  bison \
  build-essential \
  cpio \
  curl \
  flex \
  git \
  gperf \
  libcurl4-openssl-dev \
  libelf-dev \
  libffi-dev \
  libjpeg-dev \
  libpcre3-dev \
  libssl-dev \
  make \
  ninja-build \
  python3 \
  python3-pip \
  rsync \
  rustc \
  unzip \
  wget \
  u-boot-tools \
  gcc

# Install CMake
RUN git clone -b v3.16.7 https://gitlab.kitware.com/cmake/cmake.git cmake && \
  cd cmake && \
  ./bootstrap --system-curl && \
  make -j$(nproc) && \
  make install

# Install Python dependencies
COPY requirements.txt requirements.txt
COPY registers-generator/requirements.txt registers-generator/requirements.txt
RUN pip3 install -r requirements.txt
RUN rm requirements.txt registers-generator/requirements.txt

# Install Zephyr
RUN wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.10.3/zephyr-sdk-0.10.3-setup.run && \
  chmod +x zephyr-sdk-0.10.3-setup.run
RUN ./zephyr-sdk-0.10.3-setup.run -- -d /zephyr-sdk-0.10.3
RUN apt install --no-install-recommends -y git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
  make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1
ENV ZEPHYR_TOOLCHAIN_VARIANT=zephyr
ENV ZEPHYR_SDK_INSTALL_DIR=/zephyr-sdk-0.10.3
RUN wget https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/64dbc3e610d79db74f3ff1395fc9b1bf891f73c2/scripts/requirements.txt
RUN pip3 install -r requirements.txt
RUN rm requirements.txt

# Install Chisel
RUN apt install -y default-jdk
RUN wget www.scala-lang.org/files/archive/scala-2.13.0.deb
RUN echo "deb https://repo.scala-sbt.org/scalasbt/debian all main" | tee /etc/apt/sources.list.d/sbt.list
RUN echo "deb https://repo.scala-sbt.org/scalasbt/debian /" | tee /etc/apt/sources.list.d/sbt_old.list
RUN curl -sL "https://keyserver.ubuntu.com/pks/lookup?op=get&search=0x2EE0EA64E40A89B84B2DF73499E82A75642AC823" | apt-key add
RUN apt update
RUN dpkg -i scala*.deb
RUN apt install -y sbt=1.4.9

# Install make 4.3
RUN wget https://ftp.gnu.org/gnu/make/make-4.3.tar.gz
RUN tar xf make-4.3.tar.gz && cd make-4.3 && ./configure && make -j$(nproc)
RUN cp make-4.3/make /opt/.
RUN rm -rf make-4.3*
ENV PATH="/opt:${PATH}"
