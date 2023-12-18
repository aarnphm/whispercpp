#include "context.h"
#ifdef BAZEL_BUILD
#include "whisper.cpp"
#include <pybind11/pytypes.h>
#else
#include "whisper.cpp"
#include <pybind11/pytypes.h>
#endif

#if __GNUC__ > 10 || defined(__clang__)
#define STREAM_CAST
#else
#define STREAM_CAST static_cast<std::stringstream &>
#endif

#define NO_STATE_WARNING(no_state)                                             \
    do {                                                                       \
        if (no_state) {                                                        \
            fprintf(stderr,                                                    \
                    "%s#L%d: '%s' is called with 'no_state=True'. Make sure "  \
                    "to call 'init_state()' before inference\n",               \
                    __FILE__, __LINE__, __func__);                             \
        }                                                                      \
    } while (0)

#if __GNUC__ > 10 || defined(__clang__)
#define RAISE_RUNTIME_ERROR(msg)                                               \
    do {                                                                       \
        throw std::runtime_error((std::stringstream()                          \
                                  << __FILE__ << "#L"                          \
                                  << std::to_string(__LINE__) << ": " << msg   \
                                  << "\n")                                     \
                                     .str());                                  \
    } while (0)
#else
#define RAISE_RUNTIME_ERROR(msg)                                               \
    do {                                                                       \
        throw std::runtime_error(                                              \
            static_cast<std::stringstream &>(std::stringstream()               \
                                             << __FILE__ << "#L"               \
                                             << std::to_string(__LINE__)       \
                                             << ": " << msg << "\n")           \
                .str());                                                       \
    } while (0)
#endif

#define RAISE_IF_NULL(ptr)                                                     \
    do {                                                                       \
        if ((ptr) == nullptr) {                                                \
            RAISE_RUNTIME_ERROR(#ptr << " is not initialized");                \
        }                                                                      \
    } while (0)

void Context::init_state() {
    RAISE_IF_NULL(wctx);
    this->set_state(whisper_init_state(wctx));
}

Context Context::from_file(const char *filename, bool no_state) {
    Context c;
    NO_STATE_WARNING(no_state);

    if (no_state) {
        c.set_context(whisper_init_from_file_no_state(filename));
    } else {
        c.set_context(whisper_init_from_file(filename));
        c.set_init_with_state(true);
    }
    RAISE_IF_NULL(c.wctx);
    return c;
}

Context Context::from_buffer(void *buffer, size_t buffer_size, bool no_state) {
    Context c;
    NO_STATE_WARNING(no_state);

    if (no_state) {
        c.set_context(whisper_init_from_buffer_no_state(buffer, buffer_size));
    } else {
        c.set_context(whisper_init_from_buffer(buffer, buffer_size));
        c.set_init_with_state(true);
    }
    RAISE_IF_NULL(c.wctx);
    return c;
}

void Context::free_state() {
    whisper_free_state(wstate);
    this->set_state(nullptr);
}

void Context::free() {
    whisper_free(wctx);
    this->set_context(nullptr);
    this->free_state();
}

// Convert RAW PCM audio to log mel spectrogram.
// The resulting spectrogram is stored inside the provided whisper context.
// Returns 0 on success. This is the combination of whisper_pcm_to_mel and
// whisper_pcm_to_mel_phase_vocoder. pass in phase_vocoder = true to use Phase
// Vocoder. Default to false.
void Context::pc_to_mel(std::vector<float> &pcm, size_t threads,
                        bool phase_vocoder) {
    if (threads < 1)
        RAISE_RUNTIME_ERROR("threads must be >= 1");

    int res;

    if (phase_vocoder && !init_with_state) {
        RAISE_IF_NULL(wstate);
        res = whisper_pcm_to_mel_phase_vocoder_with_state(
            wctx, wstate, pcm.data(), pcm.size(), threads);
    } else if (phase_vocoder && init_with_state) {
        res = whisper_pcm_to_mel_phase_vocoder(wctx, pcm.data(), pcm.size(),
                                               threads);
    } else if (!phase_vocoder && !init_with_state) {
        RAISE_IF_NULL(wstate);
        res = whisper_pcm_to_mel_with_state(wctx, wstate, pcm.data(),
                                            pcm.size(), threads);
    } else {
        res = whisper_pcm_to_mel(wctx, pcm.data(), pcm.size(), threads);
    }

    if (res == -1) {
        RAISE_RUNTIME_ERROR("Failed to compute mel spectrogram.");
    } else if (res == 0) {
        spectrogram_initialized = true;
    } else {
        RAISE_RUNTIME_ERROR("Unknown error.");
    }
}

// Low-level API for setting custom log mel spectrogram.
// The resulting spectrogram is stored inside the provided whisper context.
void Context::set_mel(std::vector<float> &mel) {
    // n_mel sets to 80
    int res;

    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        res = whisper_set_mel_with_state(wctx, wstate, mel.data(), mel.size(),
                                         80);
    } else {
        res = whisper_set_mel(wctx, mel.data(), mel.size(), 80);
    }

    if (res == -1) {
        RAISE_RUNTIME_ERROR("Invalid number of mel bands.");
    } else if (res == 0) {
        spectrogram_initialized = true;
    } else {
        RAISE_RUNTIME_ERROR("Unknown error.");
    }
}

