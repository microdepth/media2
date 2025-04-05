#include <iostream>
#include <format>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libavutil/pixdesc.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include <libavdevice/avdevice.h>
}

using namespace std;

int main() {
    av_log_set_level(AV_LOG_DEBUG);
    avdevice_register_all();

    AVFormatContext* pFormatContext = avformat_alloc_context();

    const char* filename = "test.mov";

    int openInput = avformat_open_input(&pFormatContext, filename, nullptr, nullptr);
    if (openInput != 0) {
        char errorbuf[1024];
        av_strerror(openInput, errorbuf, sizeof(errorbuf));
        printf("error opening device: %s\n", errorbuf);
        return -1;
    }

    AVCodecParameters* pCodecParameters = nullptr;
    int audioStreamIndex = -1;
    for (int i = 0; i < pFormatContext->nb_streams; i++) {
        if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            printf("found audio stream\n");
            audioStreamIndex = i;
            pCodecParameters = pFormatContext->streams[i]->codecpar;
            break;
        }
    }

    if (audioStreamIndex == -1) {
        printf("failed to find audio stream\n");
        return -1;
    }

    const AVCodec* pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if (!pCodec) {
        printf("failed to find codec\n");
        return -1;
    }

    AVCodecContext* pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext) {
        printf("failed to allocate codec context\n");
        return -1;
    }

    if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0) {
        printf("failed to copy codec parameters into codec context\n");
        return -1;
    }

    if (avcodec_open2(pCodecContext, pCodec, nullptr) < 0) {
        printf("failed to open codec\n");
        return -1;
    }

    printf("audio stream info:\n");
    printf("sample format: %s\n", av_get_sample_fmt_name(pCodecContext->sample_fmt));
    printf("channels: %d\n", pCodecContext->ch_layout.nb_channels);
    printf("sample rate: %dHz\n", pCodecContext->sample_rate);

    AVPacket* pPacket = av_packet_alloc();
    AVFrame* pFrame = av_frame_alloc();

    FILE* outputFile = fopen("output.pcm", "wb");
    if (!outputFile) {
        printf("failed to open output file\n");
        return -1;
    }

    bool isPlanar = av_sample_fmt_is_planar(pCodecContext->sample_fmt);
    int bytes_per_sample = av_get_bytes_per_sample(pCodecContext->sample_fmt);

    int frame_number = 0;
    while (av_read_frame(pFormatContext, pPacket) >= 0) {
        if (pPacket->stream_index == audioStreamIndex) {

            if (avcodec_send_packet(pCodecContext, pPacket) < 0) {
                printf("error sending packet for decoding\n");
                continue;
            }

            while (true) {
                int ret = avcodec_receive_frame(pCodecContext, pFrame);
                if (ret == AVERROR(EAGAIN)) {
                    break;
                } else if (ret < 0) {
                    printf("error receiving decoded frame\n");
                    break;
                }

                printf("frame decoded\n");

                if (isPlanar) {
                    for (int sample = 0; sample < pFrame->nb_samples; ++sample) {
                        for (int channel = 0; channel < pCodecContext->ch_layout.nb_channels; ++channel) {
                            fwrite(pFrame->data[channel] + sample * bytes_per_sample, bytes_per_sample, 1, outputFile);
                        }
                    }
                    printf("done with frame number %d\n", frame_number);
                } else {
                    fwrite(pFrame->data[0], pFrame->nb_samples * bytes_per_sample * pCodecContext->ch_layout.nb_channels, 1, outputFile);
                }

                frame_number++;
            }
        }
        av_packet_unref(pPacket);
    }

    fclose(outputFile);
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecContext);
    avformat_close_input(&pFormatContext);

    return 0;
}
