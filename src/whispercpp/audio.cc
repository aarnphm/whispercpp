#include "audio.h"
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

namespace whisper {
PYBIND11_MODULE(audio_cpp2py_export, m) {
  m.doc() = "Experimental: Audio capture API";
  ExportAudioApi(m);
}

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

bool AudioCapture::init_device(int capture_id, int sample_rate) {
  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to initialized SDL: %s\n", SDL_GetError());
    return false;
  }

  SDL_SetHintWithPriority(SDL_HINT_AUDIO_RESAMPLING_MODE, "medium",
                          SDL_HINT_OVERRIDE);

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
    capture->callback(stream, len);
  };
  capture_spec_desired.userdata = this;

  if (capture_id >= 0) {
    // Using the given open device
    fprintf(stderr, "\n%s: Using device: '%s' ...\n", __func__,
            SDL_GetAudioDeviceName(capture_id, SDL_TRUE));
    m_dev_id = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(capture_id, SDL_TRUE),
                                   SDL_TRUE, &capture_spec_desired,
                                   &capture_spec_obtained, 0);
  } else {
    // Using the default device set by system if capture_id == 0
    fprintf(stderr, "\n:%s: Using default device...\n", __func__);
    m_dev_id = SDL_OpenAudioDevice(nullptr, SDL_TRUE, &capture_spec_desired,
                                   &capture_spec_obtained, 0);
  }

  if (!m_dev_id) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "\n%s: Failed to open audio device: %s\n", __func__,
                 SDL_GetError());
    m_dev_id = 0;
    return false;
  } else {
    fprintf(stderr, "\nOpened audio device: (id=%d, name=%s)\n", m_dev_id,
            SDL_GetAudioDeviceName(capture_id, SDL_TRUE));
    fprintf(stderr, "  - sample_rate: %d\n", capture_spec_obtained.freq);
    fprintf(stderr, "  - format: %d (required: %d)\n",
            capture_spec_obtained.format, capture_spec_desired.format);
    fprintf(stderr, "  - channels: %d (required: %d)\n",
            capture_spec_obtained.channels, capture_spec_desired.channels);
    fprintf(stderr, "  - samples per frame: %d\n\n",
            capture_spec_obtained.samples);
    printf("=====================================\n");
    printf("=== Transcription starting now... ===\n");
    printf("=====================================\n\n");
  }

  m_sample_rate = capture_spec_obtained.freq;

  m_audio.resize((m_sample_rate * m_length_ms) / 1000);

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
    // Reset current position
    m_audio_pos = 0;
    m_audio_len = 0;
  }

  return true;
};

