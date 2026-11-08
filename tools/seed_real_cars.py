#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Generate the Car Definition v2 files for the eleven real-car entries.

NOTHING HERE IS VERIFIED. When these files were generated no source could be
consulted (the session's network blocked reference sites), so every value is
an *unverified seed*: approximate, commonly published headline figures (peak
power/torque with rpm, kerb mass, layout, gear count, top speed) recalled by
the author, flagged `verified: false` with an empty `sources` list. Gear ratios
and final drive are SYNTHETIC (fitted so top gear reaches the claimed top speed
at the redline, geometric spacing) and say so in `gearbox.derivation`.
Torque curves are generated per car from that car's own peak figures (no
shared curve). Replace any value with a cited one by editing the table, adding
the URL to `sources` and flipping the matching `verified` key.

Usage: tools/seed_real_cars.py [--check]
"""
import argparse, json, math, pathlib, sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / 'data/vehicles'
PROVENANCE = ('Unverified seed: approximate commonly published figures recalled by the TrackRS author '
              'without access to a source (reference sites unreachable when generated). Gear ratios and '
              'final drive are synthetic. Do not treat as homologation data; cite a source and set verified.')
TRADEMARK = ('{marque} and {model} are trademarks of their respective owners. TrackRS is not affiliated with '
             'or endorsed by them; the name identifies the real car this data approximates.')
# id, display, marque, year, class, era, engine, layout, front share, mass kg, front weight,
# peak torque Nm @ rpm, peak power kW @ rpm, idle, redline, forward gears, top speed km/h,
# tyre radius m, width m, Cd, frontal area m2, fallback body, turbo?
CARS = [
 ('lancia_stratos_hf', 'Lancia Stratos HF', 'Lancia', 1974, 'Group 4', '1973-1981', '2.4 V6 mid-mounted', 'RWD', 0.0, 980, .42, 275, 6500, 206, 8000, 1100, 8500, 5, 230, .30, .225, .42, 1.6, 'fox_wrc', False),
 ('alpine_a110_1800', 'Alpine A110 1800', 'Alpine', 1973, 'Group 4', '1971-1975', '1.8 I4 rear-mounted', 'RWD', 0.0, 720, .38, 180, 5500, 125, 7200, 900, 7600, 5, 210, .29, .205, .42, 1.5, 'fox_wrc', False),
 ('porsche_911_sc_rs', 'Porsche 911 SC RS', 'Porsche', 1984, 'Group B', '1983-1986', '3.0 flat-6 rear-mounted', 'RWD', 0.0, 960, .38, 250, 6500, 188, 7000, 950, 7400, 5, 255, .30, .225, .38, 1.8, 'fox_wrc', False),
 ('audi_quattro_a2', 'Audi Quattro A2', 'Audi', 1983, 'Group B', '1983-1984', '2.1 I5 turbo front-mounted', 'AWD', .5, 1100, .58, 450, 5500, 250, 7500, 1100, 8000, 5, 220, .32, .225, .45, 1.9, 'evo_rally', True),
 ('peugeot_205_t16_e2', 'Peugeot 205 T16 E2', 'Peugeot', 1985, 'Group B', '1985-1986', '1.8 I4 turbo mid-mounted', 'AWD', .35, 910, .43, 490, 5500, 330, 8000, 1200, 8500, 5, 210, .31, .245, .40, 1.8, 'evo_rally', True),
 ('bmw_m3_e30_group_a', 'BMW M3 E30 Group A', 'BMW', 1987, 'Group A', '1987-1992', '2.3 I4 naturally aspirated', 'RWD', 0.0, 1150, .50, 270, 7000, 220, 8500, 1100, 9000, 5, 220, .31, .225, .38, 1.9, 'fox_wrc', False),
 ('lancia_delta_hf_integrale', 'Lancia Delta HF Integrale', 'Lancia', 1989, 'Group A', '1987-1992', '2.0 I4 turbo front-mounted', 'AWD', .47, 1150, .60, 400, 4500, 221, 7000, 950, 7500, 6, 210, .31, .225, .38, 2.0, 'evo_rally', True),
 ('toyota_celica_gt4_st185', 'Toyota Celica GT-Four ST185', 'Toyota', 1992, 'Group A', '1990-1994', '2.0 I4 turbo front-mounted', 'AWD', .45, 1200, .59, 500, 4000, 221, 6000, 1000, 7000, 6, 210, .32, .225, .36, 2.0, 'evo_rally', True),
 ('ford_escort_rs_cosworth', 'Ford Escort RS Cosworth', 'Ford', 1993, 'Group A', '1993-1996', '2.0 I4 turbo front-mounted', 'AWD', .40, 1230, .58, 500, 4000, 221, 6250, 1000, 7000, 6, 210, .32, .225, .38, 2.0, 'evo_rally', True),
 ('subaru_impreza_wrc97', 'Subaru Impreza WRC97', 'Subaru', 1997, 'World Rally Car', '1997-2000', '2.0 flat-4 turbo front-mounted', 'AWD', .50, 1230, .58, 470, 4000, 221, 5500, 1000, 7000, 6, 210, .32, .225, .36, 2.0, 'evo_rally', True),
 ('peugeot_206_wrc', 'Peugeot 206 WRC', 'Peugeot', 1999, 'World Rally Car', '1999-2003', '2.0 I4 turbo front-mounted', 'AWD', .50, 1230, .60, 535, 3500, 221, 5250, 1000, 7000, 6, 210, .32, .225, .36, 2.0, 'evo_rally', True),
]
def omega(rpm): return rpm * math.pi / 30

def torque_curve(t_peak, t_rpm, p_kw, p_rpm, idle, redline, turbo):
    """Piecewise curve from this car's own peaks: rise to peak torque, hold or
    ease to the torque that gives peak power at its rpm, then fall to the
    redline. Turbo engines rise later and harder (boost)."""
    t_at_power = min(t_peak, p_kw * 1000 / omega(p_rpm))
    points = []
    start = idle
    points.append((start, t_peak * (0.30 if turbo else 0.62)))
    points.append((start + (t_rpm - start) * (0.55 if turbo else 0.5), t_peak * (0.72 if turbo else 0.86)))
    points.append((t_rpm, t_peak))
    if p_rpm > t_rpm + 200:
        mid = (t_rpm + p_rpm) / 2
        points.append((mid, (t_peak + t_at_power) / 2 + 0.03 * t_peak if t_at_power < t_peak else t_peak))
        points.append((p_rpm, t_at_power))
    end = redline
    if end > points[-1][0] + 100:
        points.append((end, t_at_power * (0.84 if turbo else 0.88)))
    curve, last = [], 0
    for rpm, nm in points:
        rpm = round(rpm / 50) * 50
        if rpm <= last:
            continue
        curve.append([float(rpm), round(float(nm), 1)])
        last = rpm
    return curve

def synthetic_gears(count, redline, top_kmh, radius, mass, t_peak, layout, front_weight):
    """Synthetic, documented: final drive 4.1; top gear reaches the claimed top
    speed at the redline; first gear is the traction-limited launch ratio
    (driven-axle load x mu 1.3 x tyre radius / peak torque); geometric spacing."""
    final = 4.1
    top = omega(redline) * radius / (top_kmh / 3.6) / final
    driven = 1.0 if layout == 'AWD' else (1 - front_weight if layout == 'RWD' else front_weight)
    first = max(mass * 9.81 * driven * 1.3 * radius / t_peak / final, top * 1.6)
    q = (first / top) ** (1 / (count - 1))
    ratios = [round(top * q ** (count - 1 - i), 3) for i in range(count)]
    return ratios, final

def build(row):
    (cid, name, marque, year, cls, era, engine, layout, front, mass, fw, t_peak, t_rpm, p_kw, p_rpm,
     idle, redline, gears, top, radius, width, cd, area, fallback, turbo) = row
    ratios, final = synthetic_gears(gears, redline, top, radius, mass, t_peak, layout, fw)
    model = name.replace(marque + ' ', '')
    return {
        'schema': 'trackrs.car/2', 'id': cid, 'display_name': name, 'marque': marque, 'year': year,
        'class': cls, 'era': era,
        'license': {'data': 'GPL-2.0-or-later', 'model': '', 'author': 'TrackRS contributors', 'source_url': '',
                    'trademark_notice': TRADEMARK.format(marque=marque, model=model)},
        'verified': {k: False for k in ('mass', 'engine', 'gearbox', 'tyres', 'aero', 'brakes')},
        'sources': [], 'provenance': PROVENANCE,
        'body': {'mesh': '', 'fallback': fallback, 'import_scale': 1.0, 'unit': 'm'},
        'mass': {'kerb_kg': mass, 'front_weight_dist': fw},
        'engine': {'layout': engine, 'idle_rpm': idle, 'redline_rpm': redline,
                   'torque_curve_nm': torque_curve(t_peak, t_rpm, p_kw, p_rpm, idle, redline, turbo)},
        'drivetrain': {'layout': layout, 'front_torque_share': front,
                       'gearbox': {'ratios': ratios, 'final_drive': final, 'reverse': round(ratios[0] * 1.05, 3),
                                   'shift_time_s': 0.12 if year >= 1997 else 0.25,
                                   'derivation': 'synthetic: final drive 4.1, top gear reaches the claimed top speed at the redline, first gear traction-limited (mu 1.3), geometric spacing'}},
        'tyres': {'radius_m': radius, 'width_m': width},
        'aero': {'cd': cd, 'frontal_area_m2': area, 'cl_front': 0.0, 'cl_rear': 0.0},
        'brakes': {'bias_front': 0.62 if layout != 'RWD' else 0.6},
        'claims': {'top_speed_kmh': top, 'zero_to_100_s': 0},
    }

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--check', action='store_true'); a = ap.parse_args()
    stale = []
    for row in CARS:
        car = build(row)
        path = OUT / car['id'] / 'car.json'
        text = json.dumps(car, indent=2) + '\n'
        if a.check:
            if not path.exists() or path.read_text() != text:
                stale.append(str(path.relative_to(ROOT)))
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text)
    if stale:
        sys.exit('stale or missing car.json: ' + ', '.join(stale))
    print(('PASS: ' if a.check else 'wrote ') + f'{len(CARS)} car.json files')
if __name__ == '__main__': main()
