#include "helpers.h"
#include "whisper.h"
using namespace pybind11::literals;

namespace whisper {
AudioCapture::~AudioCapture() {
  if (m_dev_id) {
    SDL_CloseAudioDevice(m_dev_id);
  }
}

std::vector<int> AudioCapture::list_available_devices() {
  std::vector<int> device_ids;
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to initialized SDL: %s\n", SDL_GetError());
    return device_ids;
  }

  int num_devices = SDL_GetNumAudioDevices(SDL_TRUE);
  fprintf(stderr, "Found %d audio capture devices:\n", num_devices);
  for (int i = 0; i < num_devices; ++i) {
    fprintf(stderr, "  - Device id %d: '%s'\n", i,
            SDL_GetAudioDeviceName(i, SDL_TRUE));
    device_ids.push_back(i);
  }
  return device_ids;
};

bool AudioCapture::init(int capture_id, int sample_rate) {

  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to initialized SDL: %s\n", SDL_GetError());
    return false;
  }

  SDL_AudioSpec capture_spec_desired;
  SDL_AudioSpec capture_spec_obtained;

  SDL_zero(capture_spec_desired);
  SDL_zero(capture_spec_obtained);

  capture_spec_desired.freq = sample_rate;
  capture_spec_desired.format = AUDIO_F32;
  capture_spec_desired.channels = 1;
  capture_spec_desired.samples = 1024;
  capture_spec_desired.callback = [](void *userdata, uint8_t *stream, int len) {
    AudioCapture *capture = (AudioCapture *)userdata;
    capture->sdl_callback(stream, len);
  };
  capture_spec_desired.userdata = this;

  if (capture_id >= 0) {
    // Using the given open device
    fprintf(stderr, "Using device %d...\n", capture_id);
    m_dev_id = SDL_OpenAudioDevice(nullptr, SDL_TRUE, &capture_spec_desired,
                                   &capture_spec_obtained, 0);
  } else {
    // Using the default device set by system if capture_id == 0
    fprintf(stderr, "Using default device...\n");
    m_dev_id = SDL_OpenAudioDevice(nullptr, SDL_TRUE, &capture_spec_desired,
                                   &capture_spec_obtained, 0);
  }

  if (!m_dev_id) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to open audio device: %s\n", SDL_GetError());
    m_dev_id = 0;
    return false;
  } else {
    fprintf(stderr, "Opened audio device: (id=%d, name=%s)\n", m_dev_id,
            SDL_GetAudioDeviceName(m_dev_id, SDL_TRUE));
    fprintf(stderr, "- sample_rate: %d\n", capture_spec_obtained.freq);
    fprintf(stderr, "- format: %d (required: %d)\n",
            capture_spec_obtained.format, capture_spec_desired.format);
    fprintf(stderr, "- channels: %d (required: %d)\n",
            capture_spec_obtained.channels, capture_spec_desired.channels);
    fprintf(stderr, "- samples/frame: %d\n", capture_spec_obtained.samples);
  }

  m_sample_rate = capture_spec_obtained.freq;
  m_audio.resize((m_sample_rate * m_len_ms) / 1000);
  return true;
};

bool AudioCapture::resume() {
  if (!m_dev_id) {
    fprintf(stderr, "Failed to resume because there is no audio device to!\n");
    return false;
  }
  if (m_running) {
    fprintf(stderr, "Already running!\n");
    return false;
  }

  SDL_PauseAudioDevice(m_dev_id, 0);
  m_running = true;
  return true;
};

bool AudioCapture::pause() {
  if (!m_dev_id) {
    fprintf(stderr, "Failed to pause because there is no audio device to!\n");
    return false;
  }
  if (!m_running) {
    fprintf(stderr, "Already paused!\n");
    return false;
  }

  SDL_PauseAudioDevice(m_dev_id, 1);
  m_running = false;
  return true;
};

bool AudioCapture::clear() {
  if (!m_dev_id) {
    fprintf(stderr, "Failed to clear because there is no audio device to!\n");
    return false;
  }
  if (!m_running) {
    fprintf(stderr,
            "Failed to clear because the audio device is not running!\n");
    return false;
  }

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    // ??? Is this correct
    m_audio_pos = 0;
    m_audio_len = 0;
  }

  return true;
};

void AudioCapture::sdl_callback(uint8_t *stream, int len) {
  if (!m_running) {
    return;
  }

  const size_t num_samples = len / sizeof(float);

  m_audio_new.resize(num_samples);
  memcpy(m_audio_new.data(), stream, num_samples * sizeof(float));
  fprintf(stderr, "%zu samples, pos %zu, len %zu\n", num_samples, m_audio_pos,
          m_audio_len);

  // We want to cycle through the audio buffer, so create a mutex here
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_audio_pos + num_samples > m_audio.size()) {
      const size_t n0 = m_audio.size() - m_audio_pos;
      memcpy(&m_audio[m_audio_pos], stream, n0 * sizeof(float));
      memcpy(&m_audio[0], &stream[n0], (num_samples - n0) * sizeof(float));
      m_audio_pos = (m_audio_pos + num_samples) % m_audio.size();
      m_audio_len = m_audio.size();
    } else {
      memcpy(&m_audio[m_audio_pos], stream, num_samples * sizeof(float));
      m_audio_pos = (m_audio_pos + num_samples) % m_audio.size();
      m_audio_len = std::min(m_audio_len + num_samples, m_audio.size());
    }
  }
};

void AudioCapture::retrieve_audio(int ms, std::vector<float> &audio) {
  if (!m_dev_id) {
    fprintf(stderr,
            "Failed to retrieve audio because there is no audio device");
    return;
  }

  if (!m_running) {
    fprintf(stderr,
            "Failed to retrieve audio because the audio device is not running");
    return;
  }

  audio.clear();

  {
    std::lock_guard<std::mutex> mutex(m_mutex);
    if (ms <= 0) {
      ms = m_len_ms;
    }
    size_t num_samples = (m_sample_rate * ms) / 1000;
    if (num_samples > m_audio_len) {
      num_samples = m_audio_len;
    }

    audio.resize(num_samples);

    int s0 = m_audio_pos - num_samples;
    if (s0 < 0) {
      s0 += m_audio.size();
    }

    if (s0 + num_samples > m_audio.size()) {
      const size_t n0 = m_audio.size() - s0;
      memcpy(audio.data(), &m_audio[s0], n0 * sizeof(float));
      memcpy(&audio[n0], &m_audio[0], (num_samples - n0) * sizeof(float));
    } else {
      memcpy(audio.data(), &m_audio[s0], num_samples * sizeof(float));
    }
  }
}

bool sdl_poll_events() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT: {
      return false;
    } break;
    default:
      break;
    }
  }
  return true;
}

} // namespace whisper

void ExportHelpersApi(py::module &m) {
  m.def("sdl_poll_events", &whisper::sdl_poll_events, "Poll SDL events");
  py::class_<whisper::AudioCapture>(m, "AudioCapture")
      .def(py::init<int>())
      .def("init", &whisper::AudioCapture::init, "capture_id"_a = -1,
           "sample_rate"_a = WHISPER_SAMPLE_RATE)
      .def("list_available_devices",
           &whisper::AudioCapture::list_available_devices)
      .def("resume", &whisper::AudioCapture::resume)
      .def("pause", &whisper::AudioCapture::pause)
      .def("clear", &whisper::AudioCapture::clear)
      .def("retrieve_audio", &whisper::AudioCapture::retrieve_audio, "ms"_a,
           "audio"_a);
};
