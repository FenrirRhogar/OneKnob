/*
  ==============================================================================

    OneKnobDSP.h
    Per-sample asymmetric distortion engine with dynamic filtering,
    envelope-following DC bias (sag), and three waveshaping modes.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Waveshaping mode enumeration.
*/
enum class SaturationMode
{
    Tape = 0,
    Console,
    Destroy
};

//==============================================================================
/**
    Per-channel filter state: pre-waveshaper HPF, post-waveshaper LPF,
    and a static 15 Hz DC-blocker at the end of the chain.
*/
struct ChannelFilterState
{
    juce::dsp::StateVariableTPTFilter<float> preHPF;
    juce::dsp::StateVariableTPTFilter<float> postLPF;
    juce::dsp::StateVariableTPTFilter<float> dcBlocker;
};

//==============================================================================
/**
    The main DSP engine. All processing is per-sample so parameter modulation
    does not produce zipper noise.
*/
class OneKnobDSP
{
public:
    OneKnobDSP();
    ~OneKnobDSP() = default;

    /** Call once before processing begins (or when the sample rate changes). */
    void prepare (double sampleRate, int maxBlockSize, int numChannels);

    /** Reset all internal state (filters, envelope followers). */
    void reset();

    //==========================================================================
    // Real-time parameter setters — call these every sample (or every block
    // if you prefer; they are lightweight).
    //==========================================================================

    /** Set the input gain in linear amplitude. */
    void setInputGain (float linearGain);

    /** Set the output (compensation) gain in linear amplitude. */
    void setOutputGain (float linearGain);

    /** Set the pre-waveshaper HPF cutoff frequency in Hz. */
    void setPreHPFCutoff (float freqHz);

    /** Set the post-waveshaper LPF cutoff frequency in Hz. */
    void setPostLPFCutoff (float freqHz);

    /** Set the DC bias depth (0.0 = none, 0.4 = 40 % of envelope). */
    void setBiasDepth (float depth);

    /** Select the waveshaping algorithm. */
    void setSaturationMode (SaturationMode mode);

    //==========================================================================
    // Per-sample processing (called from the outer channel / sample loop).
    //==========================================================================

    /** Process a single sample for a given channel index. */
    float processSample (int channel, float inputSample);

private:
    //==========================================================================
    // Waveshaper kernels
    //==========================================================================
    static float waveshapeTape    (float x);
    static float waveshapeConsole (float x);
    static float waveshapeDestroy (float x);

    //==========================================================================
    // Envelope follower helpers
    //==========================================================================
    void updateEnvelopeCoefficients (double sampleRate);

    //==========================================================================
    // State
    //==========================================================================
    double currentSampleRate = 44100.0;

    // Per-channel filter banks (up to 2 channels for stereo)
    std::array<ChannelFilterState, 2> channelFilters;

    // Per-channel envelope follower state
    std::array<float, 2> envelopeState { 0.0f, 0.0f };
    float envAttackCoeff  = 0.0f;
    float envReleaseCoeff = 0.0f;

    // Current DSP parameters (set externally)
    float inputGain    = 1.0f;
    float outputGain   = 1.0f;
    float biasDepth    = 0.0f;
    SaturationMode satMode = SaturationMode::Tape;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OneKnobDSP)
};
