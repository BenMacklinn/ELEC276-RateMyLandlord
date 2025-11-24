# Multi-stage build for C++ Drogon application
FROM ubuntu:22.04 as builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libsodium-dev \
    libcurl4-openssl-dev \
    libjsoncpp-dev \
    libssl-dev \
    uuid-dev \
    zlib1g-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Install Drogon
RUN git clone https://github.com/drogonframework/drogon.git /tmp/drogon && \
    cd /tmp/drogon && \
    git submodule update --init && \
    mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# Copy source code
WORKDIR /app
COPY CMakeLists.txt ./
COPY src/ ./src/

# Build the application
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libsodium23 \
    libcurl4 \
    libjsoncpp25 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy Drogon libraries from builder
COPY --from=builder /usr/local/lib/libdrogon.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/libtrantor.so* /usr/local/lib/
RUN ldconfig

# Copy the built binary
WORKDIR /app
COPY --from=builder /app/build/rml_backend ./

# Expose port
EXPOSE 8080

# Set environment variables (can be overridden)
ENV PORT=8080

# Run the application
CMD ["./rml_backend"]

