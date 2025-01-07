# Use the official Ubuntu 20.04 as a base image
FROM ubuntu:20.04

# Update package lists and install necessary tools
RUN apt-get update && \
    apt-get install -y \
    g++ \
    cmake \
    python3 \
    wget \
    neovim \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container
WORKDIR /home/sos-sdp

# Copy the entire src folder into the container
COPY sos-sdp /home/sos-sdp/sos-sdp

RUN mkdir -p /home/sos-sdp/mosektoolslinux64x86/

# Download mosek tools
# RUN cd /home/sos-sdp/mosektoolslinux64x86 && wget https://download.mosek.com/stable/10.1.21/mosektoolslinux64x86.tar.bz2 
COPY mosektoolslinux64x86.tar.bz2 /home/sos-sdp/mosektoolslinux64x86/

# Unpack the mosek tools
RUN cd /home/sos-sdp/mosektoolslinux64x86 && tar -xvf mosektoolslinux64x86.tar.bz2

# Copy the mosek
# COPY mosektoolslinux64x86 /home/sos-sdp/mosektoolslinux64x86

# Create the /home/sos-sdp/mosek directory
RUN mkdir -p /root/mosek/

# Copy the license file
COPY mosek.lic /root/mosek/mosek.lic

# Set the MOSEK_LIB environment variable
ENV MOSEK_LIB /home/sos-sdp/mosektoolslinux64x86/mosek/10.1/tools/platform/linux64x86

RUN cd /home/sos-sdp/mosektoolslinux64x86/mosek/10.1/tools/platform/linux64x86/src/fusion_cxx && make

RUN cp /home/sos-sdp/mosektoolslinux64x86/mosek/10.1/tools/platform/linux64x86/src/fusion_cxx/libfusion64.so.10.1 /home/sos-sdp/mosektoolslinux64x86/mosek/10.1/tools/platform/linux64x86/bin/

RUN ln -s /home/sos-sdp/mosektoolslinux64x86/mosek/10.1/tools/platform/linux64x86/bin/libfusion64.so.10.1 /home/sos-sdp/mosektoolslinux64x86/mosek/10.1/tools/platform/linux64x86/bin/libfusion64.so

RUN cd /home/sos-sdp/sos-sdp && mkdir build && cd build && cmake .. && make

# Set the working directory inside the container
WORKDIR /app
ENTRYPOINT ["/home/sos-sdp/sos-sdp/build/sos-sdp"]

# in order to build: docker build --platform linux/amd64 -t sos-sdp .