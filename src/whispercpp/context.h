#pragma once

#ifdef BAZEL_BUILD
#include "pybind11/functional.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "whisper.h"
#else
#include "pybind11/functional.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "whisper.h"
#endif
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace py = pybind11;
using namespace pybind11::literals;

struct SamplingType {
    virtual ~SamplingType() = default;
    virtual whisper_sampling_strategy to_enum() = 0;

    SamplingType *build() { return this; };
};

struct SamplingGreedy : public SamplingType {
  public:
    int best_of; // ref:
                 // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L264
    SamplingGreedy() : best_of(1) {}
    SamplingGreedy(int best_of) : best_of(best_of) {}

    SamplingGreedy *with_best_of(int best_of) {
        this->best_of = best_of;
        return this;
    }

    whisper_sampling_strategy to_enum() override {
        return WHISPER_SAMPLING_GREEDY;
    }
};

struct SamplingBeamSearch : public SamplingType {
  public:
    int beam_size; // ref:
                   // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L265
    float patience; // NOTE: upstream not implemented, ref:
                    // https://arxiv.org/pdf/2204.05424.pdf
    SamplingBeamSearch() : beam_size(-1), patience(-1.0f) {}

    SamplingBeamSearch(int beam_size, float patience)
        : beam_size(beam_size), patience(patience) {}

    SamplingBeamSearch *with_beam_size(int beam_size) {
        this->beam_size = beam_size;
        return this;
    }
    SamplingBeamSearch *with_patience(float patience) {
        this->patience = patience;
        return this;
    }

    whisper_sampling_strategy to_enum() override {
        return WHISPER_SAMPLING_BEAM_SEARCH;
    }
};

struct SamplingStrategies {
  private:
    std::shared_ptr<SamplingType> sampling_strategy_;

  public:
    SamplingStrategies(std::shared_ptr<SamplingType> &&sampling_strategy = {})
        : sampling_strategy_(sampling_strategy) {}

    SamplingType *build() { return sampling_strategy_.get()->build(); }

    void set_strategy(std::shared_ptr<SamplingType> &&strategy) {
        this->sampling_strategy_ = strategy;
    }

    whisper_sampling_strategy to_enum() {
        return sampling_strategy_->to_enum();
    }

    static SamplingStrategies from_enum(whisper_sampling_strategy *enum_);
    static SamplingStrategies from_sampling_strategy(SamplingType *st);
};

struct Context;

template <typename CB> struct CallbackAndContext {
    struct Container {
        std::shared_ptr<CB> callback;
        // this is null unless this Params was submitted to
        // whisper_full{,_parallel} in this case Params is copied and given a
        // pointer to the Context, to give the callbacks
        Context *context;

        Container() = default;
        Container(Container const &other) = default;
        Container &operator=(Container const &other) = default;
        Container(Container &&other) = delete;
        Container &operator=(Container &&other) = delete;
    };

    std::shared_ptr<Container> data;

    CallbackAndContext() : data(std::make_shared<Container>()) {}

    CallbackAndContext(CallbackAndContext const &other)
        : data(std::make_shared<Container>(*other.data)) {}

    CallbackAndContext &operator=(CallbackAndContext const &other) {
        data = std::make_shared<Container>(*other.data);
        return *this;
    }

    CallbackAndContext(CallbackAndContext &&other) : data(other.data){};

    CallbackAndContext &operator=(CallbackAndContext &&other) {
        data = other.data;
        return *this;
    }
};

struct Params {
  public:
    typedef std::function<void(Context &, int)> NewSegmentCallback;

  private:
    std::shared_ptr<whisper_full_params> fp;
    std::string language;

    CallbackAndContext<NewSegmentCallback> new_segment_callback;

    friend struct Context;

    // this copies Params for submitting it to whisper_full{,_parallel}
    // A single Params can be used for multiple whisper_full{,_parallel} calls,
    // potentially in parallel.
    // But the Context using this Params has to be stored for it to be passed
    // as an argument to the callbacks.
    // So create a copy of this Params whenever it is used by a
    // whisper_full{,_parallel} and setup the copy with the correct Context.
    Params copy_for_full(Context &context);

  public:
    Params();

    Params(std::shared_ptr<whisper_full_params> &&fp,
           CallbackAndContext<NewSegmentCallback> new_segment_callback)
        : fp(fp), new_segment_callback(new_segment_callback){};

    Params(Params const &);
    Params &operator=(Params const &);

    Params(Params &&) = default;
    Params &operator=(Params &&) = default;

