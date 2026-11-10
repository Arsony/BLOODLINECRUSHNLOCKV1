#pragma once

#include "PluginProcessor.h"
#include "Bloodline.h"

namespace crushcol
{
    // Bloodline palette (CrushAudio Skin Kit design tokens)
    const juce::Colour red       { 0xffc51f38 };   // blood: active energy
    const juce::Colour glow      { 0xffe12b49 };   // blood glow
    const juce::Colour deep      { 0xff2a0710 };   // dark crimson behind the dial
    const juce::Colour ink       { 0xff070708 };   // void
    const juce::Colour panel     { 0xff141418 };   // buttons / panels
    const juce::Colour silver    { 0xffe6e7ea };
    const juce::Colour silverDim { 0xff777980 };
}

//==============================================================================
// Knob drawing: one big "percent" dial + small knobs
struct CrushLNF : public juce::LookAndFeel_V4
{
    CrushLNF()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::TextButton::buttonColourId, crushcol::panel);
        setColour (juce::TextButton::buttonOnColourId, crushcol::red);
        setColour (juce::TextButton::textColourOffId, crushcol::silver);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider& s) override
    {
        using namespace juce;
        const bool big = s.getName() == "drive";
        auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (big ? 10.0f : 6.0f);

        // big dial keeps a short pointer so the percent read-out sits clear in the middle
        bl::drawMetalKnob (g, b, pos, start, end, false, big ? 0.62f : 0.30f);

        if (big)
        {
            const auto c = b.getCentre();
            const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
            const String txt = String ((int) std::round (s.getValue())) + "%";
            g.setFont (Font (FontOptions (r * 0.36f, Font::bold)));
            auto tb = Rectangle<float> (r * 0.9f, r * 0.5f).withCentre (c.translated (0.0f, -r * 0.02f));
            g.setColour (crushcol::glow.withAlpha (0.35f));
            g.drawText (txt, tb.expanded (2.0f), Justification::centred);
            g.setColour (crushcol::silver);
            g.drawText (txt, tb, Justification::centred);

            g.setFont (Font (FontOptions (11.0f, Font::bold)));
            g.setColour (crushcol::silver.withAlpha (0.7f));
            g.drawText ("DRIVE", Rectangle<float> (80.0f, 14.0f).withCentre (c.translated (0.0f, r * 0.30f)),
                        Justification::centred);
        }
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& bt, const juce::Colour&,
                               bool, bool) override
    {
        auto r = bt.getLocalBounds().toFloat().reduced (1.5f);
        const bool on = bt.getToggleState();
        g.setColour (on ? crushcol::red : crushcol::panel);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (on ? crushcol::glow : crushcol::silverDim);
        g.drawRoundedRectangle (r, 4.0f, 1.2f);
    }
};

//==============================================================================
// Row/column of buttons bound to a choice parameter (SOFT/HARD/HIT, KICK/SNARE/...)
class ChoiceStrip : public juce::Component
{
public:
    ChoiceStrip (juce::RangedAudioParameter& p, juce::StringArray names, bool vertical)
        : labels (std::move (names)), vert (vertical),
          attach (p, [this] (float v) { index = juce::roundToInt (v); repaint(); }, nullptr)
    {
        attach.sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        const auto b = getLocalBounds().toFloat();
        const int n = labels.size();

        for (int i = 0; i < n; ++i)
        {
            Rectangle<float> r = vert
                ? Rectangle<float> (b.getX(), b.getY() + (float) i * b.getHeight() / (float) n, b.getWidth(), b.getHeight() / (float) n)
                : Rectangle<float> (b.getX() + (float) i * b.getWidth() / (float) n, b.getY(), b.getWidth() / (float) n, b.getHeight());
            r = r.reduced (2.5f);

            const bool on = (i == index);
            if (on)
            {
                g.setColour (crushcol::red.withAlpha (0.3f));
                g.fillRoundedRectangle (r.expanded (3.0f), 5.0f);
            }
            g.setColour (on ? crushcol::red : crushcol::panel);
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (on ? crushcol::glow : crushcol::silverDim);
            g.drawRoundedRectangle (r, 4.0f, 1.2f);

            g.setColour (on ? Colours::white : crushcol::silver.withAlpha (0.8f));
            g.setFont (Font (FontOptions (14.0f, Font::bold)));
            g.drawText (labels[i], r, Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int n = labels.size();
        int i = vert ? (e.y * n) / juce::jmax (1, getHeight())
                     : (e.x * n) / juce::jmax (1, getWidth());
        i = juce::jlimit (0, n - 1, i);
        attach.setValueAsCompleteGesture ((float) i);
    }

private:
    juce::StringArray labels;
    bool vert;
    int index = 0;
    juce::ParameterAttachment attach;
};

//==============================================================================
class CrushEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit CrushEditor (CrushProcessor&);
    ~CrushEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    CrushProcessor& proc;
    CrushLNF lnf;

    juce::Slider drive, mid, mix, out;
    juce::TextButton safety { "SAFETY" };
    ChoiceStrip modeStrip, catStrip;

    std::unique_ptr<SA> driveA, midA, mixA, outA;
    std::unique_ptr<BA> safetyA;

    struct Streak { float angle, r0, len, width, alpha; };
    std::vector<Streak> streaks;
    float meter = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrushEditor)
};
