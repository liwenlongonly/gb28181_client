//
// Created by liwen on 2024-12-24.
//

#include <map>

#include "rtp_client.h"
#include "net_connect.h"
#include "device_cfg.h"
#include "defer.h"
#include "sip_client.h"
#include "rtp_headers.h"
// media-server include
#include "rtp-payload.h"
#include "mov-reader.h"
#include "mov-format.h"
#include "mpeg-ps.h"
#include "mpeg4-avc.h"
#include "mpeg4-hevc.h"
#include "mov-file-buffer.h"

NS_BEGIN

#define BUFFER_SIZE 2 * 1024 * 1024

struct MediaContext {
    uint8_t s_buffer[BUFFER_SIZE]= {0};
    uint8_t s_packet[BUFFER_SIZE]= {0};

    int object;
    int streamId;

    struct mpeg4_hevc_t s_hevc;
    struct mpeg4_avc_t s_avc;

    uint8_t ps_buffer[BUFFER_SIZE] = {0};
    uint8_t rtp_buffer[BUFFER_SIZE+4] = {0};

    int64_t v_pts, v_dts;

    ps_muxer_t *ps{nullptr};
    uint32_t ssrc{};

    void *rtp_encoder{nullptr};

    std::shared_ptr<NetConnect> net_connect;

    bool send_packet_error{false};
};

inline const char *ftimestamp(uint32_t t, char *buf) {
    sprintf(buf, "%02u:%02u:%02u.%03u", t / 3600000, (t / 60000) % 60, (t / 1000) % 60, t % 1000);
    return buf;
}

static void *ps_alloc(void *param, size_t bytes) {
    MediaContext *mediaContext = (MediaContext *) param;
    assert(bytes <= sizeof(mediaContext->ps_buffer));
    return mediaContext->ps_buffer;
}

static void ps_free(void * /*param*/, void * /*packet*/) {
    return;
}

static int ps_write(void *param, int stream, void *packet, size_t bytes) {
    MediaContext *mediaContext = (MediaContext *) param;
    return rtp_payload_encode_input(mediaContext->rtp_encoder, packet, bytes, (unsigned int)mediaContext->v_dts);
}

static void *rtp_alloc(void *param, int bytes) {
    MediaContext *mediaContext = (MediaContext *) param;
    assert(bytes <= sizeof(mediaContext->rtp_buffer)-4);
    return mediaContext->rtp_buffer+4;
}

static void rtp_free(void * /*param*/, void * /*packet*/) {

}

static int rtp_encode_packet(void *param, const void *packet, int bytes, uint32_t timestamp, int flags) {
    MediaContext *mediaContext = (MediaContext *) param;
    erizo::RtpHeader* ptr = reinterpret_cast<erizo::RtpHeader*>((char *)packet);
    LOG_DEBUG(MSG_LOG,"RTP data ssrc:{} seq:{} len:{} ts:{}", ptr->getSSRC(), ptr->getSeqNumber() , bytes ,ptr->getTimestamp());
    if(mediaContext->net_connect){
       int ret = mediaContext->net_connect->sendPacket(static_cast<const char *>(packet), bytes);
       if(ret < 0){
           mediaContext->send_packet_error = true;
       }
    }
    return 0;
}

static int rtp_payload_codec_create(MediaContext *ctx, int payload, const char *encoding,
                                        uint16_t seq, uint32_t ssrc) {
    struct rtp_payload_t handler;
    handler.alloc = rtp_alloc;
    handler.free = rtp_free;
    handler.packet = rtp_encode_packet;
    ctx->rtp_encoder = rtp_payload_encode_create(payload, encoding, seq, ssrc, &handler, ctx);
    return 0;
}

