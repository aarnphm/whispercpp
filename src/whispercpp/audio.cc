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
    capture_spec_desired.callback = [](void *userdata, uint8_t *stream,
                                       int len) {
        AudioCapture *capture = (AudioCapture *)userdata;
        capture->callback(stream, len);
    };
    capture_spec_desired.userdata = this;

    if (capture_id >= 0) {
        // Using the given open device
        fprintf(stderr, "\n%s: Using device: '%s' ...\n", __func__,
                SDL_GetAudioDeviceName(capture_id, SDL_TRUE));
        m_dev_id = SDL_OpenAudioDevice(
            SDL_GetAudioDeviceName(capture_id, SDL_TRUE), SDL_TRUE,
            &capture_spec_desired, &capture_spec_obtained, 0);
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
    }

    m_sample_rate = capture_spec_obtained.freq;

    m_audio.resize((m_sample_rate * m_length_ms) / 1000);

    return true;
};

bool AudioCapture::resume() {
    if (!m_dev_id) {
        fprintf(stderr,
                "Failed to resume because there is no audio device to!\n");
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
        fprintf(stderr,
                "Failed to pause because there is no audio device to!\n");
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
        fprintf(stderr,
                "Failed to clear because there is no audio device to!\n");
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
            memcpy(&m_audio[0], &stream[n0],
                   (num_samples - n0) * sizeof(float));

            m_audio_pos = (m_audio_pos + num_samples) % m_audio.size();
            m_audio_len = m_audio.size();
        } else {
            memcpy(&m_audio[m_audio_pos], stream, num_samples * sizeof(float));

            m_audio_pos = (m_audio_pos + num_samples) % m_audio.size();
            m_audio_len = std::min(m_audio_len + num_samples, m_audio.size());
        }
    }
};

