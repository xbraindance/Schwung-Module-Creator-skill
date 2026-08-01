#!/usr/bin/env python3
"""
Fit a BD-VHS MachineProfile row from a real swept-sine measurement.

The profiles shipped in core/src/ModelProfiles.h are designed by ear from the
characteristic response of each class of recording machine. If you have access
to actual hardware, this turns a measurement into a profile row instead.

Procedure
---------
1. Play a logarithmic sweep through the machine and record the result. The
   test suite can generate a suitable sweep:

       ./bd_vhs_tests --dump /tmp/dump      # bd-vhs_sweep_clean.wav is the reference

   Record that file through the hardware and keep the result at the same
   sample rate.

2. Run:

       python3 fit_profile.py reference.wav recorded.wav --name "VHS SP"

3. Paste the emitted row into kProfiles[] in core/src/ModelProfiles.h.

Requires numpy and scipy.
"""

from __future__ import annotations

import argparse
import struct
import sys
import wave

try:
    import numpy as np
    from scipy.optimize import least_squares
except ImportError:  # pragma: no cover
    sys.exit("this tool needs numpy and scipy: pip install numpy scipy")


# ---------------------------------------------------------------------------
# I/O
# ---------------------------------------------------------------------------

def read_wav(path: str) -> tuple[np.ndarray, int]:
    """Reads a mono or stereo WAV as float64, returning (mono_samples, rate)."""
    with wave.open(path, "rb") as w:
        rate = w.getframerate()
        channels = w.getnchannels()
        width = w.getsampwidth()
        raw = w.readframes(w.getnframes())

    if width == 2:
        data = np.frombuffer(raw, dtype="<i2").astype(np.float64) / 32768.0
    elif width == 4:
        # Either 32-bit PCM or IEEE float; float is what our dumps produce.
        try:
            data = np.frombuffer(raw, dtype="<f4").astype(np.float64)
        except ValueError:
            data = np.frombuffer(raw, dtype="<i4").astype(np.float64) / 2147483648.0
    elif width == 3:
        as_bytes = np.frombuffer(raw, dtype=np.uint8).reshape(-1, 3)
        ints = (as_bytes[:, 0].astype(np.int32)
                | (as_bytes[:, 1].astype(np.int32) << 8)
                | (as_bytes[:, 2].astype(np.int32) << 16))
        ints[ints >= 1 << 23] -= 1 << 24
        data = ints.astype(np.float64) / 8388608.0
    else:
        sys.exit(f"unsupported sample width {width * 8} bit in {path}")

    if channels > 1:
        data = data.reshape(-1, channels).mean(axis=1)

    return data, rate


# ---------------------------------------------------------------------------
# Measurement
# ---------------------------------------------------------------------------

