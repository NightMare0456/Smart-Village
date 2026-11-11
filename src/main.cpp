
// SMART VILLAGE | Day and Night Scenario 
//
// Controls:  D = day   N = night   A = automatic day/night cycle
//            R = toggle rain   S = stop rain   SPACE = pause   ESC = quit
//
// The scene is designed in a 1457 x 965 "pixel" space (y pointing DOWN) and is
// converted to OpenGL coordinates in vertex(), so circles stay perfectly round.

#include "glad.h"
#include "glfw3.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>
#include <vector>

constexpr float DESIGN_W = 1457.0f;
constexpr float DESIGN_H = 965.0f;
constexpr int WIDTH = 1280;
constexpr int HEIGHT = 848;   // keeps the same aspect ratio as the design space
constexpr float PI = 3.14159265359f;

struct Vertex {
    float x, y, r, g, b, a;
};

struct Color {
    float r, g, b, a;
};

std::vector<Vertex> vertices;

// ---------------------------------------------------------------- state
float dayAmount = 1.0f;
float targetDay = 1.0f;
float rainAmount = 0.0f;
bool automaticMode = false;
bool raining = false;
bool paused = false;
float animationTime = 0.0f;

// values computed once per frame in drawScene()
float gNight = 0.0f;
float gTime = 0.0f;
float lampOn = 0.0f;

// ---------------------------------------------------------------- shaders
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
out vec4 vertexColor;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vertexColor = aColor;
})";

const char* fragmentShaderSource = R"(
#version 330 core
in vec4 vertexColor;
out vec4 FragColor;
void main() {
    FragColor = vertexColor;
})";

// ---------------------------------------------------------------- math helpers
float mixf(float a, float b, float t) { return a + (b - a) * t; }

float clamp01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }

float smoothstep(float a, float b, float x) {
    float t = clamp01((x - a) / (b - a));
    return t * t * (3.0f - 2.0f * t);
}

float wrapf(float v, float m) {
    v = std::fmod(v, m);
    if (v < 0.0f) v += m;
    return v;
}

float hashf(float n) {
    float s = std::sin(n * 12.9898f) * 43758.5453f;
    return s - std::floor(s);
}

Color rgb(float r, float g, float b, float a = 1.0f) { return {r, g, b, a}; }

Color dim(Color c, float k) { return {c.r * k, c.g * k, c.b * k, c.a}; }

Color mixc(Color a, Color b, float t) {
    return {mixf(a.r, b.r, t), mixf(a.g, b.g, t),
            mixf(a.b, b.b, t), mixf(a.a, b.a, t)};
}

// blend between a day colour and a night colour
Color tone(Color day, Color night) { return mixc(day, night, gNight); }

// ---------------------------------------------------------------- primitives
void vertexC(float x, float y, Color c) {
    vertices.push_back({x / DESIGN_W * 2.0f - 1.0f,
                        1.0f - y / DESIGN_H * 2.0f,
                        c.r, c.g, c.b, c.a});
}

void triangle(float ax, float ay, float bx, float by,
              float cx, float cy, Color c) {
    vertexC(ax, ay, c);
    vertexC(bx, by, c);
    vertexC(cx, cy, c);
}

void rect(float x, float y, float w, float h, Color c) {
    triangle(x, y, x + w, y, x + w, y + h, c);
    triangle(x, y, x + w, y + h, x, y + h, c);
}

void gradRect(float x, float y, float w, float h, Color top, Color bottom) {
    vertexC(x, y, top);
    vertexC(x + w, y, top);
    vertexC(x + w, y + h, bottom);
    vertexC(x, y, top);
    vertexC(x + w, y + h, bottom);
    vertexC(x, y + h, bottom);
}

void circle(float cx, float cy, float radius, Color c, int steps = 32) {
    for (int i = 0; i < steps; ++i) {
        float a1 = 2.0f * PI * i / steps;
        float a2 = 2.0f * PI * (i + 1) / steps;
        triangle(cx, cy,
                 cx + std::cos(a1) * radius, cy + std::sin(a1) * radius,
                 cx + std::cos(a2) * radius, cy + std::sin(a2) * radius, c);
    }
}

// convex (or star-shaped from the first point) polygon
void polygon(const std::vector<std::pair<float, float>>& p, Color c) {
    if (p.size() < 3) return;
    for (size_t i = 1; i + 1 < p.size(); ++i) {
        triangle(p[0].first, p[0].second,
                 p[i].first, p[i].second,
                 p[i + 1].first, p[i + 1].second, c);
    }
}

void line(float x1, float y1, float x2, float y2, float width, Color c) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.0001f) return;

    float px = -dy / len * width * 0.5f;
    float py = dx / len * width * 0.5f;

    triangle(x1 + px, y1 + py, x1 - px, y1 - py, x2 - px, y2 - py, c);
    triangle(x1 + px, y1 + py, x2 - px, y2 - py, x2 + px, y2 + py, c);
}

void ring(float cx, float cy, float rx, float ry, float th, Color c,
          int steps = 40) {
    for (int i = 0; i < steps; ++i) {
        float a1 = 2.0f * PI * i / steps;
        float a2 = 2.0f * PI * (i + 1) / steps;
        line(cx + std::cos(a1) * rx, cy + std::sin(a1) * ry,
             cx + std::cos(a2) * rx, cy + std::sin(a2) * ry, th, c);
    }
}

// ---------------------------------------------------------------- tiny stroke font
void glyph(char ch, float x, float y, float w, float h, float th, Color c) {
    auto seg = [&](float x1, float y1, float x2, float y2) {
        line(x + x1 * w, y + y1 * h, x + x2 * w, y + y2 * h, th, c);
    };

    switch (ch) {
    case 'S':
        seg(1, 0, 0, 0); seg(0, 0, 0, 0.5f); seg(0, 0.5f, 1, 0.5f);
        seg(1, 0.5f, 1, 1); seg(1, 1, 0, 1);
        break;
    case 'U':
        seg(0, 0, 0, 1); seg(0, 1, 1, 1); seg(1, 1, 1, 0);
        break;
    case 'N':
        seg(0, 1, 0, 0); seg(0, 0, 1, 1); seg(1, 1, 1, 0);
        break;
    case 'R':
        seg(0, 1, 0, 0); seg(0, 0, 1, 0); seg(1, 0, 1, 0.5f);
        seg(1, 0.5f, 0, 0.5f); seg(0.3f, 0.5f, 1, 1);
        break;
    case 'I':
        seg(0.5f, 0, 0.5f, 1);
        break;
    case 'E':
        seg(1, 0, 0, 0); seg(0, 0, 0, 1); seg(0, 1, 1, 1);
        seg(0, 0.5f, 0.8f, 0.5f);
        break;
    case 'C':
        seg(1, 0, 0, 0); seg(0, 0, 0, 1); seg(0, 1, 1, 1);
        break;
    case 'H':
        seg(0, 0, 0, 1); seg(1, 0, 1, 1); seg(0, 0.5f, 1, 0.5f);
        break;
    case 'O':
        seg(0, 0, 1, 0); seg(1, 0, 1, 1); seg(1, 1, 0, 1); seg(0, 1, 0, 0);
        break;
    case 'L':
        seg(0, 0, 0, 1); seg(0, 1, 1, 1);
        break;
    case 'P':
        seg(0, 1, 0, 0); seg(0, 0, 1, 0); seg(1, 0, 1, 0.5f); seg(1, 0.5f, 0, 0.5f);
        break;
    case 'A':
        seg(0, 1, 0.5f, 0); seg(0.5f, 0, 1, 1); seg(0.2f, 0.65f, 0.8f, 0.65f);
        break;
    case 'Y':
        seg(0, 0, 0.5f, 0.5f); seg(1, 0, 0.5f, 0.5f); seg(0.5f, 0.5f, 0.5f, 1);
        break;
    case 'G':
        seg(1, 0, 0, 0); seg(0, 0, 0, 1); seg(0, 1, 1, 1); seg(1, 1, 1, 0.5f); seg(1, 0.5f, 0.5f, 0.5f);
        break;
    case 'D':
        seg(0, 0, 0, 1); seg(0, 0, 0.6f, 0); seg(0.6f, 0, 1, 0.3f); seg(1, 0.3f, 1, 0.7f);
        seg(1, 0.7f, 0.6f, 1); seg(0.6f, 1, 0, 1);
        break;
    default:
        break;
    }
}