// Run the Whisper encoder on the log mel spectrogram stored inside the
// provided whisper context. Make sure to call whisper_pcm_to_mel() or
// whisper_set_mel() first. offset can be used to specify the offset of the
// first frame in the spectrogram. Returns 0 on success
void Context::encode(size_t offset, size_t threads) {
    if (!spectrogram_initialized) {
        RAISE_RUNTIME_ERROR("spectrogram not initialized");
    }
    if (threads < 1)
        throw std::invalid_argument("threads must be >= 1");
    int res;

    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        res = whisper_encode_with_state(wctx, wstate, offset, threads);
    } else {
        res = whisper_encode(wctx, offset, threads);
    }

    if (res == -1) {
        RAISE_RUNTIME_ERROR("whisper_encode failed");
    } else if (res == 0) {
        encode_completed = true;
    } else {
        RAISE_RUNTIME_ERROR("Unknown error.");
    }
}

// Run the Whisper decoder to obtain the logits and probabilities for the next
// token. Make sure to call whisper_encode() first. tokens + n_tokens is the
// provided context for the decoder. n_past is the number of tokens to use
// from previous decoder calls. Returns 0 on success
void Context::decode(std::vector<whisper_token> *token, size_t n_past,
                     size_t threads) {
    if (!encode_completed) {
        RAISE_RUNTIME_ERROR("encode not completed.");
    }
    if (threads < 1)
        throw std::invalid_argument("threads must be >= 1");

    int res;

    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        res = whisper_decode_with_state(wctx, wstate, token->data(),
                                        token->size(), n_past, threads);
    } else {
        res =
            whisper_decode(wctx, token->data(), token->size(), n_past, threads);
    }

    if (res == -1) {
        RAISE_RUNTIME_ERROR("whisper_decode failed");
    } else if (res == 0) {
        decode_once = true;
    } else {
        RAISE_RUNTIME_ERROR("Unknown error.");
    }
}

// Run the Whisper decoder to obtain the logits and probabilities for the next
// token. Make sure to call whisper_encode() first. tokens + n_tokens is the
// provided context for the decoder. n_past is the number of tokens to use
// from previous decoder calls. Returns vec<whisper_token> on success.
std::vector<whisper_token> Context::tokenize(std::string *text,
                                             size_t max_tokens) {
    std::vector<whisper_token> tokens;
    tokens.reserve(max_tokens);

    int ret = whisper_tokenize(wctx, text->c_str(), tokens.data(), max_tokens);

    if (ret == -1) {
        RAISE_RUNTIME_ERROR("Too many results tokens.");
    } else {
        // ret != -1 then length of the vector is at least ret tokens
        tokens.reserve(ret);
    }
    return tokens;
};

// Returns id of a given language, raise exception if not found
int Context::lang_str_to_id(const char *lang) {
    int id = whisper_lang_id(lang);

    if (id == -1)
        RAISE_RUNTIME_ERROR("Invalid language");

    return id;
}

// Returns short string of specified language id, raise exception if nullptr
// is returned
const char *Context::lang_id_to_str(size_t id) {
    const char *lang = whisper_lang_str(id);
    if (lang == nullptr)
        RAISE_RUNTIME_ERROR("Invalid language id");
    return lang;
}

