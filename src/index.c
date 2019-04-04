#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <stdio.h>

int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, uint8_t **output, uint32_t *frame_index, uint32_t *output_size_list) {
  int ret;

  ret = avcodec_send_frame(ctx, frame);
  if (ret < 0) {
    exit(1);
  }

  while (ret >= 0) {
    ret = avcodec_receive_packet(ctx, pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      return;
    }
    if (ret < 0) {
      exit(1);
    }
    memcpy(*output, pkt->data, pkt->size);
    *output += pkt->size;
    output_size_list[*frame_index] = pkt->size;
    *frame_index = *frame_index + 1;
    av_packet_unref(pkt);
  }

  return;
}

uint32_t pcm2aac(uint32_t sample_rate, uint8_t *input, uint32_t input_size, uint8_t *output, uint32_t *output_size_list) {
  const AVCodec *codec;
  AVCodecContext *c = NULL;
  AVPacket *pkt;
  AVFrame *frame;
  uint32_t *samples; // 4 bits
  SwrContext *swr;
  int ret = 0;
  uint8_t *outs[1];
  outs[0] = (uint8_t *)malloc(input_size * 2);

/*
  FILE* ofp;

  ofp = fopen("fltp.raw", "w");
*/
  codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
  if (!codec) {
    exit(1);
  }
  c = avcodec_alloc_context3(codec);
  if (!c) {
    exit(1);
  }

  swr = swr_alloc();
  if (!swr) {
    exit(1);
  }

  av_opt_set_int(swr, "in_channel_layout", AV_CH_LAYOUT_MONO, 0);
  av_opt_set_int(swr, "in_sample_rate", sample_rate, 0);
  av_opt_set_sample_fmt(swr, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);

  av_opt_set_int(swr, "out_channel_layout", AV_CH_LAYOUT_MONO, 0);
  av_opt_set_int(swr, "out_sample_rate", sample_rate, 0);
  av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);

  if ((ret = swr_init(swr)) < 0) {
    exit(1);
  }

  swr_convert(swr, &outs, input_size * 2, &input, input_size / 2);


  c->bit_rate = 64000;
  c->sample_fmt = AV_SAMPLE_FMT_FLTP;
  if (!check_sample_fmt(codec, c->sample_fmt)) {
    exit(1);
  }
  c->sample_rate = sample_rate;
  c->channel_layout = AV_CH_LAYOUT_MONO;
  c->channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_MONO);

  if (avcodec_open2(c, codec, NULL) < 0) {
    exit(1);
  }

  pkt = av_packet_alloc();
  if (!pkt) {
    exit(1);
  }

  frame = av_frame_alloc();
  if (!frame) {
    exit(1);
  }

  frame->nb_samples     = c->frame_size;
  frame->format         = c->sample_fmt;
  frame->channel_layout = c->channel_layout;

  int i = 0, j = 0;
  uint32_t frame_index = 0;
  uint32_t output_size = 0;

  ret = av_frame_get_buffer(frame, 0);
  if (ret < 0) {
    exit(1);
  }
  uint32_t* input_fltp_32 = (uint32_t *)outs[0];
  /*
  fwrite(input_fltp_32, sizeof(uint32_t), input_size / 2, ofp);
  */

  while (i < input_size / 2) {
    ret = av_frame_make_writable(frame);
    if (ret < 0) {
      exit(1);
    }
    samples = (uint32_t *)frame->data[0];
    for (j = 0; j < c->frame_size; j++) {
      // overflow
      if (i + j >= input_size / 2) {
        samples[j] = 0x00000000;
      } else {
        samples[j] = input_fltp_32[i + j];
      }
    }
    encode(c, frame, pkt, &output, &frame_index, output_size_list);
    i += c->frame_size;
  }

  encode(c, NULL, pkt, &output, &frame_index, output_size_list);


  av_frame_free(&frame);
  av_packet_free(&pkt);
  avcodec_free_context(&c);

  return frame_index;
}

/*
int main() {
  FILE *fp;

  fp = fopen("pcm.raw", "r");
  if (fp == NULL) {
    perror("error open file");
    exit(2);
  }
  fseek(fp, 0L, SEEK_END);
  uint32_t size = ftell(fp);
  fseek(fp, 0L, SEEK_SET);

  uint8_t *input = malloc(size);
  fread(input, sizeof(uint8_t), size, fp);

  printf("size1: %d \n", size);
  uint8_t *output = (uint8_t *)malloc(size);
  uint32_t output_size_list[200];
  uint8_t pkg_count = pcm2aac(16000, input, size, output, output_size_list);

  printf("pkg_count: %d\n", pkg_count);
  int output_size = 0;
  for (int i = 0; i < pkg_count; i++) {
    printf("frame%d: %d", i, output_size_list[i]);
    output_size += output_size_list[i];
  }
  printf("output_size: %d\n", output_size);

  fp = fopen("acc.raw", "w");
  fwrite(output, sizeof(uint8_t), output_size, fp);
  return 0;
}
*/
