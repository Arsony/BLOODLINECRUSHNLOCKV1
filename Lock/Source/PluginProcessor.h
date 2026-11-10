#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class LockProcessor : public juce::AudioProcessor
{
public:
    LockProcessor();
    ~LockProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Lock"; }
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

    // read by the editor for the display
    std::atomic<float> beatFrame { 0.0f };  // 0..1 position inside the current grid cell
    std::atomic<float> pulse { 0.0f };      // 1 right on the pulse, falls to 0
    std::atomic<bool>  synced { false };    // true while following the host transport

private:
    double sr = 44100.0;
    double freeRunPpq = 0.0;
    float gainState = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LockProcessor)
};
