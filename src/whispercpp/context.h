#ifdef BAZEL_BUILD
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "whisper.h"
#else
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "whispercpp/whisper.h"
#endif
#include <string>
#include <vector>

namespace py = pybind11;
using namespace pybind11::literals;

struct SamplingGreedy {
  int best_of; // ref:
               // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L264
};
struct SamplingBeamSearch {
  int beam_size; // ref:
                 // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L265
  float patience; // NOTE: upstream not implemented, ref:
                  // https://arxiv.org/pdf/2204.05424.pdf
};

struct SamplingStrategies {
  enum StrategyType { GREEDY, BEAM_SEARCH } type;
  union {
    struct SamplingGreedy greedy;
    struct SamplingBeamSearch beam_search;
  };
};

class FullParams {
public:
  whisper_full_params fp;

  static FullParams
  from_sampling_strategy(SamplingStrategies sampling_strategies);

  // Set the number of threads to use for decoding.
  // Defaults to min(4, std::thread::hardware_concurrency()).
  void set_n_threads(size_t threads);
  size_t get_n_threads();

  // Set max tokens from past text as prompt for decoder.
  // defaults to 16384
  void set_n_max_text_ctx(size_t max_text_ctx);
  size_t get_n_max_text_ctx();

  // Set offset in milliseconds to start decoding from.
  // defaults to 0
  void set_offset_ms(size_t offset);
  size_t get_offset_ms();

  // Set audio duration in milliseconds to decode.
  // defaults to 0 (decode until end of audio)
  void set_duration_ms(size_t duration);
  size_t get_duration_ms();

  // Whether to translate to output to language specified under `language`
  // parameter. Defaults to false.
  void set_translate(bool translate);
  bool get_translate();

  // Do not use past translation (if any) as initial prompt for the decoder.
  // Defaults to false.
  void set_no_context(bool no_context);
  bool get_no_context();

  // Force single segment output. This may be useful for streaming.
  // Defaults to false
  void set_single_segment(bool single_segment);
  bool get_single_segment();

  // Whether to print special tokens (<SOT>, <EOT>, <BEG>)
  // Defaults to false
  void set_print_special(bool print_special);
  bool get_print_special();

  // Whether to print progress information
  // Defaults to false
  void set_print_progress(bool print_progress);
  bool get_print_progress();

  // Print results from within whisper.cpp.
  // Try to use the callback methods instead:
  // [set_new_segment_callback](FullParams::set_new_segment_callback),
  // [set_new_segment_callback_user_data](FullParams::set_new_segment_callback_user_data).
  // Defaults to false
  void set_print_realtime(bool print_realtime);
  bool get_print_realtime();

  // Whether to print timestamps for each text segment when printing realtime
  // Only has an effect if [set_print_realtime](FullParams::set_print_realtime)
  // is set to true. Defaults to true.
  void set_print_timestamps(bool print_timestamps);
  bool get_print_timestamps();

  // [EXPERIMENTAL] token-level timestamps
  // default to false
  void set_token_timestamps(bool token_timestamps);
  bool get_token_timestamps();

  // [EXPERIMENTAL] Set timestamp token probability threshold.
  // Defaults to 0.01
  void set_thold_pt(float thold_pt);
  float get_thold_pt();

  // [EXPERIMENTAL] Set timestamp token sum probability threshold.
  // Defaults to 0.01
  void set_thold_ptsum(float thold_ptsum);
  float get_thold_ptsum();

  // [EXPERIMENTAL] max segment length in characters
  // defaults to 0 (no limit)
  void set_max_len(size_t max_len);
  size_t get_max_len();

  // [EXPERIMENTAL] split on word rather on token (in conjunction with max_len)
  // defaults to false
  void set_split_on_word(bool split_on_word);
  bool get_split_on_word();

  // [EXPERIMENTAL] Set the maximum tokens per segment. Default to 0 (no limit).
  void set_max_tokens(size_t max_tokens);
  size_t get_max_tokens();

  // [EXPERIMENTAL] Speed-up techniques (can reduce the quality of output)
  // Speed-up the audio by 2x using Phase Vocoder
  // defaults to false
  void set_speed_up(bool speed_up);
  bool get_speed_up();

  // [EXPERIMENTAL] Speed-up techniques (can reduce the quality of output)
  // Overwrite the audio context size. Default to 0 to use the default value
  void set_audio_ctx(size_t audio_ctx);
  size_t get_audio_ctx();

  // Set tokens to provide the model as initial input.
  // These tokens are prepended to any existing text content from a previous
  // call.
  // Calling this more than once will overwrite the previous tokens.
  // Defaults to an empty vector.
  void set_tokens(std::vector<int> &tokens);
  const whisper_token *get_prompt_tokens();
  size_t get_prompt_n_tokens();

  // Set target language.
  // For auto-detection, set this either to 'auto' or nullptr.
  // defaults to 'en'.
  void set_language(std::string *language);
  std::string get_language();