// language functions. Returns a vector of probabilities for each language.
std::vector<float> Context::lang_detect(size_t offset_ms, size_t threads) {
    if (!spectrogram_initialized) {
        RAISE_RUNTIME_ERROR("Spectrogram not initialized");
    }
    if (threads < 1)
        throw std::invalid_argument("threads must be >= 1");

    int res;

    std::vector<float> lang_probs(whisper_lang_max_id());

    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        res = whisper_lang_auto_detect_with_state(wctx, wstate, offset_ms,
                                                  threads, lang_probs.data());
    } else {
        res = whisper_lang_auto_detect(wctx, offset_ms, threads,
                                       lang_probs.data());
    }

    if (res == -1) {
        RAISE_RUNTIME_ERROR(STREAM_CAST(std::stringstream()
                                        << "offset " << offset_ms
                                        << "ms is before the start of audio.")
                                .str());
    } else if (res == -2) {
        RAISE_RUNTIME_ERROR(STREAM_CAST(std::stringstream()
                                        << "offset " << offset_ms
                                        << "ms is past the end of the audio.")
                                .str());
    } else if (res == -6) {
        RAISE_RUNTIME_ERROR("Failed to encode.");
    } else if (res == -7) {
        RAISE_RUNTIME_ERROR("Failed to decode.");
    } else {
        return lang_probs;
    }
}

// Get mel spectrogram length
size_t Context::n_len() {
    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        return whisper_n_len_from_state(wstate);
    } else {
        return whisper_n_len(wctx);
    }
}

// Get number of mels per segment
size_t Context::model_n_mels() { return whisper_model_n_mels(wctx); }
// Get number of vocab
size_t Context::n_vocab() { return whisper_n_vocab(wctx); }
// Get number of text context
size_t Context::n_text_ctx() { return whisper_n_text_ctx(wctx); }
// Get number of audio context
size_t Context::n_audio_ctx() { return whisper_n_audio_ctx(wctx); }
// check if the model is multilingual
bool Context::is_multilingual() { return whisper_is_multilingual(wctx) != 0; }

// Token logits obtained from the last call to whisper_decode()
// The logits for the last token are stored in the last row
// Rows: n_tokens
// Cols: n_vocab
std::vector<std::vector<float>> Context::get_logits(int segment) {
    if (!spectrogram_initialized)
        RAISE_RUNTIME_ERROR("spectrogram not initialized");

    float *ret;
    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        ret = whisper_get_logits_from_state(wstate);
    } else {
        ret = whisper_get_logits(wctx);
    }

    if (ret == nullptr)
        RAISE_RUNTIME_ERROR("Failed to get logits");

    std::vector<std::vector<float>> logits;

    int num_vocab = this->n_vocab();
    int num_tokens = this->full_n_segments();

    for (int i = 0; i < num_tokens; i++) {
        std::vector<float> r;
        for (int j = 0; j < num_vocab; j++) {
            int idx = (i * num_vocab) + j;
            r.push_back(ret[idx]);
        }
        logits.push_back(r);
    }
    return logits;
}

// Convert token id to string. Use the vocabulary in provided context
std::string Context::token_to_str(whisper_token token_id) {
    const char *ret = whisper_token_to_str(wctx, token_id);
    if (ret == nullptr)
        RAISE_RUNTIME_ERROR("Failed to convert token to string.");

    return std::string(ret);
}

// Run the entire model:
// PCM -> log mel spectrogram -> encoder -> decoder -> text
//
// Uses the specified decoding strategy to obtain the text. This is
// usually the only function you need to call as an end user.
int Context::full(Params params, std::vector<float> data) {
    if (wctx == nullptr) {
        RAISE_RUNTIME_ERROR("context is not initialized (due to "
                            "either 'free()' is called or "
                            "failed to create the context). Try "
                            "to initialize with 'from_file' "
                            "or 'from_buffer' and try again.");
    }

    Params copy = params.copy_for_full(*this);
    int ret;

    if (init_with_state) {
        ret = whisper_full(wctx, *copy.get(), data.data(), data.size());
    } else {
        RAISE_IF_NULL(wstate);
        ret = whisper_full_with_state(wctx, wstate, *copy.get(), data.data(),
                                      data.size());
    }

    if (ret == -1) {
        RAISE_RUNTIME_ERROR(
            "Failed to compute log mel spectrogram with 'speed_up=True'.");
    } else if (ret == -2) {
        RAISE_RUNTIME_ERROR("Failed to compute log mel spectrogram with.");
    } else if (ret == -3) {
        RAISE_RUNTIME_ERROR("Failed to auto-detect language.");
    } else if (ret == -5) {
        RAISE_RUNTIME_ERROR(
            STREAM_CAST(std::stringstream()
                        << "audio_ctx is larger than maximum allowed ("
                        << std::to_string(params.get()->audio_ctx) << " > "
                        << this->n_audio_ctx() << ").")
                .str());
    } else if (ret == -6) {
        RAISE_RUNTIME_ERROR("Failed to encode.");
    } else if (ret == -7 || ret == -8) {
        RAISE_RUNTIME_ERROR("Failed to decode.");
    } else {
        return ret;
    }
};

