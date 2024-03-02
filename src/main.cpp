#include "Log.h"
#include "AACMediaSource.h"
#include "AACMediaSink.h"
#include "H264MediaSource.h"
#include "H264MediaSink.h"
#include "MediaSession.h"
#include "MediaSessionManager.h"
#include "RtspServer.h"
#include "EventScheduler.h"
#include "SocketsOps.h"
#include "ThreadPool.h"
#include "UsageEnvironment.h"

int main() {
    sockets::initNetWork();
    srand(time(nullptr));  // 时间初始化

    EventScheduler* scheduler =
        EventScheduler::createNew(EventScheduler::POLLER_SELECT);
    ThreadPool* thread_pool = ThreadPool::createNew(1);
    MediaSessionManager* session_manger = MediaSessionManager::createNew();
    UsageEnvironment* env = UsageEnvironment::createNew(scheduler, thread_pool);

    Ipv4Address rtsp_addr("127.0.0.1", 8554);
    RtspServer* rtsp_server = RtspServer::createNew(env, session_manger, rtsp_addr);

    LOGI("----------session init start------");
    {
        MediaSession* session = MediaSession::createNew("test");
        MediaSource* source =
            H264MediaSource::createNew(env, "../res/output.h264");
        Sink* sink =
            H264MediaSink::createNew(env, source, 23.976);
        session->addSink(MediaSession::TrackId0, sink);

        source = AACMediaSource::createNew(env, "../res/output.aac");
        sink = AACMediaSink::createNew(env, source, 48000, 2);
        session->addSink(MediaSession::TrackId1, sink);

        // session->startMulticast(); //多播
        session_manger->addSession(session);
    }
    LOGI("----------session init end------");

    rtsp_server->start();

    env->scheduler()->loop();
    return 0;
}