void drawText(const char* s, float x, float y, float w, float h,
              float gap, float th, Color c) {
    for (; *s; ++s) {
        if (*s != ' ') glyph(*s, x, y, w, h, th, c);
        x += w + gap;
    }
}

// ---------------------------------------------------------------- sky
Color skyTop() {
    Color c = tone(rgb(0.30f, 0.62f, 0.92f), rgb(0.02f, 0.025f, 0.085f));
    Color g = tone(rgb(0.50f, 0.56f, 0.64f), rgb(0.035f, 0.035f, 0.09f));
    return mixc(c, g, rainAmount * 0.65f);
}

Color skyBottom() {
    Color c = tone(rgb(0.68f, 0.86f, 0.97f), rgb(0.045f, 0.055f, 0.15f));
    Color g = tone(rgb(0.62f, 0.67f, 0.73f), rgb(0.05f, 0.05f, 0.12f));
    return mixc(c, g, rainAmount * 0.65f);
}

Color skyAt(float y) { return mixc(skyTop(), skyBottom(), clamp01(y / 600.0f)); }

void drawSky() {
    gradRect(0.0f, 0.0f, DESIGN_W, 600.0f, skyTop(), skyBottom());
    rect(0.0f, 600.0f, DESIGN_W, DESIGN_H - 600.0f, skyBottom());
}

void drawStars(float t) {
    float a = smoothstep(0.40f, 0.85f, gNight);
    if (a < 0.01f) return;

    for (int i = 0; i < 180; ++i) {
        float x = hashf(i * 3.1f + 1.0f) * DESIGN_W;
        float y = hashf(i * 7.7f + 3.0f) * DESIGN_H * 0.62f;
        float sz = 1.1f + hashf(i * 5.3f + 2.0f) * 1.5f;
        float tw = 0.7f + 0.3f * std::sin(t * 2.0f + i);
        circle(x, y, sz, rgb(1.0f, 1.0f, 0.95f, a * tw), 6);
    }
}

void drawSunAndMoon() {
    const float cx = 1259.0f;
    const float cy = 62.0f;

    float sunA = 1.0f - smoothstep(0.30f, 0.50f, gNight);
    if (sunA > 0.01f) {
        circle(cx, cy, 100.0f, rgb(1.0f, 0.85f, 0.35f, 0.14f * sunA), 40);
        circle(cx, cy, 75.0f, rgb(1.0f, 0.85f, 0.35f, 0.22f * sunA), 40);
        circle(cx, cy, 54.0f, rgb(1.0f, 0.78f, 0.22f, sunA), 40);
        circle(cx, cy, 46.0f, rgb(1.0f, 0.90f, 0.45f, sunA), 40);
    }

    if (gNight > 0.5f) {
        float moonA = smoothstep(0.50f, 0.70f, gNight);
        circle(cx, cy, 58.0f, rgb(0.96f, 0.95f, 0.78f, moonA), 40);
        circle(cx - 24.0f, cy - 18.0f, 50.0f, skyAt(cy - 18.0f), 40);   // crescent bite
    }
}

// (x, y) = bottom-centre of the cloud, w = total width
void cloud(float x, float y, float w, Color c) {
    circle(x - 0.29f * w, y - 0.21f * w, 0.21f * w, c);
    circle(x - 0.02f * w, y - 0.36f * w, 0.26f * w, c);
    circle(x + 0.26f * w, y - 0.22f * w, 0.23f * w, c);
    rect(x - 0.29f * w, y - 0.30f * w, 0.55f * w, 0.30f * w, c);
}

void drawClouds(float t) {
    Color base = tone(rgb(0.96f, 0.98f, 1.0f), rgb(0.31f, 0.33f, 0.46f));
    Color grey = tone(rgb(0.58f, 0.61f, 0.67f), rgb(0.22f, 0.24f, 0.34f));
    Color c = mixc(base, grey, rainAmount * 0.7f);

    auto drift = [&](float x0, float speed) {
        return wrapf(x0 + 200.0f + t * speed, DESIGN_W + 400.0f) - 200.0f;
    };

    cloud(drift(772.0f, 10.0f), 230.0f, 160.0f, c);
    cloud(drift(1150.0f, 6.0f), 98.0f, 128.0f, c);

    float third = smoothstep(0.40f, 0.80f, dayAmount);
    if (third > 0.01f) {
        Color c3 = c;
        c3.a = third;
        cloud(drift(300.0f, 8.0f), 170.0f, 120.0f, c3);
    }
}

// ---------------------------------------------------------------- landscape
void mountain(float cx, float base, float halfW, float h) {
    Color main = tone(rgb(0.42f, 0.55f, 0.62f), rgb(0.19f, 0.19f, 0.21f));
    Color shade = tone(rgb(0.33f, 0.45f, 0.52f), rgb(0.12f, 0.12f, 0.14f));

    float apexY = base - h;

    triangle(cx - halfW, base, cx, apexY, cx + halfW, base, main);
    triangle(cx, apexY, cx + halfW, base, cx + halfW * 0.12f, base, shade);

    // snow cap (daytime only)
    float snowA = smoothstep(0.30f, 0.80f, dayAmount);
    if (snowA > 0.01f) {
        float y2 = apexY + 0.20f * h;
        polygon({{cx, apexY},
                 {cx + halfW * 0.20f, y2},
                 {cx + halfW * 0.08f, y2 - 0.04f * h},
                 {cx, y2 + 0.03f * h},
                 {cx - halfW * 0.07f, y2 - 0.03f * h},
                 {cx - halfW * 0.20f, y2}},
                rgb(0.95f, 0.97f, 0.98f, snowA));
    }
}

void hill(float cx, float base, float halfW, float h, bool darker) {
    Color c = darker
        ? tone(rgb(0.12f, 0.42f, 0.18f), rgb(0.0f, 0.20f, 0.04f))
        : tone(rgb(0.18f, 0.52f, 0.24f), rgb(0.01f, 0.28f, 0.05f));
    triangle(cx - halfW, base, cx, base - h, cx + halfW, base, c);
}

void drawBackground() {
    mountain(264.0f, 600.0f, 230.0f, 312.0f);
    mountain(676.0f, 600.0f, 215.0f, 385.0f);
    mountain(1142.0f, 600.0f, 235.0f, 365.0f);

    hill(454.0f, 600.0f, 190.0f, 302.0f, false);
    hill(932.0f, 600.0f, 190.0f, 284.0f, false);
    hill(1280.0f, 600.0f, 200.0f, 220.0f, false);

    hill(464.0f, 600.0f, 95.0f, 205.0f, true);
    hill(941.0f, 600.0f, 100.0f, 202.0f, true);
}

// ---------------------------------------------------------------- trees
void pine(float x, float baseY, float h) {
    Color trunk = tone(rgb(0.45f, 0.26f, 0.12f), rgb(0.30f, 0.15f, 0.08f));
    Color light = tone(rgb(0.10f, 0.50f, 0.20f), rgb(0.0f, 0.27f, 0.04f));
    Color dark = tone(rgb(0.07f, 0.40f, 0.15f), rgb(0.0f, 0.20f, 0.03f));

    float trunkH = 0.28f * h;
    float trunkW = 0.10f * h;
    float F = 0.72f * h;
    float foliageBottom = baseY - trunkH;
    float top = foliageBottom - F;

    rect(x - trunkW * 0.5f, foliageBottom - 0.04f * h,
         trunkW, trunkH + 0.04f * h, trunk);

    struct Tier { float t0, t1, hw; };
    const Tier tiers[3] = {{0.48f, 1.00f, 0.33f},
                           {0.25f, 0.70f, 0.245f},
                           {0.00f, 0.40f, 0.17f}};

    for (const Tier& tr : tiers) {
        float apexY = top + tr.t0 * F;
        float baseTy = top + tr.t1 * F;
        float hw = tr.hw * F;
        triangle(x, apexY, x - hw, baseTy, x, baseTy, light);
        triangle(x, apexY, x, baseTy, x + hw, baseTy, dark);
    }
}

// ---------------------------------------------------------------- buildings
void window(float x, float y, float w, float h) {
    Color glass = tone(rgb(0.55f, 0.80f, 0.95f), rgb(1.0f, 0.78f, 0.30f));
    Color frame = tone(rgb(0.95f, 0.95f, 0.90f), rgb(0.36f, 0.20f, 0.09f));
    float th = std::max(2.0f, w * 0.07f);

    rect(x, y, w, h, glass);
    line(x + w * 0.5f, y, x + w * 0.5f, y + h, th, frame);
    line(x, y + h * 0.5f, x + w, y + h * 0.5f, th, frame);
    line(x, y, x + w, y, th, frame);
    line(x, y + h, x + w, y + h, th, frame);
    line(x, y, x, y + h, th, frame);
    line(x + w, y, x + w, y + h, th, frame);
}

