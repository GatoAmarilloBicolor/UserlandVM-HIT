/*
 * HaikuMediaKit.h - Complete Haiku Media Kit Interface
 * 
 * Interface for all Haiku media operations: BSoundPlayer, BSoundRecorder, BMediaNode, BMediaRoster
 * Provides cross-platform Haiku media functionality
 */

#pragma once

#include "HaikuAPIVirtualizer.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>

// Haiku Media Kit constants
#define HAIKU_MAX_SOUND_PLAYERS       16
#define HAIKU_MAX_SOUND_RECORDERS      8
#define HAIKU_MAX_MEDIA_NODES          64
#define HAIKU_MAX_AUDIO_BUFFERS         8192
#define HAIKU_MAX_AUDIO_BUFFER_SIZE       8192
#define HAIKU_MAX_VIDEO_BUFFERS         4
#define HAIKU_MAX_VIDEO_FRAME_SIZE       1920 * 1080 * 3  // 1920x1080 RGB

// Haiku audio formats
#define HAIKU_AUDIO_FORMAT_PCM_8         0x01
#define HAIKU_AUDIO_FORMAT_PCM_16        0x02
#define HAIKU_AUDIO_FORMAT_PCM_24        0x03
#define HAIKU_AUDIO_FORMAT_PCM_32        0x04
#define HAIKU_AUDIO_FORMAT_FLOAT_32       0x05

// Haiku media node types
#define HAIKU_MEDIA_NODE_AUDIO_OUTPUT    1
#define HAIKU_MEDIA_NODE_AUDIO_INPUT     2
#define HAIKU_MEDIA_NODE_VIDEO_OUTPUT    3
#define HAIK_MEDIA_NODE_VIDEO_INPUT     4
#define HAIKU_MEDIA_NODE_MIXER           5
#define HAIKU_MEDIA_NODE_DECODER         6
#define HAIKU_MEDIA_NODE_ENCODER         7
#define HAIKU_MEDIA_NODE_CONTROL         8

// Haiku media file types
#define HAIKU_MEDIA_FILE_TYPE_AUDIO        1
#define HAIKU_MEDIA_FILE_TYPE_VIDEO        2
#define HAIKU_MEDIA_FILE_TYPE_MIDI         3
#define HAIKU_MEDIA_FILE_TYPE_IMAGE       4
#define HAIKU_MEDIA_FILE_TYPE_MEDIA_CONTAINER 5

// Haiku video frame formats
#define HAIKU_VIDEO_FORMAT_RGB32          1
#define HAIKU_VIDEO_FORMAT_RGBA32         2
#define HAIKU_VIDEO_FORMAT_YUV420          3
#define HAIKU_VIDEO_FORMAT_NV12            4

// ============================================================================
// HAIKU MEDIA KIT DATA STRUCTURES
// ============================================================================

/**
 * Haiku audio buffer information
 */
struct HaikuAudioBuffer {
    uint8_t* data;
    size_t size;
    size_t position;
    uint32_t sample_format;
    uint32_t sample_rate;
    uint32_t channels;
    bool is_looping;
    uint32_t id;
    
    HaikuAudioBuffer() : data(nullptr), size(0), position(0), sample_format(HAIKU_AUDIO_FORMAT_PCM_16),
                   sample_rate(44100), channels(2), is_looping(false), id(0) {}
    
    ~HaikuAudioBuffer() {
        if (data) {
            delete[] data;
        }
    }
    
    bool IsValid() const {
        return data != nullptr && size > 0;
    }
    
    size_t Available() const {
        return size - position;
    }
    
    size_t Remaining() const {
        return size - position;
    }
    
    size_t SamplesAvailable() const {
        size_t bytes_available = Available();
        return bytes_available / (GetBytesPerSample());
    }
    
