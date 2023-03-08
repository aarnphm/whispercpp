#include "context.h"
#include <assert.h>
#include <sstream>

#define WITH_DEPRECATION(depr)                                                 \
  PyErr_WarnEx(PyExc_DeprecationWarning,                                       \
               "Setting '" depr                                                \
               "' as an attribute is deprecated and will be remove in "        \
               "future release. Use 'with_" depr "()' instead.",               \
               1)

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
  // whisper_token_data -> TokenData
  py::class_<whisper_token_data>(m, "TokenData", "Data for the token")
      .def_readonly("id", &whisper_token_data::id)
      .def_readonly("tid", &whisper_token_data::tid)
      .def_readonly("p", &whisper_token_data::p)
      .def_readonly("plog", &whisper_token_data::plog)
      .def_readonly("pt", &whisper_token_data::pt)
      .def_readonly("ptsum", &whisper_token_data::ptsum)
      .def_readonly("t0", &whisper_token_data::t0)
      .def_readonly("t1", &whisper_token_data::t1)
      .def_readonly("vlen", &whisper_token_data::vlen)
      .def("__repr__", [](const whisper_token_data &t) {
        std::stringstream s;
        s << "("
          << "id=" << t.id << ", "
          << "tid=" << t.tid << ", "
          << "p=" << t.p << ", "
          << "plog=" << t.plog << ", "
          << "pt=" << t.pt << ", "
          << "ptsum=" << t.ptsum << ", "
          << "t0=" << t.t0 << ", "
          << "t1=" << t.t1 << ", "
          << "vlen=" << t.vlen << ")";

        return s.str();
      });

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

SamplingStrategies
SamplingStrategies::from_enum(whisper_sampling_strategy *enum_) {
  switch (*enum_) {
  case WHISPER_SAMPLING_GREEDY:
    return SamplingStrategies(std::make_shared<SamplingGreedy>());
  case WHISPER_SAMPLING_BEAM_SEARCH:
    return SamplingStrategies(std::make_shared<SamplingBeamSearch>());
  default:
    throw std::runtime_error("Unknown sampling strategy");
  }
};

SamplingStrategies
SamplingStrategies::from_sampling_strategy(SamplingType *st) {
  SamplingStrategies ss;
  switch (st->to_enum()) {
  case WHISPER_SAMPLING_GREEDY:
    ss.set_strategy(std::make_shared<SamplingGreedy>((SamplingGreedy &)(*st)));
    break;
  case WHISPER_SAMPLING_BEAM_SEARCH:
    ss.set_strategy(
        std::make_shared<SamplingBeamSearch>((SamplingBeamSearch &)(*st)));
    break;
  default:
    throw std::runtime_error("Unknown sampling strategy");
  }
  return ss;
};

void new_segment_callback_handler(whisper_context *ctx, int n_new,
                                  void *user_data) {
  auto new_segment_callback =
      (CallbackAndContext<Params::NewSegmentCallback>::Container *)user_data;
  auto callback = new_segment_callback->callback;
  if (callback != nullptr) {
    (*callback)(*new_segment_callback->context, n_new);
  }
}

Params Params::from_enum(whisper_sampling_strategy *enum_) {
  SamplingStrategies ss = SamplingStrategies::from_enum(enum_);
  return Params::from_sampling_strategy(&ss);
}

Params Params::from_sampling_strategy(SamplingStrategies *ss) {

  auto strategy = ss->build();

  whisper_full_params fp = ::whisper_full_default_params(ss->to_enum());
  CallbackAndContext<NewSegmentCallback> new_segment_callback;
  fp.new_segment_callback = new_segment_callback_handler;
  fp.new_segment_callback_user_data = new_segment_callback.data.get();

  switch (strategy->to_enum()) {
  case WHISPER_SAMPLING_GREEDY:
    fp.greedy.best_of = ((SamplingGreedy *)strategy)->best_of;
    break;
  case WHISPER_SAMPLING_BEAM_SEARCH:
    fp.beam_search.patience = ((SamplingBeamSearch *)strategy)->patience;
    fp.beam_search.beam_size = ((SamplingBeamSearch *)strategy)->beam_size;
    break;
  default:
    throw std::runtime_error("Unknown sampling strategy");
  }
  return Params(std::make_shared<whisper_full_params>(fp),
                new_segment_callback);
};

