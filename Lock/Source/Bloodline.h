#pragma once
// ============================================================================
//  CrushAudio - THE BLOODLINE COLLECTION : shared skin helpers (header only)
//  Palette + drawing primitives ported from the Skin Kit (tokens, crest, knob,
//  sigil, panel). Same file is used by SonyTune, Crush and Lock.
// ============================================================================
#include <juce_graphics/juce_graphics.h>
#include <cmath>

namespace bl
{
    const juce::Colour voidC        { 0xff070708 };
    const juce::Colour obsidian     { 0xff0d0d10 };
    const juce::Colour panel        { 0xff141418 };
    const juce::Colour raised       { 0xff1b1b20 };
    const juce::Colour silver       { 0xffbfc0c5 };
    const juce::Colour silverBright { 0xffe6e7ea };
    const juce::Colour silverDim    { 0xff777980 };
    const juce::Colour crimson      { 0xff8f1024 };
    const juce::Colour blood        { 0xffc51f38 };
    const juce::Colour glow         { 0xffe12b49 };
    const juce::Colour text         { 0xffe7e5e6 };
    const juce::Colour line         { 0xff343239 };

    inline juce::Font serif (float h)
    {
        return juce::Font (juce::FontOptions (h, juce::Font::bold).withName (juce::Font::getDefaultSerifFontName()));
    }

