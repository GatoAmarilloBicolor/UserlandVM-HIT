/*
 * Copyright (c) 2024 UserlandVM Contributors
 * Distributed under the terms of the MIT License
 */

#ifndef _MEDIA_KIT_H
#define _MEDIA_KIT_H

#include "../../../SupportDefs.h"
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include <functional>
#include <map>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <chrono>

namespace BPrivate {

class MediaNode;
class MediaInput;
class MediaOutput;
class MediaBuffer;
class MediaFormat;
class MediaTrack;
class MediaFile;
class BSoundPlayer;
class BSound;
class BMediaAddOn;
class BTimeSource;

// Core media node type
enum media_type {
    B_MEDIA_UNKNOWN_TYPE = 0,
    B_MEDIA_RAW_AUDIO = 1,
    B_MEDIA_RAW_VIDEO = 2,
    B_MEDIA_ENCODED_AUDIO = 3,
    B_MEDIA_ENCODED_VIDEO = 4,
    B_MEDIA_MULTISTREAM = 5,
    B_MEDIA_MEDIA_INTERFACE = 6
};

// Media node kinds
enum media_node_kind {
    B_BUFFER_PRODUCER = 0x1,
    B_BUFFER_CONSUMER = 0x2,
    B_TIME_SOURCE = 0x4,
    B_CONTROLLABLE = 0x8,
    B_FILE_INTERFACE = 0x10
};

// Run modes
enum run_mode {
    B_INACTIVE = 0,
    B_RECORDING = 1,
    B_RENDERING = 2
};

// Media request info
struct media_request_info {
    int32 type;
    int32 status;
    void* user_data;
    int32 user_data_type;
    int32 source;
    int32 destination;
    media_buffer_header* buffer_header;
    bigtime_t start_time;
    bigtime_t finish_time;
};

// Media file format
struct media_file_format {
    char short_name[64];
    char pretty_name[64];
    char mime_type[64];
    char extensions[256];
    uint32 capabilities;
    uint32 flags;
};

// Media header
struct media_header {
    int32 type;
    int32 size_used;
    int32 size_available;
    uint32 buffer_flags;
    int64 start_time;
    int64 time_source;
    int32 orig_size;
    int64 file_pos;
    uint32 user_flags;
    int32 user_data_type;
    char user_data[48];
};

// Media buffer header
struct media_buffer_header {
    int32 type;
    int32 size_used;
    int32 size_available;
    uint32 buffer_flags;
    int64 start_time;
    int64 time_source;
    int32 orig_size;
    int64 file_pos;
    uint32 user_flags;
    int32 user_data_type;
    char user_data[48];
};

// Raw audio format
struct media_raw_audio_format {
    uint32 format;
    uint32 channel_mask;
    uint32 valid_bits;
    uint32 byte_order;
    uint32 frame_rate;
    uint32 buffer_size;
    uint32 channel_count;
    uint32 latency;
    uint32 sample_rate;
};

// Media format description
struct media_format {
    media_type type;
    char user_data[92];
    
    // Audio format for B_MEDIA_RAW_AUDIO
    struct {
        uint32 format;
        uint32 channel_mask;
        uint32 valid_bits;
        uint32 byte_order;
        uint32 frame_rate;
        uint32 buffer_size;
        uint32 channel_count;
        uint32 latency;
        uint32 sample_rate;
    } audio;
    
    // Video format for B_MEDIA_RAW_VIDEO
    struct {
        uint32 field_rate;
        uint32 field_count;
        uint32 interlace;
        uint32 orientation;
        uint32 pixel_width_aspect;
        uint32 pixel_height_aspect;
        uint32 display_width;
        uint32 display_height;
        uint32 display_x_offset;
        uint32 display_y_offset;
        uint32 bytes_per_row;
        uint32 pixel_offset;
        uint32 line_offset;
        uint32 format;
        uint32 padding;
    } video;
};

// Node and type aliases
typedef int32 media_node_id;
typedef media_node_id time_source;

// Message class for simple implementation
class SimpleMessage {
public:
    SimpleMessage() {}
    ~SimpleMessage() {}
    