static void mov_video_info(void *param, uint32_t track, uint8_t object,
                           int width, int height, const void *extra, size_t bytes) {
    MediaContext *mediaContext = (MediaContext *) param;
    mediaContext->object = object;
    if (MOV_OBJECT_H264 == object) {
        mpeg4_avc_decoder_configuration_record_load((const uint8_t *) extra, bytes, &mediaContext->s_avc);
        mediaContext->streamId = ps_muxer_add_stream(mediaContext->ps, PSI_STREAM_H264, extra, bytes);
        rtp_payload_codec_create(mediaContext, 96, "PS", 0, mediaContext->ssrc);
    } else if (MOV_OBJECT_HEVC == object) {
        mpeg4_hevc_decoder_configuration_record_load((const uint8_t *) extra, bytes, &mediaContext->s_hevc);
        mediaContext->streamId = ps_muxer_add_stream(mediaContext->ps, PSI_STREAM_H265, extra, bytes);
        rtp_payload_codec_create(mediaContext, 96, "PS", 0, mediaContext->ssrc);
    }
}

static void mov_onread(void *param, uint32_t track, const void *buffer, size_t bytes,
                       int64_t pts, int64_t dts, int flags) {
    MediaContext *mediaContext = (MediaContext *) param;
    static char s_pts[64], s_dts[64];
    static int64_t x_pts, x_dts;

    switch (mediaContext->object) {
        case MOV_OBJECT_H264: {
//            printf("[H264] pts: %s, dts: %s, diff: %03d/%03d, bytes: %u%s\n", ftimestamp(pts, s_pts),
//                   ftimestamp(dts, s_dts), (int) (pts - mediaContext->v_pts), (int) (dts - mediaContext->v_dts),
//                   (unsigned int) bytes, flags ? " [I]" : "[P]");
            mediaContext->v_pts = pts;
            mediaContext->v_dts = dts;

            assert(h264_is_new_access_unit((const uint8_t *) buffer + 4, bytes - 4));
            int n = h264_mp4toannexb(&mediaContext->s_avc, buffer, bytes, mediaContext->s_packet, sizeof(mediaContext->s_packet));
            ps_muxer_input(mediaContext->ps, mediaContext->streamId, flags ? 0x01 : 0x00, pts * 90, dts * 90,
                           mediaContext->s_packet, n);
            break;
        }
        case MOV_OBJECT_HEVC: {
            uint8_t nalu_type = (((const uint8_t *) buffer)[4] >> 1) & 0x3F;

//            printf("[H265] pts: %s, dts: %s, diff: %03d/%03d, bytes: %u%s,%d\n", ftimestamp(pts, s_pts),
//                   ftimestamp(dts, s_dts), (int) (pts - mediaContext->v_pts), (int) (dts - mediaContext->v_dts),
//                   (unsigned int) bytes, flags ? " [I]" : "[P]", (unsigned int) nalu_type);
            mediaContext->v_pts = pts;
            mediaContext->v_dts = dts;

            assert(h265_is_new_access_unit((const uint8_t *) buffer + 4, bytes - 4));
            int n = h265_mp4toannexb(&mediaContext->s_hevc, buffer, bytes, mediaContext->s_packet, sizeof(mediaContext->s_packet));
            ps_muxer_input(mediaContext->ps, mediaContext->streamId, flags ? 0x01 : 0x00, pts * 90, dts * 90,
                           mediaContext->s_packet, n);
            break;
        }
        default:
//            printf("[X] pts: %s, dts: %s, diff: %03d/%03d, bytes: %u\n", ftimestamp(pts, s_pts), ftimestamp(dts, s_dts),
//                   (int) (pts - x_pts), (int) (dts - x_dts), (unsigned int) bytes);
            x_pts = pts;
            x_dts = dts;
            break;
    }
}

struct mov_packet_t{
    int flags;
    int64_t pts;
    int64_t dts;
    uint32_t track;

    void* ptr;
    size_t bytes;
};

static void* onalloc(void* param, uint32_t track, size_t bytes, int64_t pts, int64_t dts, int flags)
{
    // emulate allocation
    struct mov_packet_t* pkt = (struct mov_packet_t*)param;
    if (pkt->bytes < bytes)
        return NULL;
    pkt->flags = flags;
    pkt->pts = pts;
    pkt->dts = dts;
    pkt->track = track;
    pkt->bytes = bytes;
    return pkt->ptr;
}