// Split the input audio in chunks and process each chunk separately using
// whisper_full_with_state()
//
// Result is stored in the default state of the context
//
// Not thread safe if executed in parallel on the same context. It seems
// this approach can offer some speedup in some cases. However, the
// transcription accuracy can be worse at the beginning and end of each chunk.
int Context::full_parallel(Params params, std::vector<float> data,
                           int num_processor) {

    if (wstate != nullptr && num_processor > 1) {
        // NOTE: need to point the current state for the context to work.
        wctx->state = wstate;
    }

    if (num_processor == 1) {
        return this->full(params, data);
    }

    Params copy = params.copy_for_full(*this);
    int ret = whisper_full_parallel(wctx, *copy.get(), data.data(), data.size(),
                                    num_processor);

    if (ret == -1) {
        RAISE_RUNTIME_ERROR(
            "Failed to compute log mel spectrogram with 'speed_up=True'.");
    } else if (ret == -2) {
        RAISE_RUNTIME_ERROR("Failed to compute log mel spectrogram with.");
    } else if (ret == -3) {
        RAISE_RUNTIME_ERROR("Failed to auto-detect language.");
    } else if (ret == -5) {
        RAISE_RUNTIME_ERROR(
            STREAM_CAST(std::stringstream()
                        << "audio_ctx is larger than maximum allowed ("
                        << std::to_string(params.get()->audio_ctx) << " > "
                        << this->n_audio_ctx() << ").")
                .str());
    } else if (ret == -6) {
        RAISE_RUNTIME_ERROR("Failed to encode.");
    } else if (ret == -7 || ret == -8) {
        RAISE_RUNTIME_ERROR("Failed to decode.");
    } else {
        return ret;
    }
};

// Number of generated text segments
// A segment can be a few words, a sentence, or even a paragraph.
int Context::full_n_segments() {
    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        return whisper_full_n_segments_from_state(wstate);
    } else {
        return whisper_full_n_segments(wctx);
    }
}

// Language id associated with the context's default state
int Context::full_lang_id() {
    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        return whisper_full_lang_id_from_state(wstate);
    } else {

        return whisper_full_lang_id(wctx);
    }
}

// Get the start time of the specified segment.
int Context::full_get_segment_t0(int segment) {
    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        return whisper_full_get_segment_t0_from_state(wstate, segment);
    } else {
        return whisper_full_get_segment_t0(wctx, segment);
    }
}

// Get the end time of the specified segment.
int Context::full_get_segment_t1(int segment) {
    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        return whisper_full_get_segment_t1_from_state(wstate, segment);
    } else {
        return whisper_full_get_segment_t1(wctx, segment);
    }
}

// Get the text of the specified segment.
const char *Context::full_get_segment_text(int segment) {
    const char *ret;

    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        ret = whisper_full_get_segment_text_from_state(wstate, segment);
    } else {
        ret = whisper_full_get_segment_text(wctx, segment);
    }

    if (ret == nullptr)
        RAISE_RUNTIME_ERROR("nullptr.");
    return ret;
}

// Get numbers of tokens in specified segments.
int Context::full_n_tokens(int segment) {
    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        return whisper_full_n_tokens_from_state(wstate, segment);
    } else {
        return whisper_full_n_tokens(wctx, segment);
    }
}

// Get the token text of the specified token in the specified segment.
std::string Context::full_get_token_text(int segment, int token) {
    const char *ret;

    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        ret = whisper_full_get_token_text_from_state(wctx, wstate, segment,
                                                     token);
    } else {
        ret = whisper_full_get_token_text(wctx, segment, token);
    }

    if (ret == nullptr)
        RAISE_RUNTIME_ERROR("nullptr.");

    return std::string(ret);
}

// Get the token text of the specified token in the specified segment
whisper_token Context::full_get_token_id(int segment, int token) {
    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        return whisper_full_get_token_id_from_state(wstate, segment, token);
    } else {
        return whisper_full_get_token_id(wctx, segment, token);
    }
}

// Get token data for the specified token in the specified segment.
// This contains probabilities, timestamps, etc.
whisper_token_data Context::full_get_token_data(int segment, int token) {
    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        return whisper_full_get_token_data_from_state(wstate, segment, token);
    } else {
        return whisper_full_get_token_data(wctx, segment, token);
    }
}