    status_t AddInt32(const char* name, int32 value) {
        fInt32Data[name] = value;
        return B_OK;
    }
    
    status_t FindInt32(const char* name, int32* value) const {
        auto it = fInt32Data.find(name);
        if (it != fInt32Data.end()) {
            *value = it->second;
            return B_OK;
        }
        return B_NAME_NOT_FOUND;
    }
    
private:
    std::map<std::string, int32> fInt32Data;
};

// Simple string implementation
class BString {
public:
    BString() {}
    BString(const char* str) : fData(str ? str : "") {}
    ~BString() {}
    
    const char* String() const { return fData.c_str(); }
    void SetTo(const char* str) { fData = str ? str : ""; }
    
private:
    std::string fData;
};

// Simple file class
class BFile {
public:
    BFile(const char* path, uint32 openMode) {
        if (openMode & B_READ_ONLY) {
            fFile = fopen(path, "rb");
        } else {
            fFile = fopen(path, "w+b");
        }
    }
    
    ~BFile() {
        if (fFile) {
            fclose(fFile);
        }
    }
    
    status_t InitCheck() const {
        return fFile ? B_OK : B_ERROR;
    }
    
    ssize_t Read(void* buffer, size_t size) {
        if (!fFile) return B_ERROR;
        return fread(buffer, 1, size, fFile);
    }
    
    ssize_t Write(const void* buffer, size_t size) {
        if (!fFile) return B_ERROR;
        return fwrite(buffer, 1, size, fFile);
    }
    
    off_t Seek(off_t position, uint32 seekMode) {
        if (!fFile) return B_ERROR;
        int whence = (seekMode == 0x02) ? SEEK_END : 
                     (seekMode == 0x01) ? SEEK_CUR : SEEK_SET;
        return fseek(fFile, position, whence);
    }
    
    status_t GetSize(off_t* size) const {
        if (!fFile || !size) return B_BAD_VALUE;
        long currentPos = ftell(fFile);
        fseek(fFile, 0, SEEK_END);
        *size = ftell(fFile);
        fseek(fFile, currentPos, SEEK_SET);
        return B_OK;
    }
    
private:
    FILE* fFile;
};

// Data IO class
class BDataIO {
public:
    virtual ~BDataIO() {}
    virtual ssize_t Read(void* buffer, size_t size) = 0;
    virtual ssize_t Write(const void* buffer, size_t size) = 0;
};

// Flavor info for add-ons
struct flavor_info {
    char name[64];
    char info[256];
    int32 kind;
    int32 priority;
    uint32 version;
    uint32 internal_id;
    uint32 possible_count;
    uint32 in_format_count;
    uint32 out_format_count;
    void* in_formats;
    void* out_formats;
};

// Message type alias
typedef SimpleMessage BMessage;

// Media buffer
class MediaBuffer {
public:
    MediaBuffer(int32 size);
    ~MediaBuffer();
    
    void* Data();
    int32 Size() const;
    int32 SizeUsed() const;
    void SetSizeUsed(int32 size);
    
    const media_buffer_header& Header() const;
    media_buffer_header& Header();
    
    status_t SetHeader(const media_buffer_header& header);
    status_t SetTo(void* data, int32 size);
    
    void Clone(MediaBuffer* clone);
    status_t Recycle();
    
private:
    void* fData;
    int32 fSize;
    media_buffer_header fHeader;
    MediaNode* fOwner;
    bool fReclaimed;
};

// Media format description
class MediaFormat {
public:
    MediaFormat();
    MediaFormat(const media_format& format);
    ~MediaFormat();
    
    status_t SetTo(const media_format& format);
    status_t Get(media_format& format) const;
    
    bool IsVideo() const;
    bool IsAudio() const;
    bool Matches(const MediaFormat& other) const;
    
    status_t Clear();
    status_t MakeEmpty();
    
    status_t SetAudioFormat(uint32 sampleRate, uint32 channelCount, uint32 format);
    status_t SetVideoFormat(uint32 width, uint32 height, uint32 fieldRate, uint32 format);
    
private:
    media_format fFormat;
    mutable std::mutex fLock;
};

// Media connection
class MediaConnection {
public:
    MediaConnection();
    ~MediaConnection();
    
