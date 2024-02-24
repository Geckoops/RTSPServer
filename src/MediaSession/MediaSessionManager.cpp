#include "MediaSessionManager.h"

#include "MediaSession.h"

MediaSessionManager* MediaSessionManager::createNew() {
    return new MediaSessionManager();
}

MediaSessionManager::MediaSessionManager() {}

MediaSessionManager::~MediaSessionManager() {}

bool MediaSessionManager::addSession(MediaSession* session) {
    if (session_map_.contains(session->name())) {
        return false;
    } else {
        session_map_.insert({session->name(), session});
        return true;
    }
}

bool MediaSessionManager::removeSession(MediaSession* session) {
    auto it = session_map_.find(session->name());
    if (it == session_map_.end()) {
        return false;
    } else {
        session_map_.erase(it);
        return true;
    }
}

MediaSession* MediaSessionManager::getSession(const std::string& name) {
    auto it = session_map_.find(name);
    return it == session_map_.end() ? nullptr : it->second;
}
