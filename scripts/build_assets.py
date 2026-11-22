#!/usr/bin/env python3
"""Crawlmaster v1.0.0 AAA 그래픽 에셋 파이프라인 (v2 고품질 화풍 보정판).
AI 회화풍 원화(14종)만을 텍스처/인체 기반으로 활용해 하드 컷 경계나 평면 벡터 느낌 없이
5개 바이옴 PBR 텍스처(알베도 + Sobel 노멀맵), 4종 초상화, 24종 몬스터/보스, 프롭, UI 프레임을 생성한다.
"""

from __future__ import annotations

import collections
import math
import random
from pathlib import Path
from PIL import Image, ImageChops, ImageDraw, ImageEnhance, ImageFilter, ImageOps

BRAIN_DIR = Path("/home/eunho1/.gemini/antigravity/brain/3198b272-922f-4869-8fc2-1d90681651c0")
ASSETS = Path("assets/textures")


def find_source(prefix: str) -> Path:
    matches = sorted(BRAIN_DIR.glob(f"{prefix}_*.jpg"))
    if not matches:
        raise FileNotFoundError(f"Missing generated source image for prefix: {prefix}")
    return matches[-1]


def ensure_dirs() -> None:
    for sub in ("biomes", "backgrounds", "portraits", "monsters", "props", "ui"):
        (ASSETS / sub).mkdir(parents=True, exist_ok=True)


def make_seamless(img: Image.Image, size: int = 512) -> Image.Image:
    im = img.convert("RGB").resize((size, size), Image.Resampling.LANCZOS)
    half = size // 2
    shifted = ImageChops.offset(im, half, half)
    mask = Image.new("L", (size, size), 255)
    md = ImageDraw.Draw(mask)
    band = int(size * 0.14)
    for i in range(band):
        val = int(255 * (i / float(band)))
        md.rectangle([i, i, size - 1 - i, size - 1 - i], outline=val)
    mask = mask.filter(ImageFilter.GaussianBlur(radius=size * 0.03))
    return Image.composite(im, shifted, mask)


def generate_normal_map(albedo: Image.Image, strength: float = 2.8) -> Image.Image:
    gray = albedo.convert("L")
    w, h = gray.size
    px = gray.load()
    out = Image.new("RGB", (w, h))
    opx = out.load()

    def sample(x: int, y: int) -> float:
        return px[x % w, y % h] / 255.0

    for y in range(h):
        for x in range(w):
            tl = sample(x - 1, y - 1)
            t = sample(x, y - 1)
            tr = sample(x + 1, y - 1)
            l = sample(x - 1, y)
            r = sample(x + 1, y)
            bl = sample(x - 1, y + 1)
            b = sample(x, y + 1)
            br = sample(x + 1, y + 1)
            dx = (tr + 2.0 * r + br) - (tl + 2.0 * l + bl)
            dy = (bl + 2.0 * b + br) - (tl + 2.0 * t + tr)
            nx = -dx * strength
            ny = -dy * strength
            nz = 1.0
            inv = 1.0 / math.sqrt(nx * nx + ny * ny + nz * nz)
            opx[x, y] = (
                int((nx * inv * 0.5 + 0.5) * 255.0),
                int((ny * inv * 0.5 + 0.5) * 255.0),
                int((nz * inv * 0.5 + 0.5) * 255.0),
            )
    return out


def grade_rgb(img: Image.Image, rm: float, gm: float, bm: float,
              radd: int = 0, gadd: int = 0, badd: int = 0,
              sat: float = 1.0, contrast: float = 1.0) -> Image.Image:
    im = img.convert("RGB")
    if sat != 1.0:
        im = ImageEnhance.Color(im).enhance(sat)
    if contrast != 1.0:
        im = ImageEnhance.Contrast(im).enhance(contrast)
    r, g, b = im.split()
    r = r.point(lambda v: max(0, min(255, int(v * rm + radd))))
    g = g.point(lambda v: max(0, min(255, int(v * gm + gadd))))
    b = b.point(lambda v: max(0, min(255, int(v * bm + badd))))
    return Image.merge("RGB", (r, g, b))


def grade_rgba(img: Image.Image, rm: float, gm: float, bm: float,
               radd: int = 0, gadd: int = 0, badd: int = 0,
               sat: float = 1.0, contrast: float = 1.0) -> Image.Image:
    rgba = img.convert("RGBA")
    a = rgba.getchannel("A")
    rgb = grade_rgb(rgba.convert("RGB"), rm, gm, bm, radd, gadd, badd, sat, contrast)
    rgb.putalpha(a)
    return rgb


def extract_on_black(img: Image.Image, size: int = 512, threshold: int = 22) -> Image.Image:
    im = img.convert("RGBA").resize((size, size), Image.Resampling.LANCZOS)
    w, h = im.size
    px = im.load()

    bg = bytearray(w * h)
    q: collections.deque[tuple[int, int]] = collections.deque()

    def is_bg_pixel(x: int, y: int) -> bool:
        r, g, b, _ = px[x, y]
        return max(r, g, b) <= threshold

    for x in range(w):
        for y in (0, h - 1):
            if is_bg_pixel(x, y) and not bg[y * w + x]:
                bg[y * w + x] = 1
                q.append((x, y))
    for y in range(h):
        for x in (0, w - 1):
            if is_bg_pixel(x, y) and not bg[y * w + x]:
                bg[y * w + x] = 1
                q.append((x, y))

    while q:
        cx, cy = q.popleft()
        for nx, ny in ((cx - 1, cy), (cx + 1, cy), (cx, cy - 1), (cx, cy + 1)):
            if 0 <= nx < w and 0 <= ny < h:
                idx = ny * w + nx
                if not bg[idx] and is_bg_pixel(nx, ny):
                    bg[idx] = 1
                    q.append((nx, ny))

    alpha = Image.new("L", (w, h), 255)
    apx = alpha.load()
    for y in range(h):
        row = y * w
        for x in range(w):
            if bg[row + x]:
                r, g, b, _ = px[x, y]
                mx = max(r, g, b)
                apx[x, y] = 0 if mx < 10 else int((mx - 10) * 5)
    alpha = alpha.filter(ImageFilter.GaussianBlur(radius=0.85))
    im.putalpha(alpha)
    return im