Params::Params() {
  fp->new_segment_callback = new_segment_callback_handler;
  fp->new_segment_callback_user_data = new_segment_callback.data.get();
}

Params::Params(Params const &other)
    : fp(other.fp), new_segment_callback(other.new_segment_callback) {
  fp->new_segment_callback = new_segment_callback_handler;
  fp->new_segment_callback_user_data = new_segment_callback.data.get();
}

Params &Params::operator=(Params const &other) {
  fp = other.fp;
  new_segment_callback = other.new_segment_callback;
  fp->new_segment_callback = new_segment_callback_handler;
  fp->new_segment_callback_user_data = new_segment_callback.data.get();
  return *this;
}

Params Params::copy_for_full(Context &context) {
  Params params(*this);
  if (params.new_segment_callback.data) {
    params.new_segment_callback.data->context = &context;
  }
  return params;
}

// Set tokens to provide the model as initial input.
// These tokens are prepended to any existing text content
// from a previous call. Calling this more than once will
// overwrite the previous tokens. Defaults to an empty
// vector.
void Params::set_tokens(std::vector<int> &tokens) {
  fp->prompt_tokens = reinterpret_cast<whisper_token *>(&tokens);
  fp->prompt_n_tokens = tokens.size();
}

// called for every newly generated text segments
// Do not use this function unless you know what you are
// doing. Defaults to None.
void Params::set_new_segment_callback(NewSegmentCallback callback) {
  (*new_segment_callback.data).callback =
      std::make_shared<NewSegmentCallback>(callback);
}

// Set the callback for starting the encoder.
// Do not use this function unless you know what you are
// doing. Defaults to None.
void Params::set_encoder_begin_callback(
    whisper_encoder_begin_callback callback) {
  fp->encoder_begin_callback = callback;
}
// Set the user data to be passed to the encoder begin
// callback. Defaults to None. See
// set_encoder_begin_callback.
void Params::set_encoder_begin_callback_user_data(void *user_data) {
  fp->encoder_begin_callback_user_data = user_data;
}

// Set the callback for each decoder to filter obtained
// logits. Do not use this function unless you know what you
// are doing. Defaults to None.
void Params::set_logits_filter_callback(
    whisper_logits_filter_callback callback) {
  fp->logits_filter_callback = callback;
}
// Set the user data to be passed to the logits filter
// callback. Defaults to None. See
// set_logits_filter_callback.
void Params::set_logits_filter_callback_user_data(void *user_data) {
  fp->logits_filter_callback_user_data = user_data;
};

typedef std::function<void(Context &, int, py::object &)> NewSegmentCallback;

