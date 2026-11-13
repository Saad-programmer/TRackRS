#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Phase 4d class packs: Car Definition v2 entries, fictional liveries and
the pack manifests for `class-rally-raid` and `class-heritage-raid`.

THE PERFORMANCE FIGURES ARE CLASS-TYPICAL PLACEHOLDERS, NOT THE NAMED CAR'S
DATA. Each entry identifies a real car (with the trademark notice the
validator requires), but its mass, torque, power, gearing and aero are
typical values for its class and era, chosen by the TrackRS author so the
car drives plausibly until cited figures replace them. Every `verified` key
is false, `sources` is empty, and the provenance says exactly this.

Models: no mesh is shipped. Each car borrows a fictional legacy body and is
listed as "Name (placeholder model)" until a mesh that meets
docs/ASSET_SOURCING.md arrives. Liveries are fictional (generated sponsor
marks, no team or sponsor scheme), with opaque backings over the borrowed
bodies' legacy lettering.

Not modelled: electric rally-raid prototypes (single-speed, range extender)
need the Phase 6 powertrain, so none is listed. 37-inch tyres exceed the
validator's 0.45 m radius limit; such cars use 0.45 m.

Usage: tools/seed_class_packs.py [--check]
"""
import argparse
import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from seed_real_cars import synthetic_gears, torque_curve  # noqa: E402  (shared, documented derivations)

ROOT = pathlib.Path(__file__).resolve().parents[1]
PACKS = ROOT / 'data/packs'
PROVENANCE = ('Class-typical placeholder figures for a {cls} car of its era, chosen by the TrackRS author; NOT '
              'data of this specific car. Gear ratios and final drive are synthetic. Replace each value with a '
              'cited figure, add the URL to sources and set the matching verified key.')
TRADEMARK = ('{marque} and {model} are trademarks of their respective owners. TrackRS is not affiliated with or '
             'endorsed by them; the name identifies the real car this entry stands for. No logo, badge or team '
             'livery is reproduced.')
# Livery slot sets measured on the borrowed bodies' atlases (Phase M2/4c).
SLOTS = {
    'cordo_super500': {'number_panels': [{'rect': [0.398, 0.52, 0.14, 0.117]}, {'rect': [0.461, 0.828, 0.133, 0.125]}],
                       'sponsor_slots': [[0.54, 0.51, 0.32, 0.14], [0.214, 0.82, 0.24, 0.15], [0.762, 0.229, 0.098, 0.049]],
                       'bonnet': [0.117, 0.117, 0.102, 0.254]},
    'fox_kitcar': {'number_panels': [],
                   'sponsor_slots': [[0.258, 0.719, 0.18, 0.07], [0.258, 0.859, 0.18, 0.07]],
                   'bonnet': [0.454, 0.405, 0.093, 0.132]},
}
SOURCE_COLOURS = {'cordo_super500': ('#0044aa', '#ffff00'), 'fox_kitcar': ('#3f8799', '#e36c1c')}

# id, display name, marque, year, class, era, engine, layout, front share, mass kg, front weight,
# peak torque Nm @ rpm, peak power kW @ rpm, idle, redline, gears, top km/h, tyre radius m, width m,
# Cd, frontal area m2, fallback body, turbo?, primary, secondary, number
PACK_CARS = {
    'class-rally-raid': [
        ('toyota_gr_dkr_hilux_t1plus', 'Toyota GR DKR Hilux T1+', 'Toyota', 2022, 'Rally-raid T1+', '2022-',
         '3.5 V6 twin-turbo front-mid (restricted)', 'AWD', .5, 2000, .52, 620, 3500, 290, 5000, 900, 6500, 6, 170,
         .45, .30, .5, 3.0, 'cordo_super500', True, '#c9c9c4', '#8b1e1e', 301),
        ('prodrive_hunter_t1plus', 'Prodrive Hunter T1+', 'Prodrive', 2021, 'Rally-raid T1+', '2021-',
         '3.5 V6 twin-turbo front-mid (restricted)', 'AWD', .5, 2000, .52, 620, 3500, 290, 5000, 900, 6500, 6, 170,
         .45, .30, .5, 3.0, 'cordo_super500', True, '#1d2f45', '#d9a441', 302),
        ('ford_raptor_t1plus', 'Ford Raptor T1+', 'Ford', 2024, 'Rally-raid T1+', '2024-',
         '3.5 V6 twin-turbo front-mid (restricted)', 'AWD', .5, 2050, .53, 620, 3500, 290, 5000, 900, 6500, 6, 170,
         .45, .30, .52, 3.0, 'cordo_super500', True, '#e6e2d6', '#2b5d3a', 303),
        ('canam_maverick_xrs_t4', 'Can-Am Maverick XRS (T4)', 'Can-Am', 2023, 'Rally-raid T4 SSV', '2021-',
         '0.9 three-cylinder turbo mid (CVT approximated by 5 ratios)', 'AWD', .4, 900, .42, 190, 6000, 145, 7250,
         1200, 8500, 5, 140, .41, .26, .55, 1.9, 'fox_kitcar', True, '#f0c419', '#202020', -1),
    ],
    'class-heritage-raid': [
        ('porsche_959_raid_1986', 'Porsche 959 (rally-raid, 1986)', 'Porsche', 1986, 'Heritage raid prototype',
         '1984-1986', '2.85 flat-6 twin-turbo rear-mounted', 'AWD', .4, 1260, .42, 500, 5000, 290, 6500, 950,
         7500, 5, 210, .36, .25, .38, 2.0, 'cordo_super500', True, '#efefea', '#2c4f9e', 185),
        ('peugeot_405_t16_gr_1988', 'Peugeot 405 T16 Grand Raid', 'Peugeot', 1988, 'Heritage raid prototype',
         '1988-1990', '1.9 I4 turbo mid-mounted', 'AWD', .45, 1280, .45, 480, 4500, 290, 6500, 1000, 7500, 5, 200,
         .37, .25, .42, 2.2, 'cordo_super500', True, '#f2f2ee', '#1c1c1c', 205),
        ('citroen_zx_rallye_raid_1991', 'Citroen ZX Rallye-Raid', 'Citroen', 1991, 'Heritage raid prototype',
         '1991-1997', '2.0 I4 turbo front-mounted', 'AWD', .5, 1500, .5, 420, 4500, 230, 6000, 950, 7000, 5, 190,
         .38, .25, .45, 2.3, 'cordo_super500', True, '#d63a2b', '#f2f2ee', 207),
        ('mitsubishi_pajero_evolution_1998', 'Mitsubishi Pajero Evolution (raid, 1998)', 'Mitsubishi', 1998,
         'Heritage raid production-based', '1997-2001', '3.5 V6 front-mounted', 'AWD', .5, 1900, .55, 400, 4500,
         210, 6500, 850, 7000, 5, 190, .40, .27, .5, 2.8, 'cordo_super500', False, '#e8e6df', '#b0262c', 209),
        ('range_rover_classic_raid_1981', 'Range Rover (raid, 1981)', 'Land Rover', 1981,
         'Heritage raid production-based', '1979-1985', '3.5 V8 front-mounted', 'AWD', .5, 1700, .52, 270, 3000,
         115, 5000, 700, 5500, 4, 160, .37, .22, .55, 2.8, 'cordo_super500', False, '#c4b38a', '#3a3a32', 211),
    ],
}
PACK_INFO = {
    'class-rally-raid': ('Rally-raid class pack',
                         'Modern rally-raid cars (T1+ prototypes, T4 SSV) with class-typical placeholder figures and '
                         'placeholder models. Real makes are named for identification only.'),
    'class-heritage-raid': ('Heritage raid class pack',
                            '1980s-1990s raid prototypes and production-based raid cars with class-typical placeholder '
                            'figures and placeholder models. Real makes are named for identification only.'),
}


def build(row):
    (cid, name, marque, year, cls, era, engine, layout, front, mass, fw, t_peak, t_rpm, p_kw, p_rpm, idle,
     redline, gears, top, radius, width, cd, area, fallback, turbo, *_livery) = row
    ratios, final = synthetic_gears(gears, redline, top, radius, mass, t_peak, layout, fw)
    model = name.replace(marque + ' ', '')
    return {
        'schema': 'trackrs.car/2', 'id': cid, 'display_name': name, 'marque': marque, 'year': year,
        'class': cls, 'era': era,
        'license': {'data': 'GPL-2.0-or-later', 'model': '', 'author': 'TrackRS contributors', 'source_url': '',
                    'trademark_notice': TRADEMARK.format(marque=marque, model=model)},
        'verified': {k: False for k in ('mass', 'engine', 'gearbox', 'tyres', 'aero', 'brakes')},
        'sources': [], 'provenance': PROVENANCE.format(cls=cls),
        'body': {'mesh': '', 'fallback': fallback, 'import_scale': 1.0, 'unit': 'm'},
        'mass': {'kerb_kg': mass, 'front_weight_dist': fw},
        'engine': {'layout': engine, 'idle_rpm': idle, 'redline_rpm': redline,
                   'torque_curve_nm': torque_curve(t_peak, t_rpm, p_kw, p_rpm, idle, redline, turbo)},
        'drivetrain': {'layout': layout, 'front_torque_share': front,
                       'gearbox': {'ratios': ratios, 'final_drive': final, 'reverse': round(ratios[0] * 1.05, 3),
                                   'shift_time_s': 0.12 if year >= 1997 else 0.25,
                                   'derivation': 'synthetic: final drive 4.1, top gear reaches the claimed top speed '
                                                 'at the redline, first gear traction-limited (mu 1.3), geometric spacing'}},
        'tyres': {'radius_m': radius, 'width_m': width},
        'aero': {'cd': cd, 'frontal_area_m2': area, 'cl_front': 0.0, 'cl_rear': 0.0},
        'brakes': {'bias_front': 0.6},
        'claims': {'top_speed_kmh': top, 'zero_to_100_s': 0},
    }


def livery(row, seed):
    cid, fallback = row[0], row[23]
    primary, secondary, number = row[25], row[26], row[27]
    slots = SLOTS[fallback]
    src = SOURCE_COLOURS[fallback]
    sponsor = [{'rect': r, 'backing': primary} for r in slots['sponsor_slots']]
    sponsor.append({'rect': slots['bonnet'], 'rotation': 90, 'backing': primary})
    return {'schema': 'trackrs.livery/1', 'id': cid + '_fictional',
            'note': 'Fictional livery (placeholder model): generated sponsor marks, no team, sponsor or manufacturer '
                    'scheme or logo.',
            'primary': primary, 'primary_source': src[0], 'secondary': secondary, 'secondary_source': src[1],
            'number': number, 'number_colour': '#111111', 'panel_colour': '#f2f2f2',
            'number_panels': slots['number_panels'] if number >= 0 else [], 'sponsor_slots': sponsor,
            'sponsor_seed': seed}


def manifest(pack_id, cars):
    name, description = PACK_INFO[pack_id]
    notices = ['Placeholder models: no car mesh is shipped; every car borrows a fictional legacy body until a mesh '
               'meeting docs/ASSET_SOURCING.md is supplied.',
               'Class-typical placeholder performance figures, not the named cars\' data (verified: false).',
               'Real makes are trademarks of their owners and are named for identification only; no logo, badge, '
               'team or sponsor livery is reproduced.']
    licence = {'spdx': 'GPL-2.0-or-later', 'attribution': 'TrackRS contributors', 'verified': True}
    return {'schema': 'trackrs.pack/1', 'id': pack_id, 'name': name, 'description': description,
            'version': '0.1.0', 'requires_engine': '>=0.6.7 <1.0', 'content_api': 1,
            'authors': ['TrackRS contributors'],
            'license': {'spdx': 'GPL-2.0-or-later', 'attribution': 'TrackRS contributors'},
            'notices': notices,
            'assets': [{'path': 'pack.json', 'kind': 'data', 'license': licence},
                       {'path': 'README.md', 'kind': 'data', 'license': licence},
                       {'path': 'vehicles/**', 'kind': 'vehicle',
                        'license': {'spdx': 'GPL-2.0-or-later',
                                    'attribution': 'Car definitions and fictional liveries generated by '
                                                   'tools/seed_class_packs.py (TrackRS contributors); placeholder '
                                                   'models borrow the fictional legacy bodies ' +
                                                   ', '.join(sorted({c[23] for c in cars})) + '.',
                                    'verified': True}}]}


def readme(pack_id, cars):
    name, description = PACK_INFO[pack_id]
    lines = [f'# {name} (`{pack_id}`)', '', description, '',
             'Generated by `tools/seed_class_packs.py`; do not edit by hand. The full roster with licence, verified '
             'and model status is in [docs/CARS.md](../../../docs/CARS.md); the mesh specification for these cars '
             'is in [docs/ASSET_SOURCING.md](../../../docs/ASSET_SOURCING.md).', '', '| id | car | class | model |',
             '|---|---|---|---|']
    for c in cars:
        lines.append(f'| `{c[0]}` | {c[1]} | {c[4]} | placeholder (borrows `{c[23]}`) |')
    return '\n'.join(lines) + '\n'


def files():
    out = {}
    seed = 400
    for pack_id, cars in PACK_CARS.items():
        root = PACKS / pack_id
        out[root / 'pack.json'] = json.dumps(manifest(pack_id, cars), indent=2) + '\n'
        out[root / 'README.md'] = readme(pack_id, cars)
        for row in cars:
            seed += 1
            out[root / 'vehicles' / row[0] / 'car.json'] = json.dumps(build(row), indent=2) + '\n'
            out[root / 'vehicles' / row[0] / 'livery.json'] = json.dumps(livery(row, seed), indent=2) + '\n'
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true')
    a = ap.parse_args()
    stale = []
    for path, text in files().items():
        if a.check:
            if not path.exists() or path.read_text(encoding='utf-8') != text:
                stale.append(str(path.relative_to(ROOT)))
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding='utf-8')
    if stale:
        sys.exit('stale or missing (run tools/seed_class_packs.py): ' + ', '.join(stale))
    print(('PASS: ' if a.check else 'wrote ') + f'{len(files())} class-pack files')


if __name__ == '__main__':
    main()