void house(float x, float baseY, float w, float h, float rise,
           bool litRoof, bool chimney) {
    Color wall = tone(rgb(0.86f, 0.60f, 0.36f), rgb(0.50f, 0.30f, 0.15f));
    Color roof = tone(rgb(0.68f, 0.21f, 0.17f), rgb(0.40f, 0.06f, 0.08f));
    Color roofLit = tone(rgb(0.88f, 0.32f, 0.24f), rgb(0.76f, 0.13f, 0.16f));
    Color brick = tone(rgb(0.55f, 0.18f, 0.15f), rgb(0.33f, 0.06f, 0.07f));
    Color door = tone(rgb(0.35f, 0.20f, 0.10f), rgb(0.20f, 0.10f, 0.05f));
    Color knob = rgb(0.98f, 0.75f, 0.25f);

    float top = baseY - h;
    float cx = x + w * 0.5f;
    float oh = w * 0.11f;

    rect(x, top, w, h, wall);

    if (chimney) {
        rect(cx + w * 0.20f, top - rise - 22.0f,
             w * 0.10f, rise * 0.75f + 22.0f, brick);
    }

    triangle(x - oh, top, cx, top - rise, x + w + oh, top, roof);

    if (litRoof) {
        float f = 0.71f;
        float yb = top - rise + f * rise;
        float hwv = (w * 0.5f + oh) * f;
        triangle(cx, top - rise, cx - hwv, yb, cx + hwv, yb, roofLit);
    }

    rect(x - oh, top - 2.0f, w + 2.0f * oh, 5.0f, dim(roof, 0.8f));

    float ws = w * 0.22f;
    window(x + w * 0.10f, top + h * 0.19f, ws, ws);
    window(x + w * 0.70f, top + h * 0.19f, ws, ws);

    float dh = h * 0.55f;
    rect(x + w * 0.40f, baseY - dh, w * 0.22f, dh, door);
    circle(x + w * 0.58f, baseY - h * 0.275f, w * 0.02f, knob, 10);

    if (chimney) {
        float chx = cx + w * 0.25f;
        float chTop = top - rise - 22.0f;
        Color smoke = tone(rgb(0.80f, 0.80f, 0.84f), rgb(0.45f, 0.45f, 0.58f));
        for (int k = 0; k < 4; ++k) {
            float phase = std::fmod(gTime * 0.25f + k * 0.25f, 1.0f);
            smoke.a = (1.0f - phase) * 0.35f;
            circle(chx + std::sin(phase * 6.0f + k) * 8.0f + phase * 22.0f,
                   chTop - phase * 75.0f,
                   6.0f + phase * 13.0f, smoke, 16);
        }
    }
}

void lamp(float x) {
    Color pole = tone(rgb(0.30f, 0.32f, 0.36f), rgb(0.27f, 0.28f, 0.33f));
    rect(x - 3.0f, 573.0f, 6.0f, 97.0f, pole);

    if (lampOn > 0.01f) {
        circle(x, 573.0f, 52.0f, rgb(0.85f, 0.80f, 0.25f, 0.20f * lampOn), 40);
        circle(x, 573.0f, 34.0f, rgb(0.95f, 0.85f, 0.30f, 0.10f * lampOn), 32);
    }

    circle(x, 573.0f, 12.0f,
           tone(rgb(0.80f, 0.82f, 0.78f), rgb(1.0f, 0.86f, 0.20f)), 20);
}

void bench(float x, float y, float w) {
    Color seat = tone(rgb(0.52f, 0.32f, 0.16f), rgb(0.02f, 0.28f, 0.33f));
    Color leg = tone(rgb(0.30f, 0.18f, 0.09f), rgb(0.03f, 0.15f, 0.20f));

    rect(x, y, w, 20.0f, seat);
    rect(x + w * 0.08f, y + 20.0f, 7.0f, 45.0f, leg);
    rect(x + w * 0.92f - 7.0f, y + 20.0f, 7.0f, 45.0f, leg);
}

void school() {
    Color front = tone(rgb(0.82f, 0.38f, 0.22f), rgb(0.58f, 0.20f, 0.11f));
    Color side = dim(front, 0.72f);
    Color roof = tone(rgb(0.52f, 0.52f, 0.56f), rgb(0.30f, 0.30f, 0.33f));
    Color door = tone(rgb(0.35f, 0.20f, 0.10f), rgb(0.20f, 0.09f, 0.05f));
    Color text = tone(rgb(0.10f, 0.04f, 0.04f), rgb(0.07f, 0.02f, 0.03f));

    polygon({{827.0f, 795.0f}, {862.0f, 777.0f},
             {1140.0f, 777.0f}, {1096.0f, 795.0f}}, roof);

    rect(836.0f, 795.0f, 260.0f, 170.0f, front);

    polygon({{1096.0f, 795.0f}, {1140.0f, 777.0f},
             {1140.0f, 945.0f}, {1096.0f, 965.0f}}, side);

    drawText("SUNRISE SCHOOL", 850.0f, 825.0f, 11.0f, 18.0f, 6.0f, 2.4f, text);

    window(858.0f, 868.0f, 38.0f, 40.0f);
    window(1038.0f, 868.0f, 38.0f, 40.0f);

    rect(937.0f, 858.0f, 59.0f, 107.0f, door);
    circle(985.0f, 915.0f, 3.5f, rgb(0.95f, 0.72f, 0.25f), 10);

    // flag on the roof
    line(988.0f, 777.0f, 988.0f, 735.0f, 2.5f, rgb(0.25f, 0.22f, 0.18f));
    polygon({{990.0f, 737.0f}, {1020.0f, 744.0f}, {990.0f, 752.0f}},
            rgb(0.05f, 0.60f, 0.28f));
}

// ---------------------------------------------------------------- road & car
void drawRoad() {
    Color roadC = tone(rgb(0.30f, 0.30f, 0.33f), rgb(0.18f, 0.18f, 0.20f));
    Color curb = tone(rgb(0.75f, 0.75f, 0.75f), rgb(0.55f, 0.55f, 0.55f));
    Color yellow = tone(rgb(0.98f, 0.82f, 0.20f), rgb(0.95f, 0.80f, 0.20f));
    Color white = tone(rgb(0.96f, 0.96f, 0.96f), rgb(0.86f, 0.86f, 0.86f));

    // main road + kerbs
    rect(0.0f, 663.0f, DESIGN_W, 97.0f, roadC);
    rect(0.0f, 663.0f, DESIGN_W, 6.0f, curb);
    rect(0.0f, 753.0f, 713.0f, 7.0f, curb);
    rect(813.0f, 753.0f, DESIGN_W - 813.0f, 7.0f, curb);

    // side road going down
    rect(713.0f, 755.0f, 100.0f, DESIGN_H - 755.0f, roadC);
    for (float y = 765.0f; y < DESIGN_H; y += 83.0f) {
        rect(759.5f, y, 5.0f, 43.0f, yellow);
    }

    // zebra crossing (full width of the road)
    for (int i = 0; i < 9; ++i) {
        rect(639.0f, 674.0f + i * 9.0f, 88.0f, 5.0f, white);
    }

    // stop lines for the two traffic directions
    rect(616.0f, 711.0f, 5.0f, 41.0f, white);
    rect(751.0f, 670.0f, 5.0f, 38.0f, white);

    // centre dashes (leave the zebra-crossing area clear)
    for (float x = 18.0f; x < DESIGN_W; x += 128.0f) {
        const float dashEnd = x + 70.0f;
        const bool overlapsZebra = (x < 727.0f && dashEnd > 639.0f);
        if (!overlapsZebra) {
            rect(x, 708.0f, 70.0f, 6.0f, yellow);
        }
    }
}