def add_radial_glow(canvas: Image.Image, cx: int, cy: int, radius: int,
                    color: tuple[int, int, int], peak_alpha: int = 165) -> None:
    glow = Image.new("RGBA", canvas.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(glow)
    steps = 20
    for i in range(steps, 0, -1):
        t = i / float(steps)
        r = int(radius * t)
        a = int(peak_alpha * ((1.0 - t) ** 1.9) * 0.42)
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(color[0], color[1], color[2], a))
    glow = glow.filter(ImageFilter.GaussianBlur(radius=max(3, radius // 7)))
    canvas.alpha_composite(glow)


def build_backgrounds() -> None:
    Image.open(find_source("title_bg")).convert("RGB").resize((1280, 720), Image.Resampling.LANCZOS).save(
        ASSETS / "backgrounds/title_bg.png", optimize=True)
    Image.open(find_source("town_bg")).convert("RGB").resize((1280, 720), Image.Resampling.LANCZOS).save(
        ASSETS / "backgrounds/town_bg.png", optimize=True)
    print("Built backgrounds")


def paint_textured_rect(base: Image.Image, tex: Image.Image, box: tuple[int, int, int, int],
                        border_color: tuple[int, int, int], border_w: int = 6) -> None:
    x0, y0, x1, y1 = box
    crop = tex.resize((x1 - x0, y1 - y0), Image.Resampling.LANCZOS)
    base.paste(crop, (x0, y0))
    d = ImageDraw.Draw(base)
    for i in range(border_w):
        shade = 0.65 + 0.35 * math.sin(math.pi * i / max(1, border_w - 1))
        col = (int(border_color[0] * shade), int(border_color[1] * shade), int(border_color[2] * shade))
        d.rectangle([x0 + i, y0 + i, x1 - i, y1 - i], outline=col)


def build_biomes() -> None:
    wall_cat = make_seamless(Image.open(find_source("wall_catacombs")), 512)
    floor_cat = make_seamless(Image.open(find_source("floor_catacombs")), 512)
    wall_cis = make_seamless(Image.open(find_source("wall_cistern")), 512)
    floor_cis = make_seamless(Image.open(find_source("floor_cistern")), 512)
    wall_fnd = make_seamless(Image.open(find_source("wall_foundry")), 512)
    golem_src = Image.open(find_source("mon_ember_golem")).convert("RGB")
    slime_src = Image.open(find_source("mon_mire_slime")).convert("RGB")

    # 3층: 저주받은 납골회랑 (실제 석조 텍스처 + 창백한 골회색/자주 톤 + 자연스럽게 침식된 감실 2개)
    wall_oss = grade_rgb(wall_cat, 0.90, 0.85, 1.06, 12, 8, 18, sat=0.28, contrast=1.22)
    skull_a = slime_src.crop((545, 265, 675, 405)).resize((88, 96), Image.Resampling.LANCZOS)
    skull_b = ImageOps.mirror(slime_src.crop((345, 370, 470, 500)).resize((86, 94), Image.Resampling.LANCZOS))
    for (gx, gy), sk in (((148, 178), skull_a), ((368, 356), skull_b)):
        sk_g = grade_rgb(sk, 0.82, 0.78, 0.92, -4, -4, 6, sat=0.2, contrast=1.22)
        shadow = Image.new("RGBA", wall_oss.size, (0, 0, 0, 0))
        ImageDraw.Draw(shadow).ellipse([gx - 54, gy - 58, gx + 54, gy + 58], fill=(12, 9, 18, 210))
        shadow = shadow.filter(ImageFilter.GaussianBlur(10))
        wall_oss = Image.alpha_composite(wall_oss.convert("RGBA"), shadow).convert("RGB")
        sm = Image.new("L", sk_g.size, 0)
        ImageDraw.Draw(sm).ellipse([10, 10, sk_g.width - 10, sk_g.height - 10], fill=215)
        sm = sm.filter(ImageFilter.GaussianBlur(9))
        wall_oss.paste(sk_g, (gx - sk_g.width // 2, gy - sk_g.height // 2), sm)
    floor_oss = grade_rgb(floor_cat, 0.94, 0.90, 1.06, 10, 8, 16, sat=0.25, contrast=1.18)

    # 4층 바닥: 흑요석 대장간 용암 균열 바닥
    floor_fnd = grade_rgb(floor_cis, 0.80, 0.56, 0.46, -12, -16, -20, sat=0.72, contrast=1.32)
    lava_glow = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    ld = ImageDraw.Draw(lava_glow)
    rng = random.Random(404)
    for _ in range(16):
        x0, y0 = rng.randint(24, 488), rng.randint(24, 488)
        pts = [(x0, y0)]
        for _s in range(5):
            x0 = max(6, min(506, x0 + rng.randint(-48, 48)))
            y0 = max(6, min(506, y0 + rng.randint(-48, 48)))
            pts.append((x0, y0))
        ld.line(pts, fill=(215, 58, 14, 180), width=6)
        ld.line(pts, fill=(255, 168, 46, 235), width=2)
    lava_blur = lava_glow.filter(ImageFilter.GaussianBlur(3.0))
    floor_fnd = Image.alpha_composite(
        Image.alpha_composite(floor_fnd.convert("RGBA"), lava_blur), lava_glow
    ).convert("RGB")

    # 5층: 용의 성소 (광택 흑요석 + 풍화된 브러시드 금박 띠 + 붉은 용염 기운)
    wall_snc = grade_rgb(wall_fnd, 0.78, 0.68, 0.72, -6, -8, -4, sat=0.62, contrast=1.28)
    gold_tex = grade_rgb(wall_cat, 1.65, 1.32, 0.62, 18, 8, -18, sat=1.35, contrast=1.25)
    for y_band in (36, 450):
        paint_textured_rect(wall_snc, gold_tex, (0, y_band, 512, y_band + 26), (178, 140, 62), 3)
    snc_rgba = wall_snc.convert("RGBA")
    for rx in range(64, 512, 128):
        add_radial_glow(snc_rgba, rx, 49, 26, (255, 92, 34), 130)
        add_radial_glow(snc_rgba, rx, 463, 26, (255, 92, 34), 130)
    wall_snc = snc_rgba.convert("RGB")

    floor_snc = grade_rgb(floor_cat, 0.54, 0.48, 0.56, -12, -14, -10, sat=0.45, contrast=1.38)
    paint_textured_rect(floor_snc, gold_tex, (20, 20, 492, 34), (165, 126, 52), 2)
    paint_textured_rect(floor_snc, gold_tex, (20, 478, 492, 492), (165, 126, 52), 2)
    paint_textured_rect(floor_snc, gold_tex, (20, 20, 34, 492), (165, 126, 52), 2)
    paint_textured_rect(floor_snc, gold_tex, (478, 20, 492, 492), (165, 126, 52), 2)

    biomes = {
        "catacombs": (wall_cat, floor_cat, grade_rgb(wall_cat, 0.55, 0.58, 0.55, -8, -8, -8)),
        "cistern": (wall_cis, floor_cis, grade_rgb(wall_cis, 0.52, 0.62, 0.68, -8, -6, -4)),
        "ossuary": (wall_oss, floor_oss, grade_rgb(wall_oss, 0.52, 0.50, 0.62, -6, -6, -4)),
        "foundry": (wall_fnd, floor_fnd, grade_rgb(wall_fnd, 0.58, 0.45, 0.40, -8, -8, -8)),
        "sanctum": (wall_snc, floor_snc, grade_rgb(wall_snc, 0.48, 0.44, 0.54, -8, -8, -6)),
    }
    for name, (w_img, f_img, c_img) in biomes.items():
        w_img.save(ASSETS / f"biomes/wall_{name}.png", optimize=True)
        generate_normal_map(w_img, 3.0).save(ASSETS / f"biomes/wall_{name}_n.png", optimize=True)
        f_img.save(ASSETS / f"biomes/floor_{name}.png", optimize=True)
        generate_normal_map(f_img, 2.4).save(ASSETS / f"biomes/floor_{name}_n.png", optimize=True)
        c_img.save(ASSETS / f"biomes/ceiling_{name}.png", optimize=True)

    # 공유 건축 텍스처 — 모두 실제 석재/철제/목재 질감으로 구성 (평면 단색 제거!)
    wood_tex = grade_rgb(wall_cat, 1.18, 0.78, 0.48, -4, -12, -18, sat=1.1, contrast=1.32)
    iron_tex = grade_rgb(wall_fnd, 0.92, 0.95, 1.05, 4, 4, 8, sat=0.25, contrast=1.35)

    # 1. 나무 철제 문
    door_wood = wall_cat.copy()
    paint_textured_rect(door_wood, wood_tex, (76, 34, 436, 512), (108, 98, 86), 12)
    dd = ImageDraw.Draw(door_wood)
    for px in range(122, 412, 46):
        dd.line([(px, 46), (px, 512)], fill=(26, 16, 10), width=4)
    for py in (140, 276, 408):
        paint_textured_rect(door_wood, iron_tex, (84, py, 428, py + 24), (48, 50, 56), 2)
    dd.ellipse([334, 266, 386, 318], outline=(188, 154, 84), width=7)
    door_wood.save(ASSETS / "biomes/door_wood.png", optimize=True)
    generate_normal_map(door_wood, 3.0).save(ASSETS / "biomes/door_wood_n.png", optimize=True)

    # 2. 잠긴 철창 문
    door_locked = wall_fnd.copy()
    paint_textured_rect(door_locked, grade_rgb(wall_fnd, 0.3, 0.32, 0.4, -10, -10, -8),
                        (76, 32, 436, 512), (96, 88, 80), 12)
    for bx in range(108, 416, 40):
        paint_textured_rect(door_locked, iron_tex, (bx - 6, 44, bx + 6, 512), (38, 40, 44), 2)
    for by in (152, 332):
        paint_textured_rect(door_locked, iron_tex, (84, by - 9, 428, by + 9), (38, 40, 44), 2)
    paint_textured_rect(door_locked, gold_tex, (224, 214, 288, 280), (224, 184, 88), 4)
    ImageDraw.Draw(door_locked).polygon([(256, 234), (266, 266), (246, 266)], fill=(20, 14, 10))
    door_locked.save(ASSETS / "biomes/door_locked.png", optimize=True)
    generate_normal_map(door_locked, 3.0).save(ASSETS / "biomes/door_locked_n.png", optimize=True)

    # 3. 보스 게이트 (흑요석 리벳 철문 + 브러시드 금박 프레임 + 유화 용암 노심 봉인!)
    boss_gate = wall_snc.copy()
    dark_obs = grade_rgb(wall_fnd, 0.72, 0.56, 0.62, -10, -12, -8, sat=0.55, contrast=1.38)
    paint_textured_rect(boss_gate, dark_obs, (58, 24, 454, 512), (192, 152, 68), 14)
    paint_textured_rect(boss_gate, gold_tex, (248, 36, 264, 512), (230, 186, 88), 3)
    # 실제 유화 용암 노심(mon_ember_golem 가슴 코어)을 중앙 봉인 문장으로 합성!
    core = golem_src.crop((390, 260, 620, 490)).resize((176, 176), Image.Resampling.LANCZOS)
    core_mask = Image.new("L", (176, 176), 0)
    ImageDraw.Draw(core_mask).ellipse([6, 6, 170, 170], fill=255)
    core_mask = core_mask.filter(ImageFilter.GaussianBlur(8))
    bg_rgba = boss_gate.convert("RGBA")
    add_radial_glow(bg_rgba, 256, 252, 138, (255, 88, 26), 175)
    bg_rgba.paste(core.convert("RGBA"), (256 - 88, 252 - 88), core_mask)
    ImageDraw.Draw(bg_rgba).ellipse([168, 164, 344, 340], outline=(228, 186, 88, 245), width=8)
    ImageDraw.Draw(bg_rgba).ellipse([178, 174, 334, 330], outline=(92, 58, 20, 220), width=4)
    boss_gate = bg_rgba.convert("RGB")
    boss_gate.save(ASSETS / "biomes/boss_gate.png", optimize=True)
    generate_normal_map(boss_gate, 3.2).save(ASSETS / "biomes/boss_gate_n.png", optimize=True)

    # 4. 계단 (실제 석재 질감 계단)
    stairs_down = floor_cat.copy()
    for i in range(7):
        y = 66 + i * 54
        f = max(0.22, 1.0 - i * 0.12)
        step = grade_rgb(floor_cat, f, f * 0.96, f * 0.92, -6, -6, -6)
        paint_textured_rect(stairs_down, step, (64, y, 448, y + 48), (44, 40, 36), 3)
    stairs_down.save(ASSETS / "biomes/stairs_down.png", optimize=True)
    generate_normal_map(stairs_down, 2.8).save(ASSETS / "biomes/stairs_down_n.png", optimize=True)

    stairs_up = floor_cat.copy()
    for i in range(7):
        y = 66 + i * 54
        f = min(1.45, 0.55 + i * 0.14)
        step = grade_rgb(floor_cat, f, f * 0.96, f * 0.88, 4, 2, -2)
        paint_textured_rect(stairs_up, step, (64, y, 448, y + 48), (92, 82, 68), 3)
    stairs_up.save(ASSETS / "biomes/stairs_up.png", optimize=True)
    generate_normal_map(stairs_up, 2.8).save(ASSETS / "biomes/stairs_up_n.png", optimize=True)

    # 5. 비밀 벽 (자연스러운 석조 균열 + 희미한 금빛 빛샘)
    secret_wall = wall_cat.convert("RGBA")
    crack = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    cd = ImageDraw.Draw(crack)
    pts = [(254, 44), (242, 152), (264, 248), (246, 362), (258, 472)]
    cd.line(pts, fill=(255, 206, 86, 170), width=5)
    cd.line(pts, fill=(255, 244, 180, 225), width=2)
    secret_wall = Image.alpha_composite(secret_wall, crack.filter(ImageFilter.GaussianBlur(3.5)))
    secret_wall = Image.alpha_composite(secret_wall, crack).convert("RGB")
    secret_wall.save(ASSETS / "biomes/secret_wall.png", optimize=True)
    generate_normal_map(secret_wall, 3.0).save(ASSETS / "biomes/secret_wall_n.png", optimize=True)
    print("Built all 5 biome PBR texture sets + architectural doors/gates/stairs + normal maps")


def build_portraits() -> None:
    warrior_src = Image.open(find_source("hero_warrior")).convert("RGB")
    cultist_src = Image.open(find_source("mon_ash_cultist")).convert("RGB")
    town_src = Image.open(find_source("town_bg")).convert("RGB")

    # 1. 전사
    warrior_src.resize((256, 256), Image.Resampling.LANCZOS).save(ASSETS / "portraits/hero_warrior.png", optimize=True)

    # 2. 마법사 (사파이어 아케인 후드)
    mage_bg = grade_rgb(town_src.crop((320, 60, 860, 600)).resize((256, 256), Image.Resampling.LANCZOS),
                        0.55, 0.72, 1.25, -10, 0, 18, sat=1.1, contrast=1.1)
    mage_bust = extract_on_black(cultist_src.crop((245, 30, 760, 545)), 256, threshold=24)
    r, g, b, a = mage_bust.split()
    mage_bust = Image.merge("RGBA", (
        b.point(lambda v: min(255, int(v * 0.75 + 18))),
        g.point(lambda v: min(255, int(v * 0.92 + 24))),
        r.point(lambda v: min(255, int(v * 1.18 + 32))),
        a,
    ))
    mage = mage_bg.convert("RGBA")
    add_radial_glow(mage, 44, 156, 95, (86, 175, 255), 185)
    mage.alpha_composite(mage_bust)
    add_radial_glow(mage, 128, 86, 46, (120, 210, 255), 115)
    mage.convert("RGB").save(ASSETS / "portraits/hero_mage.png", optimize=True)

    # 3. 도적 (에메랄드 섀도우 암살자)
    rogue_bg = grade_rgb(town_src.crop((40, 120, 580, 660)).resize((256, 256), Image.Resampling.LANCZOS),
                         0.45, 0.78, 0.62, -14, -4, -10, sat=0.85, contrast=1.2)
    rogue_bust = ImageOps.mirror(extract_on_black(cultist_src.crop((260, 35, 755, 530)), 256, threshold=24))
    r, g, b, a = rogue_bust.split()
    rogue_bust = Image.merge("RGBA", (
        g.point(lambda v: min(255, int(v * 0.78 + 10))),
        r.point(lambda v: min(255, int(v * 0.88 + 16))),
        b.point(lambda v: min(255, int(v * 0.85 + 14))),
        a,
    ))
    rogue = rogue_bg.convert("RGBA")
    add_radial_glow(rogue, 210, 118, 92, (80, 224, 148), 155)
    rogue.alpha_composite(rogue_bust)
    rogue.convert("RGB").save(ASSETS / "portraits/hero_rogue.png", optimize=True)

    # 4. 성직자 (성당 빛 후광 + 은백/금빛 후드 사제 — 전사와 즉각 구분되는 실루엣/색조)
    cleric_bg = grade_rgb(town_src.crop((140, 20, 740, 620)).resize((256, 256), Image.Resampling.LANCZOS),
                          1.28, 1.12, 0.78, 12, 8, -4, sat=0.95, contrast=1.15)
    cleric_bust = extract_on_black(cultist_src.crop((245, 25, 765, 545)), 256, threshold=24)
    r, g, b, a = cleric_bust.split()
    cleric_bust = Image.merge("RGBA", (
        r.point(lambda v: min(255, int(v * 1.12 + 48))),
        g.point(lambda v: min(255, int(v * 1.06 + 40))),
        b.point(lambda v: min(255, int(v * 0.88 + 20))),
        a,
    ))
    cleric = cleric_bg.convert("RGBA")
    add_radial_glow(cleric, 128, 86, 118, (255, 234, 152), 215)
    cleric.alpha_composite(cleric_bust)
    add_radial_glow(cleric, 128, 66, 62, (255, 246, 205), 145)
    cleric.convert("RGB").save(ASSETS / "portraits/hero_cleric.png", optimize=True)
    print("Built all 4 painterly portraits")


def paint_organic_dragon_wings(size: int, wing_tint: tuple[float, float, float],
                               glow_color: tuple[int, int, int],
                               texture_src: Image.Image) -> Image.Image:
    """곡선 아치와 소프트 셰이딩이 들어간 회화풍 용 날개 레이어 (직선 벡터선 없음)."""
    mask = Image.new("L", (size, size), 0)
    md = ImageDraw.Draw(mask)

    # 부드러운 다중 점 곡선 박쥐-드래곤 날개 실루엣
    def build_half(side: int) -> list[tuple[float, float]]:
        pts = [
            (0.06, 0.44), (0.14, 0.28), (0.26, 0.14), (0.37, 0.10), (0.45, 0.18),
            (0.48, 0.34), (0.45, 0.56), (0.39, 0.50), (0.33, 0.62), (0.27, 0.54),
            (0.20, 0.66), (0.12, 0.58), (0.04, 0.52),
        ]
        if side < 0:
            return [(size * (0.5 - x), size * y) for x, y in pts]
        return [(size * (0.5 + x), size * y) for x, y in pts]

    md.polygon(build_half(-1), fill=235)
    md.polygon(build_half(1), fill=235)
    # 둥근 깎임으로 박쥐 날개 막 굴곡 표현
    for cx, cy, r in ((0.18, 0.64, 0.08), (0.30, 0.62, 0.07), (0.82, 0.64, 0.08), (0.70, 0.62, 0.07)):
        md.ellipse([size * (cx - r), size * (cy - r), size * (cx + r), size * (cy + r)], fill=0)

    mask = mask.filter(ImageFilter.GaussianBlur(3.2))

    # 실제 유화 용암/비늘 텍스처를 입히고 중앙으로 갈수록 어둡게 음영 처리
    tex = grade_rgb(texture_src.resize((size, size), Image.Resampling.LANCZOS),
                    wing_tint[0], wing_tint[1], wing_tint[2], contrast=1.35).convert("RGBA")
    # 자연스러운 용 날개 뼈대·막 음영 (부드러운 블러 처리된 유기적 명암)
    ribs = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    rd = ImageDraw.Draw(ribs)
    for side in (-1, 1):
        sx = lambda x: size * (0.5 + side * x)
        sy = lambda y: size * y
        # 주 날개뼈 아치 및 방사형 지골 음영
        rd.line([(sx(0.06), sy(0.45)), (sx(0.22), sy(0.20)), (sx(0.37), sy(0.11)), (sx(0.46), sy(0.24))],
                fill=(38, 18, 14, 210), width=max(4, size // 64))
        for tip_x, tip_y in ((0.45, 0.52), (0.33, 0.59), (0.20, 0.63)):
            rd.line([(sx(0.24), sy(0.22)), (sx(tip_x), sy(tip_y))],
                    fill=(46, 20, 14, 175), width=max(3, size // 96))
    ribs = ribs.filter(ImageFilter.GaussianBlur(2.2))
    tex = Image.alpha_composite(tex, ribs)
    tex.putalpha(mask)
    add_radial_glow(tex, int(size * 0.24), int(size * 0.34), int(size * 0.22), glow_color, 115)
    add_radial_glow(tex, int(size * 0.76), int(size * 0.34), int(size * 0.22), glow_color, 115)
    # 알파 마스크 재적용해 발광이 날개 실루엣 밖으로 번지지 않고 은은하게 감싸도록 정리
    a = ImageChops.darker(tex.getchannel("A"), mask.filter(ImageFilter.GaussianBlur(4.0)))
    tex.putalpha(a)
    return tex


def build_monsters() -> None:
    kobold = extract_on_black(Image.open(find_source("mon_kobold")), 512, 22)
    goblin = extract_on_black(Image.open(find_source("mon_goblin")), 512, 22)
    warden = extract_on_black(Image.open(find_source("mon_crypt_warden")), 512, 22)
    slime = extract_on_black(Image.open(find_source("mon_mire_slime")), 512, 22)
    golem = extract_on_black(Image.open(find_source("mon_ember_golem")), 512, 22)
    cultist = extract_on_black(Image.open(find_source("mon_ash_cultist")), 512, 22)
    # 벽돌 무늬나 검은 여백이 없는 순수 용암/비늘 중앙 영역만 크롭하여 유기적 날개 막 텍스처로 사용
    golem_full = Image.open(find_source("mon_ember_golem")).convert("RGB")
    gw, gh = golem_full.size
    lava_membrane = golem_full.crop((int(gw * 0.30), int(gh * 0.24), int(gw * 0.70), int(gh * 0.64)))
    kobold_full = Image.open(find_source("mon_kobold")).convert("RGB")
    kw, kh = kobold_full.size
    scale_membrane = kobold_full.crop((int(kw * 0.36), int(kh * 0.28), int(kw * 0.64), int(kh * 0.56)))

    def save_mon(name: str, im: Image.Image) -> None:
        im.resize((512, 512), Image.Resampling.LANCZOS).save(ASSETS / f"monsters/{name}.png", optimize=True)

    # 원화 6종 직접 저장
    save_mon("mon_kobold", kobold)
    save_mon("mon_goblin", goblin)
    save_mon("mon_crypt_warden", warden)
    save_mon("mon_mire_slime", slime)
    save_mon("mon_ember_golem", golem)
    save_mon("mon_ash_cultist", cultist)

    # 7. mon_skeleton (창백한 백골 전사)
    skel = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    sk_body = ImageOps.mirror(grade_rgba(warden, 1.06, 1.04, 1.12, 14, 12, 16, sat=0.16, contrast=1.24))
    skel.alpha_composite(sk_body.resize((440, 440), Image.Resampling.LANCZOS), (36, 54))
    save_mon("mon_skeleton", skel)

    # 8. mon_crypt_rat (웅크린 역병 야수)
    rat = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    rat_body = grade_rgba(kobold, 0.90, 0.72, 0.56, -8, -12, -16, sat=0.52, contrast=1.32)
    rat.alpha_composite(rat_body.resize((462, 326), Image.Resampling.LANCZOS), (25, 152))
    add_radial_glow(rat, 268, 222, 36, (255, 212, 58), 145)
    save_mon("mon_crypt_rat", rat)

    # 9. mon_giant_spider (유화 점액·독성 다중촉수 어둠 거미괴수 — 점액+흑요석 유화 합성)
    spider = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    add_radial_glow(spider, 256, 268, 175, (145, 34, 48), 130)
    sp_body = grade_rgba(slime, 0.78, 0.42, 0.55, -18, -24, -18, sat=1.15, contrast=1.42)
    spider.alpha_composite(sp_body.resize((470, 380), Image.Resampling.LANCZOS), (21, 92))
    # 독성 적안 6개와 녹색 독니 발광
    for ex, ey in ((214, 290), (234, 282), (256, 278), (278, 282), (298, 290)):
        add_radial_glow(spider, ex, ey, 16, (255, 48, 36), 235)
    add_radial_glow(spider, 256, 348, 52, (112, 245, 76), 145)
    save_mon("mon_giant_spider", spider)

    # 10. mon_goblin_shaman
    shaman = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    add_radial_glow(shaman, 130, 145, 110, (162, 74, 255), 175)
    shaman.alpha_composite(ImageOps.mirror(grade_rgba(goblin, 0.88, 0.96, 1.24, 8, -4, 22, sat=1.18, contrast=1.15)))
    add_radial_glow(shaman, 395, 115, 72, (98, 255, 152), 145)
    save_mon("mon_goblin_shaman", shaman)

    # 11. mon_drowned_knight
    drowned = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    add_radial_glow(drowned, 256, 260, 175, (42, 182, 210), 115)
    drowned.alpha_composite(ImageOps.mirror(grade_rgba(warden, 0.64, 1.08, 1.28, -10, 10, 22, sat=1.1, contrast=1.15)))
    save_mon("mon_drowned_knight", drowned)

    # 12. mon_bog_hydra (2층 보스 — 맹독 에메랄드 고대 파충류 군주: 완전한 단일 전신 + 쌍독 오라!)
    hydra = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    add_radial_glow(hydra, 256, 242, 220, (52, 218, 118), 165)
    hyd_bg = grade_rgba(slime, 0.56, 1.18, 0.82, -12, 8, -4, sat=1.2, contrast=1.15)
    hydra.alpha_composite(hyd_bg.resize((460, 440), Image.Resampling.LANCZOS), (26, 54))
    hyd_fg = grade_rgba(kobold, 0.58, 1.24, 0.86, -8, 14, 0, sat=1.28, contrast=1.22)
    hydra.alpha_composite(hyd_fg.resize((448, 448), Image.Resampling.LANCZOS), (32, 48))
    add_radial_glow(hydra, 256, 165, 86, (124, 255, 112), 120)
    save_mon("mon_bog_hydra", hydra)

    # 13. mon_ghoul
    ghoul = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    gh_body = grade_rgba(goblin, 1.06, 0.90, 1.12, 14, 4, 16, sat=0.28, contrast=1.35)
    ghoul.alpha_composite(gh_body.resize((430, 480), Image.Resampling.LANCZOS), (41, 20))
    add_radial_glow(ghoul, 256, 200, 75, (190, 62, 92), 95)
    save_mon("mon_ghoul", ghoul)

    # 14. mon_orc
    orc = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    orc_body = grade_rgba(goblin, 1.18, 0.84, 0.72, 10, -4, -10, sat=0.92, contrast=1.28)
    orc.alpha_composite(orc_body.resize((500, 480), Image.Resampling.LANCZOS), (6, 24))
    save_mon("mon_orc", orc)

    # 15. mon_bone_archer (자줏빛 영혼 사수 — 하드 벡터 활 대신 유화 후드 사수 + 발광 영혼 화살빛)
    archer = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    add_radial_glow(archer, 145, 220, 120, (168, 92, 255), 165)
    archer.alpha_composite(ImageOps.mirror(grade_rgba(cultist, 0.88, 0.84, 1.32, 10, 6, 26, sat=0.55, contrast=1.22)))
    add_radial_glow(archer, 365, 215, 76, (204, 142, 255), 155)
    save_mon("mon_bone_archer", archer)

    # 16. mon_wraith
    wraith = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    add_radial_glow(wraith, 256, 230, 180, (118, 152, 255), 145)
    wr_body = grade_rgba(cultist, 0.62, 0.96, 1.42, 8, 22, 42, sat=0.85, contrast=1.18)
    wa = wr_body.getchannel("A")
    wapx = wa.load()
    for y in range(360, 512):
        fac = max(0.0, 1.0 - (y - 360) / 152.0)
        for x in range(512):
            wapx[x, y] = int(wapx[x, y] * fac)
    wr_body.putalpha(wa)
    wraith.alpha_composite(wr_body, (0, -18))
    save_mon("mon_wraith", wraith)

    # 17. mon_ossuary_lich (3층 보스)
    lich = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    add_radial_glow(lich, 256, 220, 210, (172, 68, 255), 165)
    lich.alpha_composite(grade_rgba(warden, 1.18, 0.72, 1.38, 12, -8, 26, sat=1.28, contrast=1.22), (0, -12))
    add_radial_glow(lich, 396, 155, 88, (215, 112, 255), 190)
    save_mon("mon_ossuary_lich", lich)

    # 18. mon_orc_berserker
    bers = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    add_radial_glow(bers, 256, 240, 195, (255, 86, 36), 145)
    bers.alpha_composite(ImageOps.mirror(grade_rgba(goblin, 1.48, 0.72, 0.56, 24, -10, -16, sat=1.35, contrast=1.3)).resize((508, 492), Image.Resampling.LANCZOS), (2, 16))
    save_mon("mon_orc_berserker", bers)

    # 19. mon_hellhound
    hound = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    add_radial_glow(hound, 256, 285, 175, (255, 92, 24), 145)
    hound.alpha_composite(ImageOps.mirror(grade_rgba(golem, 1.25, 0.78, 0.55, 8, -8, -14, sat=1.25, contrast=1.25)).resize((468, 336), Image.Resampling.LANCZOS), (22, 140))
    save_mon("mon_hellhound", hound)

    # 20. mon_iron_colossus (4층 보스)
    colossus = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    add_radial_glow(colossus, 256, 215, 190, (255, 188, 62), 165)
    colossus.alpha_composite(ImageOps.mirror(grade_rgba(golem, 0.92, 0.96, 1.08, 8, 8, 12, sat=0.55, contrast=1.35)))
    add_radial_glow(colossus, 256, 205, 82, (255, 224, 110), 195)
    save_mon("mon_iron_colossus", colossus)

    # 21. mon_drake_guard
    drake = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    add_radial_glow(drake, 256, 240, 170, (238, 166, 56), 125)
    drake.alpha_composite(grade_rgba(kobold, 1.38, 0.88, 0.58, 14, -4, -12, sat=1.25, contrast=1.25))
    save_mon("mon_drake_guard", drake)

    # 22. mon_dragon_whelp (자연스러운 유화 용 날개 + 심홍 드레이크 전신 — 잘림 없음!)
    whelp = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    add_radial_glow(whelp, 256, 240, 195, (255, 78, 28), 155)
    whelp.alpha_composite(paint_organic_dragon_wings(512, (1.30, 0.50, 0.34), (255, 110, 42), lava_membrane))
    whelp.alpha_composite(grade_rgba(kobold, 1.48, 0.68, 0.52, 16, -10, -14, sat=1.35, contrast=1.26).resize((416, 416), Image.Resampling.LANCZOS), (48, 82))
    add_radial_glow(whelp, 286, 175, 58, (255, 188, 66), 145)
    save_mon("mon_dragon_whelp", whelp)

    # 23. mon_sanctum_wyrm (황금 비늘 날개 + 고대룡 전신 — 잘림 없음!)
    wyrm = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    add_radial_glow(wyrm, 256, 228, 205, (255, 196, 72), 155)
    wyrm.alpha_composite(paint_organic_dragon_wings(512, (1.42, 1.12, 0.50), (255, 212, 92), lava_membrane))
    wyrm.alpha_composite(ImageOps.mirror(grade_rgba(kobold, 1.42, 1.18, 0.62, 16, 8, -14, sat=1.28, contrast=1.28)).resize((436, 436), Image.Resampling.LANCZOS), (38, 68))
    save_mon("mon_sanctum_wyrm", wyrm)

    # 24. mon_elder_wyrm (5층 최종 보스 — 거대 흑요석 용암 날개 + 완전체 흑요석 용염 군주, 이음새 0!)
    elder = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    add_radial_glow(elder, 256, 225, 248, (255, 62, 22), 205)
    elder.alpha_composite(paint_organic_dragon_wings(512, (1.38, 0.54, 0.34), (255, 134, 42), lava_membrane))
    # 완전체 드래곤 로드 전신(비늘·뿔·꼬리·창) + 용암 글로우 2중 합성으로 압도적 위압감 구현
    elder_main = grade_rgba(kobold, 1.52, 0.66, 0.44, 18, -10, -18, sat=1.42, contrast=1.35)
    elder.alpha_composite(elder_main.resize((468, 468), Image.Resampling.LANCZOS), (22, 34))
    add_radial_glow(elder, 275, 130, 82, (255, 212, 78), 160)
    add_radial_glow(elder, 256, 252, 115, (255, 86, 26), 125)
    save_mon("mon_elder_wyrm", elder)
    print("Built all 24 seamless painterly dark-fantasy monster & boss RGBA sprites")


def build_props_and_ui() -> None:
    def new_prop() -> tuple[Image.Image, ImageDraw.ImageDraw]:
        im = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
        return im, ImageDraw.Draw(im)

    im, d = new_prop()
    add_radial_glow(im, 128, 150, 96, (235, 174, 58), 95)
    d.rounded_rectangle([38, 110, 218, 224], radius=12, fill=(92, 54, 28, 255), outline=(32, 18, 10, 255), width=5)
    d.rounded_rectangle([34, 82, 222, 134], radius=18, fill=(116, 70, 34, 255), outline=(32, 18, 10, 255), width=5)
    for bx in (72, 184):
        d.rectangle([bx - 9, 84, bx + 9, 222], fill=(184, 146, 68, 255), outline=(72, 50, 18, 255), width=2)
    d.rectangle([112, 122, 144, 158], fill=(224, 184, 82, 255), outline=(64, 44, 16, 255), width=3)
    d.ellipse([123, 134, 133, 146], fill=(32, 20, 12, 255))
    im.save(ASSETS / "props/prop_chest_closed.png", optimize=True)

    im, d = new_prop()
    d.rounded_rectangle([38, 128, 218, 224], radius=10, fill=(76, 44, 24, 255), outline=(28, 16, 10, 255), width=5)
    d.polygon([(40, 128), (58, 72), (198, 72), (216, 128)], fill=(44, 24, 14, 255), outline=(146, 114, 52, 255))
    for bx in (72, 184):
        d.rectangle([bx - 8, 128, bx + 8, 222], fill=(142, 112, 54, 255))
    im.save(ASSETS / "props/prop_chest_open.png", optimize=True)

    im, d = new_prop()
    add_radial_glow(im, 128, 86, 110, (255, 148, 42), 215)
    d.polygon([(118, 112), (138, 112), (132, 232), (124, 232)], fill=(78, 52, 32, 255), outline=(28, 18, 12, 255))
    d.rectangle([104, 102, 152, 120], fill=(74, 72, 76, 255), outline=(24, 24, 28, 255), width=2)
    d.polygon([(128, 26), (156, 86), (144, 106), (112, 106), (100, 86)], fill=(255, 126, 28, 240))
    d.polygon([(128, 46), (144, 88), (128, 102), (112, 88)], fill=(255, 228, 118, 255))
    im.save(ASSETS / "props/prop_torch.png", optimize=True)

    im, d = new_prop()
    add_radial_glow(im, 128, 138, 120, (255, 164, 62), 210)
    d.ellipse([48, 182, 208, 232], fill=(74, 70, 68, 255), outline=(34, 30, 28, 255), width=4)
    d.line([(72, 206), (186, 178)], fill=(96, 58, 30, 255), width=12)
    d.line([(72, 180), (186, 208)], fill=(82, 48, 24, 255), width=12)
    d.polygon([(128, 52), (168, 154), (150, 194), (106, 194), (88, 154)], fill=(255, 108, 26, 235))
    d.polygon([(128, 88), (152, 158), (128, 188), (104, 158)], fill=(255, 230, 128, 255))
    d.line([(128, 34), (128, 198)], fill=(196, 204, 214, 235), width=5)
    d.line([(106, 66), (150, 66)], fill=(214, 178, 84, 245), width=5)
    im.save(ASSETS / "props/prop_sanctuary.png", optimize=True)

    im, d = new_prop()
    d.rounded_rectangle([76, 172, 180, 228], radius=8, fill=(88, 84, 80, 255), outline=(32, 30, 28, 255), width=4)
    d.line([(128, 184), (176, 76)], fill=(154, 148, 138, 255), width=10)
    d.ellipse([162, 58, 194, 90], fill=(198, 54, 42, 255), outline=(242, 196, 98, 255), width=3)
    im.save(ASSETS / "props/prop_lever.png", optimize=True)

    im, d = new_prop()
    add_radial_glow(im, 128, 176, 98, (255, 76, 48), 165)
    d.ellipse([44, 134, 212, 218], outline=(255, 96, 54, 235), width=6)
    d.polygon([(128, 142), (192, 176), (128, 210), (64, 176)], outline=(255, 206, 84, 245), width=4)
    for sx in (96, 128, 160):
        d.polygon([(sx, 148), (sx + 10, 192), (sx - 10, 192)], fill=(196, 202, 210, 245))
    im.save(ASSETS / "props/prop_trap.png", optimize=True)

    im, d = new_prop()
    add_radial_glow(im, 128, 128, 105, (92, 214, 235), 145)
    d.ellipse([96, 44, 160, 108], fill=(196, 162, 132, 255), outline=(42, 34, 28, 255), width=3)
    d.polygon([(66, 234), (92, 106), (164, 106), (190, 234)], fill=(48, 96, 104, 255), outline=(24, 46, 52, 255))
    d.polygon([(84, 62), (128, 28), (172, 62), (166, 96), (90, 96)], fill=(38, 72, 82, 255))
    im.save(ASSETS / "props/prop_npc_scout.png", optimize=True)

    im, d = new_prop()
    add_radial_glow(im, 128, 128, 112, (255, 214, 88), 215)
    d.polygon([(92, 74), (164, 74), (144, 148), (112, 148)], fill=(232, 186, 68, 255), outline=(108, 72, 18, 255))
    d.rectangle([118, 148, 138, 196], fill=(214, 168, 56, 255))
    d.rounded_rectangle([88, 196, 168, 218], radius=6, fill=(238, 196, 82, 255), outline=(92, 62, 16, 255), width=3)
    im.save(ASSETS / "props/prop_relic.png", optimize=True)

    im, d = new_prop()
    add_radial_glow(im, 128, 128, 96, (132, 182, 255), 125)
    d.rounded_rectangle([68, 42, 188, 232], radius=24, fill=(76, 78, 86, 255), outline=(34, 36, 42, 255), width=5)
    for ly in range(86, 196, 22):
        d.line([(92, ly), (164, ly)], fill=(154, 212, 255, 210), width=4)
    im.save(ASSETS / "props/prop_lore.png", optimize=True)

    def make_panel(path: Path, border_outer: tuple[int, int, int], border_inner: tuple[int, int, int],
                   fill_rgb: tuple[int, int, int]) -> None:
        p = Image.new("RGBA", (96, 96), (fill_rgb[0], fill_rgb[1], fill_rgb[2], 235))
        pd = ImageDraw.Draw(p)
        pd.rectangle([0, 0, 95, 95], outline=(10, 10, 14, 255), width=2)
        pd.rectangle([2, 2, 93, 93], outline=(border_outer[0], border_outer[1], border_outer[2], 255), width=3)
        pd.rectangle([6, 6, 89, 89], outline=(border_inner[0], border_inner[1], border_inner[2], 220), width=2)
        for cx, cy in ((9, 9), (86, 9), (9, 86), (86, 86)):
            pd.ellipse([cx - 4, cy - 4, cx + 4, cy + 4],
                       fill=(border_inner[0], border_inner[1], border_inner[2], 255),
                       outline=(22, 18, 14, 255))
        p.save(path, optimize=True)

    make_panel(ASSETS / "ui/panel_9slice.png", (122, 99, 67), (186, 152, 96), (19, 20, 28))
    make_panel(ASSETS / "ui/panel_gold_9slice.png", (196, 156, 72), (244, 212, 128), (24, 22, 32))

    pg = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    add_radial_glow(pg, 32, 32, 30, (255, 255, 255), 255)
    pg.save(ASSETS / "ui/particle_glow.png", optimize=True)

    ps = Image.new("RGBA", (128, 128), (0, 0, 0, 0))
    psd = ImageDraw.Draw(ps)
    psd.arc([12, 12, 116, 116], start=205, end=345, fill=(255, 245, 220, 255), width=12)
    ps = ps.filter(ImageFilter.GaussianBlur(1.5))
    ps.save(ASSETS / "ui/particle_slash.png", optimize=True)

    vig = Image.new("RGBA", (512, 288), (0, 0, 0, 0))
    vpx = vig.load()
    for y in range(288):
        ny = (y / 287.0 - 0.5) * 2.0
        for x in range(512):
            nx = (x / 511.0 - 0.5) * 2.0
            dist = math.sqrt(nx * nx * 0.9 + ny * ny * 1.05)
            a = int(max(0.0, min(1.0, (dist - 0.58) / 0.72)) ** 1.6 * 175.0)
            vpx[x, y] = (6, 6, 10, a)
    vig.save(ASSETS / "ui/vignette.png", optimize=True)
    print("Built props, UI panels, particles, and vignette")


def main() -> None:
    ensure_dirs()
    build_backgrounds()
    build_biomes()
    build_portraits()
    build_monsters()
    build_props_and_ui()
    print("Asset build pipeline v2 completed successfully.")


if __name__ == "__main__":
    main()
