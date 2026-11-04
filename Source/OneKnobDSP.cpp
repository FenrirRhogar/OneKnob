/*
  ==============================================================================

    OneKnobDSP.cpp
    Implementation of the per-sample asymmetric distortion engine.

  ==============================================================================
*/

#include "OneKnobDSP.h"
#include <cmath>

//==============================================================================
OneKnobDSP::OneKnobDSP() {}

//==============================================================================
void OneKnobDSP::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    juce::ignoreUnused (maxBlockSize);
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (maxBlockSize);
    spec.numChannels      = 1; // filters are instantiated per-channel

    const int chCount = juce::jmin (numChannels, 2);

    for (int ch = 0; ch < chCount; ++ch)
    {
        // Pre-waveshaper high-pass
        channelFilters[static_cast<size_t>(ch)].preHPF.prepare (spec);
        channelFilters[static_cast<size_t>(ch)].preHPF.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        channelFilters[static_cast<size_t>(ch)].preHPF.setCutoffFrequency (20.0f);
        channelFilters[static_cast<size_t>(ch)].preHPF.setResonance (0.707f);

        // Post-waveshaper low-pass
        channelFilters[static_cast<size_t>(ch)].postLPF.prepare (spec);
        channelFilters[static_cast<size_t>(ch)].postLPF.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        channelFilters[static_cast<size_t>(ch)].postLPF.setCutoffFrequency (22000.0f);
        channelFilters[static_cast<size_t>(ch)].postLPF.setResonance (0.707f);

        // Static 15 Hz DC blocker (high-pass)
        channelFilters[static_cast<size_t>(ch)].dcBlocker.prepare (spec);
        channelFilters[static_cast<size_t>(ch)].dcBlocker.setType (juce::dsp::StateVariableTPTFilterType::highpass);
        channelFilters[static_cast<size_t>(ch)].dcBlocker.setCutoffFrequency (15.0f);
        channelFilters[static_cast<size_t>(ch)].dcBlocker.setResonance (0.707f);
    }

    updateEnvelopeCoefficients (sampleRate);
    reset();
}

//==============================================================================
void OneKnobDSP::reset()
{
    for (auto& cf : channelFilters)
    {
        cf.preHPF.reset();
        cf.postLPF.reset();
        cf.dcBlocker.reset();
    }

    envelopeState.fill (0.0f);
}

//==============================================================================
// Parameter setters
//==============================================================================
void OneKnobDSP::setInputGain  (float g)  { inputGain  = g; }
void OneKnobDSP::setOutputGain (float g)  { outputGain = g; }

void OneKnobDSP::setPreHPFCutoff (float freqHz)
{
    for (auto& cf : channelFilters)
        cf.preHPF.setCutoffFrequency (freqHz);
}

void OneKnobDSP::setPostLPFCutoff (float freqHz)
{
    for (auto& cf : channelFilters)
        cf.postLPF.setCutoffFrequency (freqHz);
}

void OneKnobDSP::setBiasDepth     (float d)  { biasDepth = d; }
void OneKnobDSP::setSaturationMode (SaturationMode m) { satMode = m; }

//==============================================================================
// Per-sample processing
//==============================================================================
float OneKnobDSP::processSample (int channel, float inputSample)
{
    const auto ch = static_cast<size_t> (channel);

    // ---- 1. Input gain ---------------------------------------------------
    float x = inputSample * inputGain;

    // ---- 2. Pre-waveshaper HPF -------------------------------------------
    x = channelFilters[ch].preHPF.processSample (static_cast<int>(ch), x);

    // ---- 3. Envelope follower (simple peak detector) ---------------------
    const float absX = std::fabs (x);
    if (absX > envelopeState[ch])
        envelopeState[ch] = envAttackCoeff * envelopeState[ch] + (1.0f - envAttackCoeff) * absX;
    else
        envelopeState[ch] = envReleaseCoeff * envelopeState[ch] + (1.0f - envReleaseCoeff) * absX;

    // ---- 4. Dynamic DC Bias (Sag) ----------------------------------------
    x += envelopeState[ch] * biasDepth;

    // ---- 5. Waveshaper ---------------------------------------------------
    switch (satMode)
    {
        case SaturationMode::Tape:    x = waveshapeTape    (x); break;
        case SaturationMode::Console: x = waveshapeConsole (x); break;
        case SaturationMode::Destroy: x = waveshapeDestroy (x); break;
    }

    // ---- 6. Post-waveshaper LPF ------------------------------------------
    x = channelFilters[ch].postLPF.processSample (static_cast<int>(ch), x);

    // ---- 7. DC Blocker (static 15 Hz HPF) --------------------------------
    x = channelFilters[ch].dcBlocker.processSample (static_cast<int>(ch), x);

    // ---- 8. Output gain compensation -------------------------------------
    x *= outputGain;

    return x;
}

