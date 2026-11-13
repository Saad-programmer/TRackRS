#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests for the data_overrides.json logic of tools/seed_class_packs.py."""
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from seed_class_packs import PACK_CARS, OverrideError, apply_overrides, build  # noqa: E402

URL = 'https://example.org/spec-sheet'


def expect_error(car, overrides, text):
    try:
        apply_overrides(car, overrides)
    except OverrideError as error:
        assert text in str(error), error
        return
    raise AssertionError(f'expected an error about {text}')


def test():
    car = build(PACK_CARS['class-rally-raid'][0])
    assert not any(car['verified'].values()) and car['sources'] == []

    # Full coverage of a group verifies that group only.
    out = apply_overrides(car, {'schema': 'trackrs.car-overrides/1', 'values': {
        'mass.kerb_kg': {'value': 1975, 'source': URL},
        'mass.front_weight_dist': {'value': .51, 'source': URL}}})
    assert out['mass'] == {'kerb_kg': 1975, 'front_weight_dist': .51}
    assert out['verified']['mass'] is True
    assert [g for g, v in out['verified'].items() if v] == ['mass']
    assert out['sources'] == [URL]
    assert 'mass.kerb_kg' in out['provenance']
    assert car['mass']['kerb_kg'] == 2000          # the input is not modified

    # Partial coverage replaces the value but leaves the group unverified.
    out = apply_overrides(car, {'schema': 'trackrs.car-overrides/1', 'values': {
        'engine.redline_rpm': {'value': 6800, 'source': URL}}})
    assert out['engine']['redline_rpm'] == 6800 and out['verified']['engine'] is False
    assert 'no group fully covered' in out['provenance']

    # A fully cited gearbox is marked sourced.
    out = apply_overrides(car, {'schema': 'trackrs.car-overrides/1', 'values': {
        'drivetrain.gearbox.ratios': {'value': [3, 2, 1.5, 1.2, 1.0, .85], 'source': URL},
        'drivetrain.gearbox.final_drive': {'value': 4.3, 'source': URL},
        'drivetrain.gearbox.reverse': {'value': 3.1, 'source': 'https://example.org/other'}}})
    assert out['verified']['gearbox'] and out['drivetrain']['gearbox']['derivation'] == 'sourced'
    assert out['sources'] == [URL, 'https://example.org/other']

    # Errors: missing source, non-https source, unknown value, wrong schema.
    expect_error(car, {'schema': 'trackrs.car-overrides/1', 'values': {'mass.kerb_kg': {'value': 1}}}, 'https://')
    expect_error(car, {'schema': 'trackrs.car-overrides/1',
                       'values': {'mass.kerb_kg': {'value': 1, 'source': 'http://insecure'}}}, 'https://')
    expect_error(car, {'schema': 'trackrs.car-overrides/1',
                       'values': {'engine.horsepower': {'value': 1, 'source': URL}}}, 'unknown value')
    expect_error(car, {'schema': 'other', 'values': {}}, 'schema')

    # No overrides: unchanged.
    assert apply_overrides(car, {'schema': 'trackrs.car-overrides/1', 'values': {}}) == car
    print('PASS: data_overrides.json verifies only fully cited groups and rejects 4 kinds of bad entry')


if __name__ == '__main__':
    test()
