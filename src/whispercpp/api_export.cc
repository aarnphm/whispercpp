#include "context.h"
#include <iterator>
#include <sstream>
#include <stdio.h>

namespace whisper {

struct Whisper {
  Context ctx;
  FullParams params;

  Whisper() : ctx(), params(defaults()){};
  Whisper(const char *model_path)
      : ctx(Context::from_file(model_path)), params(defaults()) {}

  static FullParams defaults() {
    SamplingStrategies st = SamplingStrategies();
    st.type = SamplingStrategies::GREEDY;
    st.greedy = SamplingGreedy();
    FullParams p = FullParams::from_sampling_strategy(st);
    // disable printing progress
    p.set_print_progress(false);
    return p;
  }

  std::string transcribe(std::vector<float> data, int num_proc) {
    std::vector<std::string> res;
    int ret;
    if (num_proc > 0) {
      ret = ctx.full_parallel(params, data, num_proc);
    } else {
      ret = ctx.full(params, data);
    }
    if (ret != 0) {
      throw std::runtime_error("transcribe failed");
    }
    for (int i = 0; i < ctx.full_n_segments(); i++) {
      res.push_back(ctx.full_get_segment_text(i));
    }

    // We are copying this in memory here, not ideal.
    const char *const delim = "";
    std::ostringstream imploded;
    std::copy(res.begin(), res.end(),
              std::ostream_iterator<std::string>(imploded, delim));
    return imploded.str();
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
      .def_property_readonly_static("context",
                                    [](Whisper &self) { return self.ctx; })
      .def_property_readonly_static("params",
                                    [](Whisper &self) { return self.params; })
      .def("transcribe", &Whisper::transcribe, "data"_a, "num_proc"_a = 1);
}
}; // namespace whisper