    status_t SetFormat(const MediaFormat& format);
    status_t GetFormat(MediaFormat& format) const;
    
    void SetSource(MediaNode* node, int32 source);
    void SetDestination(MediaNode* node, int32 destination);
    
    MediaNode* SourceNode() const;
    MediaNode* DestinationNode() const;
    
    int32 Source() const;
    int32 Destination() const;
    
private:
    MediaNode* fSourceNode;
    MediaNode* fDestinationNode;
    int32 fSource;
    int32 fDestination;
    MediaFormat fFormat;
};

// Media input/output
class MediaInput {
public:
    MediaInput(MediaNode* owner);
    ~MediaInput();
    
    status_t AcceptFormat(const MediaFormat& format);
    status_t GetFormat(MediaFormat& format) const;
    status_t SetFormat(const MediaFormat& format);
    
    status_t Connect(MediaNode* producer, int32 source, const MediaFormat& format);
    status_t Disconnect();
    
    bool IsConnected() const;
    MediaNode* Connection() const;
    
private:
    MediaNode* fOwner;
    MediaNode* fConnection;
    int32 fSource;
    MediaFormat fFormat;
    bool fConnected;
    mutable std::mutex fLock;
};

class MediaOutput {
public:
    MediaOutput(MediaNode* owner);
    ~MediaOutput();
    
    status_t AcceptFormat(const MediaFormat& format);
    status_t GetFormat(MediaFormat& format) const;
    status_t SetFormat(const MediaFormat& format);
    
    status_t Connect(MediaNode* consumer, int32 destination, const MediaFormat& format);
    status_t Disconnect();
    
    bool IsConnected() const;
    MediaNode* Connection() const;
    
private:
    MediaNode* fOwner;
    MediaNode* fConnection;
    int32 fDestination;
    MediaFormat fFormat;
    bool fConnected;
    mutable std::mutex fLock;
};

// Media node interface
class MediaNode {
public:
    MediaNode(int32 priority = 100);
    virtual ~MediaNode();
    
    // Node control
    virtual status_t NodeRegistered();
    virtual status_t InitCheck() const;
    
    // Run state control
    virtual status_t SetRunMode(run_mode mode);
    virtual status_t GetRunMode(run_mode* mode) const;
    virtual status_t SetTimeSource(BTimeSource* timeSource);
    virtual status_t RequestCompleted(const media_request_info& info);
    
    // Buffer handling
    virtual status_t GetLatency(bigtime_t* latency);
    virtual status_t GetStartTime(bigtime_t* startTime);
    
    // Format negotiation
    virtual status_t AcceptFormat(int32 destination, const MediaFormat& format);
    virtual status_t GetFormat(int32 destination, MediaFormat* format);
    virtual status_t SetFormat(int32 destination, const MediaFormat& format);
    
    // Buffer handling
    virtual status_t SendBuffer(MediaBuffer* buffer, int32 destination);
    virtual status_t ReceiveBuffer(MediaBuffer* buffer, int32 source);
    
    // Node info
    status_t GetID(int32* id) const;
    status_t GetKind(media_node_kind* kind) const;
    status_t GetName(const char** name) const;
    
    // I/O management
    int32 CountInputs() const;
    MediaInput* InputAt(int32 index);
    
    int32 CountOutputs() const;
    MediaOutput* OutputAt(int32 index);
    
    MediaInput* FindInput(int32 id);
    MediaOutput* FindOutput(int32 id);
    
protected:
    status_t RegisterInput(MediaInput* input);
    status_t RegisterOutput(MediaOutput* output);
    
private:
    int32 fID;
    int32 fKind;
    char fName[256];
    run_mode fRunMode;
    bigtime_t fLatency;
    bigtime_t fStartTime;
    
    std::vector<MediaInput*> fInputs;
    std::vector<MediaOutput*> fOutputs;
    
    mutable std::mutex fLock;
    std::atomic<bool> fRunning;
    
    static std::atomic<int32> sNextID;
};

// Media track
class MediaTrack : public MediaNode {
public:
    MediaTrack(MediaFile* file);
    virtual ~MediaTrack();
    
