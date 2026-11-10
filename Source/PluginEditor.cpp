#include "PluginEditor.h"

CrushEditor::CrushEditor (CrushProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      modeStrip (*p.apvts.getParameter ("mode"), { "SOFT", "HARD", "HIT" }, false),
      catStrip  (*p.apvts.getParameter ("cat"),  { "KICK", "SNARE", "808", "HI-HATS", "VOCALS" }, true)
{
    setSize (800, 480);

    auto initKnob = [this] (juce::Slider& s, const juce::String& name, const juce::String& suffix, bool big, int decimals)
    {
        s.setName (name);
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                               juce::MathConstants<float>::pi * 2.8f, true);
        s.setTextBoxStyle (big ? juce::Slider::NoTextBox : juce::Slider::TextBoxBelow, false, 70, 18);
        s.setTextValueSuffix (suffix);
        s.setNumDecimalPlacesToDisplay (decimals);
        s.setLookAndFeel (&lnf);
        addAndMakeVisible (s);
    };

    initKnob (drive, "drive", " %",  true,  0);
    initKnob (mid,   "mid",   " %",  false, 0);
    initKnob (mix,   "mix",   " %",  false, 0);
    initKnob (out,   "out",   " dB", false, 1);

    driveA = std::make_unique<SA> (p.apvts, "drive", drive);
    midA   = std::make_unique<SA> (p.apvts, "mid",   mid);
    mixA   = std::make_unique<SA> (p.apvts, "mix",   mix);
    outA   = std::make_unique<SA> (p.apvts, "out",   out);

    safety.setClickingTogglesState (true);
    safety.setLookAndFeel (&lnf);
    addAndMakeVisible (safety);
    safetyA = std::make_unique<BA> (p.apvts, "safety", safety);

    addAndMakeVisible (modeStrip);
    addAndMakeVisible (catStrip);

    juce::Random rng (7);
    for (int i = 0; i < 170; ++i)
        streaks.push_back ({ rng.nextFloat() * juce::MathConstants<float>::twoPi,
                             rng.nextFloat(), rng.nextFloat(),
                             0.6f + rng.nextFloat() * 1.6f,
                             0.15f + rng.nextFloat() * 0.6f });

    startTimerHz (30);
}

CrushEditor::~CrushEditor()
{
    drive.setLookAndFeel (nullptr);
    mid.setLookAndFeel (nullptr);
    mix.setLookAndFeel (nullptr);
    out.setLookAndFeel (nullptr);
    safety.setLookAndFeel (nullptr);
}

void CrushEditor::timerCallback()
{
    meter = juce::jmax (proc.outLevel.load(), meter * 0.85f);
    repaint();
}

void CrushEditor::paint (juce::Graphics& g)
{
    using namespace juce;

    const auto c = drive.getBounds().toFloat().getCentre();

    g.setGradientFill (ColourGradient (crushcol::deep, c.x, c.y, crushcol::ink, 0.0f, 0.0f, true));
    g.fillAll();

    // speed-line burst that grows with drive and output level
    const float d = (float) drive.getValue() * 0.01f;
    const float energy = 0.35f + d * 0.65f + juce::jmin (meter, 1.0f) * 0.5f;

    for (const auto& s : streaks)
    {
        const float r0 = 130.0f + s.r0 * 70.0f;
        const float r1 = r0 + (60.0f + s.len * 460.0f) * energy;
        const Point<float> dir (std::sin (s.angle), -std::cos (s.angle));

        g.setColour (crushcol::red.withAlpha (s.alpha * (0.3f + 0.7f * d)));
        g.drawLine (Line<float> (c + dir * r0, c + dir * r1), s.width);
    }

    // obsidian cards behind the knob columns
    bl::drawPanel (g, Rectangle<float> (14.0f, 76.0f, 110.0f, 352.0f));
    bl::drawPanel (g, Rectangle<float> (620.0f, 76.0f, 170.0f, 330.0f));
    bl::drawVeins (g, getLocalBounds().toFloat(), 23, 0.10f + 0.10f * juce::jmin (meter, 1.0f), 7);

    // fangs top and bottom, crest in the middle
    bl::drawFangs (g, 14.0f, (float) getWidth() - 14.0f, 0.0f, true,  26, 16.0f);
    bl::drawFangs (g, 14.0f, (float) getWidth() - 14.0f, (float) getHeight(), false, 26, 16.0f);
    bl::drawCrest (g, { 382.0f, 40.0f }, 44.0f, juce::jmin (meter, 1.0f));

    // title
    g.setColour (crushcol::silver);
    g.setFont (bl::serif (34.0f));
    g.drawText ("CRUSH", 24, 14, 200, 40, Justification::centredLeft);
    g.setColour (crushcol::glow);
    g.setFont (Font (FontOptions (11.0f, Font::bold)));
    g.drawText ("THE BITE  /  CRUSHAUDIO", 26, 50, 300, 16, Justification::centredLeft);

    // knob labels
    g.setColour (crushcol::silver.withAlpha (0.85f));
    g.setFont (Font (FontOptions (13.0f, Font::bold)));
    g.drawText ("MID", 24, 80,  90, 16, Justification::centred);
    g.drawText ("MIX", 24, 190, 90, 16, Justification::centred);
    g.drawText ("OUT", 24, 300, 90, 16, Justification::centred);

    g.setFont (Font (FontOptions (11.0f, Font::bold)));
    g.setColour (crushcol::red);
    g.drawText ("SOURCE", 630, 84, 150, 14, Justification::centred);

    // output meter
    auto bar = Rectangle<float> (170.0f, 446.0f, 300.0f, 6.0f);
    g.setColour (crushcol::silverDim.withAlpha (0.25f));
    g.fillRoundedRectangle (bar, 3.0f);
    g.setColour (crushcol::red);
    g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * juce::jlimit (0.0f, 1.0f, meter)), 3.0f);

    bl::drawWindowFrame (g, getLocalBounds().toFloat());
}

void CrushEditor::resized()
{
    drive.setBounds (150, 70, 340, 340);
    mid.setBounds (24, 96,  90, 100);
    mix.setBounds (24, 206, 90, 100);
    out.setBounds (24, 316, 90, 100);

    modeStrip.setBounds (430, 18, 240, 36);
    catStrip.setBounds  (630, 102, 150, 220);
    safety.setBounds    (630, 350, 150, 34);
}
