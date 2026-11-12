#!/usr/bin/env python3
"""Draws a WAV's waveform, level and spectrogram to a PNG, for looking at what
cannot be listened to: a pipe's speech, a demo's build.

    python3 Tools/spectrogram.py in.wav out.png [--top HZ] [--from S] [--to S] [--title TEXT]
"""
import argparse
import struct
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np


def load(path):
    """PCM WAV (16/24/32-bit integer or 32-bit float) as float frames x channels."""
    data = open(path, "rb").read()
    at, fmt, rate, channels, bits, frames = 12, 1, 48000, 2, 24, None
    while at + 8 <= len(data):
        tag, size = data[at:at + 4], struct.unpack("<I", data[at + 4:at + 8])[0]
        body = data[at + 8:at + 8 + size]
        if tag == b"fmt ":
            fmt, channels, rate, _, _, bits = struct.unpack("<HHIIHH", body[:16])
        elif tag == b"data":
            if fmt == 3:
                frames = np.frombuffer(body, dtype="<f4").astype(np.float64)
            elif bits == 16:
                frames = np.frombuffer(body, dtype="<i2") / 32768.0
            elif bits == 24:
                raw = np.frombuffer(body[:len(body) // 3 * 3], dtype=np.uint8).reshape(-1, 3).astype(np.int32)
                value = raw[:, 0] | (raw[:, 1] << 8) | (raw[:, 2] << 16)
                frames = np.where(value >= 1 << 23, value - (1 << 24), value) / float(1 << 23)
            else:
                frames = np.frombuffer(body, dtype="<i4") / float(1 << 31)
        at += 8 + size + (size & 1)
    if frames is None:
        sys.exit(f"{path}: no audio data")
    return frames.reshape(-1, channels), rate


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("wav")
    parser.add_argument("png")
    parser.add_argument("--top", type=float, default=8000.0, help="highest frequency drawn, Hz")
    parser.add_argument("--from", dest="start", type=float, default=0.0, help="first second drawn")
    parser.add_argument("--to", dest="end", type=float, default=None, help="last second drawn")
    parser.add_argument("--title", default=None)
    parser.add_argument("--fft", type=int, default=4096)
    args = parser.parse_args()

    frames, rate = load(args.wav)
    first = int(args.start * rate)
    last = len(frames) if args.end is None else min(len(frames), int(args.end * rate))
    x = frames[first:last].mean(axis=1)
    t = args.start + np.arange(len(x)) / rate

    hop = max(1, rate // 200)
    blocks = len(x) // hop
    level = 20 * np.log10(np.sqrt((x[:blocks * hop].reshape(blocks, hop) ** 2).mean(axis=1)) + 1e-9)

    fig, ax = plt.subplots(3, 1, figsize=(12, 9), sharex=True, gridspec_kw={"height_ratios": [1, 1, 3]})
    ax[0].plot(t, x, color="#3060a0", lw=0.4)
    ax[0].set_ylabel("waveform")
    ax[0].grid(alpha=0.3)
    ax[1].plot(args.start + (np.arange(blocks) + 0.5) * hop / rate, level, color="#a03030", lw=0.8)
    ax[1].set_ylabel("level, dBFS (5 ms)")
    ax[1].set_ylim(max(-90, level.max() - 70), level.max() + 3)
    ax[1].grid(alpha=0.3)
    nfft = min(args.fft, max(256, 1 << int(np.log2(max(256, len(x) // 8)))))
    ax[2].specgram(x, NFFT=nfft, Fs=rate, noverlap=nfft * 3 // 4, cmap="magma", vmin=-150, vmax=-30, scale="dB",
                   xextent=(t[0], t[-1]))
    ax[2].set_ylim(0, args.top)
    ax[2].set_ylabel("Hz")
    ax[2].set_xlabel("s")
    fig.suptitle(args.title or args.wav.split("/")[-1])
    fig.tight_layout()
    fig.savefig(args.png, dpi=80)


if __name__ == "__main__":
    main()