    static Params
    from_sampling_strategy(SamplingStrategies *sampling_strategies);
    static Params from_enum(whisper_sampling_strategy *enum_);

    whisper_full_params *get() const { return fp.get(); }

    Params *build() { return this; }

    std::string to_string() const;

    // Set the number of threads to use for decoding.
    // Defaults to min(4, std::thread::hardware_concurrency()).
    Params *with_n_threads(size_t threads) {
        fp->n_threads = threads;
        return this;
    }

    // Set max tokens from past text as prompt for decoder.
    // defaults to 16384
    Params *with_n_max_text_ctx(size_t max_text_ctx) {
        fp->n_max_text_ctx = max_text_ctx;
        return this;
    }

    // Set offset in milliseconds to start decoding from.
    // defaults to 0
    Params *with_offset_ms(size_t offset) {
        fp->offset_ms = offset;
        return this;
    }

    // Set audio duration in milliseconds to decode.
    // defaults to 0 (decode until end of audio)
    Params *with_duration_ms(size_t duration) {
        fp->duration_ms = duration;
        return this;
    }

    // Whether to translate to output to language specified under `language`
    // parameter. Defaults to false.
    Params *with_translate(bool translate) {
        fp->translate = translate;
        return this;
    }

    // Do not use past translation (if any) as initial prompt for the decoder.
    // Defaults to false.
    Params *with_no_context(bool no_context) {
        fp->no_context = no_context;
        return this;
    }

    // Force single segment output. This may be useful for streaming.
    // Defaults to false
    Params *with_single_segment(bool single_segment) {
        fp->single_segment = single_segment;
        return this;
    }

    // Whether to print special tokens (<SOT>, <EOT>, <BEG>)
    // Defaults to false
    Params *with_print_special(bool print_special) {
        fp->print_special = print_special;
        return this;
    }

    // Whether to print progress information
    // Defaults to false
    Params *with_print_progress(bool print_progress) {
        fp->print_progress = print_progress;
        return this;
    }

    // Print results from within whisper.cpp.
    // Try to use the callback methods instead:
    // [set_new_segment_callback](FullParams::set_new_segment_callback),
    // [set_new_segment_callback_user_data](FullParams::set_new_segment_callback_user_data).
    // Defaults to false
    Params *with_print_realtime(bool print_realtime) {
        fp->print_realtime = print_realtime;
        return this;
    }

    // Whether to print timestamps for each text segment when printing realtime
    // Only has an effect if
    // [set_print_realtime](FullParams::set_print_realtime) is set to true.
    // Defaults to true.
    Params *with_print_timestamps(bool print_timestamps) {
        fp->print_timestamps = print_timestamps;
        return this;
    }

    // [EXPERIMENTAL] token-level timestamps
    // default to false
    Params *with_token_timestamps(bool token_timestamps) {
        fp->token_timestamps = token_timestamps;
        return this;
    }

    // [EXPERIMENTAL] Set timestamp token probability threshold.
    // Defaults to 0.01
    Params *with_thold_pt(float thold_pt) {
        fp->thold_pt = thold_pt;
        return this;
    }

    // [EXPERIMENTAL] Set timestamp token sum probability threshold.
    // Defaults to 0.01
    Params *with_thold_ptsum(float thold_ptsum) {
        fp->thold_ptsum = thold_ptsum;
        return this;
    }

    // [EXPERIMENTAL] max segment length in characters
    // defaults to 0 (no limit)
    Params *with_max_len(size_t max_len) {
        fp->max_len = max_len;
        return this;
    }

    // [EXPERIMENTAL] split on word rather on token (in conjunction with
    // max_len) defaults to false
    Params *with_split_on_word(bool split_on_word) {
        fp->split_on_word = split_on_word;
        return this;
    }

    // [EXPERIMENTAL] Set the maximum tokens per segment. Default to 0 (no
    // limit).
    Params *with_max_tokens(size_t max_tokens) {
        fp->max_tokens = max_tokens;
        return this;
    }

    // [EXPERIMENTAL] Speed-up techniques (can reduce the quality of output)
    // Speed-up the audio by 2x using Phase Vocoder
    // defaults to false
    Params *with_speed_up(bool speed_up) {
        fp->speed_up = speed_up;
        return this;
    }

    // [EXPERIMENTAL] Speed-up techniques (can reduce the quality of output)
    // Overwrite the audio context size. Default to 0 to use the default value
    Params *with_audio_ctx(size_t audio_ctx) {
        fp->audio_ctx = audio_ctx;
        return this;
    }