void car(float x, float base, float s, Color body, bool faceRight) {
    auto X = [&](float u) { return faceRight ? x + u * s : x + (1.0f - u) * s; };
    auto box = [&](float u0, float u1, float yTop, float yBot, Color c) {
        float a = X(u0);
        float b = X(u1);
        rect(std::min(a, b), yTop, std::fabs(b - a), yBot - yTop, c);
    };

    float bodyTop = base - 0.28f * s;
    float cabinTop = base - 0.48f * s;
    float hy = base - 0.14f * s;
    float dir = faceRight ? 1.0f : -1.0f;

    // headlight beam (drawn first so the car sits on top of it)
    if (lampOn > 0.01f) {
        float fx = X(1.0f);
        polygon({{fx, hy - 3.0f},
                 {fx + dir * 0.68f * s, hy - 0.26f * s},
                 {fx + dir * 0.68f * s, hy + 0.22f * s},
                 {fx, hy + 6.0f}},
                rgb(0.85f, 0.88f, 0.55f, 0.30f * lampOn));
    }

    box(0.0f, 1.0f, bodyTop, base, body);

    polygon({{X(0.16f), bodyTop}, {X(0.36f), cabinTop},
             {X(0.72f), cabinTop}, {X(0.86f), bodyTop}},
            dim(body, 0.92f));

    Color glass = rgb(0.50f, 0.80f, 0.95f);
    float wt = cabinTop + 0.047f * s;
    float wb = bodyTop - 0.03f * s;
    polygon({{X(0.35f), wb}, {X(0.40f), wt}, {X(0.52f), wt}, {X(0.52f), wb}}, glass);
    polygon({{X(0.55f), wb}, {X(0.55f), wt}, {X(0.68f), wt}, {X(0.74f), wb}}, glass);

    // lights
    box(0.96f, 1.0f, hy - 0.05f * s, hy + 0.05f * s,
        tone(rgb(0.90f, 0.90f, 0.70f), rgb(1.0f, 0.85f, 0.25f)));
    box(0.0f, 0.03f, hy - 0.05f * s, hy + 0.04f * s, rgb(0.90f, 0.10f, 0.10f));

    // wheels
    float wheelY = base - 0.012f * s;
    float wr = 0.095f * s;
    for (float u : {0.24f, 0.77f}) {
        float wx = X(u);
        circle(wx, wheelY, wr, rgb(0.05f, 0.05f, 0.06f), 24);
        float k = 0.055f * s;
        float th = std::max(1.5f, 0.012f * s);
        line(wx - k, wheelY - k, wx + k, wheelY + k, th, rgb(0.80f, 0.80f, 0.80f));
        line(wx - k, wheelY + k, wx + k, wheelY - k, th, rgb(0.80f, 0.80f, 0.80f));
    }
}

// ---------------------------------------------------------------- traffic world
enum Signal { SIG_GREEN, SIG_YELLOW, SIG_RED };
constexpr float GREEN_T = 9.0f;
constexpr float YELLOW_T = 2.0f;
constexpr float RED_T = 8.5f;
constexpr float CYCLE_T = GREEN_T + YELLOW_T + RED_T;

float signalTimer = 3.0f;      // seconds into the traffic-light cycle

Signal currentSignal() {
    float t = std::fmod(signalTimer, CYCLE_T);
    if (t < GREEN_T) return SIG_GREEN;
    if (t < GREEN_T + YELLOW_T) return SIG_YELLOW;
    return SIG_RED;
}

float redElapsed() {   // seconds since the light turned red
    return std::fmod(signalTimer, CYCLE_T) - (GREEN_T + YELLOW_T);
}

struct Vehicle {
    float x, base, len, vmax, v;
    bool right;
    int kind;                // 0 = car, 1 = school bus
    Color dayColor, nightColor;
};

// eastbound = lower lane (facing right), westbound = upper lane (facing left)
Vehicle vehicles[4] = {
    {140.0f,  752.0f, 170.0f, 150.0f, 150.0f, true,  0, rgb(0.85f, 0.06f, 0.07f), rgb(0.82f, 0.04f, 0.06f)},
    {-380.0f, 752.0f, 230.0f, 125.0f, 125.0f, true,  1, rgb(0.98f, 0.78f, 0.10f), rgb(0.85f, 0.65f, 0.08f)},
    {1000.0f, 708.0f, 100.0f, 105.0f, 105.0f, false, 0, rgb(0.12f, 0.38f, 0.85f), rgb(0.08f, 0.25f, 0.65f)},
    {1400.0f, 708.0f, 105.0f, 115.0f, 115.0f, false, 0, rgb(0.90f, 0.90f, 0.93f), rgb(0.62f, 0.65f, 0.74f)}
};

struct Look { Color shirt, pants, hair, skin; };

Look LOOKS[8] = {
    {rgb(0.85f, 0.20f, 0.20f), rgb(0.15f, 0.20f, 0.45f), rgb(0.10f, 0.07f, 0.05f), rgb(0.80f, 0.58f, 0.42f)},
    {rgb(0.15f, 0.55f, 0.75f), rgb(0.25f, 0.25f, 0.28f), rgb(0.05f, 0.04f, 0.04f), rgb(0.65f, 0.45f, 0.32f)},
    {rgb(0.95f, 0.80f, 0.20f), rgb(0.20f, 0.35f, 0.20f), rgb(0.25f, 0.15f, 0.08f), rgb(0.85f, 0.65f, 0.50f)},
    {rgb(0.55f, 0.30f, 0.70f), rgb(0.30f, 0.20f, 0.15f), rgb(0.08f, 0.06f, 0.05f), rgb(0.72f, 0.52f, 0.38f)},
    {rgb(0.90f, 0.45f, 0.15f), rgb(0.12f, 0.12f, 0.20f), rgb(0.15f, 0.10f, 0.06f), rgb(0.78f, 0.56f, 0.40f)},
    {rgb(0.25f, 0.70f, 0.40f), rgb(0.45f, 0.30f, 0.18f), rgb(0.06f, 0.05f, 0.05f), rgb(0.62f, 0.42f, 0.30f)},
    {rgb(0.90f, 0.90f, 0.92f), rgb(0.20f, 0.28f, 0.50f), rgb(0.30f, 0.22f, 0.12f), rgb(0.82f, 0.62f, 0.47f)},
    {rgb(0.80f, 0.25f, 0.50f), rgb(0.18f, 0.18f, 0.22f), rgb(0.07f, 0.05f, 0.05f), rgb(0.70f, 0.50f, 0.36f)}
};

struct Walker { float x, footY, speed, scale; int look; float phase; };

Walker walkers[6] = {
    {210.0f,  656.0f,  28.0f, 0.90f, 0, 0.0f},
    {880.0f,  656.0f, -24.0f, 0.90f, 1, 1.5f},
    {1150.0f, 656.0f,  21.0f, 0.90f, 2, 3.0f},
    {120.0f,  783.0f, -30.0f, 1.05f, 3, 0.5f},
    {1000.0f, 783.0f,  24.0f, 1.05f, 4, 2.0f},
    {430.0f,  783.0f,  20.0f, 1.05f, 5, 4.0f}
};

// pedestrians that use the zebra crossing while the cars wait at a red light
struct Ped { float x, y; int state; float phase; int look; };   // 0 wait top, 1 down, 2 wait bottom, 3 up

Ped peds[2] = {
    {658.0f, 656.0f, 0, 0.0f, 6},
    {705.0f, 783.0f, 2, 0.0f, 7}
};

constexpr float PED_TOP = 656.0f;
constexpr float PED_BOTTOM = 783.0f;

void updateVehicles(float dt) {
    const float L = DESIGN_W + 800.0f;
    const float lo = -400.0f;
    Signal sig = currentSignal();

    for (int i = 0; i < 4; ++i) {
        Vehicle& a = vehicles[i];
        float front = a.right ? a.x + a.len : a.x;
        float factor = 1.0f;

        // keep a safe gap to the vehicle ahead in the same lane
        for (int j = 0; j < 4; ++j) {
            if (j == i || vehicles[j].right != a.right) continue;
            const Vehicle& b = vehicles[j];
            float gap = a.right ? wrapf(b.x - front, L)
                                : wrapf(a.x - (b.x + b.len), L);
            if (gap < 400.0f) factor = std::min(factor, clamp01((gap - 30.0f) / 90.0f));
        }

        // stop at a red / yellow light
        float stopLine = a.right ? 616.0f : 756.0f;
        float dist = a.right ? stopLine - front : front - stopLine;
        bool mustStop = (sig == SIG_RED) || (sig == SIG_YELLOW && dist > 40.0f);
        if (mustStop && dist > -15.0f && dist < 220.0f) {
            factor = std::min(factor, clamp01((dist - 14.0f) / 90.0f));
        }

        float target = a.vmax * factor;
        a.v += (target - a.v) * std::min(dt * 3.0f, 1.0f);
        a.x += (a.right ? a.v : -a.v) * dt;

        if (a.right && a.x > lo + L) a.x -= L;
        if (!a.right && a.x < lo) a.x += L;
    }
}

