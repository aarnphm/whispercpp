#include "context.h"
#include <algorithm>
#include <functional>
#include <numeric>
#include <sstream>

namespace whisper {

struct new_segment_callback_data {
  std::vector<std::string> *results;
};

struct Whisper {
  Context ctx;
  FullParams params;

  Whisper() : ctx(), params(defaults()){};
  Whisper(const char *model_path)
      : ctx(Context::from_file(model_path)), params(defaults()) {}

  // Set default params to recommended.
  static FullParams defaults() {
    FullParams p = FullParams::from_sampling_strategy(
        SamplingStrategies::from_strategy_type(SamplingStrategies::GREEDY));
    // disable printing progress
    p.set_print_progress(false);
    // disable realtime print, using callback
    p.set_print_realtime(false);

    // invoke new_segment_callback for faster transcription.
    p.set_new_segment_callback([](struct whisper_context *ctx, int n_new,
                                  void *user_data) {
      const auto &results = ((new_segment_callback_data *)user_data)->results;

      const int n_segments = whisper_full_n_segments(ctx);

      for (int i = n_segments - n_new; i < n_segments; i++) {
        const char *text = whisper_full_get_segment_text(ctx, i);
        results->push_back(text);
      };
    });
    return p;
  }

  std::string transcribe(std::vector<float> data, int num_proc) {
    std::vector<std::string> results;
    new_segment_callback_data user_data = {&results};
    params.set_new_segment_callback_user_data(&user_data);
    if (ctx.full_parallel(params, data, num_proc) != 0) {
      throw std::runtime_error("transcribe failed");
    }

    const char *const delim = "";
    // We are allocating a new string for every element in the vector.
    // This is not efficient, for larger files.
    return std::accumulate(results.begin(), results.end(), std::string(delim));
  };
};

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
  ExportParamsApi(m);

  py::class_<Whisper>(m, "WhisperPreTrainedModel")
      .def(py::init<>())
      .def(py::init<const char *>())
      .def_property(
          "context", [](Whisper &self) { return self.ctx; },
          [](Whisper &self, Context &ctx) { self.ctx = ctx; })
      .def_property(
          "params", [](Whisper &self) { return self.params; },
          [](Whisper &self, FullParams &params) { self.params = params; })
      .def("transcribe", &Whisper::transcribe, "data"_a, "num_proc"_a = 1);
}
}; // namespace whisper
