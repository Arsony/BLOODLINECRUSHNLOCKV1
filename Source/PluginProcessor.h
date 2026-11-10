#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class CrushProcessor : public juce::AudioProcessor
{
public:
    CrushProcessor();
    ~CrushProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Crush"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> outLevel { 0.0f };

private:
    juce::dsp::Oversampling<float> oversampling { 2, 1,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true };
    juce::dsp::DelayLine<float> dryDelay { 4096 };

    juce::dsp::IIR::Filter<float> catFilter[2], midFilter[2], dcFilter[2];
    juce::dsp::IIR::Filter<float> hpFilter[2], shelfFilter[2];
    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float> driveSm, midSm, mixSm, outSm;

    int lastCat = -1;
    float lastDrive = -1.0f, lastMid = -1.0f;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrushProcessor)
};
