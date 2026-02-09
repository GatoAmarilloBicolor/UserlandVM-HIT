#pragma once

#include "HaikuEmulationFramework.h"
#include <vector>
#include <string>

namespace HaikuEmulation {

/////////////////////////////////////////////////////////////////////////////
// Modular MediaKit - Universal Audio Kit
/////////////////////////////////////////////////////////////////////////////

class ModularMediaKit : public UniversalKit<0x02, "MediaKit", "1.0.0"> {
public:
    // Audio format definitions
    struct AudioFormat {
        float frame_rate;
        uint32_t channel_count;
        uint32_t format;
        uint32_t byte_order;
        size_t buffer_size;
        
        // Format constants
        static constexpr uint32_t FORMAT_INVALID = 0x00000000;
        static constexpr uint32_t FORMAT_U8 = 0x00000001;
        static constexpr uint32_t FORMAT_S16 = 0x00000002;
        static constexpr uint32_t FORMAT_S32 = 0x00000004;
        static constexpr uint32_t FORMAT_FLOAT = 0x00000008;
        
        static constexpr uint32_t ORDER_HOST = 0x00000000;
        static constexpr uint32_t ORDER_BIG_ENDIAN = 0x00000001;
        static constexpr uint32_t ORDER_LITTLE_ENDIAN = 0x00000002;
    };
    
    // Audio buffer
    struct AudioBuffer {
        int32_t buffer_id;
        AudioFormat format;
        void* data;
        size_t size;
        bool locked;
    };
    
    // Audio device info
    struct AudioDevice {
        std::string name;
        std::string manufacturer;
        uint32_t max_channels;
        uint32_t max_sample_rate;
        std::vector<uint32_t> supported_formats;
        bool is_default;
    };
    
    // Syscall numbers
    static constexpr uint32_t SYSCALL_INIT_AUDIO = 0x020001;
    static constexpr uint32_t SYSCALL_CLEANUP_AUDIO = 0x020002;
    static constexpr uint32_t SYSCALL_GET_AUDIO_DEVICES = 0x020003;
    static constexpr uint32_t SYSCALL_SET_AUDIO_DEVICE = 0x020004;
    
    static constexpr uint32_t SYSCALL_CREATE_AUDIO_BUFFER = 0x020100;
    static constexpr uint32_t SYSCALL_DESTROY_AUDIO_BUFFER = 0x020101;
    static constexpr uint32_t SYSCALL_LOCK_AUDIO_BUFFER = 0x020102;
    static constexpr uint32_t SYSCALL_UNLOCK_AUDIO_BUFFER = 0x020103;
    static constexpr uint32_t SYSCALL_GET_AUDIO_BUFFER_DATA = 0x020104;
    
    static constexpr uint32_t SYSCALL_PLAY_AUDIO_BUFFER = 0x020200;
    static constexpr uint32_t SYSCALL_STOP_AUDIO_BUFFER = 0x020201;
    static constexpr uint32_t SYSCALL_PAUSE_AUDIO_BUFFER = 0x020202;
    static constexpr uint32_t SYSCALL_RESUME_AUDIO_BUFFER = 0x020203;
    static constexpr uint32_t SYSCALL_SET_AUDIO_BUFFER_VOLUME = 0x020204;
    static constexpr uint32_t SYSCALL_SET_AUDIO_BUFFER_PAN = 0x020205;
    
    static constexpr uint32_t SYSCALL_START_AUDIO_STREAM = 0x020300;
    static constexpr uint32_t SYSCALL_STOP_AUDIO_STREAM = 0x020301;
    static constexpr uint32_t SYSCALL_WRITE_AUDIO_SAMPLES = 0x020302;
    static constexpr uint32_t SYSCALL_READ_AUDIO_SAMPLES = 0x020303;
    static constexpr uint32_t SYSCALL_SET_MASTER_VOLUME = 0x020304;
    static constexpr uint32_t SYSCALL_GET_MASTER_VOLUME = 0x020305;
    