    size_t GetBytesPerSample() const {
        switch (sample_format) {
            case HAIKU_AUDIO_FORMAT_PCM_8:  return 1;
            case HAIKU_AUDIO_FORMAT_PCM_16: return 2;
            case HAIKU_AUDIO_FORMAT_PCM_24: return 3;
            case HAIKU_AUDIO_FORMAT_PCM_32: return 4;
            case HAIKU_AUDIO_FORMAT_FLOAT_32: return 4;
            default: return 2;
        }
    }
};

/**
 * Haiku sound player information
 */
struct HaikuSoundPlayer {
    HaikuAudioBuffer* buffer;
    uint32_t player_state;
    float volume;
    float pan;
    bool is_playing;
    bool is_paused;
    std::thread playback_thread;
    std::atomic<bool> should_stop_playback;
    std::atomic<bool> has_stopped;
    uint32_t id;
    
    // Player states
    enum {
        PLAYER_STOPPED = 0,
        PLAYER_PLAYING = 1,
        PLAYER_PAUSED = 2,
        PLAYER_STOPPING = 3,
        PLAYER_ERROR = 4
    };
    
    HaikuSoundPlayer() : buffer(nullptr), player_state(PLAYER_STOPPED), volume(1.0f),
                     pan(0.0f), is_playing(false), is_paused(false), 
                     should_stop_playback(false), has_stopped(true), id(0) {}
    
    ~HaikuSoundPlayer();
    
    bool IsValid() const {
        return buffer != nullptr;
    }
    
    bool IsPlaying() const { return is_playing; }
    bool IsPaused() const { return is_paused; }
    bool IsStopped() const { 
        return player_state == PLAYER_STOPPED; 
    }
};

/**
 * Haiku sound recorder information
 */
struct HaikuSoundRecorder {
    HaikuAudioBuffer* buffer;
    uint32_t recorder_state;
    float gain;
    bool is_recording;
    std::thread recording_thread;
    std::atomic<bool> should_stop_recording;
    std::atomic<bool> has_stopped;
    uint32_t id;
    
    // Recorder states
    enum {
        RECORDER_STOPPED = 0,
        RECORDING = 1,
        RECORDER_STOPPING = 2,
        RECORDER_ERROR = 3
    };
    
    HaikuSoundRecorder() : buffer(nullptr), recorder_state(RECORDER_STOPPED), gain(1.0f),
                        is_recording(false), should_stop_recording(false), 
                        has_stopped(true), id(0) {}
    
    ~HaikuSoundRecorder();
    
    bool IsValid() const {
        return buffer != nullptr;
    }
    
    bool IsRecording() const { return is_recording; }
    bool IsStopped() const {
        return recorder_state == RECORDER_STOPPED;
    }
};

/**
 * Haiku video frame information
 */
struct HaikuVideoFrame {
    uint8_t* data;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint64_t timestamp;
    bool is_keyframe;
    uint32_t id;
    
    HaikuVideoFrame() : data(nullptr), width(0), height(0), format(HAIKU_VIDEO_FORMAT_RGB32),
                   timestamp(0), is_keyframe(false), id(0) {}
    
    ~HaikuVideoFrame() {
        if (data) {
            delete[] data;
        }
    }
    
    bool IsValid() const {
        return data != nullptr && width > 0 && height > 0;
    }
    
    size_t GetFrameSize() const {
        return width * height * GetBytesPerPixel();
    }
    
    size_t GetBytesPerPixel() const {
        switch (format) {
            case HAIKU_VIDEO_FORMAT_RGB32: return 4;
            case HAIKU_VIDEO_FORMAT_RGBA32: return 4;
            case HAIKU_VIDEO_FORMAT_YUV420: return 1.5;
            case HAIKU_VIDEO_FORMAT_NV12: return 1.5;
            default: return 4;
        }
    }
};

/**
 * Haiku video buffer information
 */
struct HaikuVideoBuffer {
    std::vector<HaikuVideoFrame*> frames;
    uint32_t current_frame_index;
    bool is_playing;
    uint32_t loop_frame;
    float frame_rate;
    uint32_t id;
    
    HaikuVideoBuffer() : current_frame_index(0), is_playing(false), loop_frame(0),
                   frame_rate(30.0f), id(0) {}
    
