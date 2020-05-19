// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "api/call/audio_sink.h"
#include "common_audio/resampler/include/resampler.h"

#include "export.h"
#include "refptr.h"

enum class mrsAudioTrackReadBufferPadBehavior;

namespace Microsoft {
namespace MixedReality {
namespace WebRTC {

struct AudioFrame;
class PeerConnection;

/// Implementation of |mrsAudioTrackReadBufferHandle|.
class AudioTrackReadBuffer : public webrtc::AudioTrackSinkInterface {
 public:
  /// Create a new stream which buffers |bufferMs| milliseconds of audio.
  /// WebRTC delivers audio at 10ms intervals so pass a multiple of 10.
  AudioTrackReadBuffer(rtc::scoped_refptr<webrtc::AudioTrackInterface> track,
                       int bufferMs = 500);

  /// Destructs the stream.
  ~AudioTrackReadBuffer();

  /// See |mrsAudioTrackReadBufferRead|.
  void Read(int sample_rate,
            int num_channels,
            mrsAudioTrackReadBufferPadBehavior pad_behavior,
            float data_out[],
            int* data_len_in_out,
            bool* has_overrun_out) noexcept;

  /// AudioTrackSinkInterface implementation.
  virtual void OnData(const void* audio_data,
                      int bits_per_sample,
                      int sample_rate,
                      size_t number_of_channels,
                      size_t number_of_frames);


 private:
  const rtc::scoped_refptr<webrtc::AudioTrackInterface> track_;
  struct Frame {
    std::vector<std::uint8_t> audio_data;
    uint32_t bits_per_sample;
    uint32_t sample_rate;
    uint32_t number_of_channels;
    uint32_t number_of_frames;
  };
  // Incoming frames received from webrtc - see also buffer_
  std::deque<Frame> frames_;
  // protects frames_ and has_overrun_ in Read() and audioFrameCallback()
  std::mutex frames_mutex_;
  // max ms of audio data stored in frames_
  int buffer_ms_{};
  // for debugging, we emit a sin on underrun.
  int sinwave_iter_{};
  // Have frames been dropped due to overrun after last call to Read()?
  bool has_overrun_{};

  // Outgoing data resamples to f32 format
  struct Buffer {
    std::unique_ptr<webrtc::Resampler> resampler_ = nullptr;
    std::vector<float> data_;
    int used_ = 0;
    int channels_ = 0;
    int rate_ = 0;

    Buffer();
    ~Buffer();
    int available() const { return (int)data_.size() - used_; }
    int readSome(float* dst, int dstLen) {
      int take = std::min(available(), dstLen);
      memcpy(dst, data_.data() + used_, take * sizeof(float));
      used_ += take;
      return take;
    }
    // Extract/resample data from frame and add it to our buffer.
    void addFrame(const Frame& frame, int dstSampleRate, int dstChannels);
  };
  // Only accessed from callers of Read - no locking needed.
  Buffer buffer_;
};
}  // namespace WebRTC
}  // namespace MixedReality
}  // namespace Microsoft