void AudioCapture::callback(uint8_t *stream, int len) {
  if (!m_running) {
    return;
  }

  const size_t num_samples = len / sizeof(float);

  m_audio_new.resize(num_samples);
  memcpy(m_audio_new.data(), stream, num_samples * sizeof(float));
  // fprintf(stderr, "%zu samples, pos %zu, len %zu\n", num_samples,
  // m_audio_pos, m_audio_len);

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
      ms = m_length_ms;
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

int AudioCapture::stream_transcribe(Context *ctx, Params *params,
                                    int32_t step_ms) {
  // very experiemental

  // START: DEFAULT PARAMS
  if (!step_ms) {
    step_ms = 3000;
  }

  int32_t length_ms = 10000; // 10 sec
  int32_t keep_ms = 200;

  float vad_thold = 0.6f;
  float freq_thold = 100.0f;

  bool no_context = true;
  // END: DEFAULT PARAMS

  keep_ms = std::min(keep_ms, step_ms);
  length_ms = std::max(length_ms, step_ms);

  const int num_samples_step = (1e-3 * step_ms) * WHISPER_SAMPLE_RATE;
  const int num_samples_length = (1e-3 * length_ms) * WHISPER_SAMPLE_RATE;
  const int num_samples_keep = (1e-3 * keep_ms) * WHISPER_SAMPLE_RATE;
  const int num_samples_30s = (1e-3 * 30000) * WHISPER_SAMPLE_RATE;

  const bool use_vad = num_samples_step <= 0; // sliding window mode uses VAD

  this->resume();

  std::vector<float> pcmf32(num_samples_30s, 0.0f);
  std::vector<float> pcmf32_old;
  std::vector<float> pcmf32_new(num_samples_30s, 0.0f);

  bool is_running = true;

  auto time_last = std::chrono::high_resolution_clock::now();

  while (is_running) {
    is_running = sdl_poll_events();

    if (!is_running) {
      fprintf(stderr, "Exiting ...\n");
      break;
    }

    if (PyErr_CheckSignals() != 0) {
      fprintf(stderr, "\n\nCaught Ctrl-C. Exiting ...\n");
      this->pause();
      ctx->print_timings();
      printf("\n");
      ctx->free();
      throw py::error_already_set();
    }

    // process new audio
    if (!use_vad) {
      while (true) {

        this->retrieve_audio(step_ms, pcmf32_new);

        if ((int)pcmf32_new.size() > 2 * num_samples_step) {
          fprintf(stderr,
                  "\n\n%s: WARNING: cannot process audio fast "
                  "enough, dropping "
                  "audio ...\n\n",
                  __func__);
          this->clear();
          continue;
        }

        if ((int)pcmf32_new.size() >= num_samples_step) {
          this->clear();
          break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      const int num_samples_new = pcmf32_new.size();

      // take length_ms audio from prev iteration
      const int num_samples_take = std::min(
          (int)pcmf32_old.size(),
          std::max(0, num_samples_keep + num_samples_length - num_samples_new));

      // printf("processing: take = %d, new = %d, old = %d\n",
      // num_samples_take,
      //        num_samples_new, (int)pcmf32_old.size());

      pcmf32.resize(num_samples_new + num_samples_take);

      for (int i = 0; i < num_samples_take; i++) {
        pcmf32[i] = pcmf32_old[pcmf32_old.size() - num_samples_take + i];
      }

      memcpy(pcmf32.data() + num_samples_take, pcmf32_new.data(),
             num_samples_new * sizeof(float));

      pcmf32_old = pcmf32_new;
    } else {
      const auto time_now = std::chrono::high_resolution_clock::now();
      const auto time_diff =
          std::chrono::duration_cast<std::chrono::milliseconds>(time_now -
                                                                time_last)
              .count();

      if (time_diff < 2000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }

      this->retrieve_audio(2000, pcmf32_new);

      if (::vad_simple(pcmf32_new, WHISPER_SAMPLE_RATE, 1000, vad_thold,
                       freq_thold, false)) {
        this->retrieve_audio(length_ms, pcmf32);
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }

      time_last = time_now;
    }

    // Running inference
    {
      params->with_print_progress(false)
          ->with_print_realtime(false)
          ->with_no_context(no_context)
          ->with_single_segment(!use_vad)
          // disable temperature fallback
          ->with_temperature_inc(-1.0f);

      if (ctx->full(*params, pcmf32) != 0) {
        fprintf(stderr, "%s: Failed to process audio!\n", __func__);
        return 6;
      }

      {
        const int num_segments = ctx->full_n_segments();
        for (int i = 0; i < num_segments; ++i) {
          const char *text = ctx->full_get_segment_text(i);
          m_transcript.push_back(text);
          fprintf(stderr, "%s", text);
        }
      }
    }
  }

  this->pause();
  ctx->print_timings();
  printf("\n");
  ctx->free();

  return 0;
}
} // namespace whisper

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

void ExportAudioApi(py::module &m) {
  m.def("sdl_poll_events", &sdl_poll_events, "Poll SDL events");
  py::class_<whisper::AudioCapture>(m, "AudioCapture")
      .def(py::init<int>())
      .def("init_device", &whisper::AudioCapture::init_device,
           "device_id"_a = -1, "sample_rate"_a = WHISPER_SAMPLE_RATE)
      .def_static("list_available_devices",
                  &whisper::AudioCapture::list_available_devices)
      .def_property_readonly(
          "transcript",
          [](whisper::AudioCapture &self) { return self.m_transcript; })
      .def(
          "stream_transcribe",
          [](whisper::AudioCapture &self, Context context, Params params,
             int32_t step_ms) {
            self.stream_transcribe(&context, &params, step_ms);
            return py::make_iterator(self.m_transcript.begin(),
                                     self.m_transcript.end());
          },
          py::keep_alive<0, 1>())
      .def("resume", &whisper::AudioCapture::resume)
      .def("pause", &whisper::AudioCapture::pause)
      .def("clear", &whisper::AudioCapture::clear);
};