    ~HaikuVideoBuffer() {
        for (auto frame : frames) {
            delete frame;
        }
    }
    
    bool IsValid() const {
        return !frames.empty();
    }
    
    HaikuVideoFrame* GetCurrentFrame() const {
        if (current_frame_index < frames.size()) {
            return frames[current_frame_index];
        }
        return nullptr;
    }
    
    size_t GetFrameCount() const {
        return frames.size();
    }
};

/**
 * Haiku media node information
 */
struct HaikuMediaNode {
    std::string name;
    uint32_t node_type;
    uint32_t node_id;
    std::vector<HaikuAudioBuffer*> audio_buffers;
    std::vector<HaikuVideoBuffer*> video_buffers;
    std::map<std::string, std::string> node_properties;
    bool is_active;
    std::vector<uint32_t> connected_inputs;
    std::vector<uint32_t> connected_outputs;
    uint32_t id;
    
    HaikuMediaNode() : node_type(0), node_id(0), is_active(false), id(0) {}
    
    ~HaikuMediaNode() {
        for (auto buffer : audio_buffers) {
            delete buffer;
        }
        for (auto buffer : video_buffers) {
            delete buffer;
        }
    }
    
    bool IsValid() const {
        return !name.empty() && node_type > 0;
    }
    
    bool IsAudioNode() const {
        return node_type == HAIKU_MEDIA_NODE_AUDIO_INPUT || node_type == HAIKU_MEDIA_NODE_AUDIO_OUTPUT || 
               node_type == HAIKU_MEDIA_NODE_MIXER;
    }
    
    bool IsVideoNode() const {
        return node_type == HAIKU_MEDIA_NODE_VIDEO_INPUT || node_type == HAIKU_MEDIA_NODE_VIDEO_OUTPUT ||
               node_type == HAIKU_MEDIA_NODE_DECODER || node_type == HAIKU_MEDIA_NODE_ENCODER;
    }
};

/**
 * Haiku media file information
 */
struct HaikuMediaFile {
    std::string file_path;
    std::string mime_type;
    uint32_t file_type;
    uint64_t file_size;
    std::map<std::string, std::string> metadata;
    bool is_open;
    void* file_handle;
    uint32_t id;
    
    HaikuMediaFile() : file_size(0), file_handle(nullptr), id(0) {}
    
    bool IsValid() const {
        return !file_path.empty() && !mime_type.empty();
    }
};

// ============================================================================
// HAIKU MEDIA KIT INTERFACE
// ============================================================================

/**
 * Haiku Media Kit implementation class
 * 
 * Provides complete Haiku media functionality including:
 * - Audio playback and recording
 * - Video playback and recording
 * - Media node system for routing
 * - File management for audio/video resources
 */
class HaikuMediaKitImpl : public HaikuKit {
private:
    // Media resources
    std::map<uint32_t, HaikuSoundPlayer*> sound_players;
    std::map<uint32_t, HaikuSoundRecorder*> sound_recorders;
    std::map<uint32_t, HaikuMediaNode*> media_nodes;
    std::map<uint32_t, HaikuAudioBuffer*> audio_buffers;
    std::map<uint32_t, HaikuVideoBuffer*> video_buffers;
    std::map<uint32_t, HaikuMediaFile*> media_files;
    
    // Media system state
    bool is_initialized;
    bool audio_system_active;
    bool video_system_active;
    
    // ID management
    uint32_t next_sound_player_id;
    uint32_t next_sound_recorder_id;
    uint32_t next_media_node_id;
    uint32_t next_audio_buffer_id;
    uint32_t next_video_buffer_id;
    uint32_t next_media_file_id;
    
    // Performance statistics
    struct MediaStats {
        uint32_t sound_players_created;
        uint32_t sound_recorders_created;
        uint32_t media_nodes_created;
        uint32_t audio_buffers_created;
        uint32_t video_buffers_created;
        uint32_t media_files_created;
        uint32_t audio_bytes_processed;
        uint32_t video_bytes_processed;
        uint32_t media_operations_performed;
    } media_stats;
    