    // Track info
    status_t GetInfo(media_format* format, float* frameRate, uint32* flags);
    status_t GetDuration(bigtime_t* duration);
    
    // Frame access
    status_t ReadFrames(void* buffer, int64* frameCount, 
                       media_header* header = NULL);
    status_t WriteFrames(const void* buffer, int64 frameCount,
                        media_header* header = NULL);
    
    // Key frame control
    status_t SeekToFrame(int64* frame, bigtime_t* time);
    status_t SeekToTime(bigtime_t* time, int64* frame);
    
    status_t FindKeyFrameForFrame(int64* frame);
    status_t FindKeyFrameForTime(bigtime_t* time);
    
    // Encoding/decoding
    status_t GetEncodedFormat(media_format* format);
    status_t SetEncodedFormat(const media_format& format);
    
private:
    MediaFile* fFile;
    MediaFormat fFormat;
    int64 fFrameCount;
    int64 fCurrentFrame;
    bigtime_t fDuration;
    float fFrameRate;
    uint32 fFlags;
};

// Media file
class MediaFile {
public:
    MediaFile(const char* path);
    MediaFile(BFile* file);
    ~MediaFile();
    
    status_t InitCheck() const;
    
    // File info
    status_t GetInfo(media_file_format* fileFormat);
    status_t GetCountTracks(int32* trackCount);
    
    // Track management
    MediaTrack* TrackAt(int32 index);
    MediaTrack* FindTrack(media_type type);
    
    // Format support
    status_t GetFileFormat(media_file_format* format);
    status_t SetFileFormat(const media_file_format& format);
    
    // Creation control
    status_t AddTrack(MediaTrack* track);
    status_t RemoveTrack(MediaTrack* track);
    
    // Sniffing
    static bool Sniff(const char* path);
    static bool SniffType(BDataIO* source, const char* mimeType);
    
private:
    BString fPath;
    std::unique_ptr<BFile> fFile;
    media_file_format fFileFormat;
    std::vector<MediaTrack*> fTracks;
    status_t fInitStatus;
    mutable std::mutex fLock;
};

// Sound player
class BSoundPlayer {
public:
    enum sound_player_notification {
        B_SOUND_STARTED,
        B_SOUND_STOPPED,
        B_SOUND_EMPTY_BUFFER,
        B_SOUND_BUFFER_FILLED
    };
    
    BSoundPlayer(const char* name = NULL,
                const media_format* format = NULL,
                void (*playBuffer)(void* cookie, void* buffer, size_t size, const media_raw_audio_format& format) = NULL,
                void (*notify)(void* cookie, sound_player_notification what, ...) = NULL,
                void* cookie = NULL);
    ~BSoundPlayer();
    
    status_t InitCheck() const;
    
    // Player control
    status_t Start();
    status_t Stop();
    status_t SetVolume(float volume);
    status_t SetHasData(bool hasData);
    
    // Info
    status_t GetVolume(float* volume) const;
    bool HasData() const;
    const media_format& Format() const;
    
private:
    BString fName;
    MediaFormat fFormat;
    void (*fPlayBuffer)(void*, void*, size_t, const media_raw_audio_format&);
    void (*fNotify)(void*, sound_player_notification, ...);
    void* fCookie;
    float fVolume;
    bool fHasData;
    bool fRunning;
    status_t fInitStatus;
    
    std::thread fPlayThread;
    std::atomic<bool> fShouldStop;
    
    void PlayThread();
};

// Sound effect
class BSound {
public:
    BSound(const char* path);
    BSound(BDataIO* source, const char* mimeType = NULL);
    BSound(const void* data, size_t size, const media_format& format);
    ~BSound();
    
    status_t InitCheck() const;
    
    // Playback
    status_t Play(float volume = 1.0, float pan = 0.0);
    status_t PlayOn(BSoundPlayer* player, float volume = 1.0, float pan = 0.0);
    
    // Info
    status_t GetFormat(media_format* format) const;
    bigtime_t Duration() const;
    
private:
    MediaFormat fFormat;
    void* fData;
    size_t fSize;
    bigtime_t fDuration;
    status_t fInitStatus;
    
