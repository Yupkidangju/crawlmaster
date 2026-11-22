#!/usr/bin/env python3
"""Crawlmaster v1.0.0 다크 판타지 오디오 자산 합성기 (BGM 5종 + 바이옴 환경음 5종 + SFX 24종).
표준 라이브러리(math/wave/struct/random)만으로 결정론적이고 클리핑 없는 16-bit PCM WAV를 생성한다.
"""

from __future__ import annotations

import math
import random
import struct
import wave
from pathlib import Path

SR = 22050  # 선명한 타격감과 적절한 배포 패키지 크기의 균형 (22.05kHz mono/stereo)
TWO_PI = 2.0 * math.pi


def write_wav(path: Path, samples: list[float], sample_rate: int = SR) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    peak = max((abs(s) for s in samples), default=0.0)
    scale = 0.88 / peak if peak > 0.88 else 1.0
    pcm = bytearray()
    for s in samples:
        v = max(-1.0, min(1.0, s * scale))
        pcm += struct.pack("<h", int(v * 32767.0))
    with wave.open(str(path), "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        wf.writeframes(pcm)


def lowpass(samples: list[float], cutoff_hz: float, sr: int = SR) -> list[float]:
    rc = 1.0 / (TWO_PI * max(20.0, cutoff_hz))
    dt = 1.0 / sr
    alpha = dt / (rc + dt)
    out = [0.0] * len(samples)
    prev = 0.0
    for i, x in enumerate(samples):
        prev = prev + alpha * (x - prev)
        out[i] = prev
    return out


def simple_reverb(samples: list[float], mix: float = 0.32, sr: int = SR) -> list[float]:
    delays = [int(sr * 0.037), int(sr * 0.059), int(sr * 0.089), int(sr * 0.131)]
    decays = [0.48, 0.39, 0.31, 0.24]
    n = len(samples)
    wet = [0.0] * n
    for d, g in zip(delays, decays):
        buf = [0.0] * n
        for i in range(n):
            v = samples[i]
            if i >= d:
                v += buf[i - d] * g
            buf[i] = v
            wet[i] += v * 0.25
    return [samples[i] * (1.0 - mix) + wet[i] * mix for i in range(n)]


def make_loop_seamless(samples: list[float], xfade_sec: float = 0.35, sr: int = SR) -> list[float]:
    xf = min(int(xfade_sec * sr), len(samples) // 6)
    if xf <= 1:
        return samples
    out = samples[:-xf]
    for i in range(xf):
        t = i / float(xf)
        out[i] = out[i] * t + samples[len(samples) - xf + i] * (1.0 - t)
    return out


def midi_hz(note: float) -> float:
    return 440.0 * (2.0 ** ((note - 69.0) / 12.0))


def synth_pad_chord(chords: list[tuple[list[int], float]], bpm: float, brightness: float,
                    arpeggio: bool, pulse_drums: bool, seed: int) -> list[float]:
    rng = random.Random(seed)
    beat_sec = 60.0 / bpm
    total_beats = sum(dur for _, dur in chords)
    total_n = int(total_beats * beat_sec * SR)
    buf = [0.0] * total_n

    cur_sample = 0
    for chord_idx, (notes, dur_beats) in enumerate(chords):
        seg_n = int(dur_beats * beat_sec * SR)
        seg_end = min(total_n, cur_sample + seg_n)
        seg_len = seg_end - cur_sample
        if seg_len <= 0:
            continue

        # 1. 풍성한 다크 스트링/합창 패드 (디튠 배음)
        for note in notes:
            f0 = midi_hz(note)
            for detune in (-0.004, 0.0, 0.004):
                f = f0 * (1.0 + detune)
                ph = rng.uniform(0.0, TWO_PI)
                for i in range(seg_len):
                    t = i / float(SR)
                    env = min(1.0, i / (0.18 * SR)) * min(1.0, (seg_len - i) / (0.22 * SR))
                    s = (
                        math.sin(ph + TWO_PI * f * t) * 0.55
                        + math.sin(ph + TWO_PI * 2.0 * f * t) * (0.25 * brightness)
                        + math.sin(ph + TWO_PI * 3.0 * f * t) * (0.12 * brightness)
                    )
                    buf[cur_sample + i] += s * env * (0.14 / max(1, len(notes)))

        # 2. 선택적 류트/하프 아르페지오
        if arpeggio:
            step_beats = 0.5
            steps = int(dur_beats / step_beats)
            for st in range(steps):
                note = notes[(st + chord_idx) % len(notes)] + (12 if st % 4 == 3 else 0)
                f = midi_hz(note)
                start_i = cur_sample + int(st * step_beats * beat_sec * SR)
                pluck_n = min(int(0.65 * SR), total_n - start_i)
                for i in range(max(0, pluck_n)):
                    t = i / float(SR)
                    env = math.exp(-t * 5.2) * min(1.0, t / 0.008)
                    val = (
                        math.sin(TWO_PI * f * t) * 0.65
                        + math.sin(TWO_PI * 2.0 * f * t) * 0.25 * math.exp(-t * 3.0)
                        + math.sin(TWO_PI * 3.0 * f * t) * 0.10 * math.exp(-t * 6.0)
                    )
                    buf[start_i + i] += val * env * 0.18

        # 3. 선택적 전술 전쟁 북(War Drums) 퍼커션
        if pulse_drums:
            steps = int(dur_beats * 2)
            for st in range(steps):
                start_i = cur_sample + int(st * 0.5 * beat_sec * SR)
                is_kick = (st % 4 == 0) or (st % 8 == 6)
                is_tom = (st % 4 == 2) or (st % 8 == 7)
                hit_n = min(int(0.28 * SR), total_n - start_i)
                for i in range(max(0, hit_n)):
                    t = i / float(SR)
                    if is_kick:
                        freq = 115.0 * math.exp(-t * 22.0) + 42.0
                        env = math.exp(-t * 10.5)
                        buf[start_i + i] += math.sin(TWO_PI * freq * t) * env * 0.34
                    elif is_tom:
                        freq = 165.0 * math.exp(-t * 16.0) + 74.0
                        env = math.exp(-t * 14.0)
                        noise = (rng.random() * 2.0 - 1.0) * math.exp(-t * 32.0) * 0.25
                        buf[start_i + i] += (math.sin(TWO_PI * freq * t) * 0.7 + noise) * env * 0.22

        cur_sample = seg_end

    buf = lowpass(buf, 3400.0 if brightness > 0.6 else 2200.0)
    buf = simple_reverb(buf, mix=0.38)
    return make_loop_seamless(buf, 0.45)


def synth_ambience(kind: str, duration_sec: float, seed: int) -> list[float]:
    rng = random.Random(seed)
    n = int(duration_sec * SR)
    raw = [rng.random() * 2.0 - 1.0 for _ in range(n)]

    if kind == "catacombs":
        # 지하 납골당 바람 + 멀리서 떨어지는 물방울
        wind = lowpass(raw, 280.0)
        out = [w * 0.35 for w in wind]
        for _ in range(11):
            pos = rng.randint(int(0.3 * SR), n - int(0.4 * SR))
            f0 = rng.uniform(720.0, 1180.0)
            for i in range(int(0.18 * SR)):
                t = i / float(SR)
                freq = f0 * (1.0 + 0.65 * t * 6.0)
                out[pos + i] += math.sin(TWO_PI * freq * t) * math.exp(-t * 26.0) * 0.26
        return make_loop_seamless(simple_reverb(out, 0.45), 0.4)

    if kind == "cistern":
        # 흐르는 지하 수로 + 깊은 공명
        water = lowpass(raw, 720.0)
        rumble = lowpass(raw, 140.0)
        out = []
        for i in range(n):
            t = i / float(SR)
            mod = 0.75 + 0.25 * math.sin(TWO_PI * 0.35 * t)
            out.append(water[i] * 0.24 * mod + rumble[i] * 0.42)
        return make_loop_seamless(simple_reverb(out, 0.35), 0.4)

    if kind == "ossuary":
        # 저주받은 영혼 속삭임(대역 통과 풍음 + 불협화 드론)
        wind = lowpass(raw, 520.0)
        out = []
        for i in range(n):
            t = i / float(SR)
            drone = (
                math.sin(TWO_PI * 73.42 * t) * 0.14
                + math.sin(TWO_PI * 103.83 * t) * 0.08 * (0.5 + 0.5 * math.sin(TWO_PI * 0.2 * t))
            )
            out.append(wind[i] * 0.28 + drone)
        return make_loop_seamless(simple_reverb(out, 0.5), 0.4)

    if kind == "foundry":
        # 용암 끓는 저음 + 타닥거리는 불씨
        rumble = lowpass(raw, 180.0)
        out = [r * 0.48 for r in rumble]
        for _ in range(42):
            pos = rng.randint(0, n - int(0.05 * SR))
            amp = rng.uniform(0.12, 0.34)
            for i in range(int(0.03 * SR)):
                t = i / float(SR)
                out[pos + i] += (rng.random() * 2.0 - 1.0) * math.exp(-t * 140.0) * amp
        return make_loop_seamless(simple_reverb(out, 0.25), 0.4)

    # sanctum: 태고의 성소 심연 울림 + 낮게 숨쉬는 용의 공명
    rumble = lowpass(raw, 130.0)
    out = []
    for i in range(n):
        t = i / float(SR)
        breath = 0.6 + 0.4 * math.sin(TWO_PI * 0.18 * t)
        sub = math.sin(TWO_PI * 55.0 * t) * 0.22 * breath + math.sin(TWO_PI * 82.41 * t) * 0.10
        out.append(rumble[i] * 0.42 * breath + sub)
    return make_loop_seamless(simple_reverb(out, 0.42), 0.4)


def synth_sfx(name: str, seed: int) -> list[float]:
    rng = random.Random(seed)

    def make_len(sec: float) -> int:
        return max(64, int(sec * SR))

    if name == "sfx_step":
        n = make_len(0.14)
        noise = lowpass([rng.random() * 2.0 - 1.0 for _ in range(n)], 540.0)
        return [
            (noise[i] * 0.7 + math.sin(TWO_PI * (95.0 - 280.0 * (i / SR)) * (i / SR)) * 0.5)
            * math.exp(-(i / SR) * 24.0)
            for i in range(n)
        ]

    if name == "sfx_turn":
        n = make_len(0.11)
        noise = lowpass([rng.random() * 2.0 - 1.0 for _ in range(n)], 800.0)
        return [noise[i] * math.sin(math.pi * i / n) * 0.42 for i in range(n)]

    if name == "sfx_bump":
        n = make_len(0.18)
        return [
            math.sin(TWO_PI * (78.0 * math.exp(-(i / SR) * 14.0) + 32.0) * (i / SR))
            * math.exp(-(i / SR) * 18.0)
            for i in range(n)
        ]

    if name == "sfx_door":
        n = make_len(0.42)
        out = []
        for i in range(n):
            t = i / SR
            creak = math.sin(TWO_PI * (140.0 + 75.0 * math.sin(18.0 * t)) * t) * 0.35
            thud = math.sin(TWO_PI * 65.0 * t) * math.exp(-abs(t - 0.34) * 35.0) * 0.7
            out.append((creak * math.exp(-t * 3.5) + thud))
        return simple_reverb(lowpass(out, 1600.0), 0.25)

    if name == "sfx_chest":
        n = make_len(0.55)
        notes = [523.25, 659.25, 783.99, 1046.5]
        out = [0.0] * n
        for idx, f in enumerate(notes):
            st = int(idx * 0.08 * SR)
            for i in range(st, n):
                t = (i - st) / SR
                out[i] += (math.sin(TWO_PI * f * t) + 0.35 * math.sin(TWO_PI * 2 * f * t)) * math.exp(-t * 6.0) * 0.35
        return simple_reverb(out, 0.35)

    if name == "sfx_trap":
        n = make_len(0.32)
        noise = [rng.random() * 2.0 - 1.0 for _ in range(n)]
        out = [
            (noise[i] * 0.55 + math.sin(TWO_PI * (920.0 * math.exp(-(i / SR) * 16.0) + 180.0) * (i / SR)) * 0.65)
            * math.exp(-(i / SR) * 11.0)
            for i in range(n)
        ]
        return out

    if name == "sfx_lever":
        n = make_len(0.28)
        out = []
        for i in range(n):
            t = i / SR
            clank1 = math.sin(TWO_PI * 310.0 * t) * math.exp(-t * 30.0)
            clank2 = math.sin(TWO_PI * 185.0 * t) * math.exp(-abs(t - 0.14) * 28.0)
            out.append(clank1 * 0.6 + clank2 * 0.8)
        return simple_reverb(out, 0.28)

    if name == "sfx_sanctuary":
        n = make_len(0.95)
        freqs = [293.66, 440.0, 587.33, 739.99]
        out = []
        for i in range(n):
            t = i / SR
            env = min(1.0, t / 0.12) * math.exp(-t * 2.4)
            s = sum(math.sin(TWO_PI * f * t) for f in freqs) * 0.22
            out.append(s * env)
        return simple_reverb(out, 0.45)

    if name == "sfx_sword_hit":
        n = make_len(0.24)
        noise = [rng.random() * 2.0 - 1.0 for _ in range(n)]
        out = []
        for i in range(n):
            t = i / SR
            slash = noise[i] * math.exp(-t * 22.0) * 0.6
            metal = math.sin(TWO_PI * (640.0 * math.exp(-t * 14.0) + 190.0) * t) * math.exp(-t * 16.0) * 0.7
            thump = math.sin(TWO_PI * 95.0 * t) * math.exp(-t * 18.0) * 0.55
            out.append(slash + metal + thump)
        return out

    if name == "sfx_crit":
        n = make_len(0.42)
        noise = [rng.random() * 2.0 - 1.0 for _ in range(n)]
        out = []
        for i in range(n):
            t = i / SR
            ring = (math.sin(TWO_PI * 1120.0 * t) + math.sin(TWO_PI * 1680.0 * t) * 0.6) * math.exp(-t * 7.5)
            impact = (noise[i] * 0.6 + math.sin(TWO_PI * 75.0 * t) * 0.9) * math.exp(-t * 16.0)
            out.append(ring * 0.45 + impact * 0.75)
        return simple_reverb(out, 0.32)

    if name == "sfx_miss":
        n = make_len(0.18)
        noise = lowpass([rng.random() * 2.0 - 1.0 for _ in range(n)], 1100.0)
        return [noise[i] * math.sin(math.pi * i / n) * 0.65 for i in range(n)]

    if name == "sfx_shield_block":
        n = make_len(0.28)
        out = []
        for i in range(n):
            t = i / SR
            s = (
                math.sin(TWO_PI * 420.0 * t) * 0.5
                + math.sin(TWO_PI * 685.0 * t) * 0.35
                + math.sin(TWO_PI * 115.0 * t) * 0.5
            ) * math.exp(-t * 11.0)
            out.append(s)
        return simple_reverb(out, 0.25)

    if name == "sfx_break":
        n = make_len(0.48)
        noise = [rng.random() * 2.0 - 1.0 for _ in range(n)]
        out = []
        for i in range(n):
            t = i / SR
            shatter = noise[i] * math.exp(-t * 9.0) * 0.55
            glass = (math.sin(TWO_PI * 1480.0 * t) + math.sin(TWO_PI * 2130.0 * t) * 0.7) * math.exp(-t * 12.0) * 0.4
            sub = math.sin(TWO_PI * (120.0 * math.exp(-t * 8.0) + 45.0) * t) * math.exp(-t * 7.0) * 0.7
            out.append(shatter + glass + sub)
        return simple_reverb(out, 0.35)

    if name == "sfx_fire_spell":
        n = make_len(0.52)
        noise = lowpass([rng.random() * 2.0 - 1.0 for _ in range(n)], 950.0)
        out = []
        for i in range(n):
            t = i / SR
            whoosh = noise[i] * min(1.0, t / 0.08) * math.exp(-t * 4.5) * 0.9
            roar = math.sin(TWO_PI * (140.0 * math.exp(-t * 4.0) + 55.0) * t) * math.exp(-t * 5.0) * 0.55
            out.append(whoosh + roar)
        return simple_reverb(out, 0.35)

    if name == "sfx_ice_spell":
        n = make_len(0.45)
        out = []
        for i in range(n):
            t = i / SR
            crystal = (
                math.sin(TWO_PI * 1568.0 * t)
                + math.sin(TWO_PI * 2093.0 * t) * 0.6
                + math.sin(TWO_PI * 2793.0 * t) * 0.4
            ) * math.exp(-t * 8.0) * 0.35
            out.append(crystal)
        return simple_reverb(out, 0.42)

    if name == "sfx_lightning_spell":
        n = make_len(0.38)
        out = []
        for i in range(n):
            t = i / SR
            crackle = (rng.random() * 2.0 - 1.0) * math.exp(-t * 11.0)
            zap = math.sin(TWO_PI * (1200.0 * math.exp(-t * 18.0) + 90.0) * t) * math.exp(-t * 9.0)
            out.append(crackle * 0.65 + zap * 0.6)
        return simple_reverb(out, 0.30)

    if name == "sfx_heal":
        n = make_len(0.60)
        notes = [440.0, 554.37, 659.25, 880.0]
        out = [0.0] * n
        for idx, f in enumerate(notes):
            st = int(idx * 0.07 * SR)
            for i in range(st, n):
                t = (i - st) / SR
                out[i] += math.sin(TWO_PI * f * t) * math.exp(-t * 5.0) * 0.28
        return simple_reverb(out, 0.42)

    if name == "sfx_poison":
        n = make_len(0.36)
        noise = lowpass([rng.random() * 2.0 - 1.0 for _ in range(n)], 1400.0)
        return [
            (noise[i] * 0.45 + math.sin(TWO_PI * (240.0 + 90.0 * math.sin(36.0 * (i / SR))) * (i / SR)) * 0.35)
            * math.exp(-(i / SR) * 6.5)
            for i in range(n)
        ]

    if name == "sfx_ui_hover":
        n = make_len(0.045)
        return [math.sin(TWO_PI * 620.0 * (i / SR)) * math.exp(-(i / SR) * 75.0) * 0.35 for i in range(n)]

    if name == "sfx_ui_click":
        n = make_len(0.08)
        return [
            (math.sin(TWO_PI * 520.0 * (i / SR)) * 0.55 + math.sin(TWO_PI * 1040.0 * (i / SR)) * 0.35)
            * math.exp(-(i / SR) * 45.0)
            for i in range(n)
        ]

    if name == "sfx_ui_error":
        n = make_len(0.20)
        return [
            (math.sin(TWO_PI * 175.0 * (i / SR)) + 0.5 * math.sin(TWO_PI * 233.0 * (i / SR)))
            * math.exp(-(i / SR) * 14.0) * 0.5
            for i in range(n)
        ]

    if name == "sfx_levelup":
        n = make_len(0.95)
        notes = [293.66, 369.99, 440.0, 587.33, 739.99]
        out = [0.0] * n
        for idx, f in enumerate(notes):
            st = int(idx * 0.11 * SR)
            for i in range(st, n):
                t = (i - st) / SR
                out[i] += (math.sin(TWO_PI * f * t) + 0.3 * math.sin(TWO_PI * 2 * f * t)) * math.exp(-t * 3.8) * 0.3
        return simple_reverb(out, 0.4)

    if name == "sfx_victory":
        n = make_len(1.4)
        notes = [293.66, 369.99, 440.0, 587.33, 659.25, 880.0]
        out = [0.0] * n
        for idx, f in enumerate(notes):
            st = int(idx * 0.14 * SR)
            for i in range(st, n):
                t = (i - st) / SR
                out[i] += (math.sin(TWO_PI * f * t) + 0.35 * math.sin(TWO_PI * 2 * f * t)) * math.exp(-t * 2.5) * 0.28
        return simple_reverb(out, 0.45)

    # sfx_defeat
    n = make_len(1.2)
    notes = [293.66, 261.63, 233.08, 196.0]
    out = [0.0] * n
    for idx, f in enumerate(notes):
        st = int(idx * 0.22 * SR)
        for i in range(st, n):
            t = (i - st) / SR
            out[i] += (math.sin(TWO_PI * f * t) + 0.25 * math.sin(TWO_PI * 1.5 * f * t)) * math.exp(-t * 3.2) * 0.32
    return simple_reverb(out, 0.45)


def main() -> None:
    out_dir = Path("assets/audio")
    out_dir.mkdir(parents=True, exist_ok=True)

    # 1. BGM 5종 (루프형 다크 판타지 화성 진행)
    bgm_specs = {
        "bgm_title.wav": (
            [([50, 57, 62, 65], 4), ([46, 53, 58, 62], 4), ([48, 55, 60, 64], 4), ([45, 52, 57, 61], 4)],
            72.0, 0.55, True, False, 101,
        ),
        "bgm_town.wav": (
            [([50, 57, 62, 66], 4), ([55, 62, 67, 71], 4), ([57, 64, 69, 73], 4), ([50, 57, 62, 66], 4)],
            84.0, 0.65, True, False, 102,
        ),
        "bgm_dungeon.wav": (
            [([38, 45, 50, 53], 4), ([36, 43, 48, 51], 4), ([34, 41, 46, 50], 4), ([37, 45, 49, 52], 4)],
            60.0, 0.35, False, False, 103,
        ),
        "bgm_combat.wav": (
            [([45, 52, 57, 60], 2), ([46, 53, 58, 62], 2), ([43, 50, 55, 58], 2), ([44, 52, 56, 59], 2),
             ([45, 52, 57, 60], 2), ([48, 55, 60, 63], 2), ([46, 53, 58, 62], 2), ([44, 52, 56, 59], 2)],
            126.0, 0.82, True, True, 104,
        ),
        "bgm_boss.wav": (
            [([38, 45, 50, 53, 62], 2), ([39, 46, 51, 55, 63], 2), ([38, 45, 50, 53, 62], 2), ([37, 45, 49, 52, 61], 2),
             ([34, 41, 46, 50, 58], 2), ([36, 43, 48, 52, 60], 2), ([37, 45, 49, 55, 61], 2), ([38, 45, 50, 57, 62], 2)],
            138.0, 0.92, True, True, 105,
        ),
    }
    for fname, (chords, bpm, bright, arp, drums, seed) in bgm_specs.items():
        write_wav(out_dir / fname, synth_pad_chord(chords, bpm, bright, arp, drums, seed))
        print(f"Generated {fname}")

    # 2. 바이옴 환경음 5종
    ambs = [
        ("amb_floor1.wav", "catacombs", 7.0, 201),
        ("amb_floor2.wav", "cistern", 7.0, 202),
        ("amb_floor3.wav", "ossuary", 7.0, 203),
        ("amb_floor4.wav", "foundry", 7.0, 204),
        ("amb_floor5.wav", "sanctum", 7.0, 205),
    ]
    for fname, kind, dur, seed in ambs:
        write_wav(out_dir / fname, synth_ambience(kind, dur, seed))
        print(f"Generated {fname}")

    # 3. SFX 24종
    sfx_names = [
        "sfx_step", "sfx_turn", "sfx_bump", "sfx_door", "sfx_chest", "sfx_trap",
        "sfx_lever", "sfx_sanctuary", "sfx_sword_hit", "sfx_crit", "sfx_miss",
        "sfx_shield_block", "sfx_break", "sfx_fire_spell", "sfx_ice_spell",
        "sfx_lightning_spell", "sfx_heal", "sfx_poison", "sfx_ui_hover",
        "sfx_ui_click", "sfx_ui_error", "sfx_levelup", "sfx_victory", "sfx_defeat",
    ]
    for idx, name in enumerate(sfx_names):
        write_wav(out_dir / f"{name}.wav", synth_sfx(name, 300 + idx))
    print(f"Generated {len(sfx_names)} SFX WAV files in {out_dir}")


if __name__ == "__main__":
    main()