//==============================================================================
// Waveshaper kernels
//==============================================================================

float OneKnobDSP::waveshapeTape (float x)
{
    // y = 2x - x|x|   for |x| <= 1
    // y = sgn(x)       otherwise
    if (x > 1.0f)  return 1.0f;
    if (x < -1.0f) return -1.0f;
    return 2.0f * x - x * std::fabs (x);
}

float OneKnobDSP::waveshapeConsole (float x)
{
    // Chebyshev polynomial summation using T1..T4
    // T1(x) = x
    // T2(x) = 2x^2 - 1
    // T3(x) = 4x^3 - 3x
    // T4(x) = 8x^4 - 8x^2 + 1
    //
    // Mix coefficients chosen to inject even-order harmonics while
    // strictly limiting peak amplitude and aliasing content.
    // Weighted blend:  0.5*T1 + 0.35*T2 + 0.15*T3 + 0.05*T4

    const float xc = juce::jlimit (-1.0f, 1.0f, x);
    const float x2 = xc * xc;
    const float x3 = x2 * xc;
    const float x4 = x2 * x2;

    const float t1 = xc;
    const float t2 = 2.0f * x2 - 1.0f;
    const float t3 = 4.0f * x3 - 3.0f * xc;
    const float t4 = 8.0f * x4 - 8.0f * x2 + 1.0f;

    float y = 0.5f * t1 + 0.35f * t2 + 0.15f * t3 + 0.05f * t4;

    return juce::jlimit (-1.0f, 1.0f, y);
}

float OneKnobDSP::waveshapeDestroy (float x)
{
    // Asymmetric piecewise waveshaper:
    //   Positive phase → 3rd-order polynomial saturation
    //   Negative phase → hard rational folding curve

    const float xc = juce::jlimit (-3.0f, 3.0f, x); // wider input range for extreme distortion

    if (xc >= 0.0f)
    {
        // 3rd-order polynomial: y = x - (x^3 / 3)  (classic soft-clip)
        const float x2 = xc * xc;
        const float x3 = x2 * xc;
        float y = xc - x3 / 3.0f;
        return juce::jlimit (-1.0f, 1.0f, y);
    }
    else
    {
        // Rational folding curve: y = x / (1 + |x|)  — then invert for fold
        // Negative half gets harder clipping character
        const float absX = -xc; // positive version of the negative input
        float y = -absX / (1.0f + absX * absX);
        // Fold: reflect anything past -0.5 back up
        y = y * 2.0f;
        return juce::jlimit (-1.0f, 1.0f, y);
    }
}

//==============================================================================
// Envelope follower coefficient calculation
//==============================================================================
void OneKnobDSP::updateEnvelopeCoefficients (double sampleRate)
{
    // Attack ≈ 5 ms, Release ≈ 100 ms
    const float attackTimeMs  = 5.0f;
    const float releaseTimeMs = 100.0f;

    envAttackCoeff  = std::exp (-1.0f / (static_cast<float> (sampleRate) * attackTimeMs  * 0.001f));
    envReleaseCoeff = std::exp (-1.0f / (static_cast<float> (sampleRate) * releaseTimeMs * 0.001f));
}