    // Set tokens to provide the model as initial input.
    // These tokens are prepended to any existing text content from a previous
    // call.
    // Calling this more than once will overwrite the previous tokens.
    // Defaults to an empty vector.
    void set_tokens(std::vector<int> &tokens);
    Params *with_prompt_tokens(const whisper_token *tokens) {
        fp->prompt_tokens = tokens;
        return this;
    }
    Params *with_prompt_n_tokens(size_t size) {
        fp->prompt_n_tokens = size;
        return this;
    }

    // Set target language.
    // For auto-detection, set this either to 'auto' or nullptr.
    // defaults to 'en'.
    Params *with_language(std::string language) {
        this->language = language;
        fp->language = this->language.c_str();
        return this;
    }

    // Set suppress_blank. See
    // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/decoding.py#L89
    // for more information.
    // Defaults to true.
    Params *with_suppress_blank(bool suppress_blank) {
        fp->suppress_blank = suppress_blank;
        return this;
    }

    // Set suppress none speech tokens. See
    // https://github.com/openai/whisper/blob/7858aa9c08d98f75575035ecd6481f462d66ca27/whisper/tokenizer.py#L224-L253
    // for more information.
    // Defaults to true.
    Params *with_suppress_non_speech_tokens(bool suppress_non_speech_tokens) {
        fp->suppress_non_speech_tokens = suppress_non_speech_tokens;
        return this;
    }

    // Set initial decoding temperature. Defaults to 1.0.
    // See https://ai.stackexchange.com/a/32478
    Params *with_temperature(float temperature) {
        fp->temperature = temperature;
        return this;
    }

    // Set max intial timestamps. See
    // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/decoding.py#L97
    // for more information.
    // Defaults to 1.0
    Params *with_max_initial_ts(size_t max_initial_ts) {
        fp->max_initial_ts = max_initial_ts;
        return this;
    }

    // Set length penalty. See
    // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L267
    // for more information.
    // Defaults to -1.0.
    Params *with_length_penalty(float length_penalty) {
        fp->length_penalty = length_penalty;
        return this;
    }

    // Set temperatur increase. See
    // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L274-L278
    // Defaults to 0.2
    Params *with_temperature_inc(float temperature_inc) {
        fp->temperature_inc = temperature_inc;
        return this;
    }

    // Set entropy threshold, similar to OpenAI's compression ratio threshold.
    // See
    // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L274-L278
    // for more information.
    // Defaults to 2.4.
    Params *with_entropy_thold(float entropy_thold) {
        fp->entropy_thold = entropy_thold;
        return this;
    }

    // Set logprob_thold. See
    // https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L274-L278
    // for more information.
    // Defaults to -1.0.
    Params *with_logprob_thold(float logprob_thold) {
        fp->logprob_thold = logprob_thold;
        return this;
    }

    /// Set no_speech_thold. Currently (as of v1.2.0) not implemented.
    /// Defaults to 0.6.
    Params *with_no_speech_thold(float no_speech_thold) {
        fp->no_speech_thold = no_speech_thold;
        return this;
    }

    // called for every newly generated text segments
    // Do not use this function unless you know what you are doing.
    // Defaults to None.
    void set_new_segment_callback(NewSegmentCallback callback);

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

void ExportParamsApi(py::module &m);

void ExportSamplingStrategiesApi(py::module &m);

struct Context {
  private:
    whisper_context *wctx;
    whisper_state *wstate = nullptr;

    bool init_with_state;
    bool spectrogram_initialized;
    bool encode_completed;
    bool decode_once;

  public:
    ~Context() = default;

    // setters functions
    void set_context(whisper_context *wctx) { this->wctx = wctx; }
    void set_state(whisper_state *wstate) { this->wstate = wstate; }

    // check if whether context is set with state
    void set_init_with_state(bool init_with_state) {
        this->init_with_state = init_with_state;
    }
    bool is_init_with_state() { return init_with_state; }

    void free();
    void free_state();
    void init_state();

    static Context from_file(const char *filename, bool no_state = false);
    static Context from_buffer(void *buffer, size_t buffer_size,
                               bool no_state = false);
    // TODO: implement init(loader, no_state=false) [whisper_init]

    // Convert RAW PCM audio to log mel spectrogram.
    // The resulting spectrogram is stored inside the provided whisper context.
    // Returns 0 on success. This is the combination of whisper_pcm_to_mel and
    // whisper_pcm_to_mel_phase_vocoder. pass in phase_vocoder = true to use
    // Phase Vocoder. Default to false.
    void pc_to_mel(std::vector<float> &pcm, size_t threads, bool phase_vocoder);