void ExportSamplingStrategiesApi(py::module &m) {
  py::class_<SamplingType>(m, "SamplingType")
      .def("build", &SamplingType::build, py::return_value_policy::copy)
      .def("to_enum", &SamplingType::to_enum);
  py::class_<SamplingGreedy, SamplingType>(m, "SamplingGreedyStrategy")
      .def(py::init<>())
      .def(py::init<int>())
      .def("with_best_of", &SamplingGreedy::with_best_of, "best_of"_a,
           py::return_value_policy::reference)
      .def("__repr__",
           [](const SamplingGreedy &b) {
             std::stringstream s;
             s << "SamplingGreedy(best_of=" << b.best_of << ")";
             return s.str();
           })
      .def_property(
          "best_of", [](SamplingGreedy &self) { return self.best_of; },
          [](SamplingGreedy &self, int best_of) {
            WITH_DEPRECATION("best_of");
            self.with_best_of(best_of);
          });

  py::class_<SamplingBeamSearch, SamplingType>(m, "SamplingBeamSearchStrategy")
      .def(py::init<>())
      .def(py::init<int, float>())
      .def("with_beam_size", &SamplingBeamSearch::with_beam_size, "beam_size"_a,
           py::return_value_policy::reference)
      .def_property(
          "beam_size", [](SamplingBeamSearch &self) { return self.beam_size; },
          [](SamplingBeamSearch &self, int beam_size) {
            WITH_DEPRECATION("beam_size");
            self.with_beam_size(beam_size);
          })
      .def("with_patience", &SamplingBeamSearch::with_patience, "patience"_a,
           py::return_value_policy::reference)
      .def_property(
          "patience", [](SamplingBeamSearch &self) { return self.patience; },
          [](SamplingBeamSearch &self, float patience) {
            WITH_DEPRECATION("patience");
            self.with_patience(patience);
          })
      .def("__repr__", [](const SamplingBeamSearch &b) {
        std::stringstream s;
        s << "SamplingBeamSearch(beam_size=" << b.beam_size
          << ", patience=" << b.patience << ")";
        return s.str();
      });

  py::class_<SamplingStrategies, std::shared_ptr<SamplingStrategies>>(
      m, "SamplingStrategies", "Available sampling strategy for whisper")
      .def("build", &SamplingStrategies::build,
           py::return_value_policy::reference)
      .def_static("from_strategy_type",
                  &SamplingStrategies::from_sampling_strategy,
                  "strategy_type"_a)
      .def_static("from_enum", &SamplingStrategies::from_enum,
                  "strategy_enum"_a)
      .def_property(
          "type", [](SamplingStrategies &self) { self.to_enum(); },
          [](SamplingStrategies &self, whisper_sampling_strategy type) {
            PyErr_WarnEx(PyExc_DeprecationWarning,
                         "Setting 'type' as an attribute is "
                         "deprecated and will "
                         "become a readonly attribute in the "
                         "future. Make sure to "
                         "set the strategy via "
                         "'from_strategy_type()' instead.",
                         1);
          })
      .def_property("beam_search",
                    py::cpp_function(
                        [](SamplingStrategies &self) {
                          if (self.build()->to_enum() !=
                              WHISPER_SAMPLING_BEAM_SEARCH) {
                            std::cout << "Sampling strategy is not "
                                         "of type 'beam_search'."
                                      << std::endl;
                            return py::cast<SamplingType *>(Py_None);
                          }
                          return self.build();
                        },
                        /* NOTE: We need to copy here because of
                          attributes assignment in Python:
                          beam_search =
                          SamplingStrategies.beam_search */
                        py::return_value_policy::copy),
                    py::cpp_function([](SamplingStrategies &self,
                                        SamplingBeamSearch beam_search) {
                      PyErr_WarnEx(PyExc_DeprecationWarning,
                                   "Setting 'beam_search' as an "
                                   "attribute is deprecated and "
                                   "will be removed in future "
                                   "version. Use "
                                   "'from_strategy_type()' "
                                   "instead.",
                                   1);
                      self.set_strategy(
                          std::make_shared<SamplingBeamSearch>(beam_search));
                    }),
                    py::return_value_policy::reference)
      .def_property(
          "greedy",
          py::cpp_function(
              [](SamplingStrategies &self) {
                if (self.build()->to_enum() != WHISPER_SAMPLING_GREEDY) {
                  std::cout << "Sampling strategy is "
                               "not of type 'greedy'."
                            << std::endl;
                  return py::cast<SamplingType *>(Py_None);
                }
                return self.build();
              },
              /* NOTE: We need to copy here because of
                attributes assignment in Python: greedy =
                SamplingStrategies.greedy */
              py::return_value_policy::copy),
          py::cpp_function([](SamplingStrategies &self, SamplingGreedy greedy) {
            PyErr_WarnEx(PyExc_DeprecationWarning,
                         "Setting 'greedy' as an attribute is "
                         "deprecated and will "
                         "be removed in future version. Use "
                         "'from_strategy_type()' "
                         "instead.",
                         1);
            self.set_strategy(std::make_shared<SamplingGreedy>(greedy));
          }),
          py::return_value_policy::reference);
}

