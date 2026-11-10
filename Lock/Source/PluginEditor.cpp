#include "PluginEditor.h"
#include <cmath>

namespace
{
    float gainAtPhase (float p, float depth, float k, int mode, float mix)
    {
        const float e = std::exp (-k * p);
        const float g = (mode == 0) ? 1.0f - depth * (1.0f - e) : 1.0f - depth * e;
        return 1.0f - mix + mix * g;
    }
}

LockEditor::LockEditor (LockProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      modeStrip (*p.apvts.getParameter ("mode"), { "ON BEAT", "THROUGH" }),
      rateStrip (*p.apvts.getParameter ("rate"), { "1/1", "1/2", "1/4", "1/8", "1/16" })
{
    setSize (760, 480);

    auto initKnob = [this] (juce::Slider& s, const juce::String& name, const juce::String& suffix, int decimals)
    {
        s.setName (name);
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                               juce::MathConstants<float>::pi * 2.8f, true);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
        s.setTextValueSuffix (suffix);
        s.setNumDecimalPlacesToDisplay (decimals);
        s.setLookAndFeel (&lnf);
        addAndMakeVisible (s);
    };

    initKnob (glue,  "glue",  " %", 0);
    initKnob (phase, "phase", " %", 1);
    initKnob (tight, "tight", " %", 0);
    initKnob (mix,   "mix",   " %", 0);

    glueA  = std::make_unique<SA> (p.apvts, "glue",  glue);
    phaseA = std::make_unique<SA> (p.apvts, "phase", phase);
    tightA = std::make_unique<SA> (p.apvts, "tight", tight);
    mixA   = std::make_unique<SA> (p.apvts, "mix",   mix);

    addAndMakeVisible (modeStrip);
    addAndMakeVisible (rateStrip);

    startTimerHz (30);
}

LockEditor::~LockEditor()
{
    glue.setLookAndFeel (nullptr);
    phase.setLookAndFeel (nullptr);
    tight.setLookAndFeel (nullptr);
    mix.setLookAndFeel (nullptr);
}

void LockEditor::timerCallback()
{
    repaint();
}