    // Low-level API for setting custom log mel spectrogram.
    // The resulting spectrogram is stored inside the provided whisper context.
    void set_mel(std::vector<float> &mel);

    // Run the Whisper encoder on the log mel spectrogram stored inside the
    // provided whisper context. Make sure to call whisper_pcm_to_mel() or
    // whisper_set_mel() first. offset can be used to specify the offset of the
    // first frame in the spectrogram. Returns 0 on success
    void encode(size_t offset, size_t threads);

    // Run the Whisper decoder to obtain the logits and probabilities for the
    // next token. Make sure to call whisper_encode() first. tokens + n_tokens
    // is the provided context for the decoder. n_past is the number of tokens
    // to use from previous decoder calls. Returns 0 on success
    void decode(std::vector<whisper_token> *token, size_t n_past,
                size_t threads);

    // Run the Whisper decoder to obtain the logits and probabilities for the
    // next token. Make sure to call whisper_encode() first. tokens + n_tokens
    // is the provided context for the decoder. n_past is the number of tokens
    // to use from previous decoder calls. Returns vec<whisper_token> on
    // success.
    std::vector<whisper_token> tokenize(std::string *text, size_t max_tokens);

    // Returns id of a given language, raise exception if not found
    int lang_str_to_id(const char *lang);
    // Returns short string of specified language id, raise exception if nullptr
    // is returned
    const char *lang_id_to_str(size_t id);

    // language functions. Returns a vector of probabilities for each language.
    std::vector<float> lang_detect(size_t offset_ms, size_t threads);

    size_t n_len();
    size_t n_vocab();
    size_t n_text_ctx();
    size_t n_audio_ctx();
    bool is_multilingual();

    // Token logits obtained from the last call to whisper_decode()
    // The logits for the last token are stored in the last row
    // Rows: n_tokens
    // Cols: n_vocab
    std::vector<std::vector<float>> get_logits(int segment);

    // Convert token id to string. Use the vocabulary in provided context
    std::string token_to_str(whisper_token token_id);

    // Returns largest language id
    size_t lang_max_id() { return whisper_lang_max_id(); }

    // Some special tokens
    whisper_token eot_token() { return whisper_token_eot(wctx); }
    whisper_token sot_token() { return whisper_token_sot(wctx); }
    whisper_token prev_token() { return whisper_token_prev(wctx); }
    whisper_token solm_token() { return whisper_token_solm(wctx); }
    whisper_token not_token() { return whisper_token_not(wctx); }
    whisper_token beg_token() { return whisper_token_beg(wctx); }
    whisper_token token_translate() { return whisper_token_translate(); }
    whisper_token token_transcribe() { return whisper_token_transcribe(); }
    whisper_token lang_token(int lang_id) {
        return whisper_token_lang(wctx, lang_id);
    }

    // perf inform and sys info
    void print_timings() { whisper_print_timings(wctx); }
    void reset_timings() { whisper_reset_timings(wctx); }
    std::string sys_info() { return std::string(whisper_print_system_info()); }

    // Run the entire model: PCM -> log mel spectrogram -> encoder -> decoder ->
    // text Not thread safe for same context Uses the specified decoding
    // strategy to obtain the text.
    int full(Params params, std::vector<float> data);

    // Split the input audio in chunks and process each chunk separately using
    // whisper_full_with_state() Result is stored in the default state of the
    // context Not thread safe if executed in parallel on the same context. It
    // seems this approach can offer some speedup in some cases. However, the
    // transcription accuracy can be worse at the beginning and end of each
    // chunk.
    int full_parallel(Params params, std::vector<float> data,
                      int num_processor);

    // Number of generated text segments
    // A segment can be a few words, a sentence, or even a paragraph.
    int full_n_segments();

    // Language id associated with the context's default state
    int full_lang_id();

    // Get the start and end time of the specified segment
    int full_get_segment_t0(int segment);

    // Get the end time of the specified segment.
    int full_get_segment_t1(int segment);

    // Get the text of the specified segment
    const char *full_get_segment_text(int segment);

    // Get number of tokens in the specified segment
    int full_n_tokens(int segment);

    // Get the token text of the specified token in the specified segment
    std::string full_get_token_text(int segment, int token);
    whisper_token full_get_token_id(int segment, int token);

    // Get token data for the specified token in the specified segment
    // This contains probabilities, timestamps, etc.
    whisper_token_data full_get_token_data(int segment, int token);

    // Get the probability of the specified token in the specified segment.
    float full_get_token_prob(int segment, int token);
};

void ExportContextApi(py::module &m);