    // Thread safety
    mutable std::mutex kit_mutex;
    
    // Platform-specific audio/video backends
    std::unique_ptr<class AudioBackend> audio_backend;
    std::unique_ptr<class VideoBackend> video_backend;
    
public:
    /**
     * Constructor
     */
    HaikuMediaKit();
    
    /**
     * Destructor
     */
    virtual         ~HaikuMediaKitImpl();
    
    // HaikuKit interface
    virtual status_t Initialize() override;
    virtual void Shutdown() override;
    
    /**
     * Get singleton instance
     */
    static HaikuMediaKitImpl& GetInstance();
    
    // ========================================================================
    // SOUND PLAYER OPERATIONS
    // ========================================================================
    
    /**
     * Create a new sound player
     */
    virtual uint32_t CreateSoundPlayer(uint32_t sample_format, uint32_t sample_rate, uint32_t channels);
    
    /**
     * Start playing
     */
    virtual status_t StartSoundPlayer(uint32_t player_id);
    
    /**
     * Stop playing
     */
    virtual status_t StopSoundPlayer(uint32_t player_id);
    
    /**
     * Pause playing
     */
    virtual status_t PauseSoundPlayer(uint32_t player_id);
    
    /**
     * Resume playing
     */
    virtual status_t ResumeSoundPlayer(uint32_t player_id);
    
    /**
     * Set player volume (0.0 to 1.0)
     */
    virtual status_t SetSoundPlayerVolume(uint32_t player_id, float volume);
    
    /**
     * Set player panning (-1.0 to 1.0)
     */
    virtual status_t SetSoundPlayerPan(uint32_t player_id, float pan);
    
    /**
     * Set audio buffer for player
     */
    virtual status_t SetSoundPlayerBuffer(uint32_t player_id, uint32_t audio_buffer_id);
    
    /**
     * Check if player is playing
     */
    virtual bool IsSoundPlayerPlaying(uint32_t player_id) const;
    
    /**
     * Get sound player info
     */
    virtual const HaikuSoundPlayer* GetSoundPlayer(uint32_t player_id) const;
    
    /**
     * Delete sound player
     */
    virtual void DeleteSoundPlayer(uint32_t player_id);
    
    // ========================================================================
    // SOUND RECORDER OPERATIONS
    // ========================================================================
    
    /**
     * Create a new sound recorder
     */
    virtual uint32_t CreateSoundRecorder(uint32_t sample_format, uint32_t sample_rate, uint32_t channels);
    
    /**
     * Start recording
     */
    virtual status_t StartSoundRecorder(uint32_t recorder_id);
    
    /**
     * Stop recording
     */
    virtual status_t StopSoundRecorder(uint32_t recorder_id);
    
    /**
     * Set recorder gain (0.0 to 10.0)
     */
    virtual status_t SetSoundRecorderGain(uint32_t recorder_id, float gain);
    
    /**
     * Set audio buffer for recorder
     */
    virtual status_t SetSoundRecorderBuffer(uint32_t recorder_id, uint32_t audio_buffer_id);
    
    /**
     * Check if recorder is recording
     */
    virtual bool IsSoundRecorderRecording(uint32_t recorder_id) const;
    
    /**
     * Get sound recorder info
     */
    virtual const HaikuSoundRecorder* GetSoundRecorder(uint32_t recorder_id) const;
    
    /**
     * Delete sound recorder
     */
    virtual void DeleteSoundRecorder(uint32_t recorder_id);
    
    // ========================================================================
    // AUDIO BUFFER OPERATIONS
    // ========================================================================
    
    /**
     * Create a new audio buffer
     */
    virtual uint32_t CreateAudioBuffer(uint32_t sample_format, uint32_t sample_rate, uint32_t channels, 
                                       size_t initial_capacity = HAIKU_MAX_AUDIO_BUFFER_SIZE);
    