void LockEditor::paint (juce::Graphics& g)
{
    using namespace juce;

    const Rectangle<float> d (60.0f, 74.0f, 640.0f, 180.0f);

    g.setGradientFill (ColourGradient (Colour (0xff2a0710), d.getCentreX(), d.getCentreY(),
                                       lockcol::ink, 0.0f, 0.0f, true));
    g.fillAll();

    // glow behind the display that pulses on the beat
    const float pulse = proc.pulse.load();
    g.setColour (lockcol::blood.withAlpha (0.04f + 0.14f * pulse));
    g.fillRoundedRectangle (d.expanded (10.0f), 16.0f);

    // cards behind each knob, veins, crest
    for (float x : { 50.0f, 220.0f, 390.0f, 560.0f })
        bl::drawPanel (g, Rectangle<float> (x, 262.0f, 150.0f, 164.0f));
    bl::drawVeins (g, getLocalBounds().toFloat(), 41, 0.09f + 0.10f * pulse, 7);
    bl::drawCrest (g, { 380.0f, 36.0f }, 44.0f, pulse);

    // title
    g.setColour (lockcol::silver);
    g.setFont (bl::serif (34.0f));
    g.drawText ("LOCK", 24, 14, 200, 40, Justification::centredLeft);
    g.setColour (lockcol::glow);
    g.setFont (Font (FontOptions (11.0f, Font::bold)));
    g.drawText ("THE RITUAL  /  CRUSHAUDIO", 26, 50, 300, 16, Justification::centredLeft);

    // display panel
    g.setColour (lockcol::panel);
    g.fillRoundedRectangle (d, 8.0f);
    g.setColour (lockcol::silverDim);
    g.drawRoundedRectangle (d, 8.0f, 1.2f);

    const float depth    = (float) proc.apvts.getRawParameterValue ("glue")->load()  * 0.01f;
    const float tightV   = (float) proc.apvts.getRawParameterValue ("tight")->load() * 0.01f;
    const float mixV     = (float) proc.apvts.getRawParameterValue ("mix")->load()   * 0.01f;
    const float phaseOff = (float) proc.apvts.getRawParameterValue ("phase")->load() * 0.01f;
    const int   mode     = (int)   proc.apvts.getRawParameterValue ("mode")->load();
    const float k        = 1.5f + tightV * 14.0f;

    g.saveState();
    g.reduceClipRegion (d.toNearestInt());

    bl::drawSigil (g, d.getCentre(), d.getHeight() * 0.50f,
                   proc.beatFrame.load() * MathConstants<float>::twoPi / 7.0f, pulse, 0.45f);

    // beat grid
    g.setColour (lockcol::silverDim.withAlpha (0.6f));
    for (int i = 0; i <= 2; ++i)
    {
        const float x = d.getX() + d.getWidth() * (float) i * 0.5f;
        g.drawLine (x, d.getY(), x, d.getBottom(), 1.2f);
    }

    // gain curve across two grid cells
    const float top = d.getY() + 26.0f, bot = d.getBottom() - 16.0f;
    Path curve;
    const int N = 640;
    for (int j = 0; j <= N; ++j)
    {
        const float x01 = 2.0f * (float) j / (float) N;
        const float xr = x01 - phaseOff;
        const float p = xr - std::floor (xr);
        const float gn = gainAtPhase (p, depth, k, mode, mixV);
        const float px = d.getX() + d.getWidth() * x01 * 0.5f;
        const float py = bot - gn * (bot - top);
        if (j == 0) curve.startNewSubPath (px, py); else curve.lineTo (px, py);
    }

    Path fill (curve);
    fill.lineTo (d.getRight(), bot);
    fill.lineTo (d.getX(), bot);
    fill.closeSubPath();
    g.setColour (lockcol::blood.withAlpha (0.14f));
    g.fillPath (fill);
    g.setColour (lockcol::blood.withAlpha (0.35f));
    g.strokePath (curve, PathStrokeType (7.0f));
    g.setColour (lockcol::blood);
    g.strokePath (curve, PathStrokeType (2.5f));

    // moving playhead
    const float bf = proc.beatFrame.load();
    for (int cIdx = 0; cIdx < 2; ++cIdx)
    {
        const float x = d.getX() + d.getWidth() * (bf + (float) cIdx) * 0.5f;
        g.setColour (lockcol::silver);
        g.drawLine (x, d.getY() + 4.0f, x, d.getBottom() - 4.0f, 1.6f);
    }

    g.restoreState();

    g.setFont (Font (FontOptions (11.0f, Font::bold)));
    g.setColour (lockcol::silver.withAlpha (0.75f));
    g.drawText (proc.synced.load() ? "LOCKED TO HOST TEMPO" : "FREE-RUN 120 BPM - PRESS PLAY",
                (int) d.getX() + 12, (int) d.getY() + 6, 300, 16, Justification::centredLeft);
    g.drawText ("ON THE BEAT", (int) d.getX() + 12, (int) d.getBottom() - 18, 120, 14, Justification::centredLeft);

    // knob labels
    g.setColour (lockcol::silver.withAlpha (0.85f));
    g.setFont (Font (FontOptions (13.0f, Font::bold)));
    g.drawText ("GLUE",  60,  270, 130, 16, Justification::centred);
    g.drawText ("PHASE", 230, 270, 130, 16, Justification::centred);
    g.drawText ("TIGHT", 400, 270, 130, 16, Justification::centred);
    g.drawText ("MIX",   570, 270, 130, 16, Justification::centred);

    bl::drawWindowFrame (g, getLocalBounds().toFloat());
}

void LockEditor::resized()
{
    glue.setBounds  (60,  290, 130, 130);
    phase.setBounds (230, 290, 130, 130);
    tight.setBounds (400, 290, 130, 130);
    mix.setBounds   (570, 290, 130, 130);

    modeStrip.setBounds (440, 18, 300, 36);
    rateStrip.setBounds (130, 432, 500, 34);
}