    status_t LoadFromFile(const char* path);
    status_t LoadFromData(BDataIO* source, const char* mimeType);
};

// Media add-on interface
class BMediaAddOn {
public:
    BMediaAddOn(image_id image);
    virtual ~BMediaAddOn();
    
    // Add-on info
    virtual status_t GetFlavorAt(int32 index, const flavor_info** info);
    virtual status_t GetConfigurationFor(MediaNode* node, BMessage* message);
    
    // Instantiation
    virtual MediaNode* InstantiateNodeFor(
        const flavor_info* info,
        BMessage* config,
        media_node_id* nodeId);
    
    // File format support
    virtual bool SupportsFormat(const char* format);
    virtual bool SupportsMimeType(const char* mimeType);
    
    image_id ImageID() const;
    
protected:
    virtual status_t InitCheck() const;
    
private:
    image_id fImage;
};

// Time source
class BTimeSource : public MediaNode {
public:
    BTimeSource();
    virtual ~BTimeSource();
    
    // Time control
    virtual status_t GetTime(bigtime_t* time);
    virtual status_t GetRealTime(bigtime_t* time);
    virtual status_t GetPerformanceTime(bigtime_t* time);
    
    // Rate control
    virtual status_t SetRealtimeFor(bigtime_t performanceTime, bigtime_t realTime);
    virtual status_t GetRealtimeFor(bigtime_t performanceTime, bigtime_t* realTime);
    virtual status_t GetPerformanceTimeFor(bigtime_t realTime, bigtime_t* performanceTime);
    
    // Running state
    virtual status_t Start();
    virtual status_t Stop();
    virtual status_t Seek(bigtime_t time);
    virtual status_t IsRunning(bool* isRunning);
    
    // Sync info
    virtual status_t GetTimeSource(time_source* source);
    
private:
    bigtime_t fStartTime;
    float fSpeed;
    bool fRunning;
    
    bigtime_t RealToPerformance(bigtime_t realTime) const;
    bigtime_t PerformanceToReal(bigtime_t performanceTime) const;
};

// Utility functions
status_t GetAudioIn(media_node_id* node);
status_t GetVideoIn(media_node_id* node);
status_t GetAudioOut(media_node_id* node);
status_t GetVideoOut(media_node_id* node);
status_t GetAudioMixer(media_node_id* node);

status_t GetSystemTimeSource(media_node_id* node);

status_t SetSoundPlayerVolume(int32 device, float volume);
status_t GetSoundPlayerVolume(int32 device, float* volume);

status_t PlaySound(const char* path, bool sync = false);
status_t PlaySound(BSound* sound, bool sync = false);

status_t Sleep(bigtime_t microseconds);

// Audio format utilities
media_format MakeMediaFormat(int32 audioFormat, uint32 sampleRate, uint32 channelCount);
media_format MakeVideoFormat(uint32 width, uint32 height, uint32 fieldRate, uint32 colorSpace);

status_t StringForFormat(const media_format& format, char* string, size_t bufferSize);
status_t FormatFromString(const char* string, media_format* format);

// Math utilities for media time
inline bigtime_t BytesToTime(int64 bytes, const media_format& format);
inline int64 TimeToBytes(bigtime_t time, const media_format& format);
inline int64 FrameToSample(int64 frame, const media_format& format);
inline bigtime_t SampleToTime(int64 sample, const media_format& format);

// Implementation helpers
class MediaNodeRegistry {
public:
    static MediaNodeRegistry& Instance();
    
    status_t RegisterNode(MediaNode* node);
    status_t UnregisterNode(MediaNode* node);
    
    MediaNode* FindNode(media_node_id id);
    std::vector<MediaNode*> FindNodes(media_type type);
    std::vector<MediaNode*> FindNodes(media_node_kind kind);
    
private:
    std::map<media_node_id, MediaNode*> fNodes;
    std::mutex fLock;
    std::atomic<media_node_id> fNextID{1};
};

} // namespace BPrivate

using namespace BPrivate;

#endif // _MEDIA_KIT_H