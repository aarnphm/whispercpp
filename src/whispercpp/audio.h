#pragma once

#ifdef BAZEL_BUILD
#include "context.h"
#include "examples/common.h"
#include "pybind11/pybind11.h"
#include <SDL.h>
#include <SDL_audio.h>
#include <pybind11/numpy.h>
#else
#include "common.h"
#include "context.h"
#include "pybind11/pybind11.h"
#include <SDL.h>
#include <SDL_audio.h>
#include <pybind11/numpy.h>
#endif

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

// Using SDL as audio capture
// requires SDL2. See https://www.libsdl.org/
namespace py = pybind11;

namespace whisper {

// std::make_unique for C++11
// https://stackoverflow.com/a/17902439/8643197
template <class T> struct _Unique_if {
  typedef std::unique_ptr<T> _Single_object;
};

template <class T> struct _Unique_if<T[]> {
  typedef std::unique_ptr<T[]> _Unknown_bound;
};

template <class T, size_t N> struct _Unique_if<T[N]> {
  typedef void _Known_bound;
};

template <class T, class... Args>
typename _Unique_if<T>::_Single_object make_unique(Args &&...args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T>
typename _Unique_if<T>::_Unknown_bound make_unique(size_t n) {
  typedef typename std::remove_extent<T>::type U;
  return std::unique_ptr<T>(new U[n]());
}

template <class T, class... Args>
typename _Unique_if<T>::_Known_bound make_unique(Args &&...) = delete;

// Some black magic to make zero-copy numpy array
// See https://github.com/pybind/pybind11/issues/1042#issuecomment-642215028
template <typename Sequence>
inline py::array_t<typename Sequence::value_type> as_pyarray(Sequence &&seq) {
  auto size = seq.size();
  auto data = seq.data();
  std::unique_ptr<Sequence> seq_ptr =
      whisper::make_unique<Sequence>(std::move(seq));
  auto capsule = py::capsule(seq_ptr.get(), [](void *p) {
    std::unique_ptr<Sequence>(reinterpret_cast<Sequence *>(p));
  });
  seq_ptr.release();
  return py::array(size, data, capsule);
}

class AudioCapture {
public:
  AudioCapture(int length_ms) {
    m_length_ms = length_ms;
    m_running = false;
  };
  ~AudioCapture();

  bool init_device(int device_id, int sample_rate);

  static std::vector<int> list_available_devices();

  // Needs to keep the last len_ms of audio in circular buffer
  bool resume();
  bool pause();
  bool clear();

  // implement a SDL callback
  void sdl_callback(uint8_t *stream, int len);

  // retrieve audio data from the buffer
  void retrieve_audio(int ms, std::vector<float> &audio);

  int stream_transcribe(Context *ctx, Params *params);

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

void ExportCaptureApi(py::module &m);
