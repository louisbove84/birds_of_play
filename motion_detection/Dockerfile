# Build stage
FROM ubuntu:22.04 AS builder

# Set non-interactive frontend
ENV DEBIAN_FRONTEND=noninteractive

# Set timezone
ENV TZ=UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libopencv-dev \
    libgtest-dev \
    libyaml-cpp-dev \
    git \
    wget \
    pkg-config \
    libssl-dev \
    libsasl2-dev \
    uuid-dev \
    && rm -rf /var/lib/apt/lists/*

# Build and install MongoDB C driver
RUN cd /tmp && \
    wget https://github.com/mongodb/mongo-c-driver/releases/download/1.24.3/mongo-c-driver-1.24.3.tar.gz && \
    tar xzf mongo-c-driver-1.24.3.tar.gz && \
    cd mongo-c-driver-1.24.3 && \
    mkdir cmake-build && \
    cd cmake-build && \
    cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_INSTALL_PREFIX=/usr/local .. && \
    make -j$(nproc) && \
    make install

# Build and install MongoDB C++ driver
RUN cd /tmp && \
    wget https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.8.0/mongo-cxx-driver-r3.8.0.tar.gz && \
    tar xzf mongo-cxx-driver-r3.8.0.tar.gz && \
    cd mongo-cxx-driver-r3.8.0/build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# Set working directory
WORKDIR /app

# Copy source files
COPY . .

# Create build directory and build
RUN mkdir build && cd build && \
    cmake -DCMAKE_PREFIX_PATH=/usr/local/lib/cmake .. && \
    make

# Runtime stage
FROM ubuntu:22.04

# Set non-interactive frontend
ENV DEBIAN_FRONTEND=noninteractive

# Set timezone
ENV TZ=UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libopencv-dev \
    git \
    libssl-dev \
    libsasl2-dev \
    uuid-runtime \
    && rm -rf /var/lib/apt/lists/*

# Copy MongoDB runtime libraries
COPY --from=builder /usr/local/lib/libmongoc* /usr/local/lib/
COPY --from=builder /usr/local/lib/libbson* /usr/local/lib/
COPY --from=builder /usr/local/lib/libmongocxx* /usr/local/lib/
COPY --from=builder /usr/local/lib/libbsoncxx* /usr/local/lib/

# Update library cache
RUN ldconfig

# Create non-root user
RUN useradd -m -s /bin/bash appuser

# Set working directory
WORKDIR /app

# Copy built files from builder stage
COPY --from=builder /app/build/BirdsOfPlay .
COPY --from=builder /app/build/motion_tracker_test .
COPY --from=builder /app/build/config.yaml .

# Set permissions
RUN chown -R appuser:appuser /app

# Switch to non-root user
USER appuser

# Set the entrypoint
ENTRYPOINT ["./BirdsOfPlay"] 