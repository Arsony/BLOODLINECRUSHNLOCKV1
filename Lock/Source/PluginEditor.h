#pragma once

#include "PluginProcessor.h"
#include "Bloodline.h"

namespace lockcol
{
    // Bloodline palette (CrushAudio Skin Kit design tokens)
    const juce::Colour blood     { 0xffc51f38 };   // active energy
    const juce::Colour glow      { 0xffe12b49 };
    const juce::Colour panel     { 0xff141418 };
    const juce::Colour ink       { 0xff070708 };   // void
    const juce::Colour silver    { 0xffe6e7ea };
    const juce::Colour silverDim { 0xff777980 };
}

//==============================================================================
struct LockLNF : public juce::LookAndFeel_V4
{
    LockLNF()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider& s) override
    {
        bl::drawMetalKnob (g, juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f),
                           pos, start, end, s.getName() == "phase");
    }
};

//==============================================================================
class LockChoiceStrip : public juce::Component
{
public:
    LockChoiceStrip (juce::RangedAudioParameter& p, juce::StringArray names)
        : labels (std::move (names)),
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
            auto r = Rectangle<float> (b.getX() + (float) i * b.getWidth() / (float) n, b.getY(),
                                       b.getWidth() / (float) n, b.getHeight()).reduced (2.5f);
            const bool on = (i == index);

            if (on)
            {
                g.setColour (lockcol::blood.withAlpha (0.25f));
                g.fillRoundedRectangle (r.expanded (3.0f), 5.0f);
            }
            g.setColour (on ? lockcol::blood : lockcol::panel);
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (on ? lockcol::glow : lockcol::silverDim);
            g.drawRoundedRectangle (r, 4.0f, 1.2f);

            g.setColour (on ? Colours::white : lockcol::silver.withAlpha (0.8f));
            g.setFont (Font (FontOptions (14.0f, Font::bold)));
            g.drawText (labels[i], r, Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int n = labels.size();
        int i = (e.x * n) / juce::jmax (1, getWidth());
        i = juce::jlimit (0, n - 1, i);
        attach.setValueAsCompleteGesture ((float) i);
    }

private:
    juce::StringArray labels;
    int index = 0;
    juce::ParameterAttachment attach;
};

//==============================================================================
class LockEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LockEditor (LockProcessor&);
    ~LockEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;

    LockProcessor& proc;
    LockLNF lnf;

    juce::Slider glue, phase, tight, mix;
    LockChoiceStrip modeStrip, rateStrip;
    std::unique_ptr<SA> glueA, phaseA, tightA, mixA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LockEditor)
};
