#include "JsonParser.h"

#include <fstream>

std::vector<MediaInfo> JsonParser::parse() {
    std::vector<MediaInfo> media_infos;
    const Json::Value media_array = root["media"];
    for (const auto& media : media_array) {
        MediaInfo info;
        info.name = media["name"].asString();
        info.session_name = media["session_name"].asString();
        info.enabled = media["enabled"].asBool();
        info.encrypted = media["encrypted"].asBool();
        info.secret_key = media["secret_key"].asString();
        for (const auto& track : media["track"]) {
            TrackInfo track_info;
            track_info.type = track["type"].asString();
            track_info.path = track["path"].asString();
            track_info.sample_rate = track.get("sample_rate", 0).asUInt();
            track_info.channels = track.get("channels", 0).asUInt();
            track_info.fps = track.get("fps", 0).asDouble();
            track_info.pps = track.get("pps", "").asString();
            track_info.sps = track.get("sps", "").asString();
            info.tracks.push_back(track_info);
        }
        media_infos.push_back(info);
    }
    return media_infos;
}

JsonParser::JsonParser(const std::string& filename) {
    std::ifstream file(filename);
    if (file.is_open()) {
        file >> root;
    }
    file.close();
}