    // IEmulationKit implementation
    bool Initialize() override;
    void Shutdown() override;
    
    bool HandleSyscall(uint32_t syscall_num, uint32_t* args, uint32_t* result) override;
    std::vector<uint32_t> GetSupportedSyscalls() const override;
    
    // Audio system management
    bool InitAudio();
    void CleanupAudio();
    std::vector<AudioDevice> GetAudioDevices();
    bool SetAudioDevice(const std::string& device_name);
    
    // Audio buffer management
    bool CreateAudioBuffer(const AudioFormat& format, int32_t* buffer_id);
    bool DestroyAudioBuffer(int32_t buffer_id);
    bool LockAudioBuffer(int32_t buffer_id, void** data, size_t* size);
    bool UnlockAudioBuffer(int32_t buffer_id);
    bool GetAudioBufferData(int32_t buffer_id, void** data, size_t* size);
    
    // Audio playback
    bool PlayAudioBuffer(int32_t buffer_id);
    bool StopAudioBuffer(int32_t buffer_id);
    bool PauseAudioBuffer(int32_t buffer_id);
    bool ResumeAudioBuffer(int32_t buffer_id);
    bool SetAudioBufferVolume(int32_t buffer_id, float volume);
    bool SetAudioBufferPan(int32_t buffer_id, float pan);
    
    // Audio streaming
    bool StartAudioStream(const AudioFormat& format);
    bool StopAudioStream();
    bool WriteAudioSamples(const void* samples, size_t count);
    bool ReadAudioSamples(void* samples, size_t count, size_t* samples_read);
    bool SetMasterVolume(float volume);
    float GetMasterVolume();
    
    // State management
    bool SaveState(void** data, size_t* size) override;
    bool LoadState(const void* data, size_t size) override;
    
    // Audio processing
    bool EnableAudioProcessing(bool enable);
    bool IsAudioProcessingEnabled() const;
    bool SetAudioProcessingChain(const std::vector<std::string>& processors);
    
    // Real-time audio
    bool EnableRealTimeAudio(bool enable);
    bool IsRealTimeAudioEnabled() const;
    bool SetRealTimePriority(bool high_priority);

private:
    // Audio system state
    bool audio_initialized;
    std::string current_device;
    std::vector<AudioDevice> available_devices;
    
    // Audio buffers
    std::map<int32_t, AudioBuffer> audio_buffers;
    int32_t next_buffer_id;
    
    // Audio streaming
    AudioFormat stream_format;
    bool streaming_active;
    float master_volume;
    
    // Native Haiku integration
    void* native_sound_player;
    void* native_media_roster;
    std::map<int32_t, void*> native_buffers;
    
    // Audio processing
    bool audio_processing_enabled;
    std::vector<std::string> audio_processors;
    
    // Real-time audio
    bool real_time_audio_enabled;
    bool real_time_priority;
    
    // Internal methods
    void InitializeNativeHaiku();
    void InitializeAudioProcessing();
    void InitializeRealTimeAudio();
    void CleanupNativeHaiku();
    void CleanupAudioProcessing();
    void CleanupRealTimeAudio();
    
    // Audio device discovery
    void DiscoverAudioDevices();
    bool ProbeAudioDevice(const std::string& device_name, AudioDevice& device);
    
    // Audio format conversion
    bool ConvertAudioFormat(const AudioFormat& src, const AudioFormat& dest, 
                           const void* src_data, void* dest_data, size_t frames);
    
    // Configuration hooks
    bool OnConfigure(const std::map<std::string, std::string>& config) override;
    
    // Audio processing chain
    bool ProcessAudioSamples(void* samples, size_t count, const AudioFormat& format);
};

/////////////////////////////////////////////////////////////////////////////
// Auto-registration
/////////////////////////////////////////////////////////////////////////////

HAIKU_REGISTER_KIT(ModularMediaKit);

} // namespace HaikuEmulation