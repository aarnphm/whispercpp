#include "context.h"
#include <algorithm>
#include <functional>
#include <numeric>
#include <sstream>

namespace whisper {
PYBIND11_MODULE(api, m) {
  m.doc() = "Python interface for whisper.cpp";

  // NOTE: default attributes
  m.attr("SAMPLE_RATE") = py::int_(WHISPER_SAMPLE_RATE);
  m.attr("N_FFT") = py::int_(WHISPER_N_FFT);
  m.attr("N_MEL") = py::int_(WHISPER_N_MEL);
  m.attr("HOP_LENGTH") = py::int_(WHISPER_HOP_LENGTH);
  m.attr("CHUNK_SIZE") = py::int_(WHISPER_CHUNK_SIZE);

  // NOTE: export Context API
  ExportContextApi(m);

  // NOTE: export Params API
  py::enum_<SamplingStrategies::StrategyType>(m, "StrategyType")
      .value("SAMPLING_GREEDY", SamplingStrategies::GREEDY)
      .value("SAMPLING_BEAM_SEARCH", SamplingStrategies::BEAM_SEARCH)
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
}; // namespace whisper
