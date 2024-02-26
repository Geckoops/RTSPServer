#include "Log.h"
#include "AACFileReader.h"
#include "AACStreamSender.h"
#include "H264FileReader.h"
#include "H264StreamSender.h"
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
        MediaFileReader* reader =
            H264FileReader::createNew(env, "../res/output.h264");
        StreamSender* stream_sender =
            H264StreamSender::createNew(env, reader, 23.976);
        session->addStreamSender(MediaSession::TrackId0, stream_sender);

        reader = AACFileReader::createNew(env, "../res/output.aac");
        stream_sender = AACStreamSender::createNew(env, reader, 48000, 2);
        session->addStreamSender(MediaSession::TrackId1, stream_sender);

        // session->startMulticast(); //多播
        session_manger->addSession(session);
    }
    LOGI("----------session init end------");

    rtsp_server->start();

    env->scheduler()->loop();
    return 0;
}