def magnitude_response(reference: np.ndarray, recorded: np.ndarray,
                       rate: int, num_bins: int = 240) -> tuple[np.ndarray, np.ndarray]:
    """
    Deconvolves the recording against the reference sweep and returns
    (frequencies_hz, magnitude_db) on a log-spaced grid.
    """
    n = 1 << int(np.ceil(np.log2(max(len(reference), len(recorded)) * 2)))

    ref_fft = np.fft.rfft(reference, n)
    rec_fft = np.fft.rfft(recorded, n)

    # Regularised division: keeps the very quiet bins at the extremes of the
    # sweep from exploding.
    epsilon = 1e-6 * np.max(np.abs(ref_fft))
    transfer = rec_fft / (ref_fft + epsilon)

    freqs = np.fft.rfftfreq(n, 1.0 / rate)
    mag_db = 20.0 * np.log10(np.maximum(np.abs(transfer), 1e-9))

    grid = np.geomspace(20.0, min(20000.0, rate * 0.45), num_bins)
    smoothed = np.interp(grid, freqs, mag_db)

    # Third-octave smoothing so the fit sees the shape, not the ripple.
    width = max(1, num_bins // 60)
    kernel = np.ones(width) / width
    smoothed = np.convolve(smoothed, kernel, mode="same")

    return grid, smoothed


# ---------------------------------------------------------------------------
# Model
# ---------------------------------------------------------------------------

def model_response(params: np.ndarray, freqs: np.ndarray) -> np.ndarray:
    """
    Analytic magnitude of the same cascade ModelEq builds:
    highpass, LF bump, tilt about 900 Hz, presence bell, lowpass.
    """
    (hp_hz, hp_q, bump_hz, bump_db, bump_q, tilt_db,
     pres_hz, pres_db, pres_q, lp_hz, lp_q, out_db) = params

    w = freqs / np.maximum(hp_hz, 1.0)
    highpass = 20 * np.log10(w ** 2 / np.sqrt((1 - w ** 2) ** 2 + (w / hp_q) ** 2))

    w = freqs / np.maximum(lp_hz, 1.0)
    lowpass = 20 * np.log10(1.0 / np.sqrt((1 - w ** 2) ** 2 + (w / lp_q) ** 2))

    def bell(centre_hz, gain_db, q):
        ratio = np.log2(freqs / np.maximum(centre_hz, 1.0))
        return gain_db * np.exp(-(ratio * q) ** 2)

    tilt = tilt_db * np.tanh(np.log2(freqs / 900.0) * 0.7) * 0.5

    return (highpass + lowpass
            + bell(bump_hz, bump_db, bump_q)
            + bell(pres_hz, pres_db, pres_q)
            + tilt + out_db)


INITIAL = np.array([60.0, 0.7, 110.0, 1.0, 1.0, -1.5,
                    3500.0, -3.0, 1.1, 11000.0, 0.8, 1.0])

LOWER = np.array([10.0, 0.3, 20.0, -12.0, 0.3, -18.0,
                  200.0, -18.0, 0.3, 800.0, 0.3, -12.0])

UPPER = np.array([2000.0, 3.0, 2000.0, 12.0, 4.0, 18.0,
                  12000.0, 18.0, 5.0, 20000.0, 3.0, 18.0])


def fit(freqs: np.ndarray, measured_db: np.ndarray) -> np.ndarray:
    def residual(p):
        return model_response(p, freqs) - measured_db

    result = least_squares(residual, INITIAL, bounds=(LOWER, UPPER),
                           x_scale="jac", max_nfev=20000)
    return result.x


# ---------------------------------------------------------------------------

def emit_row(name: str, p: np.ndarray, sat_bias: float, noise_tilt: float) -> str:
    return (
        '    {{ "{name}", {hp:8.1f}f, {hpq:.2f}f, {bh:6.1f}f, {bd:+.1f}f, {bq:.1f}f, '
        '{tilt:+.1f}f, {ph:7.1f}f, {pd:+.1f}f, {pq:.1f}f, {lp:8.1f}f, {lpq:.2f}f, '
        '{sat:.2f}f, {nt:+.1f}f, {out:+.1f}f }},'
    ).format(name=name, hp=p[0], hpq=p[1], bh=p[2], bd=p[3], bq=p[4], tilt=p[5],
             ph=p[6], pd=p[7], pq=p[8], lp=p[9], lpq=p[10],
             sat=sat_bias, nt=noise_tilt, out=p[11])


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("reference", help="the sweep as sent to the machine")
    ap.add_argument("recorded", help="the sweep as recovered from the machine")
    ap.add_argument("--name", default="Measured", help="profile name")
    ap.add_argument("--sat-bias", type=float, default=1.0,
                    help="extra saturation drive for this machine (not fitted)")
    ap.add_argument("--noise-tilt", type=float, default=-2.0,
                    help="hiss colouration in dB (not fitted)")
    args = ap.parse_args()

    reference, rate_a = read_wav(args.reference)
    recorded, rate_b = read_wav(args.recorded)

    if rate_a != rate_b:
        sys.exit(f"sample rates differ: {rate_a} vs {rate_b}")

    freqs, measured = magnitude_response(reference, recorded, rate_a)
    params = fit(freqs, measured)

    error = model_response(params, freqs) - measured
    print(f"# fit RMS error: {np.sqrt(np.mean(error ** 2)):.2f} dB", file=sys.stderr)
    print(f"# worst-case error: {np.max(np.abs(error)):.2f} dB", file=sys.stderr)
    print("# paste into kProfiles[] in core/src/ModelProfiles.h", file=sys.stderr)

    print(emit_row(args.name, params, args.sat_bias, args.noise_tilt))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
