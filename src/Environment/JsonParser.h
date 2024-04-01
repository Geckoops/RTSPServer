#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <json/json.h>

#include <string>
#include <vector>

struct TrackInfo {
    std::string type;
    std::string path;
    size_t sample_rate;  // aac
    size_t channels;     // aac
    double fps;          // h264
    std::string pps;     // h264(optional)
    std::string sps;     // h264(optional)
};

struct MediaInfo {
    std::string name;
    std::string session_name;
    bool enabled;
    bool encrypted;
    std::string secret_key;
    std::vector<TrackInfo> tracks;
};

class JsonParser {
   public:
    JsonParser(const std::string& filename);

    std::vector<MediaInfo> parse();

   private:
    Json::Value root;
};

#endif  // JSONPARSER_H
