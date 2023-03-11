from __future__ import annotations

import pytest

import whispercpp as w


def test_sampling_greedy():
    # test repr and init
    assert repr(w.api.SamplingGreedyStrategy()) == "SamplingGreedy(best_of=1)"
    assert repr(w.api.SamplingGreedyStrategy(2)) == "SamplingGreedy(best_of=2)"

    # test with_best_of_construction
    assert (
        repr(w.api.SamplingGreedyStrategy().with_best_of(2))
        == "SamplingGreedy(best_of=2)"
    )


def test_sampling_strategy_deprecation():
    with pytest.warns(DeprecationWarning, match=r"Setting 'best_of' as an *"):
        ss = w.api.SamplingGreedyStrategy()
        ss.best_of = 2

    with pytest.warns(DeprecationWarning, match=r"Setting 'patience' as an *"):
        ss = w.api.SamplingBeamSearchStrategy()
        ss.patience = 2

    with pytest.warns(DeprecationWarning, match=r"Setting 'beam_size' as an *"):
        ss = w.api.SamplingBeamSearchStrategy()
        ss.beam_size = 2


def test_sampling_beam_search():
    # test repr and init
    assert (
        repr(w.api.SamplingBeamSearchStrategy())
        == "SamplingBeamSearch(beam_size=-1, patience=-1)"
    )
    assert (
        repr(w.api.SamplingBeamSearchStrategy(2, 2))
        == "SamplingBeamSearch(beam_size=2, patience=2)"
    )

    assert (
        repr(w.api.SamplingBeamSearchStrategy().with_patience(2))
        == "SamplingBeamSearch(beam_size=-1, patience=2)"
    )

    assert (
        repr(w.api.SamplingBeamSearchStrategy().with_beam_size(2))
        == "SamplingBeamSearch(beam_size=2, patience=-1)"
    )

    assert (
        repr(w.api.SamplingBeamSearchStrategy().with_beam_size(2).with_patience(2))
        == "SamplingBeamSearch(beam_size=2, patience=2)"
    )


def test_sampling_strategies_from_enum():
    # test from_enum
    assert w.api.SamplingStrategies.from_enum(w.api.SAMPLING_GREEDY).build()
    assert w.api.SamplingStrategies.from_enum(w.api.SAMPLING_BEAM_SEARCH).build()


def test_sampling_strategy_from_strategy_type():
    # test from_strategy_type
    ss = w.api.SamplingGreedyStrategy()
    assert w.api.SamplingStrategies.from_strategy_type(ss).build()

    ss = w.api.SamplingBeamSearchStrategy()
    assert w.api.SamplingStrategies.from_strategy_type(ss).build()


def test_sampling_strategy_build_copy():
    ss = w.api.SamplingGreedyStrategy()
    assert w.api.SamplingStrategies.from_strategy_type(ss).build() is not ss


def test_sampling_on_setattr_warning():
    with pytest.warns(DeprecationWarning, match=r"Setting 'greedy' as an *"):
        ss = w.api.SamplingStrategies.from_enum(w.api.SAMPLING_GREEDY)
        ss.greedy = w.api.SamplingGreedyStrategy(2)

    with pytest.warns(DeprecationWarning, match=r"Setting 'beam_search' as an *"):
        ss = w.api.SamplingStrategies.from_enum(w.api.SAMPLING_BEAM_SEARCH)
        ss.beam_search = w.api.SamplingBeamSearchStrategy(2, 2)


def test_set_language():
    params = w.api.Params.from_enum(w.api.StrategyType.SAMPLING_GREEDY)
    for lang in ["en", "de", "auto", ""]:
        assert params.language != ""
        params_with_lang = params.with_language(lang)
        print(lang, params_with_lang.language)
        assert params_with_lang.language == lang