RtpClient::RtpClient() {

}

RtpClient::~RtpClient() {

}

void RtpClient::init(std::shared_ptr<DeviceConfig> config, std::shared_ptr<RtpConnectDelegate> delegate) {
    device_config_ = config;
    delegate_ = delegate;
    net_connect_ = std::make_shared<NetConnect>();
}

int RtpClient::startPushStream(const std::string &callId, std::shared_ptr<CallerParam> rtpParam) {
    rtp_param_ = rtpParam;
    call_id_ = callId;
    LOG_INFO(MSG_LOG, "callId: {} rtpProtocol:{}", callId, rtpParam->rtpProtocol);
    process_th_ = std::thread(&RtpClient::process, this);
    return 0;
}

void RtpClient::close() {
    is_runing_ = false;
    if (process_th_.joinable()) {
        process_th_.join();
    }
    if(net_connect_){
        net_connect_->close();
    }
    LOG_INFO(MSG_LOG, "{} RtpClient::close()", device_config_->deviceSipId);
}

void RtpClient::process() {

    // 初始化网络连接
    int ret = net_connect_->connect(rtp_param_->localRtpPort, rtp_param_->rtpIp
                          ,rtp_param_->rtpPort, rtp_param_->rtpProtocol);
    if(ret < 0){
        LOG_ERROR(MSG_LOG,"socket connect fail!");
        return;
    }

    is_runing_ = true;
    MediaContext *mediaContext = new(std::nothrow) MediaContext();
    if (mediaContext == nullptr) {
        return;
    }
    // 自动释放 mediaContext, 当process函数结束时
    defer([mediaContext] {
        if (mediaContext != nullptr) {
            delete mediaContext;
        }
    });

    rtp_packet_setsize(1056);

    // ps 格式封装
    struct ps_muxer_func_t handler;
    handler.alloc = ps_alloc;
    handler.write = ps_write;
    handler.free = ps_free;
    mediaContext->ps = ps_muxer_create(&handler, mediaContext);
    mediaContext->ssrc = rtp_param_->ssrc;
    mediaContext->net_connect = net_connect_;
    // 文件读取
    struct mov_file_cache_t file;
    memset(&file, 0, sizeof(file));
    file.fp = fopen(device_config_->filePath.c_str(), "rb");
    if(file.fp == nullptr){
        LOG_ERROR(MSG_LOG,"video file open fail!");
        return;
    }

    mov_reader_t *mov = mov_reader_create(mov_file_cache_buffer(), &file);

    struct mov_reader_trackinfo_t info = {mov_video_info};
    mov_reader_getinfo(mov, &info, mediaContext);
    while (is_runing_)
    {
        struct mov_packet_t pkt;
        pkt.ptr = mediaContext->s_buffer;
        pkt.bytes = sizeof(mediaContext->s_buffer);
        int r = mov_reader_read2(mov, onalloc, &pkt);
        if (r == 0)
        {
            LOG_INFO(MSG_LOG,"视频播放完成后重新开始");
            int64_t ts{0};
            mov_reader_seek(mov, &ts);
            continue;
        }else if(r < 0){
            break;
        }
        mov_onread(mediaContext, pkt.track, pkt.ptr, pkt.bytes, pkt.pts, pkt.dts, pkt.flags);
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        if(mediaContext->send_packet_error){
            LOG_ERROR(MSG_LOG,"send rtp packet error.");
            if(auto delegate = delegate_.lock()){
                delegate->onNetConnectError(call_id_);
            }
            break;
        }
    }
    if(mov){
        mov_reader_destroy(mov);
        mov = nullptr;
    }

    if(mediaContext->ps){
        ps_muxer_destroy(mediaContext->ps);
        mediaContext->ps = nullptr;
    }

    if (mediaContext->rtp_encoder) {
        rtp_payload_encode_destroy(mediaContext->rtp_encoder);
        mediaContext->rtp_encoder = nullptr;
    }
    if(file.fp){
        fclose(file.fp);
        file.fp = nullptr;
    }
}

NS_END
