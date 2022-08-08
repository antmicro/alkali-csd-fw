ARG IMAGE_BASE

FROM ${IMAGE_BASE}
ARG IMAGE_BASE
ARG REPO_ROOT

RUN echo "Using ${IMAGE_BASE} docker image as a base..."
RUN echo "Repository root set to be ${REPO_ROOT}..."

# Install system dependencies
RUN apt update -y && apt install -y \
  bc \
  bison \
  build-essential \
  cpio \
  curl \
  default-jdk \
  flex \
  g++-aarch64-linux-gnu \
  gcc-aarch64-linux-gnu \
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
  python3-sphinx \
  rsync \
  rustc \
  unzip \
  wget

# Install CMake
RUN git clone -b v3.16.7 https://gitlab.kitware.com/cmake/cmake.git cmake && \
  cd cmake && \
  ./bootstrap --system-curl && \
  make -j$(nproc) && \
  make install

# Install Python dependencies
COPY requirements.txt requirements.txt
RUN pip3 install -r requirements.txt
RUN rm requirements.txt

# Install Zephyr dependencies
RUN wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.10.3/zephyr-sdk-0.10.3-setup.run && \
  chmod +x zephyr-sdk-0.10.3-setup.run
RUN ./zephyr-sdk-0.10.3-setup.run -- -d /zephyr-sdk-0.10.3
RUN echo "export ZEPHYR_TOOLCHAIN_VARIANT=zephyr" >> /.zephyrrc
RUN echo "export ZEPHYR_SDK_INSTALL_DIR=/zephyr-sdk-0.10.3" >> /.zephyrrc
