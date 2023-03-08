#pragma once

#ifdef BAZEL_BUILD
#include "SDL.h"
#include "SDL_audio.h"
#include "examples/common.h"
#include "pybind11/functional.h"
#include "pybind11/numpy.h"
#else
#include "SDL.h"
#include "SDL_audio.h"
#include "examples/common.h"
#include "pybind11/functional.h"
#include "pybind11/numpy.h"
#endif

#include "context.h"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

// Using SDL as audio capture
// requires SDL2. See https://www.libsdl.org/
namespace py = pybind11;

namespace whisper {
class AudioCapture {
public:
  AudioCapture(int length_ms) {
    m_length_ms = length_ms;
    m_running = false;
  };
  ~AudioCapture();

  bool init_device(int capture_id, int sample_rate);

  static std::vector<int> list_available_devices();

  // Needs to keep the last len_ms of audio in circular buffer
  bool resume();
  bool pause();
  bool clear();

  // implement a SDL callback
  void callback(uint8_t *stream, int len);

  // retrieve audio data from the buffer
  void retrieve_audio(int ms, std::vector<float> &audio);

  int stream_transcribe(Context *ctx, Params *params, int32_t step_ms);

  std::vector<std::string> m_transcript;

private:
  // Default device
  SDL_AudioDeviceID m_dev_id = 0;

  int m_length_ms = 0;
  int m_sample_rate = 0;

  std::atomic_bool m_running;
  std::mutex m_mutex;

  std::vector<float> m_audio;
  std::vector<float> m_audio_new;
  size_t m_audio_pos = 0;
  size_t m_audio_len = 0;
};

} // namespace whisper

// Return false if need to quit
bool sdl_poll_events();

void ExportAudioApi(py::module &m);
