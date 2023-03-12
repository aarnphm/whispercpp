#pragma once

#ifdef BAZEL_BUILD
#include "context.h"
#include "examples/common.h"
#include "pybind11/functional.h"
#include "pybind11/numpy.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#else
#include "common.h"
#include "context.h"
#include "pybind11/functional.h"
#include "pybind11/numpy.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#endif

namespace py = pybind11;

namespace whisper {
// std::make_unique for C++11
// https://stackoverflow.com/a/17902439/8643197
template <class T> struct _Unique_if {
    typedef std::unique_ptr<T> _Single_object;
};

template <class T> struct _Unique_if<T[]> {
    typedef std::unique_ptr<T[]> _Unknown_bound;
};

template <class T, size_t N> struct _Unique_if<T[N]> {
    typedef void _Known_bound;
};

template <class T, class... Args>
typename _Unique_if<T>::_Single_object make_unique(Args &&...args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T>
typename _Unique_if<T>::_Unknown_bound make_unique(size_t n) {
    typedef typename std::remove_extent<T>::type U;
    return std::unique_ptr<T>(new U[n]());
}

template <class T, class... Args>
typename _Unique_if<T>::_Known_bound make_unique(Args &&...) = delete;

// Some black magic to make zero-copy numpy array
// See https://github.com/pybind/pybind11/issues/1042#issuecomment-642215028
template <typename Sequence>
inline py::array_t<typename Sequence::value_type> as_pyarray(Sequence &&seq) {
    auto size = seq.size();
    auto data = seq.data();
    std::unique_ptr<Sequence> seq_ptr =
        whisper::make_unique<Sequence>(std::move(seq));
    auto capsule = py::capsule(seq_ptr.get(), [](void *p) {
        std::unique_ptr<Sequence>(reinterpret_cast<Sequence *>(p));
    });
    seq_ptr.release();
    return py::array(size, data, capsule);
}
} // namespace whisper

struct WavFileWrapper {
    py::array_t<float> mono;
    std::vector<std::vector<float>> stereo;

    WavFileWrapper(std::vector<float> *mono,
                   std::vector<std::vector<float>> *stereo)
        : mono(whisper::as_pyarray(std::move(*mono))), stereo(*stereo){};

    static WavFileWrapper load_wav_file(const char *filename);
};