void AudioCapture::get(int ms, std::vector<float> &audio) {
    if (!m_dev_id) {
        fprintf(stderr,
                "Failed to retrieve audio because there is no audio device");
        return;
    }

    if (!m_running) {
        fprintf(
            stderr,
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

// excerpt from stream.cpp
struct whisper_default_params {
    // clang-format off
  int32_t n_threads    = std::min(4, (int32_t)std::thread::hardware_concurrency());
  int32_t step_ms      = 3000;
  int32_t length_ms    = 10000;
  int32_t keep_ms      = 200;
  int32_t capture_id   = -1;
  int32_t max_tokens   = 32;
  int32_t audio_ctx    = 0;
  float vad_thold      = 0.6f;
  float freq_thold     = 100.0f;
  bool speed_up        = false;
  bool translate       = false;
  bool print_special   = false;
  bool no_context      = true;
  bool no_timestamps   = true;
  std::string language = "en";
    // clang-format on
};

//  500 -> 00:05.000
// 6000 -> 01:00.000
std::string to_timestamp(int64_t t) {
    int64_t sec = t / 100;
    int64_t msec = t - sec * 100;
    int64_t min = sec / 60;
    sec = sec - min * 60;

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d.%03d", (int)min, (int)sec, (int)msec);

    return std::string(buf);
}

#define KWARGS_OR_DEFAULT(type, value)                                         \
    do {                                                                       \
        if (kwargs.contains(#value)) {                                         \
            wparams.value = kwargs[#value].cast<type>();                       \
        }                                                                      \
    } while (0)

int AudioCapture::stream_transcribe(Context *ctx, Params *params,
                                    const py::kwargs &kwargs) {
    // very experiemental
    whisper_default_params wparams;

    // START: DEFAULT PARAMS
    KWARGS_OR_DEFAULT(int32_t, n_threads);
    KWARGS_OR_DEFAULT(int32_t, step_ms);
    KWARGS_OR_DEFAULT(int32_t, length_ms);
    KWARGS_OR_DEFAULT(int32_t, keep_ms);
    KWARGS_OR_DEFAULT(int32_t, capture_id);
    KWARGS_OR_DEFAULT(int32_t, max_tokens);
    KWARGS_OR_DEFAULT(int32_t, audio_ctx);
    KWARGS_OR_DEFAULT(float, vad_thold);
    KWARGS_OR_DEFAULT(float, freq_thold);
    KWARGS_OR_DEFAULT(bool, speed_up);
    KWARGS_OR_DEFAULT(bool, translate);
    KWARGS_OR_DEFAULT(bool, print_special);
    KWARGS_OR_DEFAULT(bool, no_context);
    KWARGS_OR_DEFAULT(bool, no_timestamps);
    KWARGS_OR_DEFAULT(std::string, language);
    // END: DEFAULT PARAMS

    wparams.keep_ms = std::min(wparams.keep_ms, wparams.step_ms);
    wparams.length_ms = std::max(wparams.length_ms, wparams.step_ms);

    // clang-format off
  const int num_samples_step   = (1e-3 * wparams.step_ms) * WHISPER_SAMPLE_RATE;
  const int num_samples_length = (1e-3 * wparams.length_ms) * WHISPER_SAMPLE_RATE;
  const int num_samples_keep   = (1e-3 * wparams.keep_ms) * WHISPER_SAMPLE_RATE;
  const int num_samples_30s    = (1e-3 * 30000) * WHISPER_SAMPLE_RATE;
    // clang-format on

    const bool use_vad = num_samples_step <= 0; // sliding window mode uses VAD

    // number of steps to print new lines.
    const int n_new_line =
        !use_vad ? std::max(1, wparams.length_ms / wparams.step_ms - 1) : 1;

    wparams.no_timestamps = !use_vad;
    wparams.no_context |= use_vad;

    this->resume();

    // Check if language is valid
    ctx->lang_str_to_id(wparams.language.c_str());

    std::vector<float> pcmf32(num_samples_30s, 0.0f);
    std::vector<float> pcmf32_old;
    std::vector<float> pcmf32_new(num_samples_30s, 0.0f);

    std::vector<whisper_token> prompt_tokens;

    int n_iter = 0;
    bool is_running = true;

    // START
    {
        fprintf(stderr, "\n");
        if (!ctx->is_multilingual()) {
            if (wparams.language != "en" || wparams.translate) {
                wparams.language = "en";
                wparams.translate = false;
                fprintf(stderr,
                        "%s: WARNING: model is not multilingual, ignoring "
                        "language and "
                        "translation options\n",
                        __func__);
            }
        }
        fprintf(
            stderr,
            "%s: processing %d samples (step = %.1f sec / len = %.1f sec / "
            "keep = %.1f sec), %d threads, lang = %s, task = %s, timestamps "
            "= %d ...\n",
            __func__, num_samples_step,
            float(num_samples_step) / WHISPER_SAMPLE_RATE,
            float(num_samples_length) / WHISPER_SAMPLE_RATE,
            float(num_samples_keep) / WHISPER_SAMPLE_RATE, wparams.n_threads,
            wparams.language.c_str(),
            wparams.translate ? "translate" : "transcribe",
            wparams.no_timestamps ? 0 : 1);

        if (!use_vad) {
            fprintf(stderr, "%s: n_new_line = %d, no_context = %d\n", __func__,
                    n_new_line, wparams.no_context);
        } else {
            fprintf(stderr,
                    "%s: using VAD, will transcribe on speech activity\n",
                    __func__);
        }

        fprintf(stderr, "\n");
    }
    fprintf(stderr, "=====================================\n");
    fprintf(stderr, "=== Transcription starting now... ===\n");
    fprintf(stderr, "=====================================\n\n");

    fflush(stdout);

    auto time_last = std::chrono::high_resolution_clock::now();
    const auto time_start = time_last;

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
                this->get(wparams.step_ms, pcmf32_new);

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
            const int num_samples_take =
                std::min((int)pcmf32_old.size(),
                         std::max(0, num_samples_keep + num_samples_length -
                                         num_samples_new));

            // printf("processing: take = %d, new = %d, old = %d\n",
            // num_samples_take,
            //        num_samples_new, (int)pcmf32_old.size());

            pcmf32.resize(num_samples_new + num_samples_take);

            for (int i = 0; i < num_samples_take; i++) {
                pcmf32[i] =
                    pcmf32_old[pcmf32_old.size() - num_samples_take + i];
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

            this->get(2000, pcmf32_new);

            if (::vad_simple(pcmf32_new, WHISPER_SAMPLE_RATE, 1000,
                             wparams.vad_thold, wparams.freq_thold, false)) {
                this->get(wparams.length_ms, pcmf32);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            time_last = time_now;
        }

        // Running inference
        {
            // clang-format off
      params->with_print_progress(false)
          ->with_print_special(wparams.print_special)
          ->with_print_realtime(false)
          ->with_print_timestamps(!wparams.no_timestamps)
          ->with_translate(wparams.translate)
          ->with_single_segment(!use_vad)
          ->with_max_tokens(wparams.max_tokens)
          ->with_language(wparams.language)
          ->with_n_threads(wparams.n_threads)
          ->with_audio_ctx(wparams.audio_ctx)
          ->with_speed_up(wparams.speed_up)
          ->with_no_context(wparams.no_context)
          ->with_single_segment(!use_vad)
          ->with_temperature_inc(-1.0f) // disable temperature fallback
          ->with_prompt_tokens(wparams.no_context   ? nullptr : prompt_tokens.data())
          ->with_prompt_n_tokens(wparams.no_context ? 0       : prompt_tokens.size());
            // clang-format on

            if (!ctx->is_init_with_state()) {
                ctx->init_state();
            }

            if (ctx->full(*params, pcmf32) != 0) {
                fprintf(stderr, "%s: Failed to process audio!\n", __func__);
                return 6;
            }

            // print results
            {
                if (!use_vad) {
                    fprintf(stderr, "\33[2K\r");

                    // print long empty line to clear the previous line
                    fprintf(stderr, "%s", std::string(100, ' ').c_str());

                    fprintf(stderr, "\33[2K\r");
                } else {
                    const int64_t t1 =
                        (time_last - time_start).count() / 1000000;
                    const int64_t t0 = std::max(
                        0.0, t1 - pcmf32.size() * 1000.0 / WHISPER_SAMPLE_RATE);

                    fprintf(stderr, "\n");
                    fprintf(stderr,
                            "### Transcription %d START | t0 = %d ms | t1 = %d "
                            "ms\n",
                            n_iter, (int)t0, (int)t1);
                    fprintf(stderr, "\n");
                }

                const int n_segments = ctx->full_n_segments();
                for (int i = 0; i < n_segments; ++i) {
                    const char *text = ctx->full_get_segment_text(i);
                    m_transcript.push_back(text);

                    if (wparams.no_timestamps) {
                        fprintf(stderr, "%s", text);
                    } else {
                        const int64_t t0 = ctx->full_get_segment_t0(i);
                        const int64_t t1 = ctx->full_get_segment_t1(i);

                        fprintf(stderr, "[%s --> %s]  %s\n",
                                to_timestamp(t0).c_str(),
                                to_timestamp(t1).c_str(), text);
                    }
                }
            }

            ++n_iter;

            if (!use_vad && (n_iter % n_new_line) == 0) {
                fprintf(stderr, "\n");

                // keep part of the audio for next iteration to try to mitigate
                // word boundary issues
                pcmf32_old = std::vector<float>(pcmf32.end() - num_samples_keep,
                                                pcmf32.end());

                // Add tokens of the last full length segment as the prompt
                if (!wparams.no_context) {
                    prompt_tokens.clear();

                    const int n_segments = ctx->full_n_segments();
                    for (int segment = 0; segment < n_segments; ++segment) {
                        const int token_count = ctx->full_n_tokens(segment);
                        for (int id = 0; id < token_count; ++id) {
                            prompt_tokens.push_back(
                                ctx->full_get_token_id(segment, id));
                        }
                    }
                }
            }
        }
    }

    this->pause();

    ctx->print_timings();
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
               const py::kwargs &kwargs) {
                self.stream_transcribe(&context, &params, kwargs);
                return py::make_iterator(self.m_transcript.begin(),
                                         self.m_transcript.end());
            },
            py::keep_alive<0, 1>())
        .def("resume", &whisper::AudioCapture::resume)
        .def("pause", &whisper::AudioCapture::pause)
        .def("clear", &whisper::AudioCapture::clear);
};
