#include "context.h"
#include <assert.h>
#include <sstream>

Context Context::from_file(const char *filename) {
  Context context;
  context.ctx = whisper_init_from_file(filename);
  if (context.ctx == nullptr) {
    throw std::runtime_error("whisper_init_from_file failed");
  }
  return context;
}

Context Context::from_buffer(std::vector<char> *buffer) {
  Context context;
  context.ctx = whisper_init_from_buffer(buffer->data(), buffer->size());
  if (context.ctx == nullptr) {
    throw std::runtime_error("whisper_init_from_file failed");
  }
  return context;
}

void Context::free() { whisper_free(ctx); }

// Convert RAW PCM audio to log mel spectrogram.
// The resulting spectrogram is stored inside the provided whisper context.
// Returns 0 on success. This is the combination of whisper_pcm_to_mel and
// whisper_pcm_to_mel_phase_vocoder. pass in phase_vocoder = true to use Phase
// Vocoder. Default to false.
void Context::pc_to_mel(std::vector<float> &pcm, size_t threads,
                        bool phase_vocoder) {
  if (threads < 1)
    throw std::invalid_argument("threads must be >= 1");
  int res;
  if (phase_vocoder) {
    res =
        whisper_pcm_to_mel_phase_vocoder(ctx, pcm.data(), pcm.size(), threads);
  } else {
    res = whisper_pcm_to_mel(ctx, pcm.data(), pcm.size(), threads);
  }
  if (res == -1) {
    throw std::runtime_error("whisper_pcm_to_mel failed");
  } else if (res == 0) {
    spectrogram_initialized = true;
  } else {
    throw std::runtime_error("whisper_pcm_to_mel returned unknown error");
  }
}

// Low-level API for setting custom log mel spectrogram.
// The resulting spectrogram is stored inside the provided whisper context.
void Context::set_mel(std::vector<float> &mel) {
  // n_mel sets to 80
  int res = whisper_set_mel(ctx, mel.data(), mel.size(), 80);
  if (res == -1) {
    throw std::runtime_error("whisper_set_mel failed");
  } else if (res == 0) {
    spectrogram_initialized = true;
  } else {
    throw std::runtime_error("whisper_set_mel returned unknown error");
  }
}

// Run the Whisper encoder on the log mel spectrogram stored inside the
// provided whisper context. Make sure to call whisper_pcm_to_mel() or
// whisper_set_mel() first. offset can be used to specify the offset of the
// first frame in the spectrogram. Returns 0 on success
void Context::encode(size_t offset, size_t threads) {
  if (!spectrogram_initialized) {
    throw std::runtime_error("spectrogram not initialized");
  }
  if (threads < 1)
    throw std::invalid_argument("threads must be >= 1");
  int res = whisper_encode(ctx, offset, threads);
  if (res == -1) {
    throw std::runtime_error("whisper_encode failed");
  } else if (res == 0) {
    encode_completed = true;
  } else {
    throw std::runtime_error("whisper_encode returned unknown error");
  }
}