// Get the probability of the specified token in the specified segment.
float Context::full_get_token_prob(int segment, int token) {
    if (!init_with_state) {
        RAISE_IF_NULL(wstate);
        return whisper_full_get_token_p_from_state(wstate, segment, token);
    } else {
        return whisper_full_get_token_p(wctx, segment, token);
    }
}

void ExportContextApi(py::module &m) {
    py::class_<Context>(m, "Context", "A light wrapper around whisper_context")
        .def_static("from_file", &Context::from_file, "filename"_a,
                    "no_state"_a = false)
        .def_static(
            "from_buffer",
            [](py::buffer buffer, bool no_state) {
                const py::buffer_info buf_info = buffer.request();
                return Context::from_buffer(buf_info.ptr, buf_info.size,
                                            no_state);
            },
            "buffer"_a, "no_state"_a = false, py::keep_alive<0, 1>())
        .def("init_state", &Context::init_state,
             py::return_value_policy::take_ownership, py::keep_alive<0, 1>())
        // free will delete the context, hence the take_ownership
        .def("free", &Context::free)
        .def("pc_to_mel", &Context::pc_to_mel, "pcm"_a, "threads"_a = 1,
             "phase_vocoder"_a = false)
        .def("set_mel", &Context::set_mel, "mel"_a)
        .def("encode", &Context::encode, "offset"_a, "threads"_a = 1)
        .def("decode", &Context::decode, "tokens"_a, "n_past"_a,
             "threads"_a = 1)
        .def("tokenize", &Context::tokenize, "text"_a, "max_tokens"_a)
        .def("lang_str_to_id", &Context::lang_str_to_id, "lang"_a)
        .def("lang_id_to_str", &Context::lang_id_to_str, "id"_a)
        .def("lang_detect", &Context::lang_detect, "offset_ms"_a,
             "threads"_a = 1)
        .def("get_logits", &Context::get_logits, "segment"_a)
        .def("token_to_str", &Context::token_to_str, "token_id"_a)
        .def(
            "token_to_bytes",
            [](Context &context, whisper_token token_id) {
                return py::bytes(context.token_to_str(token_id));
            },
            "token_id"_a)
        .def("lang_token", &Context::lang_token, "lang_id"_a)
        .def_property_readonly("lang_max_id", &Context::lang_max_id)
        .def_property_readonly("is_initialized", &Context::is_init_with_state)
        .def_property_readonly("n_len", &Context::n_len)
        .def_property_readonly("n_vocab", &Context::n_vocab)
        .def_property_readonly("n_text_ctx", &Context::n_text_ctx)
        .def_property_readonly("n_audio_ctx", &Context::n_audio_ctx)
        .def_property_readonly("is_multilingual", &Context::is_multilingual)
        .def_property_readonly("eot_token", &Context::eot_token)
        .def_property_readonly("sot_token", &Context::sot_token)
        .def_property_readonly("prev_token", &Context::prev_token)
        .def_property_readonly("solm_token", &Context::solm_token)
        .def_property_readonly("not_token", &Context::not_token)
        .def_property_readonly("beg_token", &Context::beg_token)
        .def_property_readonly("token_translate", &Context::token_translate)
        .def_property_readonly("token_transcribe", &Context::token_transcribe)
        .def("print_timings", &Context::print_timings)
        .def("reset_timings", &Context::reset_timings)
        .def("sys_info", &Context::sys_info)
        .def("full", &Context::full, "params"_a, "data"_a,
             py::call_guard<py::gil_scoped_release>())
        .def("full_parallel", &Context::full_parallel, "params"_a, "data"_a,
             "num_processor"_a, py::call_guard<py::gil_scoped_release>(),
             py::keep_alive<1, 2>())
        .def("full_n_segments", &Context::full_n_segments)
        .def("full_lang_id", &Context::full_lang_id)
        .def("full_get_segment_start", &Context::full_get_segment_t0,
             "segment"_a)
        .def("full_get_segment_end", &Context::full_get_segment_t1, "segment"_a)
        .def("full_get_segment_text", &Context::full_get_segment_text,
             "segment"_a)
        .def("full_n_tokens", &Context::full_n_tokens, "segment"_a)
        .def("full_get_token_text", &Context::full_get_token_text, "segment"_a,
             "token"_a)
        .def("full_get_token_id", &Context::full_get_token_id, "segment"_a,
             "token"_a)
        .def("full_get_token_data", &Context::full_get_token_data, "segment"_a,
             "token"_a)
        .def("full_get_token_prob", &Context::full_get_token_prob, "segment"_a,
             "token"_a);
}
