FROM node:20

# Install FFmpeg and clean up to reduce image size
RUN apt-get update && \
    apt-get install -y ffmpeg && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