void ExportParamsApi(py::module &m) {

  py::class_<Params>(m, "Params", "Whisper parameters container")
      .def_static("from_sampling_strategy", &Params::from_sampling_strategy,
                  "sampling_strategy"_a)
      .def_static("from_enum", &Params::from_enum, "enum"_a)
      .def("build", &Params::build, py::return_value_policy::copy)
      // TODO: __repr__
      // NOTE: Setting num_threads
      .def("with_num_threads", &Params::with_n_threads, "n_threads"_a,
           py::return_value_policy::reference)
      // NOTE setting num_threads
      .def_property(
          "num_threads", [](Params &self) { return self.get()->n_threads; },
          [](Params &self, int n_threads) {
            WITH_DEPRECATION("num_threads");
            self.with_n_threads(n_threads);
          })
      // NOTE setting num_max_text_ctx
      .def("with_num_max_text_ctx", &Params::with_n_max_text_ctx,
           "max_text_ctx"_a, py::return_value_policy::reference)
      .def_property(
          "num_max_text_ctx",
          [](Params &self) { return self.get()->n_max_text_ctx; },
          [](Params &self, size_t max_text_ctx) {
            WITH_DEPRECATION("num_max_text_ctx");
            self.with_n_max_text_ctx(max_text_ctx);
          })
      // NOTE setting offset_ms
      .def("with_offset_ms", &Params::with_offset_ms, "offset"_a,
           py::return_value_policy::reference)
      .def_property(
          "offset_ms", [](Params &self) { return self.get()->offset_ms; },
          [](Params &self, size_t offset) {
            WITH_DEPRECATION("offset_ms");
            self.with_offset_ms(offset);
          })
      // NOTE setting duration_ms
      .def("with_duration_ms", &Params::with_duration_ms, "duration"_a,
           py::return_value_policy::reference)
      .def_property(
          "duration_ms", [](Params &self) { return self.get()->duration_ms; },
          [](Params &self, size_t duration) {
            WITH_DEPRECATION("duration_ms");
            self.with_duration_ms(duration);
          })
      // NOTE setting translate
      .def("with_translate", &Params::with_translate, "translate"_a,
           py::return_value_policy::reference)
      .def_property(
          "translate", [](Params &self) { return self.get()->translate; },
          [](Params &self, bool translate) {
            WITH_DEPRECATION("translate");
            self.with_translate(translate);
          })
      // NOTE setting no_context
      .def("with_no_context", &Params::with_no_context, "no_context"_a,
           py::return_value_policy::reference)
      .def_property(
          "no_context", [](Params &self) { return self.get()->no_context; },
          [](Params &self, bool no_context) {
            WITH_DEPRECATION("no_context");
            self.with_no_context(no_context);
          })
      // NOTE setting single_segment
      .def("with_single_segment", &Params::with_single_segment,
           "single_segment"_a, py::return_value_policy::reference)
      .def_property(
          "single_segment",
          [](Params &self) { return self.get()->single_segment; },
          [](Params &self, bool single_segment) {
            WITH_DEPRECATION("single_segment");
            self.with_single_segment(single_segment);
          })
      // NOTE setting print_special
      .def("with_print_special", &Params::with_print_special, "print_special"_a,
           py::return_value_policy::reference)
      .def_property(
          "print_special",
          [](Params &self) { return self.get()->print_special; },
          [](Params &self, bool print_special) {
            WITH_DEPRECATION("print_special");
            self.with_print_special(print_special);
          })
      // NOTE setting print_progress
      .def("with_print_progress", &Params::with_print_progress,
           "print_progress"_a, py::return_value_policy::reference)
      .def_property(
          "print_progress",
          [](Params &self) { return self.get()->print_progress; },
          [](Params &self, bool print_progress) {
            WITH_DEPRECATION("print_progress");
            self.with_print_progress(print_progress);
          })
      // NOTE setting print_realtime
      .def("with_print_realtime", &Params::with_print_realtime,
           "print_realtime"_a, py::return_value_policy::reference)
      .def_property(
          "print_realtime",
          [](Params &self) { return self.get()->print_realtime; },
          [](Params &self, bool print_realtime) {
            WITH_DEPRECATION("print_realtime");
            self.with_print_realtime(print_realtime);
          })
      // NOTE setting print_timestamps
      .def("with_print_timestamps", &Params::with_print_timestamps,
           "print_timestamps"_a, py::return_value_policy::reference)
      .def_property(
          "print_timestamps",
          [](Params &self) { return self.get()->print_timestamps; },
          [](Params &self, bool print_timestamps) {
            WITH_DEPRECATION("print_timestamps");
            self.with_print_timestamps(print_timestamps);
          })
      // NOTE setting token_timestamps
      .def("with_token_timestamps", &Params::with_token_timestamps,
           "token_timestamps"_a, py::return_value_policy::reference)
      .def_property(
          "token_timestamps",
          [](Params &self) { return self.get()->token_timestamps; },
          [](Params &self, bool token_timestamps) {
            WITH_DEPRECATION("token_timestamps");
            self.with_token_timestamps(token_timestamps);
          })
      // NOTE setting timestamp_token_probability_threshold
      .def("with_timestamp_token_probability_threshold", &Params::with_thold_pt,
           "thold_pt"_a, py::return_value_policy::reference)
      .def_property(
          "timestamp_token_probability_threshold",
          [](Params &self) { return self.get()->thold_pt; },
          [](Params &self, float thold_pt) {
            WITH_DEPRECATION("timestamp_token_probability_threshold");
            self.with_thold_pt(thold_pt);
          })
      // NOTE setting timestamp_token_sum_probability_threshold
      .def("with_timestamp_token_sum_probability_threshold",
           &Params::with_thold_ptsum, "thold_ptsum"_a,
           py::return_value_policy::reference)
      .def_property(
          "timestamp_token_sum_probability_threshold",
          [](Params &self) { return self.get()->thold_ptsum; },
          [](Params &self, float thold_ptsum) {
            WITH_DEPRECATION("timestamp_token_sum_probability_threshold");
            self.with_thold_ptsum(thold_ptsum);
          })
      // NOTE setting max_segment_length
      .def("with_max_segment_length", &Params::with_max_len, "max_len"_a,
           py::return_value_policy::reference)
      .def_property(
          "max_segment_length",
          [](Params &self) { return self.get()->max_len; },
          [](Params &self, size_t max_len) {
            WITH_DEPRECATION("max_segment_length");
            self.with_duration_ms(max_len);
          })
      // NOTE setting split_on_word
      .def("with_split_on_word", &Params::with_split_on_word, "split_on_word"_a,
           py::return_value_policy::reference)
      .def_property(
          "split_on_word",
          [](Params &self) { return self.get()->split_on_word; },
          [](Params &self, bool split_on_word) {
            WITH_DEPRECATION("split_on_word");
            self.with_split_on_word(split_on_word);
          })
      // NOTE setting max_tokens
      .def("with_max_tokens", &Params::with_max_tokens, "max_tokens"_a,
           py::return_value_policy::reference)
      .def_property(
          "max_tokens", [](Params &self) { return self.get()->max_tokens; },
          [](Params &self, size_t max_tokens) {
            WITH_DEPRECATION("max_tokens");
            self.with_max_tokens(max_tokens);
          })
      // NOTE setting speed_up
      .def("with_speed_up", &Params::with_speed_up, "speed_up"_a,
           py::return_value_policy::reference)
      .def_property(
          "speed_up", [](Params &self) { return self.get()->speed_up; },
          [](Params &self, bool speed_up) {
            WITH_DEPRECATION("speed_up");
            self.with_speed_up(speed_up);
          })
      // NOTE setting audio_ctx
      .def("with_audio_ctx", &Params::with_audio_ctx, "audio_ctx"_a,
           py::return_value_policy::reference)
      .def_property(
          "audio_ctx", [](Params &self) { return self.get()->audio_ctx; },
          [](Params &self, size_t audio_ctx) {
            WITH_DEPRECATION("audio_ctx");
            self.with_audio_ctx(audio_ctx);
          })
      // NOTE set tokens
      .def("set_tokens", &Params::set_tokens, "tokens"_a)
      .def_property_readonly(
          "prompt_tokens",
          [](Params &self) { return self.get()->prompt_tokens; })
      .def_property_readonly(
          "prompt_num_tokens",
          [](Params &self) { return self.get()->prompt_n_tokens; })
      // NOTE set language
      .def("with_language", &Params::with_language, "language"_a,
           py::return_value_policy::reference)
      .def_property(
          "language", [](Params &self) { return self.get()->language; },
          [](Params &self, const char *language) {
            WITH_DEPRECATION("language");
            self.with_language(language);
          })
      // NOTE setting suppress_blank
      .def("with_suppress_blank", &Params::with_suppress_blank,
           "suppress_blank"_a, py::return_value_policy::reference)
      .def_property(
          "suppress_blank",
          [](Params &self) { return self.get()->suppress_blank; },
          [](Params &self, bool suppress_blank) {
            WITH_DEPRECATION("suppress_blank");
            self.with_suppress_blank(suppress_blank);
          })
      // NOTE setting suppress_non_speech_tokens
      .def("with_suppress_non_speech_tokens",
           &Params::with_suppress_non_speech_tokens,
           "suppress_non_speech_tokens"_a, py::return_value_policy::reference)
      .def_property(
          "suppress_non_speech_tokens",
          [](Params &self) { return self.get()->suppress_non_speech_tokens; },
          [](Params &self, bool suppress_non_speech_tokens) {
            WITH_DEPRECATION("suppress_non_speech_tokens");
            self.with_suppress_non_speech_tokens(suppress_non_speech_tokens);
          })
      // NOTE setting temperature
      .def("with_temperature", &Params::with_temperature, "temperature"_a,
           py::return_value_policy::reference)
      .def_property(
          "temperature", [](Params &self) { return self.get()->temperature; },
          [](Params &self, float temperature) {
            WITH_DEPRECATION("temperature");
            self.with_temperature(temperature);
          })
      // NOTE setting max_initial_timestamps
      .def("with_max_initial_timestamps", &Params::with_max_initial_ts,
           "max_initial_ts"_a, py::return_value_policy::reference)
      .def_property(
          "max_initial_timestamps",
          [](Params &self) { return self.get()->max_initial_ts; },
          [](Params &self, size_t max_initial_ts) {
            WITH_DEPRECATION("max_initial_timestamps");
            self.with_max_initial_ts(max_initial_ts);
          })
      // NOTE setting length_penalty
      .def("with_length_penalty", &Params::with_length_penalty,
           "length_penalty"_a, py::return_value_policy::reference)
      .def_property(
          "length_penalty",
          [](Params &self) { return self.get()->length_penalty; },
          [](Params &self, float length_penalty) {
            WITH_DEPRECATION("length_penalty");
            self.with_length_penalty(length_penalty);
          })
      // NOTE setting temperature_inc
      .def("with_temperature_inc", &Params::with_temperature_inc,
           "temperature_inc"_a, py::return_value_policy::reference)
      .def_property(
          "temperature_inc",
          [](Params &self) { return self.get()->temperature_inc; },
          [](Params &self, float temperature_inc) {
            WITH_DEPRECATION("temperature_inc");
            self.with_temperature_inc(temperature_inc);
          })
      // NOTE setting entropy_thold
      .def("with_entropy_thold", &Params::with_entropy_thold, "entropy_thold"_a,
           py::return_value_policy::reference)
      .def_property(
          "entropy_threshold",
          [](Params &self) { return self.get()->entropy_thold; },
          [](Params &self, float entropy_thold) {
            WITH_DEPRECATION("entropy_threshold");
            self.with_entropy_thold(entropy_thold);
          })
      // NOTE setting logprob_thold
      .def("with_logprob_thold", &Params::with_logprob_thold, "logprob_thold"_a,
           py::return_value_policy::reference)
      .def_property(
          "logprob_threshold",
          [](Params &self) { return self.get()->logprob_thold; },
          [](Params &self, float logprob_thold) {
            WITH_DEPRECATION("logprob_threshold");
            self.with_logprob_thold(logprob_thold);
          })
      // NOTE setting no_speech_thold
      .def("with_no_speech_thold", &Params::with_no_speech_thold,
           "no_speech_thold"_a, py::return_value_policy::reference)
      .def_property(
          "no_speech_threshold",
          [](Params &self) { return self.get()->no_speech_thold; },
          [](Params &self, float no_speech_thold) {
            WITH_DEPRECATION("no_speech_threshold");
            self.with_no_speech_thold(no_speech_thold);
          })
      .def(
          "on_new_segment",
          [](Params &self, NewSegmentCallback &callback,
             py::object &user_data) {
            using namespace std::placeholders;
            self.set_new_segment_callback(std::bind(
                [](NewSegmentCallback &callback, py::object &user_data,
                   Context &ctx,
                   int n_new) mutable { (callback)(ctx, n_new, user_data); },
                std::move(callback), std::move(user_data), _1, _2));
          },
          "callback"_a, "user_data"_a = py::none(), py::keep_alive<1, 2>(),
          py::keep_alive<1, 3>());
  // TODO: encoder_begin_callback and logits_filter_callback are still missing
}