void updatePeople(float dt) {
    for (Walker& w : walkers) {
        w.x += w.speed * dt;
        w.phase += dt * 6.5f;
        if (w.x > DESIGN_W + 60.0f) w.x = -60.0f;
        if (w.x < -60.0f) w.x = DESIGN_W + 60.0f;
    }

    Signal sig = currentSignal();
    float re = redElapsed();
    bool canStart = (sig == SIG_RED) && re > 1.6f && (RED_T - re) > 3.6f;

    for (Ped& p : peds) {
        if (p.state == 0 && canStart) p.state = 1;
        if (p.state == 2 && canStart) p.state = 3;

        if (p.state == 1) {
            p.y += 42.0f * dt;
            p.phase += dt * 7.0f;
            if (p.y >= PED_BOTTOM) { p.y = PED_BOTTOM; p.state = 2; }
        } else if (p.state == 3) {
            p.y -= 42.0f * dt;
            p.phase += dt * 7.0f;
            if (p.y <= PED_TOP) { p.y = PED_TOP; p.state = 0; }
        }
    }
}

void updateWorld(float dt) {
    signalTimer = std::fmod(signalTimer + dt, CYCLE_T);
    updateVehicles(dt);
    updatePeople(dt);
}

// ---------------------------------------------------------------- people
Color litc(Color c) {
    return tone(c, rgb(c.r * 0.45f, c.g * 0.50f, c.b * 0.70f, c.a));
}

void umbrella(float x, float canopyY, float r, Color c, float handleX, float handleY) {
    line(x, canopyY, handleX, handleY, 2.0f, rgb(0.12f, 0.12f, 0.12f));
    std::vector<std::pair<float, float>> pts;
    for (int i = 0; i <= 12; ++i) {
        float a = PI + PI * i / 12.0f;
        pts.push_back({x + std::cos(a) * r, canopyY + std::sin(a) * r * 0.55f});
    }
    polygon(pts, c);
}

// mode 0 = walking sideways, 2 = walking toward/away from us, 3 = standing
void person(float x, float footY, float s, int mode, float phase,
            const Look& L, float dir, bool brolly) {
    Color shirt = litc(L.shirt);
    Color pants = litc(L.pants);
    Color hair = litc(L.hair);
    Color skin = litc(L.skin);

    float legLen = 26.0f * s;
    float torso = 24.0f * s;
    float headR = 7.5f * s;
    float hipY = footY - legLen;
    float shY = hipY - torso;
    float headY = shY - headR - 1.0f * s;
    float sw = std::sin(phase);

    // legs
    if (mode == 0) {
        float a = 0.55f * sw;
        line(x, hipY, x + dir * std::sin(a) * legLen, hipY + std::cos(a) * legLen, 6.0f * s, dim(pants, 0.8f));
        line(x, hipY, x - dir * std::sin(a) * legLen, hipY + std::cos(a) * legLen, 6.0f * s, pants);
    } else {
        float l1 = (mode == 2) ? std::max(0.0f, sw) * 6.0f * s : 0.0f;
        float l2 = (mode == 2) ? std::max(0.0f, -sw) * 6.0f * s : 0.0f;
        line(x - 4.0f * s, hipY, x - 4.0f * s, footY - l1, 6.0f * s, pants);
        line(x + 4.0f * s, hipY, x + 4.0f * s, footY - l2, 6.0f * s, pants);
    }

    // torso
    rect(x - 7.0f * s, shY, 14.0f * s, torso + 2.0f * s, shirt);

    // arms
    float armLen = 20.0f * s;
    float handX = x, handY = shY + 3.0f * s + armLen;
    if (mode == 0) {
        float a = 0.6f * sw;
        float hx = x + dir * std::sin(a) * armLen;
        float hy = shY + 3.0f * s + std::cos(a) * armLen;
        line(x, shY + 3.0f * s, x - dir * std::sin(a) * armLen, hy, 4.5f * s, dim(shirt, 0.85f));
        line(x, shY + 3.0f * s, hx, hy, 4.5f * s, shirt);
        circle(hx, hy, 2.6f * s, skin, 8);
        handX = hx; handY = hy;
    } else {
        float sw2 = (mode == 2) ? sw * 3.0f * s : 0.0f;
        line(x - 8.5f * s, shY + 4.0f * s, x - 9.0f * s, shY + 4.0f * s + armLen - sw2, 4.5f * s, dim(shirt, 0.85f));
        line(x + 8.5f * s, shY + 4.0f * s, x + 9.0f * s, shY + 4.0f * s + armLen + sw2, 4.5f * s, shirt);
        circle(x + 9.0f * s, shY + 4.0f * s + armLen + sw2, 2.6f * s, skin, 8);
        handX = x + 9.0f * s; handY = shY + 4.0f * s + armLen * 0.55f;
    }

    // umbrella (behind the head)
    if (brolly) {
        Color uc = litc(rgb(L.shirt.r * 0.8f + 0.1f, L.shirt.g * 0.8f + 0.05f, L.shirt.b * 0.8f + 0.1f));
        umbrella(x, headY - headR - 9.0f * s, 26.0f * s, uc, handX, handY);
    }

    // head
    circle(x, headY - 1.2f * s, headR + 0.5f * s, hair, 16);
    circle(x + (mode == 0 ? dir * 1.5f * s : 0.0f), headY + 1.2f * s, headR - 0.8f * s, skin, 16);
}

void sitChild(float x, float seatY, float s, float dir, const Look& L) {
    Color shirt = litc(L.shirt);
    Color pants = litc(L.pants);
    Color hair = litc(L.hair);
    Color skin = litc(L.skin);

    line(x, seatY - 2.0f * s, x + dir * 14.0f * s, seatY, 5.0f * s, pants);
    line(x + dir * 14.0f * s, seatY, x + dir * 14.0f * s, seatY + 12.0f * s, 5.0f * s, pants);
    rect(x - 5.0f * s, seatY - 18.0f * s, 10.0f * s, 18.0f * s, shirt);
    line(x, seatY - 14.0f * s, x + dir * 9.0f * s, seatY - 9.0f * s, 3.5f * s, shirt);
    circle(x, seatY - 24.0f * s, 6.5f * s, hair, 14);
    circle(x + dir * s, seatY - 22.5f * s, 5.4f * s, skin, 14);
}

void drawWalkers(bool upperPath) {
    bool brolly = rainAmount > 0.45f;
    for (const Walker& w : walkers) {
        bool isUpper = w.footY < 700.0f;
        if (isUpper != upperPath) continue;
        person(w.x, w.footY, w.scale, 0, w.phase, LOOKS[w.look],
               w.speed >= 0.0f ? 1.0f : -1.0f, brolly);
    }
}

void drawCrossingPeople() {
    bool brolly = rainAmount > 0.45f;
    for (const Ped& p : peds) {
        int mode = (p.state == 1 || p.state == 3) ? 2 : 3;
        person(p.x, p.y, 1.0f, mode, p.phase, LOOKS[p.look], 1.0f, brolly);
    }
}

// ---------------------------------------------------------------- footpaths & traffic lights
void drawFootpaths(bool upper) {
    Color path = tone(rgb(0.74f, 0.71f, 0.65f), rgb(0.27f, 0.27f, 0.31f));
    Color tile = tone(rgb(0.62f, 0.59f, 0.54f), rgb(0.20f, 0.20f, 0.24f));
    Color edge = tone(rgb(0.86f, 0.84f, 0.79f), rgb(0.36f, 0.36f, 0.41f));

    if (upper) {
        rect(0.0f, 633.0f, DESIGN_W, 30.0f, path);
        rect(0.0f, 633.0f, DESIGN_W, 3.0f, edge);
        for (float x = 0.0f; x < DESIGN_W; x += 48.0f) rect(x, 636.0f, 2.0f, 27.0f, tile);
    } else {
        const float seg[2][2] = {{0.0f, 713.0f}, {813.0f, DESIGN_W}};
        for (const auto& s : seg) {
            rect(s[0], 760.0f, s[1] - s[0], 30.0f, path);
            rect(s[0], 787.0f, s[1] - s[0], 3.0f, edge);
            for (float x = s[0]; x < s[1]; x += 48.0f) rect(x, 760.0f, 2.0f, 27.0f, tile);
        }
    }
}