    /**
     * Write audio data to buffer
     */
    virtual status_t WriteToAudioBuffer(uint32_t buffer_id, const void* data, size_t bytes, 
                                       bool wait_for_space = false);
    
    /**
     * Read audio data from buffer
     */
    virtual size_t ReadFromAudioBuffer(uint32_t buffer_id, void* data, size_t max_bytes);
    
    /**
     * Get buffer size
     */
    virtual uint32_t GetAudioBufferSize(uint32_t buffer_id) const;
    
    /**
     * Get buffer position
     */
    virtual uint32_t GetAudioBufferPosition(uint32_t buffer_id) const;
    
    /**
     * Set buffer position
     */
    virtual status_t SetAudioBufferPosition(uint32_t buffer_id, uint32_t position);
    
    /**
     * Get available samples in buffer
     */
    virtual uint32_t GetAvailableSamples(uint32_t buffer_id) const;
    
    /**
     * Get bytes per sample
     */
    virtual uint32_t GetBytesPerSample(uint32_t buffer_id) const;
    
    /**
     * Set buffer looping
     */
    virtual status_t SetAudioBufferLooping(uint32_t buffer_id, bool is_looping);
    
    /**
     * Get audio buffer info
     */
    virtual const HaikuAudioBuffer* GetAudioBuffer(uint32_t buffer_id) const;
    
    /**
     * Delete audio buffer
     */
    virtual void DeleteAudioBuffer(uint32_t buffer_id);
    
    // ========================================================================
    // VIDEO BUFFER OPERATIONS
    // ========================================================================
    
    /**
     * Create a new video buffer
     */
    virtual uint32_t CreateVideoBuffer(uint32_t width, uint32_t height, uint32_t format,
                                       float frame_rate = 30.0f,
                                       bool allocate_frames = true);
    
    /**
     * Add video frame to buffer
     */
    virtual status_t AddVideoFrame(uint32_t video_buffer_id, const uint8_t* frame_data,
                                uint32_t width, uint32_t height, uint32_t format,
                                uint64_t timestamp, bool is_keyframe);
    
    /**
     * Get video frame from buffer
     */
    virtual const HaikuVideoFrame* GetVideoFrame(uint32_t video_buffer_id, uint32_t frame_index);
    
    /**
     * Get current frame index
     */
    virtual uint32_t GetCurrentVideoFrameIndex(uint32_t video_buffer_id) const;
    
    /**
     * Get frame count
     */
    virtual uint32_t GetVideoFrameCount(uint32_t video_buffer_id) const;
    
    /**
     * Set video buffer looping
     */
    virtual status_t SetVideoBufferLooping(uint32_t video_buffer_id, bool loop_frame);
    
    /**
     * Get video frame rate
     */
    virtual float GetVideoFrameRate(uint32_t video_buffer_id) const;
    
    /**
     * Get video buffer info
     */
    virtual const HaikuVideoBuffer* GetVideoBuffer(uint32_t video_buffer_id) const;
    
    /**
     * Delete video buffer
     */
    virtual void DeleteVideoBuffer(uint32_t video_buffer_id);
    
    // ========================================================================
    // MEDIA NODE OPERATIONS
    // ========================================================================
    
    /**
     * Create a new media node
     */
    virtual uint32_t CreateMediaNode(const char* name, uint32_t node_type);
    
    /**
     * Connect media nodes
     */
    virtual status_t ConnectMediaNodes(uint32_t source_node_id, uint32_t destination_node_id);
    
    /**
     * Disconnect media nodes
     */
    virtual status_t DisconnectMediaNodes(uint32_t node1_id, uint32_t node2_id);
    
    /**
     * Add audio buffer to node
     */
    virtual status_t AddAudioBufferToNode(uint32_t node_id, uint32_t audio_buffer_id);
    
    /**
     * Add video buffer to node
     */
    virtual status_t AddVideoBufferToNode(uint32_t node_id, uint32_t video_buffer_id);
    
    /**
     * Set node active state
     */
    virtual status_t SetMediaNodeActive(uint32_t node_id, bool is_active);
    
