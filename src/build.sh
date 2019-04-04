#! /bin/sh
#
# build.sh
# Copyright (C) 2018 disoul <disoul@DiSouldeMacBook-Pro.local>
#
# Distributed under terms of the MIT license.
#
FFMPEG_PREFIX=/Users/disoul/github/libs/FFmpeg/inst_aac
FFMPEG_LIBS="$FFMPEG_PREFIX/lib/libavcodec.a $FFMPEG_PREFIX/lib/libavutil.a $FFMPEG_PREFIX/lib/libswresample.a"

# 256M
TOTAL_MEMORY=268435456

export EXPORTED_FUNCTIONS="['_pcm2aac']";

emcc $FFMPEG_LIBS ./index.c -I $FFMPEG_PREFIX/include -L $FFMPEG_PREFIX/lib \
  -lavcodec -lswresample -lavutil \
  -s 'EXTRA_EXPORTED_RUNTIME_METHODS=["ccall", "cwrap"]'\
  -s ALLOW_MEMORY_GROWTH=1 \
  -s DISABLE_EXCEPTION_CATCHING=0 \
  -s TOTAL_MEMORY=${TOTAL_MEMORY} \
  -s EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS}" \
	-s ENVIRONMENT=web \
	-s NO_FILESYSTEM=1 \
	-s WASM=1 \
	-O3 \
  -o aac_wasm.js