void trafficLight(float x, float baseY) {
    Signal s = currentSignal();
    Color pole = tone(rgb(0.22f, 0.23f, 0.26f), rgb(0.15f, 0.16f, 0.19f));
    Color housing = rgb(0.07f, 0.07f, 0.08f);

    rect(x - 2.5f, baseY - 100.0f, 5.0f, 100.0f, pole);
    rect(x - 10.0f, baseY - 160.0f, 20.0f, 60.0f, housing);

    struct Lens { float y; Color on, off; bool lit; };
    Lens lens[3] = {
        {baseY - 147.0f, rgb(1.0f, 0.15f, 0.10f), rgb(0.28f, 0.05f, 0.05f), s == SIG_RED},
        {baseY - 130.0f, rgb(1.0f, 0.82f, 0.10f), rgb(0.30f, 0.24f, 0.03f), s == SIG_YELLOW},
        {baseY - 113.0f, rgb(0.10f, 0.95f, 0.30f), rgb(0.04f, 0.25f, 0.08f), s == SIG_GREEN}
    };

    for (const Lens& l : lens) {
        if (l.lit) {
            Color glow = l.on;
            glow.a = 0.28f;
            circle(x, l.y, 17.0f, glow, 20);
        }
        circle(x, l.y, 6.5f, l.lit ? l.on : l.off, 16);
    }
}

// ---------------------------------------------------------------- shops
void smallShop(float x, float baseY) {
    const float w = 138.0f;
    const float h = 100.0f;
    float top = baseY - h;

    Color wall = tone(rgb(0.30f, 0.62f, 0.58f), rgb(0.12f, 0.30f, 0.30f));
    Color roof = tone(rgb(0.50f, 0.55f, 0.60f), rgb(0.18f, 0.20f, 0.26f));
    Color roofLine = dim(roof, 0.75f);
    Color board = tone(rgb(0.92f, 0.78f, 0.25f), rgb(1.0f, 0.82f, 0.30f));
    Color boardText = rgb(0.22f, 0.07f, 0.03f);
    Color inside = tone(rgb(0.38f, 0.30f, 0.24f), rgb(0.95f, 0.70f, 0.35f));
    Color wood = tone(rgb(0.50f, 0.30f, 0.14f), rgb(0.26f, 0.15f, 0.08f));

    rect(x, top, w, h, wall);

    // tin roof
    polygon({{x - 8.0f, top + 4.0f}, {x + 14.0f, top - 22.0f},
             {x + w - 14.0f, top - 22.0f}, {x + w + 8.0f, top + 4.0f}}, roof);
    for (float rx = x + 6.0f; rx < x + w - 4.0f; rx += 9.0f) {
        line(rx, top + 3.0f, x + 14.0f + (rx - x) * 0.78f, top - 21.0f, 1.2f, roofLine);
    }

    // sign board
    rect(x + 20.0f, top + 8.0f, w - 40.0f, 21.0f, board);
    drawText("SHOP", x + 31.0f, top + 12.0f, 14.0f, 13.0f, 6.0f, 2.4f, boardText);

    // open counter with goods
    rect(x + 14.0f, top + 44.0f, w - 28.0f, h - 44.0f - 8.0f, inside);
    for (int i = 0; i < 2; ++i) {
        float sy = top + 58.0f + i * 15.0f;
        line(x + 14.0f, sy, x + w - 14.0f, sy, 2.0f, wood);
        for (int k = 0; k < 8; ++k) {
            float hx = hashf(i * 17.0f + k * 3.3f);
            Color pc = litc(rgb(0.4f + hx * 0.6f, 0.3f + hashf(k * 5.1f + i) * 0.6f, 0.2f + hashf(k * 7.7f) * 0.6f));
            rect(x + 18.0f + k * 13.0f, sy - 9.0f, 9.0f, 9.0f, pc);
        }
    }

    // shopkeeper behind the counter
    person(x + w * 0.5f, baseY - 8.0f, 0.62f, 3, 0.0f, LOOKS[2], 1.0f, false);

    // counter front
    rect(x + 10.0f, baseY - 22.0f, w - 20.0f, 14.0f, wood);

    // striped awning
    for (int i = 0; i < 7; ++i) {
        float sw = (w - 4.0f) / 7.0f;
        float sx = x + 2.0f + i * sw;
        Color c = (i % 2 == 0) ? tone(rgb(0.85f, 0.15f, 0.15f), rgb(0.55f, 0.10f, 0.12f))
                               : tone(rgb(0.95f, 0.95f, 0.95f), rgb(0.55f, 0.57f, 0.62f));
        polygon({{sx, top + 31.0f}, {sx + sw, top + 31.0f},
                 {sx + sw + 2.0f, top + 45.0f}, {sx - 2.0f, top + 45.0f}}, c);
    }

    // glow from the open front at night
    if (lampOn > 0.01f) {
        rect(x + 14.0f, top + 44.0f, w - 28.0f, h - 44.0f - 8.0f,
             rgb(1.0f, 0.75f, 0.35f, 0.10f * lampOn));
    }
}

void superShop() {
    const float x0 = 450.0f;
    const float x1 = 705.0f;

    Color front = tone(rgb(0.93f, 0.95f, 0.97f), rgb(0.50f, 0.54f, 0.62f));
    Color side = dim(front, 0.75f);
    Color roof = tone(rgb(0.45f, 0.48f, 0.52f), rgb(0.22f, 0.24f, 0.29f));
    Color band = tone(rgb(0.10f, 0.58f, 0.30f), rgb(0.06f, 0.42f, 0.22f));
    Color text = tone(rgb(1.0f, 1.0f, 1.0f), rgb(1.0f, 0.95f, 0.55f));
    Color glass = tone(rgb(0.62f, 0.80f, 0.92f), rgb(1.0f, 0.92f, 0.65f));
    Color frame = tone(rgb(0.25f, 0.28f, 0.32f), rgb(0.12f, 0.13f, 0.16f));
    Color shelf = tone(rgb(0.55f, 0.55f, 0.58f), rgb(0.35f, 0.33f, 0.30f));

    polygon({{441.0f, 812.0f}, {470.0f, 796.0f}, {716.0f, 796.0f}, {705.0f, 812.0f}}, roof);
    rect(x0, 812.0f, x1 - x0, 153.0f, front);
    polygon({{705.0f, 812.0f}, {716.0f, 796.0f}, {716.0f, 948.0f}, {705.0f, 965.0f}}, side);

    // sign band
    rect(x0, 818.0f, x1 - x0, 36.0f, band);
    if (lampOn > 0.01f) {
        rect(x0, 818.0f, x1 - x0, 36.0f, rgb(0.6f, 1.0f, 0.6f, 0.12f * lampOn));
    }
    drawText("SUPER SHOP", x0 + 16.0f, 826.0f, 15.0f, 20.0f, 8.0f, 3.0f, text);

    // striped awning
    for (int i = 0; i < 9; ++i) {
        float sx = x0 + i * 28.3f;
        Color c = (i % 2 == 0) ? tone(rgb(0.85f, 0.15f, 0.15f), rgb(0.55f, 0.10f, 0.12f))
                               : tone(rgb(0.97f, 0.97f, 0.97f), rgb(0.58f, 0.60f, 0.66f));
        polygon({{sx, 856.0f}, {sx + 28.3f, 856.0f}, {sx + 31.0f, 878.0f}, {sx - 3.0f, 878.0f}}, c);
    }

    // two display windows with shelves and products
    const float wx[2] = {464.0f, 610.0f};
    for (int w = 0; w < 2; ++w) {
        float x = wx[w];
        rect(x - 3.0f, 881.0f, 87.0f, 70.0f, frame);
        rect(x, 884.0f, 81.0f, 64.0f, glass);
        for (int r = 0; r < 3; ++r) {
            float sy = 904.0f + r * 18.0f;
            for (int k = 0; k < 6; ++k) {
                float hh = 9.0f + hashf(w * 31.0f + r * 7.0f + k) * 7.0f;
                Color pc = litc(rgb(0.35f + hashf(k * 3.1f + r + w) * 0.65f,
                                    0.25f + hashf(k * 5.7f + r * 2.0f) * 0.65f,
                                    0.20f + hashf(k * 9.3f + w) * 0.70f));
                rect(x + 5.0f + k * 12.5f, sy - hh, 9.0f, hh, pc);
            }
            rect(x, sy, 81.0f, 2.5f, shelf);
        }
    }

    // glass sliding door
    rect(554.0f, 880.0f, 48.0f, 85.0f, frame);
    rect(558.0f, 884.0f, 19.0f, 81.0f, glass);
    rect(579.0f, 884.0f, 19.0f, 81.0f, glass);

    // base strip
    rect(x0, 955.0f, x1 - x0, 10.0f, dim(front, 0.8f));
}

