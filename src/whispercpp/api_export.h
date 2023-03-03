#include "audio.h"
#include "context.h"

namespace py = pybind11;

struct WavFileWrapper {
  py::array_t<float> mono;
  std::vector<std::vector<float>> stereo;

  WavFileWrapper(std::vector<float> *mono,
                 std::vector<std::vector<float>> *stereo)
      : mono(whisper::as_pyarray(std::move(*mono))), stereo(*stereo){};

  static WavFileWrapper load_wav_file(const char *filename);
};
