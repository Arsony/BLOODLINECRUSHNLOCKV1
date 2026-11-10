#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    // Per-source "voicing": where the pre-saturation push happens (frequency, Q, max gain in dB)
    struct Voicing { float freq, q, gainDb; };
    constexpr Voicing kVoicing[] = {
        {   85.0f, 0.9f, 5.0f },   // Kick
        { 1600.0f, 0.8f, 6.0f },   // Snare
        {   60.0f, 0.7f, 4.0f },   // 808
        { 7500.0f, 0.6f, 5.0f },   // Hi-Hats
        { 3000.0f, 0.7f, 4.0f }    // Vocals (presence)
    };

    // High-pass on the saturated path only (dry keeps full low end), per source
    constexpr float kHpFreq[] = { 20.0f, 120.0f, 20.0f, 300.0f, 90.0f };

    inline float shapeSample (float x, int mode, float g, float levels)
    {
        const float v = x * g;

        switch (mode)
        {
            case 0: // SOFT: smooth tanh saturation
                return std::tanh (v);

            case 1: // HARD: near-hard clip with a tiny soft knee
            {
                const float a = std::abs (v);
                const float k = 0.85f;
                if (a <= k) return v;
                const float over = (a - k) / (1.0f - k);
                return std::copysign (k + (1.0f - k) * std::tanh (over), v);
            }

            default: // HIT: asymmetric saturation + bit crush
            {
                const float bias = 0.25f;
                float y = std::tanh (v + bias) - std::tanh (bias);
                y = std::round (y * levels) / levels;
                return y;
            }
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout CrushProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 }, "Drive",
                NormalisableRange<float> (0.0f, 100.0f, 0.1f), 45.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mid", 1 }, "Mid",
                NormalisableRange<float> (0.0f, 100.0f, 0.1f), 40.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                NormalisableRange<float> (0.0f, 100.0f, 0.1f), 70.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output",
                NormalisableRange<float> (-18.0f, 6.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode",
                StringArray { "Soft", "Hard", "Hit" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "cat", 1 }, "Source",
                StringArray { "Kick", "Snare", "808", "Hi-Hats", "Vocals" }, 4));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "safety", 1 }, "Safety", true));

    return layout;
}

CrushProcessor::CrushProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool CrushProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void CrushProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    oversampling.initProcessing ((size_t) samplesPerBlock);
    oversampling.reset();
    const float latency = oversampling.getLatencyInSamples();
    setLatencySamples ((int) std::round (latency));

    dryDelay.prepare (spec);
    dryDelay.setDelay (latency);
    dryDelay.reset();

    dryBuffer.setSize (2, samplesPerBlock);

    auto dcCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 15.0f);
    for (int ch = 0; ch < 2; ++ch)
    {
        catFilter[ch].reset();
        hpFilter[ch].reset();
        shelfFilter[ch].reset();
        midFilter[ch].reset();
        dcFilter[ch].coefficients = dcCoeffs;
        dcFilter[ch].reset();
    }

    const auto rampSeconds = 0.04;
    driveSm.reset (sampleRate, rampSeconds);
    midSm.reset   (sampleRate, rampSeconds);
    mixSm.reset   (sampleRate, rampSeconds);
    outSm.reset   (sampleRate, rampSeconds);

    driveSm.setCurrentAndTargetValue (apvts.getRawParameterValue ("drive")->load() * 0.01f);
    midSm.setCurrentAndTargetValue   (apvts.getRawParameterValue ("mid")->load()   * 0.01f);
    mixSm.setCurrentAndTargetValue   (apvts.getRawParameterValue ("mix")->load()   * 0.01f);
    outSm.setCurrentAndTargetValue   (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("out")->load()));

    lastCat = -1; lastDrive = -1.0f; lastMid = -1.0f;
}

void CrushProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int nCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    if (nCh == 0 || n == 0)
        return;

    if (dryBuffer.getNumSamples() < n)
        dryBuffer.setSize (2, n, false, false, true);

    // ---- read parameters
    driveSm.setTargetValue (apvts.getRawParameterValue ("drive")->load() * 0.01f);
    midSm.setTargetValue   (apvts.getRawParameterValue ("mid")->load()   * 0.01f);
    mixSm.setTargetValue   (apvts.getRawParameterValue ("mix")->load()   * 0.01f);
    outSm.setTargetValue   (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("out")->load()));

    const int  mode   = (int) apvts.getRawParameterValue ("mode")->load();
    const int  cat    = juce::jlimit (0, 4, (int) apvts.getRawParameterValue ("cat")->load());
    const bool safety = apvts.getRawParameterValue ("safety")->load() > 0.5f;

    const float drive   = driveSm.skip (n);
    const float midAmt  = midSm.skip (n);
    const float mix     = mixSm.skip (n);
    const float outGain = outSm.skip (n);

    // ---- update pre-saturation EQ when something changed
    if (cat != lastCat || std::abs (drive - lastDrive) > 0.002f || std::abs (midAmt - lastMid) > 0.002f)
    {
        const auto& v = kVoicing[cat];
        auto catC = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, v.freq, v.q, juce::Decibels::decibelsToGain (v.gainDb * drive));
        auto midC = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, 1400.0f, 0.6f, juce::Decibels::decibelsToGain (8.0f * midAmt));

        auto hpC = juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, kHpFreq[cat]);

        // vocals: tame sibilance/harshness created by saturation (scales with drive)
        const float shelfDb = (cat == 4) ? -4.0f * drive : 0.0f;
        auto shelfC = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            currentSampleRate, 6500.0f, 0.7f, juce::Decibels::decibelsToGain (shelfDb));

        for (int ch = 0; ch < 2; ++ch)
        {
            catFilter[ch].coefficients = catC;
            midFilter[ch].coefficients = midC;
            hpFilter[ch].coefficients = hpC;
            shelfFilter[ch].coefficients = shelfC;
        }
        lastCat = cat; lastDrive = drive; lastMid = midAmt;
    }

    // ---- keep dry signal (latency compensated) and apply pre-EQ
    for (int ch = 0; ch < nCh; ++ch)
    {
        auto* d   = buffer.getWritePointer (ch);
        auto* dry = dryBuffer.getWritePointer (ch);

        for (int i = 0; i < n; ++i)
        {
            dryDelay.pushSample (ch, d[i]);
            dry[i] = dryDelay.popSample (ch);

            float x = hpFilter[ch].processSample (d[i]);
            x = catFilter[ch].processSample (x);
            x = midFilter[ch].processSample (x);
            d[i] = x;
        }
    }

    // ---- saturation at 2x oversampling
    const float g      = 1.0f + 9.0f * std::pow (drive, 1.5f);
    const float bits   = 14.0f - (cat == 4 ? 5.0f : 8.0f) * drive;
    const float levels = std::pow (2.0f, bits - 1.0f);

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) nCh, (size_t) n);
    auto up = oversampling.processSamplesUp (block);

    for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
    {
        auto* p = up.getChannelPointer (ch);
        for (size_t s = 0; s < up.getNumSamples(); ++s)
            p[s] = shapeSample (p[s], mode, g, levels);
    }

    oversampling.processSamplesDown (block);

    // ---- DC removal, loudness compensation, mix, output, safety
    const float comp = 1.0f / std::pow (g, 0.45f);
    float peak = 0.0f;

    for (int ch = 0; ch < nCh; ++ch)
    {
        auto* d   = buffer.getWritePointer (ch);
        auto* dry = dryBuffer.getReadPointer (ch);

        for (int i = 0; i < n; ++i)
        {
            float wet = shelfFilter[ch].processSample (dcFilter[ch].processSample (d[i])) * comp;
            float y   = (dry[i] * (1.0f - mix) + wet * mix) * outGain;

            if (safety)
                y = 0.95f * std::tanh (y / 0.95f);

            peak = juce::jmax (peak, std::abs (y));
            d[i] = y;
        }
    }

    outLevel.store (juce::jmax (peak, outLevel.load() * 0.9f));
}

juce::AudioProcessorEditor* CrushProcessor::createEditor()
{
    return new CrushEditor (*this);
}

void CrushProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void CrushProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CrushProcessor();
}
