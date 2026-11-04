/*
  ==============================================================================

    PluginProcessor.h
    One Knob — asymmetric distortion plugin.
    APVTS with macroDrive (float 0-1) and satFlavor (choice: Tape/Console/Destroy).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "OneKnobDSP.h"
#include "BypassFader.h"
#include "LevelCompensation.h"

//==============================================================================
class OneKnobAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    OneKnobAudioProcessor();
    ~OneKnobAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    /** Public access to the APVTS for the editor. */
    juce::AudioProcessorValueTreeState apvts;

    /** Peak output level of the last block, for the editor's glow and meter. */
    std::atomic<float> outputPeak { 0.0f };

    /** Creates the parameter layout used by the APVTS. */
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    //==============================================================================
    OneKnobDSP dspEngine;

    /** Smoother for the macro drive parameter to avoid zipper noise. */
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDrive;

    /** The per-mode level trim, smoothed so a mode change never steps the output gain. */
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedTrimDb;

    /** 8x oversampling keeps the waveshapers' harmonics from folding back as aliasing.
        Linear-phase FIR with an integer latency, which is reported to the host. */
    juce::dsp::Oversampling<float> oversampling { 2, 3, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true };

    /** Crossfades to the latency-aligned dry signal when the host (or the UI) bypasses the plugin. */
    oneknob::BypassFader bypassFader;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OneKnobAudioProcessor)
};