    /**
     * Set node property
     */
    virtual status_t SetMediaNodeProperty(uint32_t node_id, const char* key, 
                                       const char* value);
    
    /**
     * Get node property
     */
    virtual std::string GetMediaNodeProperty(uint32_t node_id, const char* key);
    
    /**
     * Get media node info
     */
    virtual const HaikuMediaNode* GetMediaNode(uint32_t node_id) const;
    
    /**
     * Delete media node
     */
    virtual void DeleteMediaNode(uint32_t node_id);
    
    // ========================================================================
    // MEDIA FILE OPERATIONS
    // ========================================================================
    
    /**
     * Create a new media file
     */
    virtual uint32_t CreateMediaFile(const char* file_path);
    
    /**
     * Open media file for reading
     */
    virtual status_t OpenMediaFile(uint32_t file_id, uint32_t open_mode);
    
    /**
     * Open media file for writing
     */
    virtual status_t CreateMediaFileForWriting(const char* file_path, const char* mime_type);
    
    /**
     * Close media file
     */
    virtual status_t CloseMediaFile(uint32_t file_id);
    
    /**
     * Get file size
     */
    virtual uint64_t GetMediaFileSize(uint32_t file_id) const;
    
    /**
     * Read from media file
     */
    virtual size_t ReadFromMediaFile(uint32_t file_id, void* buffer, size_t max_size);
    
    /**
     * Write to media file
     */
    virtual size_t WriteToMediaFile(uint32_t file_id, const void* buffer, size_t size,
                                   bool wait_for_space = false);
    
    /**
     * Get media file metadata
     */
    virtual std::string GetMediaFileMetadata(uint32_t file_id, const char* key);
    
    /**
     * Set media file metadata
     */
    virtual status_t SetMediaFileMetadata(uint32_t file_id, const char* key, const char* value);
    
    /**
     * Get media file info
     */
    virtual const HaikuMediaFile* GetMediaFile(uint32_t file_id) const;
    
    /**
     * Delete media file
     */
    virtual void DeleteMediaFile(uint32_t file_id);
    
    // ========================================================================
    // MEDIA OPERATIONS
    // ========================================================================
    
    /**
     * Get media statistics
     */
    virtual void GetMediaStatistics(uint32_t* sound_player_count, uint32_t* sound_recorder_count,
                                      uint32_t* media_node_count, uint32_t* audio_buffer_count,
                                      uint32_t* video_buffer_count, 
                                      uint32_t* media_file_count,
                                      uint32_t* audio_bytes_processed, uint32_t* video_bytes_processed,
                                      uint32_t* media_operations) const;
    
    /**
     * Get detailed media statistics
     */
    virtual void GetDetailedMediaStats(struct MediaStats* stats) const;
    
    /**
     * Dump media state for debugging
     */
    virtual void DumpMediaState() const;
    
    /**
     * Test audio system
     */
    virtual status_t TestAudioSystem();
    
    /**
     * Test video system
     */
    virtual status_t TestVideoSystem();
    
private:
    /**
     * Initialize audio backend
     */
    status_t InitializeAudioBackend();
    
    /**
     * Initialize video backend
     */
    status_t InitializeVideoBackend();
    
    /**
     * Process audio frame in playback
     */
    void ProcessAudioFrame(HaikuAudioBuffer* buffer, HaikuSoundPlayer* player);
    
    /**
     * Process video frame in playback
     */
    void ProcessVideoFrame(HaikuVideoBuffer* buffer, HaikuMediaNode* node);
    
    /**
     * Record audio frame
     */
    void RecordAudioFrame(HaikuAudioBuffer* buffer, HaikuSoundRecorder* recorder);
    
    /**
     * Record video frame
     */
    void RecordVideoFrame(HaikuVideoFrame* frame, HaikuMediaNode* node);
    
    /**
     * Cleanup resources
     */
    void CleanupResources();
    
    /**
     * Platform-specific audio implementation
     */
    class AudioBackend;
    
    /**
     * Platform-specific video implementation
     */
    class VideoBackend;
};