  // Set suppress_blank. See
  // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/decoding.py#L89
  // for more information.
  // Defaults to true.
  void set_suppress_blank(bool suppress_blank);
  bool get_suppress_blank();

  // Set suppress none speech tokens. See
  // https://github.com/openai/whisper/blob/7858aa9c08d98f75575035ecd6481f462d66ca27/whisper/tokenizer.py#L224-L253
  // for more information.
  // Defaults to true.
  void set_suppress_none_speech_tokens(bool suppress_non_speech_tokens);
  bool get_suppress_none_speech_tokens();

  // Set initial decoding temperature. Defaults to 1.0.
  // See https://ai.stackexchange.com/a/32478
  void set_temperature(float temperature);
  float get_temperature();

  // Set max intial timestamps. See
  // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/decoding.py#L97
  // for more information.
  // Defaults to 1.0
  void set_max_intial_ts(size_t max_intial_ts);
  size_t get_max_intial_ts();

  // Set length penalty. See
  // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L267
  // for more information.
  // Defaults to -1.0.
  void set_length_penalty(float length_penalty);
  float get_length_penalty();

  // Set temperatur increase. See
  // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L274-L278
  // Defaults to 0.2
  void set_temperature_inc(float temperature_inc);
  float get_temperature_inc();

  // Set entropy threshold, similar to OpenAI's compression ratio threshold.
  // See
  // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L274-L278
  // for more information.
  // Defaults to 2.4.
  void set_entropy_thold(float entropy_thold);
  float get_entropy_thold();

  // Set logprob_thold. See
  // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L274-L278
  // for more information.
  // Defaults to -1.0.
  void set_logprob_thold(float logprob_thold);
  float get_logprob_thold();

  /// Set no_speech_thold. Currently (as of v1.2.0) not implemented.
  /// Defaults to 0.6.
  void set_no_speech_thold(float no_speech_thold);
  float get_no_speech_thold();

  // called for every newly generated text segments
  // Do not use this function unless you know what you are doing.
  // Defaults to None.
  void
  set_new_segment_callback(whisper_new_segment_callback new_segment_callback);
  // Set the user data to be passed to the new segment callback.
  // Defaults to None. See set_new_segment_callback.
  void set_new_segment_callback_user_data(void *user_data);
  // Set the callback for starting the encoder.
  // Do not use this function unless you know what you are doing.
  // Defaults to None.
  void set_encoder_begin_callback(whisper_encoder_begin_callback callback);
  // Set the user data to be passed to the encoder begin callback.
  // Defaults to None. See set_encoder_begin_callback.
  void set_encoder_begin_callback_user_data(void *user_data);
  // Set the callback for each decoder to filter obtained logits.
  // Do not use this function unless you know what you are doing.
  // Defaults to None.
  void set_logits_filter_callback(whisper_logits_filter_callback callback);
  // Set the user data to be passed to the logits filter callback.
  // Defaults to None. See set_logits_filter_callback.
  void set_logits_filter_callback_user_data(void *user_data);
};

class Context {
public:
  whisper_context *ctx;
  bool spectrogram_initialized;
  bool encode_completed;
  bool decode_once;
  static Context from_file(const char *filename);
  static Context from_buffer(std::vector<char> *buffer);
  void free();
  void pc_to_mel(std::vector<float> &pcm, size_t threads, bool phase_vocoder);
  void set_mel(std::vector<float> &mel);

  void encode(size_t offset, size_t threads);

  void decode(std::vector<whisper_token> *token, size_t n_past, size_t threads);

  std::vector<whisper_token> tokenize(std::string *text, size_t max_tokens);
  size_t lang_max_id();

  int lang_str_to_id(std::string *lang);
  std::string lang_id_to_str(size_t id);
  std::vector<float> lang_detect(size_t offset_ms, size_t threads);

  size_t n_len();
  size_t n_vocab();
  size_t n_text_ctx();
  size_t n_audio_ctx();
  bool is_multilingual();

  std::vector<std::vector<float>> get_logits(int segment);

  std::string token_to_str(whisper_token token_id);
  whisper_token eot_token();
  whisper_token sot_token();
  whisper_token prev_token();
  whisper_token solm_token();
  whisper_token not_token();
  whisper_token beg_token();
  whisper_token lang_token(int lang_id);
  whisper_token token_translate();
  whisper_token token_transcribe();

  void print_timings();
  void reset_timings();
  std::string sys_info();

  int full(FullParams params, std::vector<float> data);

  int full_parallel(FullParams params, std::vector<float> data,
                    int num_processor);

  int full_n_segments();
  int full_lang_id();
  int full_get_segment_t0(int segment);
  int full_get_segment_t1(int segment);

  std::string full_get_segment_text(int segment);
  int full_n_tokens(int segment);

  std::string full_get_token_text(int segment, int token);

  whisper_token full_get_token_id(int segment, int token);
  whisper_token_data full_get_token_data(int segment, int token);

  float full_get_token_prob(int segment, int token);
};

void ExportContextApi(py::module &m);
void ExportParamsApi(py::module &m);
