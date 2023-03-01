#include "context.h"
#include <assert.h>

Context Context::from_file(const char *filename) {
  Context context;
  context.ctx = whisper_init_from_file(filename);
  return context;
}

Context Context::from_buffer(std::vector<char> *buffer) {
  Context context;
  context.ctx = whisper_init_from_buffer(buffer->data(), buffer->size());
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
int Context::lang_str_to_id(std::string *lang) {
  int id = whisper_lang_id(lang->c_str());
  if (id == -1) {
    throw std::runtime_error("invalid language");
  } else {
    return id;
  }
}

// Returns short string of specified language id, raise exception if nullptr
// is returned
std::string Context::lang_id_to_str(size_t id) {
  const char *lang = whisper_lang_str(id);
  if (lang == nullptr) {
    throw std::runtime_error("invalid language id");
  } else {
    return std::string(lang);
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
int Context::full(FullParams params, std::vector<float> data) {
  int ret = whisper_full(ctx, params.fp, data.data(), data.size());
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
int Context::full_parallel(FullParams params, std::vector<float> data,
                           int num_processor) {
  int ret = whisper_full_parallel(ctx, params.fp, data.data(), data.size(),
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

FullParams
FullParams::from_sampling_strategy(SamplingStrategies sampling_strategies) {

  FullParams rt;
  whisper_sampling_strategy ss;

  switch (sampling_strategies.type) {
  case SamplingStrategies::GREEDY:
    ss = whisper_sampling_strategy::WHISPER_SAMPLING_GREEDY;
    break;
  case SamplingStrategies::BEAM_SEARCH:
    ss = whisper_sampling_strategy::WHISPER_SAMPLING_BEAM_SEARCH;
    break;
  default:
    throw std::invalid_argument("Invalid sampling strategy");
  }
  whisper_full_params fp = whisper_full_default_params(ss);

  switch (sampling_strategies.type) {
  case SamplingStrategies::GREEDY:
    fp.greedy.best_of = sampling_strategies.greedy.best_of;
    break;
  case SamplingStrategies::BEAM_SEARCH:
    fp.beam_search.beam_size = sampling_strategies.beam_search.beam_size;
    fp.beam_search.patience = sampling_strategies.beam_search.patience;
    break;
  }

  rt.fp = fp;
  return rt;
};

// Set the number of threads to use for decoding.
// Defaults to min(4, std::thread::hardware_concurrency()).
void FullParams::set_n_threads(size_t threads) { fp.n_threads = threads; }
size_t FullParams::get_n_threads() { return fp.n_threads; }

// Set max tokens from past text as prompt for decoder.
// defaults to 16384
void FullParams::set_n_max_text_ctx(size_t max_text_ctx) {
  fp.n_max_text_ctx = max_text_ctx;
}
size_t FullParams::get_n_max_text_ctx() { return fp.n_max_text_ctx; }

// Set offset in milliseconds to start decoding from.
// defaults to 0
void FullParams::set_offset_ms(size_t offset) { fp.offset_ms = offset; }
size_t FullParams::get_offset_ms() { return fp.offset_ms; }

// Set audio duration in milliseconds to decode.
// defaults to 0 (decode until end of audio)
void FullParams::set_duration_ms(size_t duration) { fp.duration_ms = duration; }
size_t FullParams::get_duration_ms() { return fp.duration_ms; }

// Whether to translate to output to language specified under `language`
// parameter. Defaults to false.
void FullParams::set_translate(bool translate) { fp.translate = translate; }
bool FullParams::get_translate() { return fp.translate; }

// Do not use past translation (if any) as initial prompt for the decoder.
// Defaults to false.
void FullParams::set_no_context(bool no_context) { fp.no_context = no_context; }
bool FullParams::get_no_context() { return fp.no_context; }

// Force single segment output. This may be useful for streaming.
// Defaults to false
void FullParams::set_single_segment(bool single_segment) {
  fp.single_segment = single_segment;
}
bool FullParams::get_single_segment() { return fp.single_segment; }

// Whether to print special tokens (<SOT>, <EOT>, <BEG>)
// Defaults to false
void FullParams::set_print_special(bool print_special) {
  fp.print_special = print_special;
}
bool FullParams::get_print_special() { return fp.print_special; }

// Whether to print progress information
// Defaults to false
void FullParams::set_print_progress(bool print_progress) {
  fp.print_progress = print_progress;
}
bool FullParams::get_print_progress() { return fp.print_progress; }

// Print results from within whisper.cpp.
// Try to use the callback methods instead:
// [set_new_segment_callback](FullParams::set_new_segment_callback),
// [set_new_segment_callback_user_data](FullParams::set_new_segment_callback_user_data).
// Defaults to false
void FullParams::set_print_realtime(bool print_realtime) {
  fp.print_realtime = print_realtime;
}
bool FullParams::get_print_realtime() { return fp.print_realtime; }

// Whether to print timestamps for each text segment when printing realtime
// Only has an effect if [set_print_realtime](FullParams::set_print_realtime)
// is set to true. Defaults to true.
void FullParams::set_print_timestamps(bool print_timestamps) {
  fp.print_timestamps = print_timestamps;
}
bool FullParams::get_print_timestamps() { return fp.print_timestamps; }

// [EXPERIMENTAL] token-level timestamps
// default to false
void FullParams::set_token_timestamps(bool token_timestamps) {
  fp.token_timestamps = token_timestamps;
}
bool FullParams::get_token_timestamps() { return fp.token_timestamps; }

// [EXPERIMENTAL] Set timestamp token probability threshold.
// Defaults to 0.01
void FullParams::set_thold_pt(float thold_pt) { fp.thold_pt = thold_pt; }
float FullParams::get_thold_pt() { return fp.thold_pt; }

// [EXPERIMENTAL] Set timestamp token sum probability threshold.
// Defaults to 0.01
void FullParams::set_thold_ptsum(float thold_ptsum) {
  fp.thold_ptsum = thold_ptsum;
}
float FullParams::get_thold_ptsum() { return fp.thold_ptsum; }

// [EXPERIMENTAL] max segment length in characters
// defaults to 0 (no limit)
void FullParams::set_max_len(size_t max_len) { fp.max_len = max_len; }
size_t FullParams::get_max_len() { return fp.max_len; }

// [EXPERIMENTAL] split on word rather on token (in conjunction with max_len)
// defaults to false
void FullParams::set_split_on_word(bool split_on_word) {
  fp.split_on_word = split_on_word;
}
bool FullParams::get_split_on_word() { return fp.split_on_word; }

// [EXPERIMENTAL] Set the maximum tokens per segment. Default to 0 (no limit).
void FullParams::set_max_tokens(size_t max_tokens) {
  fp.max_tokens = max_tokens;
}
size_t FullParams::get_max_tokens() { return fp.max_tokens; }

// [EXPERIMENTAL] Speed-up techniques (can reduce the quality of output)
// Speed-up the audio by 2x using Phase Vocoder
// defaults to false
void FullParams::set_speed_up(bool speed_up) { fp.speed_up = speed_up; }
bool FullParams::get_speed_up() { return fp.speed_up; }

// [EXPERIMENTAL] Speed-up techniques (can reduce the quality of output)
// Overwrite the audio context size. Default to 0 to use the default value
void FullParams::set_audio_ctx(size_t audio_ctx) { fp.audio_ctx = audio_ctx; }
size_t FullParams::get_audio_ctx() { return fp.audio_ctx; }

// Set tokens to provide the model as initial input.
// These tokens are prepended to any existing text content from a previous
// call.
// Calling this more than once will overwrite the previous tokens.
// Defaults to an empty vector.
void FullParams::set_tokens(std::vector<int> &tokens) {
  fp.prompt_tokens = reinterpret_cast<whisper_token *>(&tokens);
  fp.prompt_n_tokens = tokens.size();
}
const whisper_token *FullParams::get_prompt_tokens() {
  return fp.prompt_tokens;
}
size_t FullParams::get_prompt_n_tokens() { return fp.prompt_n_tokens; }

// Set target language.
// For auto-detection, set this either to 'auto' or nullptr.
// defaults to 'en'.
void FullParams::set_language(std::string *language) {
  fp.language = language->c_str();
}
std::string FullParams::get_language() { return std::string(fp.language); }

// Set suppress_blank. See
// https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/decoding.py#L89
// for more information.
// Defaults to true.
void FullParams::set_suppress_blank(bool suppress_blank) {
  fp.suppress_blank = suppress_blank;
}
bool FullParams::get_suppress_blank() { return fp.suppress_blank; }

// Set suppress none speech tokens. See
// https://github.com/openai/whisper/blob/7858aa9c08d98f75575035ecd6481f462d66ca27/whisper/tokenizer.py#L224-L253
// for more information.
// Defaults to true.
void FullParams::set_suppress_none_speech_tokens(
    bool suppress_non_speech_tokens) {
  fp.suppress_non_speech_tokens = suppress_non_speech_tokens;
}
bool FullParams::get_suppress_none_speech_tokens() {
  return fp.suppress_non_speech_tokens;
}

// Set initial decoding temperature. Defaults to 1.0.
// See https://ai.stackexchange.com/a/32478
void FullParams::set_temperature(float temperature) {
  fp.temperature = temperature;
}
float FullParams::get_temperature() { return fp.temperature; }

// Set max intial timestamps. See
// https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/decoding.py#L97
// for more information.
// Defaults to 1.0
void FullParams::set_max_intial_ts(size_t max_intial_ts) {
  fp.max_initial_ts = max_intial_ts;
}
size_t FullParams::get_max_intial_ts() { return fp.max_initial_ts; }

// Set length penalty. See
// https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L267
// for more information.
// Defaults to -1.0.
void FullParams::set_length_penalty(float length_penalty) {
  fp.length_penalty = length_penalty;
}
float FullParams::get_length_penalty() { return fp.length_penalty; }

// Set temperatur increase. See
// https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L274-L278
// Defaults to 0.2
void FullParams::set_temperature_inc(float temperature_inc) {
  fp.temperature_inc = temperature_inc;
}
float FullParams::get_temperature_inc() { return fp.temperature_inc; }

// Set entropy threshold, similar to OpenAI's compression ratio threshold.
// See
// https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L274-L278
// for more information.
// Defaults to 2.4.
void FullParams::set_entropy_thold(float entropy_thold) {
  fp.entropy_thold = entropy_thold;
}
float FullParams::get_entropy_thold() { return fp.entropy_thold; }

// Set logprob_thold. See
// https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L274-L278
// for more information.
// Defaults to -1.0.
void FullParams::set_logprob_thold(float logprob_thold) {
  fp.logprob_thold = logprob_thold;
}
float FullParams::get_logprob_thold() { return fp.logprob_thold; }

/// Set no_speech_thold. Currently (as of v1.2.0) not implemented.
/// Defaults to 0.6.
void FullParams::set_no_speech_thold(float no_speech_thold) {
  fp.no_speech_thold = no_speech_thold;
}
float FullParams::get_no_speech_thold() { return fp.no_speech_thold; }

// called for every newly generated text segments
// Do not use this function unless you know what you are doing.
// Defaults to None.
void FullParams::set_new_segment_callback(
    whisper_new_segment_callback new_segment_callback) {
  fp.new_segment_callback = new_segment_callback;
}
// Set the user data to be passed to the new segment callback.
// Defaults to None. See set_new_segment_callback.
void FullParams::set_new_segment_callback_user_data(void *user_data) {
  fp.new_segment_callback_user_data = user_data;
}

// Set the callback for starting the encoder.
// Do not use this function unless you know what you are doing.
// Defaults to None.
void FullParams::set_encoder_begin_callback(
    whisper_encoder_begin_callback callback) {
  fp.encoder_begin_callback = callback;
}
// Set the user data to be passed to the encoder begin callback.
// Defaults to None. See set_encoder_begin_callback.
void FullParams::set_encoder_begin_callback_user_data(void *user_data) {
  fp.encoder_begin_callback_user_data = user_data;
}

// Set the callback for each decoder to filter obtained logits.
// Do not use this function unless you know what you are doing.
// Defaults to None.
void FullParams::set_logits_filter_callback(
    whisper_logits_filter_callback callback) {
  fp.logits_filter_callback = callback;
}
// Set the user data to be passed to the logits filter callback.
// Defaults to None. See set_logits_filter_callback.
void FullParams::set_logits_filter_callback_user_data(void *user_data) {
  fp.logits_filter_callback_user_data = user_data;
}

SamplingStrategies SamplingStrategies::from_strategy_type(StrategyType type) {
  switch (type) {
  case GREEDY:
    return SamplingStrategies(SamplingGreedy());
  case BEAM_SEARCH:
    return SamplingStrategies(SamplingBeamSearch());
  default:
    throw std::invalid_argument("Invalid strategy type");
  };
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
      .def("n_len", &Context::n_len)
      .def("n_vocab", &Context::n_vocab)
      .def("n_text_ctx", &Context::n_text_ctx)
      .def("n_audio_ctx", &Context::n_audio_ctx)
      .def("is_multilingual", &Context::is_multilingual)
      .def("get_logits", &Context::get_logits, "segment"_a)
      .def("token_to_str", &Context::token_to_str, "token_id"_a)
      .def("eot_token", &Context::eot_token)
      .def("sot_token", &Context::sot_token)
      .def("prev_token", &Context::prev_token)
      .def("solm_token", &Context::solm_token)
      .def("not_token", &Context::not_token)
      .def("beg_token", &Context::beg_token)
      .def("lang_token", &Context::lang_token, "lang_id"_a)
      .def("token_translate", &Context::token_translate)
      .def("token_transcribe", &Context::token_transcribe)
      .def("print_timings", &Context::print_timings)
      .def("reset_timings", &Context::reset_timings)
      .def("sys_info", &Context::sys_info)
      .def("full", &Context::full, "params"_a, "data"_a)
      .def("full_parallel", &Context::full_parallel, "params"_a, "data"_a,
           "num_processor"_a)
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

void ExportParamsApi(py::module &m) {
  py::enum_<SamplingStrategies::StrategyType>(m, "StrategyType")
      .value("GREEDY", SamplingStrategies::GREEDY)
      .value("BEAM_SEARCH", SamplingStrategies::BEAM_SEARCH)
      .export_values();

  py::class_<SamplingGreedy>(m, "SamplingGreedyStrategy")
      .def(py::init<>())
      .def_property(
          "best_of", [](SamplingGreedy &self) { return self.best_of; },
          [](SamplingGreedy &self, int best_of) { self.best_of = best_of; });

  py::class_<SamplingBeamSearch>(m, "SamplingBeamSearchStrategy")
      .def(py::init<>())
      .def_property(
          "beam_size", [](SamplingBeamSearch &self) { return self.beam_size; },
          [](SamplingBeamSearch &self, int beam_size) {
            self.beam_size = beam_size;
          })
      .def_property(
          "patience", [](SamplingBeamSearch &self) { return self.patience; },
          [](SamplingBeamSearch &self, float patience) {
            self.patience = patience;
          });

  py::class_<SamplingStrategies>(m, "SamplingStrategies",
                                 "Available sampling strategy for whisper")
      .def_static("from_strategy_type", &SamplingStrategies::from_strategy_type,
                  "strategy"_a)
      .def_property(
          "type", [](SamplingStrategies &self) { return self.type; },
          [](SamplingStrategies &self, SamplingStrategies::StrategyType type) {
            self.type = type;
          })
      .def_property(
          "greedy", [](SamplingStrategies &self) { return self.greedy; },
          [](SamplingStrategies &self, SamplingGreedy greedy) {
            self.greedy = greedy;
          })
      .def_property(
          "beam_search",
          [](SamplingStrategies &self) { return self.beam_search; },
          [](SamplingStrategies &self, SamplingBeamSearch beam_search) {
            self.beam_search = beam_search;
          });

  py::class_<FullParams>(m, "Params", "Whisper parameters container")
      .def_static("from_sampling_strategy", &FullParams::from_sampling_strategy,
                  "sampling_strategy"_a)
      .def_property("num_threads", &FullParams::get_n_threads,
                    &FullParams::set_n_threads)
      .def_property("num_max_text_ctx", &FullParams::get_n_max_text_ctx,
                    &FullParams::set_n_max_text_ctx)
      .def_property("offset_ms", &FullParams::get_offset_ms,
                    &FullParams::set_offset_ms)
      .def_property("duration_ms", &FullParams::get_duration_ms,
                    &FullParams::set_duration_ms)
      .def_property("translate", &FullParams::get_translate,
                    &FullParams::set_translate)
      .def_property("no_context", &FullParams::get_no_context,
                    &FullParams::set_no_context)
      .def_property("single_segment", &FullParams::get_single_segment,
                    &FullParams::set_single_segment)
      .def_property("print_special", &FullParams::get_print_special,
                    &FullParams::set_print_special)
      .def_property("print_progress", &FullParams::get_print_progress,
                    &FullParams::set_print_progress)
      .def_property("print_realtime", &FullParams::get_print_realtime,
                    &FullParams::set_print_realtime)
      .def_property("print_timestamps", &FullParams::get_print_timestamps,
                    &FullParams::set_print_timestamps)
      .def_property("token_timestamps", &FullParams::get_token_timestamps,
                    &FullParams::set_token_timestamps)
      .def_property("timestamp_token_probability_threshold",
                    &FullParams::get_thold_pt, &FullParams::set_thold_pt)
      .def_property("timestamp_token_sum_probability_threshold",
                    &FullParams::get_thold_ptsum, &FullParams::set_thold_ptsum)
      .def_property("max_segment_length", &FullParams::get_max_len,
                    &FullParams::set_max_len)
      .def_property("split_on_word", &FullParams::get_split_on_word,
                    &FullParams::set_split_on_word)
      .def_property("max_tokens", &FullParams::get_max_tokens,
                    &FullParams::set_max_tokens)
      .def_property("speed_up", &FullParams::get_speed_up,
                    &FullParams::set_speed_up)
      .def_property("audio_ctx", &FullParams::get_audio_ctx,
                    &FullParams::set_audio_ctx)
      .def("set_tokens", &FullParams::set_tokens, "tokens"_a)
      .def_property_readonly("prompt_tokens", &FullParams::get_prompt_tokens)
      .def_property_readonly("prompt_num_tokens",
                             &FullParams::get_prompt_n_tokens)
      .def_property("language", &FullParams::get_language,
                    &FullParams::set_language)
      .def_property("suppress_blank", &FullParams::get_suppress_blank,
                    &FullParams::set_suppress_blank)
      .def_property("suppress_none_speech_tokens",
                    &FullParams::get_suppress_none_speech_tokens,
                    &FullParams::set_suppress_none_speech_tokens)
      .def_property("temperature", &FullParams::get_temperature,
                    &FullParams::set_temperature)
      .def_property("max_intial_timestamps", &FullParams::get_max_intial_ts,
                    &FullParams::set_max_intial_ts)
      .def_property("length_penalty", &FullParams::get_length_penalty,
                    &FullParams::set_length_penalty)
      .def_property("temperature_inc", &FullParams::get_temperature_inc,
                    &FullParams::set_temperature_inc)
      .def_property("entropy_threshold", &FullParams::get_entropy_thold,
                    &FullParams::set_entropy_thold)
      .def_property("logprob_threshold", &FullParams::get_logprob_thold,
                    &FullParams::set_logprob_thold)
      .def_property("no_speech_threshold", &FullParams::get_no_speech_thold,
                    &FullParams::set_no_speech_thold);
  // TODO: idk what to do with setting all the callbacks for FullParams. API are
  // there, but need more time investingating conversion from Python callback to
  // C++ callback
}