// ---------------------------------------------------------------- playground
void playground(float t) {
    Color sand = tone(rgb(0.86f, 0.76f, 0.50f), rgb(0.30f, 0.28f, 0.22f));
    Color frame = tone(rgb(0.20f, 0.45f, 0.85f), rgb(0.10f, 0.22f, 0.42f));
    Color chain = tone(rgb(0.35f, 0.35f, 0.38f), rgb(0.20f, 0.20f, 0.24f));
    Color seat = tone(rgb(0.90f, 0.25f, 0.20f), rgb(0.50f, 0.14f, 0.12f));
    Color plank = tone(rgb(0.95f, 0.60f, 0.15f), rgb(0.55f, 0.32f, 0.10f));
    Color chute = tone(rgb(0.98f, 0.82f, 0.15f), rgb(0.55f, 0.45f, 0.10f));
    Color wood = tone(rgb(0.55f, 0.35f, 0.18f), rgb(0.28f, 0.17f, 0.09f));
    Color boardC = tone(rgb(0.95f, 0.88f, 0.55f), rgb(0.55f, 0.50f, 0.30f));
    Color boardText = rgb(0.30f, 0.12f, 0.05f);

    // sandy ground
    polygon({{1152.0f, 925.0f}, {1170.0f, 902.0f}, {DESIGN_W, 902.0f},
             {DESIGN_W, DESIGN_H}, {1152.0f, DESIGN_H}}, sand);

    // sign "PLAYGROUND"
    rect(1346.0f, 824.0f, 4.0f, 70.0f, wood);
    rect(1440.0f, 824.0f, 4.0f, 70.0f, wood);
    rect(1336.0f, 800.0f, 118.0f, 26.0f, boardC);
    drawText("PLAYGROUND", 1341.0f, 807.0f, 8.0f, 12.0f, 3.4f, 1.8f, boardText);

    // swing set
    const float scx = 1230.0f;
    const float beamY = 838.0f;
    line(scx - 35.0f, beamY, scx + 35.0f, beamY, 6.0f, frame);
    line(scx - 35.0f, beamY, scx - 55.0f, 950.0f, 5.0f, frame);
    line(scx - 35.0f, beamY, scx - 13.0f, 950.0f, 5.0f, frame);
    line(scx + 35.0f, beamY, scx + 13.0f, 950.0f, 5.0f, frame);
    line(scx + 35.0f, beamY, scx + 55.0f, 950.0f, 5.0f, frame);

    for (int k = 0; k < 2; ++k) {
        float bx = scx - 14.0f + k * 28.0f;
        float ang = 0.45f * std::sin(t * 1.9f + k * 2.2f);
        float sx = bx + 60.0f * std::sin(ang);
        float sy = beamY + 60.0f * std::cos(ang);
        line(bx - 4.0f, beamY, sx - 8.0f, sy, 1.5f, chain);
        line(bx + 4.0f, beamY, sx + 8.0f, sy, 1.5f, chain);
        line(sx - 10.0f, sy, sx + 10.0f, sy, 5.0f, seat);
        if (k == 0) sitChild(sx, sy - 2.0f, 0.62f, 1.0f, LOOKS[2]);
    }

    // seesaw
    const float ssx = 1316.0f;
    float th = 0.26f * std::sin(t * 1.3f);
    float ax = ssx - 34.0f * std::cos(th), ay = 929.0f + 34.0f * std::sin(th);
    float bx2 = ssx + 34.0f * std::cos(th), by2 = 929.0f - 34.0f * std::sin(th);
    triangle(ssx - 10.0f, 950.0f, ssx, 929.0f, ssx + 10.0f, 950.0f, frame);
    sitChild(ax, ay - 3.0f, 0.55f, 1.0f, LOOKS[5]);
    sitChild(bx2, by2 - 3.0f, 0.55f, -1.0f, LOOKS[7]);
    line(ax, ay, bx2, by2, 6.0f, plank);

    // slide
    const float sl = 1352.0f;
    line(sl + 25.0f, 950.0f, sl + 25.0f, 884.0f, 4.0f, frame);
    line(sl + 47.0f, 950.0f, sl + 47.0f, 884.0f, 4.0f, frame);
    rect(sl + 20.0f, 878.0f, 32.0f, 6.0f, frame);
    line(sl, 950.0f, sl + 22.0f, 892.0f, 3.0f, frame);
    line(sl + 12.0f, 950.0f, sl + 25.0f, 905.0f, 3.0f, frame);
    for (int r = 0; r < 4; ++r) {
        float yy = 935.0f - r * 14.0f;
        float xx = sl + 5.0f + (950.0f - yy) * 0.21f;
        line(xx, yy, xx + 12.0f, yy, 2.0f, frame);
    }
    line(sl + 52.0f, 886.0f, sl + 92.0f, 940.0f, 11.0f, chute);
    line(sl + 54.0f, 880.0f, sl + 94.0f, 934.0f, 2.5f, dim(chute, 0.7f));
    rect(sl + 90.0f, 938.0f, 9.0f, 5.0f, dim(chute, 0.8f));
}

void drawPlaygroundKid(float t) {
    float kx = 1250.0f + 50.0f * std::sin(t * 0.55f);
    float dir = std::cos(t * 0.55f) >= 0.0f ? 1.0f : -1.0f;
    person(kx, 962.0f, 0.6f, 0, t * 6.0f, LOOKS[4], dir, rainAmount > 0.45f);
}

// ---------------------------------------------------------------- bus
void bus(float x, float base, float len, Color body, bool faceRight) {
    auto X = [&](float u) { return faceRight ? x + u * len : x + (1.0f - u) * len; };
    auto box = [&](float u0, float u1, float yTop, float yBot, Color c) {
        float a = X(u0);
        float b = X(u1);
        rect(std::min(a, b), yTop, std::fabs(b - a), yBot - yTop, c);
    };

    float top = base - 0.36f * len;
    float hy = base - 0.12f * len;
    float dir = faceRight ? 1.0f : -1.0f;

    if (lampOn > 0.01f) {
        float fx = X(1.0f);
        polygon({{fx, hy - 3.0f}, {fx + dir * 0.45f * len, hy - 0.16f * len},
                 {fx + dir * 0.45f * len, hy + 0.14f * len}, {fx, hy + 6.0f}},
                rgb(0.85f, 0.88f, 0.55f, 0.28f * lampOn));
    }

    box(0.0f, 1.0f, top, base - 0.05f * len, body);
    box(0.0f, 1.0f, base - 0.17f * len, base - 0.145f * len, dim(body, 0.55f));

    Color glass = tone(rgb(0.55f, 0.80f, 0.95f), rgb(1.0f, 0.85f, 0.45f));
    for (int i = 0; i < 6; ++i) {
        float u0 = 0.07f + i * 0.125f;
        box(u0, u0 + 0.10f, top + 0.05f * len, top + 0.17f * len, glass);
    }
    box(0.88f, 0.985f, top + 0.05f * len, top + 0.22f * len, glass);

    box(0.96f, 1.0f, hy - 0.04f * len, hy + 0.04f * len,
        tone(rgb(0.90f, 0.90f, 0.70f), rgb(1.0f, 0.85f, 0.25f)));
    box(0.0f, 0.02f, hy - 0.04f * len, hy + 0.04f * len, rgb(0.90f, 0.10f, 0.10f));

    float wheelY = base - 0.015f * len;
    float wr = 0.062f * len;
    for (float u : {0.19f, 0.78f}) {
        float wx = X(u);
        circle(wx, wheelY, wr, rgb(0.05f, 0.05f, 0.06f), 24);
        circle(wx, wheelY, wr * 0.45f, rgb(0.70f, 0.70f, 0.72f), 16);
    }
}

void drawVehicle(const Vehicle& v) {
    Color c = tone(v.dayColor, v.nightColor);
    if (v.kind == 1) bus(v.x, v.base, v.len, c, v.right);
    else car(v.x, v.base, v.len, c, v.right);
}