// Run the Whisper decoder to obtain the logits and probabilities for the next
// token. Make sure to call whisper_encode() first. tokens + n_tokens is the
// provided context for the decoder. n_past is the number of tokens to use
// from previous decoder calls. Returns 0 on success
void Context::decode(std::vector<whisper_token> *token, size_t n_past,
                     size_t threads) {
  if (!encode_completed) {
    throw std::runtime_error("encode not completed");
  }
  if (threads < 1)
    throw std::invalid_argument("threads must be >= 1");
  int res = whisper_decode(ctx, token->data(), token->size(), n_past, threads);
  if (res == -1) {
    throw std::runtime_error("whisper_decode failed");
  } else if (res == 0) {
    decode_once = true;
  } else {
    throw std::runtime_error("whisper_decode returned unknown error");
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
  int ret = whisper_tokenize(ctx, text->c_str(), tokens.data(), max_tokens);
  if (ret == -1) {
    throw std::runtime_error("invalid text");
  } else {
    // ret != -1 then length of the vector is at least ret tokens
    tokens.reserve(ret);
  }
  return tokens;
};

// Returns largest language id
size_t Context::lang_max_id() { return whisper_lang_max_id(); }

// Returns id of a given language, raise exception if not found
int Context::lang_str_to_id(const char *lang) {
  int id = whisper_lang_id(lang);
  if (id == -1) {
    throw std::runtime_error("invalid language");
  } else {
    return id;
  }
}

// Returns short string of specified language id, raise exception if nullptr
// is returned
const char *Context::lang_id_to_str(size_t id) {
  const char *lang = whisper_lang_str(id);
  if (lang == nullptr) {
    throw std::runtime_error("invalid language id");
  } else {
    return lang;
  }
}

// language functions. Returns a vector of probabilities for each language.
std::vector<float> Context::lang_detect(size_t offset_ms, size_t threads) {
  if (!spectrogram_initialized) {
    throw std::runtime_error("spectrogram not initialized");
  }
  if (threads < 1)
    throw std::invalid_argument("threads must be >= 1");
  std::vector<float> lang_probs(whisper_lang_max_id());
  int res =
      whisper_lang_auto_detect(ctx, offset_ms, threads, lang_probs.data());
  if (res == -1) {
    throw std::runtime_error("whisper_lang_detect failed");
  } else {
    assert(res == (int)lang_probs.size());
    return lang_probs;
  }
}

// Get mel spectrogram length
size_t Context::n_len() { return whisper_n_len(ctx); }

// Get number of vocab
size_t Context::n_vocab() { return whisper_n_vocab(ctx); }

// Get number of text context
size_t Context::n_text_ctx() { return whisper_n_text_ctx(ctx); }

// Get number of audio context
size_t Context::n_audio_ctx() { return whisper_n_audio_ctx(ctx); }

// check if the model is multilingual
bool Context::is_multilingual() { return whisper_is_multilingual(ctx) != 0; }

// Token logits obtained from last call to decode()
std::vector<std::vector<float>> Context::get_logits(int segment) {
  if (!spectrogram_initialized) {
    throw std::runtime_error("spectrogram not initialized");
  }
  float *ret = whisper_get_logits(ctx);
  if (ret == nullptr) {
    throw std::runtime_error("whisper_get_logits failed");
  }
  std::vector<std::vector<float>> logits;
  int num_vocab = n_vocab();
  int num_tokens = full_n_tokens(segment);
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
  const char *ret = whisper_token_to_str(ctx, token_id);
  if (ret == nullptr) {
    throw std::runtime_error("whisper_token_to_str failed");
  }
  return std::string(ret);
}

// Some special tokens
whisper_token Context::eot_token() { return whisper_token_eot(ctx); }
whisper_token Context::sot_token() { return whisper_token_sot(ctx); }
whisper_token Context::prev_token() { return whisper_token_prev(ctx); }
whisper_token Context::solm_token() { return whisper_token_solm(ctx); }
whisper_token Context::not_token() { return whisper_token_not(ctx); }
whisper_token Context::beg_token() { return whisper_token_beg(ctx); }
whisper_token Context::lang_token(int lang_id) {
  return whisper_token_lang(ctx, lang_id);
}
// task tokens
whisper_token Context::token_translate() { return whisper_token_translate(); }
whisper_token Context::token_transcribe() { return whisper_token_transcribe(); }

// perf inform and sys info
void Context::print_timings() { whisper_print_timings(ctx); }
void Context::reset_timings() { whisper_reset_timings(ctx); }
std::string Context::sys_info() {
  return std::string(whisper_print_system_info());
}

// Run the entire model:
// PCM -> log mel spectrogram -> encoder -> decoder -> text
//
// Uses the specified decoding strategy to obtain the text. This is
// usually the only function you need to call as an end user.
int Context::full(Params params, std::vector<float> data) {
  Params copy = params.copy_for_full(*this);
  int ret = whisper_full(ctx, *copy.get(), data.data(), data.size());
  if (ret == -1) {
    throw std::runtime_error("unable to calculate spectrogram");
  } else if (ret == 7) {
    throw std::runtime_error("unable to encode");
  } else if (ret == 8) {
    throw std::runtime_error("unable to decode");
  } else if (ret == 0) {
    return ret;
  } else {
    throw std::runtime_error("unknown error");
  }
};

// Split the input audio into chunks and delegate to full
// Transcription accuracy can be worse at the beggining and end of the chunks.
int Context::full_parallel(Params params, std::vector<float> data,
                           int num_processor) {
  Params copy = params.copy_for_full(*this);
  int ret = whisper_full_parallel(ctx, *copy.get(), data.data(), data.size(),
                                  num_processor);
  if (ret == -1) {
    throw std::runtime_error("unable to calculate spectrogram");
  } else if (ret == 7) {
    throw std::runtime_error("unable to encode");
  } else if (ret == 8) {
    throw std::runtime_error("unable to decode");
  } else if (ret == 0) {
    return ret;
  } else {
    throw std::runtime_error("unknown error");
  }
};

// Number of generated text segments.
// A segment can be a few words, a sentence, or even a paragraph.
int Context::full_n_segments() { return whisper_full_n_segments(ctx); }

// Get language id associated with current context
int Context::full_lang_id() { return whisper_full_lang_id(ctx); }

// Get the start time of the specified segment.
int Context::full_get_segment_t0(int segment) {
  return whisper_full_get_segment_t0(ctx, segment);
}

// Get the end time of the specified segment.
int Context::full_get_segment_t1(int segment) {
  return whisper_full_get_segment_t1(ctx, segment);
}

// Get the text of the specified segment.
const char *Context::full_get_segment_text(int segment) {
  const char *ret = whisper_full_get_segment_text(ctx, segment);
  if (ret == nullptr) {
    throw std::runtime_error("null pointer");
  }
  return ret;
}

// Get numbers of tokens in specified segments.
int Context::full_n_tokens(int segment) {
  return whisper_full_n_tokens(ctx, segment);
}

// Get the token text of the specified token in the specified segment.
std::string Context::full_get_token_text(int segment, int token) {
  const char *ret = whisper_full_get_token_text(ctx, segment, token);
  if (ret == nullptr) {
    throw std::runtime_error("null pointer");
  }
  return std::string(ret);
}
whisper_token Context::full_get_token_id(int segment, int token) {
  return whisper_full_get_token_id(ctx, segment, token);
}

// Get token data for the specified token in the specified segment.
// This contains probabilities, timestamps, etc.
whisper_token_data Context::full_get_token_data(int segment, int token) {
  return whisper_full_get_token_data(ctx, segment, token);
}

// Get the probability of the specified token in the specified segment.
float Context::full_get_token_prob(int segment, int token) {
  return whisper_full_get_token_p(ctx, segment, token);
}

void ExportContextApi(py::module &m) {
  py::class_<Context>(m, "Context", "A light wrapper around whisper_context")
      .def_static("from_file", &Context::from_file, "filename"_a)
      .def_static("from_buffer", &Context::from_buffer, "buffer"_a)
      .def("free", &Context::free)
      .def("pc_to_mel", &Context::pc_to_mel, "pcm"_a, "threads"_a = 1,
           "phase_vocoder"_a = false)
      .def("set_mel", &Context::set_mel, "mel"_a)
      .def("encode", &Context::encode, "offset"_a, "threads"_a = 1)
      .def("decode", &Context::decode, "tokens"_a, "n_past"_a, "threads"_a = 1)
      .def("tokenize", &Context::tokenize, "text"_a, "max_tokens"_a)
      .def("lang_max_id", &Context::lang_max_id)
      .def("lang_str_to_id", &Context::lang_str_to_id, "lang"_a)
      .def("lang_id_to_str", &Context::lang_id_to_str, "id"_a)
      .def("lang_detect", &Context::lang_detect, "offset_ms"_a, "threads"_a = 1)
      .def("get_logits", &Context::get_logits, "segment"_a)
      .def("token_to_str", &Context::token_to_str, "token_id"_a)
      .def("lang_token", &Context::lang_token, "lang_id"_a)
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
           "num_processor"_a, py::call_guard<py::gil_scoped_release>())
      .def("full_n_segments", &Context::full_n_segments)
      .def("full_lang_id", &Context::full_lang_id)
      .def("full_get_segment_start", &Context::full_get_segment_t0, "segment"_a)
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