    // carved obsidian card
    inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius = 9.f)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff19191e), r.getX(), r.getY(),
                                                 juce::Colour (0xff0d0d10), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, radius);
        g.setColour (line);
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.f);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + radius, r.getRight() - radius);
    }

    // L-shaped corner marks, like the original artwork
    inline void drawBrackets (juce::Graphics& g, juce::Rectangle<float> r, float len, juce::Colour c, float t = 1.4f)
    {
        g.setColour (c);
        const float x0 = r.getX(), y0 = r.getY(), x1 = r.getRight(), y1 = r.getBottom();
        g.drawLine (x0, y0, x0 + len, y0, t);  g.drawLine (x0, y0, x0, y0 + len, t);
        g.drawLine (x1, y0, x1 - len, y0, t);  g.drawLine (x1, y0, x1, y0 + len, t);
        g.drawLine (x0, y1, x0 + len, y1, t);  g.drawLine (x0, y1, x0, y1 - len, t);
        g.drawLine (x1, y1, x1 - len, y1, t);  g.drawLine (x1, y1, x1, y1 - len, t);
    }

    // machined silver edge + inner hairline + crimson corner brackets
    inline void drawWindowFrame (juce::Graphics& g, juce::Rectangle<float> b)
    {
        g.setGradientFill (juce::ColourGradient (silverBright.withAlpha (0.60f), b.getX(), b.getY(),
                                                 silverDim.withAlpha (0.25f), b.getRight(), b.getBottom(), false));
        g.drawRect (b.reduced (0.5f), 1.5f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawRect (b.reduced (5.f), 1.f);
        drawBrackets (g, b.reduced (9.f), 16.f, blood, 1.6f);
    }

    // thin crimson veins crawling over the background
    inline void drawVeins (juce::Graphics& g, juce::Rectangle<float> a, int seed, float alpha, int count = 9)
    {
        juce::Random rng (seed);
        for (int i = 0; i < count; ++i)
        {
            float x = a.getX() + rng.nextFloat() * a.getWidth();
            float y = a.getY() + rng.nextFloat() * a.getHeight();
            float ang = rng.nextFloat() * juce::MathConstants<float>::twoPi;
            juce::Path p;
            p.startNewSubPath (x, y);
            for (int s = 0; s < 14; ++s)
            {
                ang += (rng.nextFloat() - 0.5f) * 1.2f;
                x += std::cos (ang) * (10.f + rng.nextFloat() * 14.f);
                y += std::sin (ang) * (10.f + rng.nextFloat() * 14.f);
                p.lineTo (x, y);
            }
            g.setColour (crimson.withAlpha (alpha));
            g.strokePath (p, juce::PathStrokeType (0.8f, juce::PathStrokeType::curved));
        }
    }

    // the Bloodline crest: silver six-point star, crimson diamond, glowing gem
    inline void drawCrest (juce::Graphics& g, juce::Point<float> c, float size, float gem)
    {
        const float R = size * 0.5f;
        g.setColour (juce::Colour (0xff55545b));
        g.drawEllipse (c.x - R, c.y - R, R * 2.f, R * 2.f, 1.2f);
        const float r2 = R * 0.89f;
        g.setColour (crimson);
        g.drawEllipse (c.x - r2, c.y - r2, r2 * 2.f, r2 * 2.f, 1.f);

        juce::Path star;
        star.addStar (c, 6, R * 0.43f, R * 0.91f, 0.f);
        g.setGradientFill (juce::ColourGradient (silverBright, c.x - R, c.y - R, silverDim, c.x + R, c.y + R, false));
        g.strokePath (star, juce::PathStrokeType (juce::jmax (1.2f, size * 0.0125f * 2.f), juce::PathStrokeType::mitered));

        juce::Path dia;
        dia.addStar (c, 4, R * 0.20f, R * 0.54f, 0.f);
        g.setColour (glow.withAlpha (0.25f));
        g.strokePath (dia, juce::PathStrokeType (juce::jmax (3.f, size * 0.05f), juce::PathStrokeType::mitered));
        g.setColour (blood);
        g.strokePath (dia, juce::PathStrokeType (juce::jmax (1.2f, size * 0.02f), juce::PathStrokeType::mitered));

        const float gr = R * 0.085f;
        const float halo = R * (0.22f + 0.28f * gem);
        g.setColour (glow.withAlpha (0.18f + 0.42f * gem));
        g.fillEllipse (c.x - halo, c.y - halo, halo * 2.f, halo * 2.f);
        g.setColour (blood);
        g.fillEllipse (c.x - gr, c.y - gr, gr * 2.f, gr * 2.f);

        g.setColour (silver);
        g.drawLine (c.x, c.y - R * 1.04f, c.x, c.y - R * 0.86f, 1.2f);
        g.drawLine (c.x, c.y + R * 0.86f, c.x, c.y + R * 1.04f, 1.2f);
        g.drawLine (c.x - R * 1.04f, c.y, c.x - R * 0.86f, c.y, 1.2f);
        g.drawLine (c.x + R * 0.86f, c.y, c.x + R * 1.04f, c.y, 1.2f);
    }

    // a row of silver fangs hanging from (or rising from) a baseline
    inline void drawFangs (juce::Graphics& g, float x0, float x1, float baseY, bool pointDown, int count, float h)
    {
        const float w = (x1 - x0) / (float) count;
        const float mid = (x0 + x1) * 0.5f, half = (x1 - x0) * 0.5f;
        for (int i = 0; i < count; ++i)
        {
            const float cx = x0 + w * ((float) i + 0.5f);
            const float t = std::abs ((cx - mid) / half);
            const float hh = h * (1.f - 0.55f * t);
            const float tip = pointDown ? baseY + hh : baseY - hh;
            juce::Path f;
            f.addTriangle (cx - w * 0.38f, baseY, cx + w * 0.38f, baseY, cx, tip);
            g.setGradientFill (juce::ColourGradient (silverBright, cx, baseY, silverDim, cx, tip, false));
            g.fillPath (f);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.strokePath (f, juce::PathStrokeType (0.8f));
            if (t < 0.12f)   // blood on the centre fangs
            {
                g.setColour (blood.withAlpha (0.85f));
                g.fillEllipse (cx - 1.4f, tip + (pointDown ? -4.f : 1.f), 2.8f, 3.f);
            }
        }
    }

    // sevenfold sigil (LOCK). rot is in radians; 7-fold symmetry makes 2pi/7 a seamless loop
    inline void drawSigil (juce::Graphics& g, juce::Point<float> c, float r, float rot, float pulse, float alpha)
    {
        g.setColour (silver.withAlpha (0.55f * alpha));
        g.drawEllipse (c.x - r, c.y - r, r * 2.f, r * 2.f, 1.2f);
        const float r2 = r * 0.86f;
        g.setColour (crimson.withAlpha (0.9f * alpha));
        g.drawEllipse (c.x - r2, c.y - r2, r2 * 2.f, r2 * 2.f, 1.f);

        juce::Path outer;
        outer.addStar (c, 7, r * 0.42f, r * 0.96f, rot);
        g.setColour (silver.withAlpha (0.60f * alpha));
        g.strokePath (outer, juce::PathStrokeType (1.2f, juce::PathStrokeType::mitered));

        juce::Path inner;
        inner.addStar (c, 7, r * 0.16f, r * 0.56f, -rot * 1.5f);
        g.setColour (glow.withAlpha ((0.25f + 0.5f * pulse) * alpha));
        g.strokePath (inner, juce::PathStrokeType (5.f, juce::PathStrokeType::mitered));
        g.setColour (blood.withAlpha (alpha));
        g.strokePath (inner, juce::PathStrokeType (1.8f, juce::PathStrokeType::mitered));

        const float dr = 3.f + 3.f * pulse;
        g.setColour (blood.withAlpha (alpha));
        g.fillEllipse (c.x - dr, c.y - dr, dr * 2.f, dr * 2.f);
    }

    // brushed-metal rotary: crimson value arc, tick ring, ridged silver rim, dark face, blood pointer
    inline void drawMetalKnob (juce::Graphics& g, juce::Rectangle<float> area, float pos, float start, float end,
                               bool bipolar = false, float pointerStart = 0.30f)
    {
        using namespace juce;
        const auto c = area.getCentre();
        const float R = jmin (area.getWidth(), area.getHeight()) * 0.5f;
        const float ang = start + pos * (end - start);
        const float arcR = R * 0.90f;
        const float arcW = jmax (3.f, R * 0.075f);

        Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.f, start, end, true);
        g.setColour (silverDim.withAlpha (0.30f));
        g.strokePath (track, PathStrokeType (arcW, PathStrokeType::curved, PathStrokeType::rounded));

        const float from = bipolar ? (start + end) * 0.5f : start;
        Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.f, jmin (from, ang), jmax (from, ang), true);
        g.setColour (glow.withAlpha (0.20f));
        g.strokePath (val, PathStrokeType (arcW * 2.4f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (blood);
        g.strokePath (val, PathStrokeType (arcW, PathStrokeType::curved, PathStrokeType::rounded));

        g.setColour (silverDim.withAlpha (0.55f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = start + (end - start) * (float) i / 10.f;
            g.drawLine (Line<float> (c.getPointOnCircumference (R * 0.745f, a),
                                     c.getPointOnCircumference (R * 0.80f, a)), 1.2f);
        }

        const float bodyR = R * 0.66f;
        g.setColour (Colours::black.withAlpha (0.55f));
        g.fillEllipse (c.x - bodyR - 2.f, c.y - bodyR + 3.f, bodyR * 2.f + 4.f, bodyR * 2.f + 4.f);

        ColourGradient rim (Colour (0xfff1f1f3), c.x - bodyR, c.y - bodyR,
                            Colour (0xffc9cacf), c.x + bodyR, c.y + bodyR, false);
        rim.addColour (0.25, Colour (0xff5b5b62));
        rim.addColour (0.60, Colour (0xff19191e));
        g.setGradientFill (rim);
        g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.f, bodyR * 2.f);

        g.setColour (Colours::black.withAlpha (0.35f));
        for (int i = 0; i < 36; ++i)
        {
            const float a = (float) i * MathConstants<float>::twoPi / 36.f;
            g.drawLine (Line<float> (c.getPointOnCircumference (bodyR * 0.93f, a),
                                     c.getPointOnCircumference (bodyR * 0.995f, a)), 1.f);
        }

        const float faceR = bodyR * 0.82f;
        ColourGradient face (Colour (0xff55555c), c.x - faceR * 0.3f, c.y - faceR * 0.5f,
                             Colour (0xff09090c), c.x + faceR, c.y + faceR, true);
        face.addColour (0.45, Colour (0xff242429));
        g.setGradientFill (face);
        g.fillEllipse (c.x - faceR, c.y - faceR, faceR * 2.f, faceR * 2.f);
        g.setColour (Colour (0xff08080a));
        g.drawEllipse (c.x - faceR, c.y - faceR, faceR * 2.f, faceR * 2.f, 1.5f);

        const auto p0 = c.getPointOnCircumference (bodyR * pointerStart, ang);
        const auto p1 = c.getPointOnCircumference (bodyR * 0.90f, ang);
        const float pw = jmax (2.5f, R * 0.07f);
        g.setColour (glow.withAlpha (0.35f));
        g.drawLine (Line<float> (p0, p1), pw * 2.2f);
        g.setColour (blood);
        g.drawLine (Line<float> (p0, p1), pw);
        g.setColour (silverBright);
        g.fillEllipse (p1.x - pw * 0.45f, p1.y - pw * 0.45f, pw * 0.9f, pw * 0.9f);
    }
}
