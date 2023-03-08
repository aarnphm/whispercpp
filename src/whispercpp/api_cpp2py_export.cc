#include "api_cpp2py_export.h"

WavFileWrapper WavFileWrapper::load_wav_file(const char *filename) {
  std::vector<float> pcmf32;
  std::vector<std::vector<float>> pcmf32s;
  if (!::read_wav(filename, pcmf32, pcmf32s, false)) {
    throw std::runtime_error("Failed to load wav file");
  }
  return WavFileWrapper(&pcmf32, &pcmf32s);
}

namespace py = pybind11;
using namespace pybind11::literals;

namespace whisper {

PYBIND11_MODULE(api_cpp2py_export, m) {
  m.doc() = "Python interface for whisper.cpp";

  // NOTE: default attributes
  m.attr("SAMPLE_RATE") = py::int_(WHISPER_SAMPLE_RATE);
  m.attr("N_FFT") = py::int_(WHISPER_N_FFT);
  m.attr("N_MEL") = py::int_(WHISPER_N_MEL);
  m.attr("HOP_LENGTH") = py::int_(WHISPER_HOP_LENGTH);
  m.attr("CHUNK_SIZE") = py::int_(WHISPER_CHUNK_SIZE);

  py::enum_<whisper_sampling_strategy>(m, "StrategyType")
      .value("SAMPLING_GREEDY",
             whisper_sampling_strategy::WHISPER_SAMPLING_GREEDY)
      .value("SAMPLING_BEAM_SEARCH",
             whisper_sampling_strategy::WHISPER_SAMPLING_BEAM_SEARCH)
      .export_values();

  m.def("load_wav_file", &WavFileWrapper::load_wav_file, "filename"_a,
        py::return_value_policy::reference);

  py::class_<WavFileWrapper>(m, "Wavfile",
                             "A light wrapper for the processed wav file.")
      .def_property_readonly(
          "stereo", [](WavFileWrapper &self) { return self.stereo; },
          py::return_value_policy::reference)
      .def_property_readonly(
          "mono", [](WavFileWrapper &self) { return self.mono; },
          py::return_value_policy::reference);

  // NOTE: export Context API
  ExportContextApi(m);

  // NOTE: export Params API
  ExportSamplingStrategiesApi(m);
  ExportParamsApi(m);
}
}; // namespace whisper
