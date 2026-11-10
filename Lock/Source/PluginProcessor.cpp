#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    // grid cell length in quarter-note beats: 1/1, 1/2, 1/4, 1/8, 1/16
    constexpr double kCellBeats[] = { 4.0, 2.0, 1.0, 0.5, 0.25 };
}

juce::AudioProcessorValueTreeState::ParameterLayout LockProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "rate", 1 }, "Rate",
                StringArray { "1/1", "1/2", "1/4", "1/8", "1/16" }, 2));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode",
                StringArray { "On Beat", "Through" }, 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "glue", 1 }, "Glue",
                NormalisableRange<float> (0.0f, 100.0f, 0.1f), 60.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "phase", 1 }, "Phase",
                NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "tight", 1 }, "Tight",
                NormalisableRange<float> (0.0f, 100.0f, 0.1f), 40.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    return layout;
}

LockProcessor::LockProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool LockProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void LockProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    gainState = 1.0f;
}

void LockProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int nCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    if (nCh == 0 || n == 0)
        return;

    const int    rate     = juce::jlimit (0, 4, (int) apvts.getRawParameterValue ("rate")->load());
    const int    mode     = (int) apvts.getRawParameterValue ("mode")->load();
    const double cellBeats = kCellBeats[rate];
    const double phaseOff = apvts.getRawParameterValue ("phase")->load() * 0.01;
    const float  depth    = apvts.getRawParameterValue ("glue")->load()  * 0.01f;
    const float  tight    = apvts.getRawParameterValue ("tight")->load() * 0.01f;
    const float  mix      = apvts.getRawParameterValue ("mix")->load()   * 0.01f;
    const float  k        = 1.5f + tight * 14.0f;   // how fast the pulse falls away

    // ---- tempo + position from the host (free-runs at 120 BPM when stopped)
    double bpm = 120.0;
    double ppq = freeRunPpq;
    bool playing = false;

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())
                if (*b > 20.0) bpm = *b;

            if (pos->getIsPlaying())
                if (auto p = pos->getPpqPosition())
                {
                    ppq = *p;
                    playing = true;
                }
        }
    }

    const double ppqPerSample = bpm / 60.0 / sr;
    const float  coef = std::exp (-1.0f / (0.0012f * (float) sr));   // ~1 ms click guard

    float* chans[2] = { buffer.getWritePointer (0), nCh > 1 ? buffer.getWritePointer (1) : nullptr };
    float lastPulse = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const double cellPos = ppq / cellBeats - phaseOff;
        const float  p = (float) (cellPos - std::floor (cellPos));   // 0 = on the pulse
        const float  e = std::exp (-k * p);

        // ON BEAT: full level on the pulse, glued/tucked in between
        // THROUGH: ducks on the pulse and phases back in after it
        const float target = (mode == 0) ? 1.0f - depth * (1.0f - e)
                                         : 1.0f - depth * e;

        gainState = target + coef * (gainState - target);
        const float g = 1.0f - mix + mix * gainState;

        chans[0][i] *= g;
        if (chans[1] != nullptr) chans[1][i] *= g;

        ppq += ppqPerSample;
        lastPulse = e;
    }

    freeRunPpq = ppq;

    const double bf = ppq / cellBeats;
    beatFrame.store ((float) (bf - std::floor (bf)));
    pulse.store (lastPulse);
    synced.store (playing);
}

juce::AudioProcessorEditor* LockProcessor::createEditor()
{
    return new LockEditor (*this);
}

void LockProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void LockProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LockProcessor();
}
