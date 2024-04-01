#include "AACMediaSink.h"
#include "AACMediaSource.h"
#include "EventScheduler.h"
#include "H264MediaSink.h"
#include "H264MediaSource.h"
#include "JsonParser.h"
#include "Log.h"
#include "MediaSession.h"
#include "MediaSessionManager.h"
#include "RtspServer.h"
#include "SocketsOps.h"
#include "ThreadPool.h"
#include "UsageEnvironment.h"

int main(int argc, char** argv) {
    sockets::initNetWork();
    srand(time(nullptr));  // 时间初始化

    JsonParser json_parser("../res/config.json");
    std::vector<MediaInfo> media_infos = json_parser.parse();
    if (media_infos.empty()) {
        LOGE("parse config.json failed");
        return -1;
    }

    auto scheduler = std::shared_ptr<EventScheduler>(
        EventScheduler::createNew(EventScheduler::POLLER_SELECT));
    auto thread_pool = std::shared_ptr<ThreadPool>(ThreadPool::createNew(1));
    auto session_manger =
        std::shared_ptr<MediaSessionManager>(MediaSessionManager::createNew());
    auto env = std::shared_ptr<UsageEnvironment>(
        UsageEnvironment::createNew(scheduler.get(), thread_pool.get()));

    Ipv4Address rtsp_addr("127.0.0.1", 8554);
    RtspServer* rtsp_server =
        RtspServer::createNew(env.get(), session_manger.get(), rtsp_addr);

    std::map<std::string, bool> session_map;

    for (const auto& info : media_infos) {
        if (info.enabled == false) {
            continue;
        }
        if (info.name.empty()) {
            LOGE("cannot create media session in root");
            return -1;
        }
        if (session_map.find(info.name) != session_map.end()) {
            LOGE("media session name is duplicated");
            return -1;
        } else {
            session_map[info.name] = true;
        }

        auto session = MediaSession::createNew(info.session_name);

        for (auto& media : info.tracks) {
            if (media.type == "h264") {
                MediaSource* source =
                    H264MediaSource::createNew(env.get(), media.path);
                Sink* sink =
                    H264MediaSink::createNew(env.get(), source, media.fps);
                session->addSink(MediaSession::TrackId0, sink);
            } else if (media.type == "aac") {
                MediaSource* source =
                    AACMediaSource::createNew(env.get(), media.path);
                Sink* sink = AACMediaSink::createNew(
                    env.get(), source, media.sample_rate, media.channels);
                session->addSink(MediaSession::TrackId1, sink);
            } else {
                LOGE("unsupport media type:%s", media.type.c_str());
            }
        }

        session_manger->addSession(session);
        LOGI("add media session:%s", info.name.c_str());
    }

    rtsp_server->start();

    env->scheduler()->loop();
    return 0;
}