#pragma once
#include "context.h"

#define WITH_DEPRECATION(depr)                                                 \
  PyErr_WarnEx(PyExc_DeprecationWarning,                                       \
               "Setting '" depr                                                \
               "' as an attribute is deprecated and will be remove in "        \
               "future release. Use 'with_" depr "()' instead.",               \
               1)

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
    : fp(std::move(other.fp)),
      new_segment_callback(other.new_segment_callback) {
  fp->new_segment_callback = new_segment_callback_handler;
  fp->new_segment_callback_user_data = new_segment_callback.data.get();
}

Params &Params::operator=(Params const &other) {
  new_segment_callback = other.new_segment_callback;
  fp = other.fp;
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
          py::return_value_policy::reference)
      // FIXME: with_strategy
      // .def("with_strategy",
      // &SamplingStrategies::with_sampling_strategy,
      //      "strategy"_a);
      ;
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