// ---------------------------------------------------------------- pond
void drawPond(float t) {
    const float dx = -125.0f;
    Color water = tone(rgb(0.25f, 0.62f, 0.85f), rgb(0.02f, 0.22f, 0.34f));
    Color shine = tone(rgb(0.85f, 0.95f, 1.0f), rgb(0.75f, 0.90f, 1.0f));

    polygon({{172.0f + dx, 860.0f}, {250.0f + dx, 800.0f}, {345.0f + dx, 805.0f},
             {422.0f + dx, 840.0f}, {354.0f + dx, 905.0f}, {255.0f + dx, 918.0f}}, water);

    line(239.0f + dx, 855.0f, 309.0f + dx, 855.0f, 2.5f, shine);
    line(312.0f + dx, 872.0f, 364.0f + dx, 872.0f, 2.5f, shine);
    ring(297.0f + dx, 838.0f, 20.0f, 7.0f, 2.0f, shine);

    if (rainAmount > 0.01f) {
        for (int k = 0; k < 4; ++k) {
            float v = t * 0.7f + k * 0.33f;
            float phase = v - std::floor(v);
            int cyc = static_cast<int>(std::floor(v));
            float px = 225.0f + dx + hashf(cyc * 1.7f + k * 9.1f) * 120.0f;
            float py = 835.0f + hashf(cyc * 2.3f + k * 4.7f) * 55.0f;
            float rx = 4.0f + phase * 22.0f;
            Color rc = shine;
            rc.a = (1.0f - phase) * 0.8f * rainAmount;
            ring(px, py, rx, rx * 0.4f, 1.6f, rc, 24);
        }
    }
}

// ---------------------------------------------------------------- rain
void drawRain(float t) {
    if (rainAmount < 0.01f) return;

    // slight overcast tint
    rect(0.0f, 0.0f, DESIGN_W, DESIGN_H,
         mixc(rgb(0.35f, 0.40f, 0.48f, 0.22f * rainAmount),
              rgb(0.0f, 0.0f, 0.05f, 0.22f * rainAmount), gNight));

    // splashes on the road and footpaths
    Color sp = tone(rgb(0.85f, 0.92f, 1.0f), rgb(0.70f, 0.75f, 1.0f));
    for (int i = 0; i < 26; ++i) {
        float v = t * 2.2f + hashf(i * 1.9f) * 5.0f;
        float phase = v - std::floor(v);
        int cyc = static_cast<int>(std::floor(v));
        float x = hashf(i * 4.3f + cyc * 0.37f) * DESIGN_W;
        float y = 640.0f + hashf(i * 6.1f + cyc * 0.91f) * 145.0f;
        float rx = 2.0f + phase * 7.0f;
        Color c = sp;
        c.a = (1.0f - phase) * 0.7f * rainAmount;
        ring(x, y, rx, rx * 0.4f, 1.2f, c, 12);
    }

    const int N = 280;
    const float M = DESIGN_W + 300.0f;

    Color c = tone(rgb(0.55f, 0.72f, 0.95f, 0.65f), rgb(0.58f, 0.58f, 1.0f, 0.85f));
    c.a *= rainAmount;

    for (int i = 0; i < N; ++i) {
        float h1 = hashf(i * 1.37f + 1.0f);
        float h2 = hashf(i * 2.71f + 5.0f);
        float h3 = hashf(i * 3.14f + 9.0f);

        float speed = 560.0f + h3 * 300.0f;
        float d = t * speed;

        float x = wrapf(h1 * M - 0.32f * d, M) - 100.0f;
        float y = wrapf(h2 * (DESIGN_H + 80.0f) + d, DESIGN_H + 80.0f) - 40.0f;
        float len = 22.0f + h3 * 20.0f;

        line(x, y, x + len * 0.30f, y - len, 1.6f, c);
    }
}

// ---------------------------------------------------------------- whole scene
void drawScene(float t) {
    vertices.clear();

    gTime = t;
    gNight = 1.0f - dayAmount;
    lampOn = smoothstep(0.35f, 0.65f, gNight);

    drawSky();
    drawStars(t);
    drawSunAndMoon();
    drawClouds(t);
    drawBackground();

    // grass strip behind the road + upper footpath
    rect(0.0f, 588.0f, DESIGN_W, 90.0f,
         tone(rgb(0.30f, 0.64f, 0.26f), rgb(0.01f, 0.27f, 0.07f)));
    drawFootpaths(true);

    // small village shop + row of houses
    smallShop(12.0f, 607.0f);
    house(182.0f, 607.0f, 193.0f, 127.0f, 83.0f, false, false);
    house(456.0f, 607.0f, 193.0f, 127.0f, 83.0f, false, false);
    house(740.0f, 607.0f, 193.0f, 127.0f, 83.0f, false, false);
    house(1051.0f, 607.0f, 228.0f, 142.0f, 80.0f, true, true);

    pine(1342.0f, 607.0f, 282.0f);

    // street lamps: positioned in the gaps beside the houses
    lamp(420.0f);
    lamp(700.0f);
    lamp(970.0f);

    // people on the far footpath
    drawWalkers(true);

    // lower lawn + near footpath
    rect(0.0f, 755.0f, DESIGN_W, DESIGN_H - 755.0f,
         tone(rgb(0.26f, 0.58f, 0.22f), rgb(0.0f, 0.22f, 0.06f)));
    rect(0.0f, 760.0f, DESIGN_W, 25.0f,
         tone(rgb(0.18f, 0.45f, 0.17f), rgb(0.0f, 0.14f, 0.05f)));
    drawFootpaths(false);

    drawPond(t);
    school();
    superShop();
    playground(t);

    drawRoad();

    // vehicles (far lane first)
    drawVehicle(vehicles[2]);
    drawVehicle(vehicles[3]);
    drawVehicle(vehicles[0]);
    drawVehicle(vehicles[1]);

    // near-side traffic lights
    trafficLight(596.0f, 788.0f);
    trafficLight(822.0f, 788.0f);

    // trees on grass only
    pine(345.0f, 905.0f, 200.0f);

    // people in front of the road
    drawCrossingPeople();
    drawWalkers(false);
    drawPlaygroundKid(t);

    drawRain(t);
}

// ---------------------------------------------------------------- input
void keyCallback(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_D) { automaticMode = false; targetDay = 1.0f; }
    if (key == GLFW_KEY_N) { automaticMode = false; targetDay = 0.0f; }
    if (key == GLFW_KEY_A) automaticMode = true;
    if (key == GLFW_KEY_R) raining = !raining;
    if (key == GLFW_KEY_S) raining = false;
    if (key == GLFW_KEY_SPACE) paused = !paused;
}

unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char log[1024] = {};
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Shader compilation error:\n" << log << '\n';
    }
    return shader;
}

// ---------------------------------------------------------------- main
int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed.\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        WIDTH, HEIGHT, "SMART VILLAGE | Day and Night Scenario",
        nullptr, nullptr);

    if (!window) {
        std::cerr << "Could not create window. OpenGL 3.3 may not be supported.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD initialization failed.\n";
        glfwTerminate();
        return -1;
    }

    // transparency (glows, headlight beams, rain, smoke, ripples)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    if (!linked) {
        char log[1024] = {};
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "Shader program link error:\n" << log << '\n';
        glfwTerminate();
        return -1;
    }

    unsigned int VAO = 0;
    unsigned int VBO = 0;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    float previousTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        float now = static_cast<float>(glfwGetTime());
        float deltaTime = std::min(now - previousTime, 0.05f);
        previousTime = now;

        if (!paused) {
            if (automaticMode) {
                targetDay = (std::sin(now * 0.20f) + 1.0f) * 0.5f;
            }

            dayAmount += (targetDay - dayAmount) * std::min(deltaTime * 1.5f, 1.0f);

            float rainTarget = raining ? 1.0f : 0.0f;
            rainAmount += (rainTarget - rainAmount) * std::min(deltaTime * 2.0f, 1.0f);

            updateWorld(deltaTime);

            animationTime = now;
        }

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        const float sceneAspect = static_cast<float>(WIDTH) / HEIGHT;

        float viewportWidth = static_cast<float>(framebufferWidth);
        float viewportHeight = viewportWidth / sceneAspect;

        if (viewportHeight > framebufferHeight) {
            viewportHeight = static_cast<float>(framebufferHeight);
            viewportWidth = viewportHeight * sceneAspect;
        }

        int viewportX = static_cast<int>((framebufferWidth - viewportWidth) * 0.5f);
        int viewportY = static_cast<int>((framebufferHeight - viewportHeight) * 0.5f);

        glViewport(viewportX, viewportY,
                   static_cast<int>(viewportWidth),
                   static_cast<int>(viewportHeight));

        glClearColor(0.025f, 0.035f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        drawScene(animationTime);

        glUseProgram(program);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER,
                     vertices.size() * sizeof(Vertex),
                     vertices.data(),
                     GL_DYNAMIC_DRAW);

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glfwTerminate();
    return 0;
}