// Measures how much each mode changes the level at every knob position (0 %, 1 %, ... 100 %)
// and prints a corrected levelTrimDb table for Source/LevelCompensation.h.
//   OneKnob_calibrate > table.txt
// The trim is a pure output gain after the waveshaper, so one run gives an exact correction
// for the test signal: noise at -18 dBFS RMS, the level a typical mix bus or track sits at.

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

#include <cstdio>

namespace
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 512;

    juce::AudioBuffer<float> testSignal (int numSamples)
    {
        juce::AudioBuffer<float> b (2, numSamples);
        juce::Random random (7);
        for (int ch = 0; ch < 2; ++ch)
        {
            float lp = 0.0f;
            for (int n = 0; n < numSamples; ++n)
            {
                lp += 0.15f * ((random.nextFloat() * 2.0f - 1.0f) - lp);
                b.setSample (ch, n, lp);
            }
        }
        b.applyGain (0.125f / b.getRMSLevel (0, 0, numSamples));
        return b;
    }

    float levelChangeDb (int mode, float drive, const juce::AudioBuffer<float>& input)
    {
        std::unique_ptr<OneKnobAudioProcessor> p (dynamic_cast<OneKnobAudioProcessor*> (createPluginFilter()));
        p->setPlayConfigDetails (2, 2, sampleRate, blockSize);
        for (auto [id, value] : { std::pair { "satFlavor", (float) mode }, std::pair { "macroDrive", drive } })
        {
            auto* param = p->apvts.getParameter (id);
            param->setValueNotifyingHost (param->convertTo0to1 (value));
        }
        p->prepareToPlay (sampleRate, blockSize);

        juce::AudioBuffer<float> out (input);
        juce::MidiBuffer midi;
        for (int start = 0; start < out.getNumSamples(); start += blockSize)
        {
            juce::AudioBuffer<float> block (out.getArrayOfWritePointers(), 2, start, juce::jmin (blockSize, out.getNumSamples() - start));
            p->processBlock (block, midi);
        }

        const int skip = 4800;   // filters and smoothing settle
        auto rms = [skip] (const juce::AudioBuffer<float>& b)
        {
            return std::sqrt (0.5f * (juce::square (b.getRMSLevel (0, skip, b.getNumSamples() - skip))
                                      + juce::square (b.getRMSLevel (1, skip, b.getNumSamples() - skip))));
        };
        return juce::Decibels::gainToDecibels (rms (out) / rms (input));
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    const auto input = testSignal (48000);

    std::printf ("inline constexpr std::array<std::array<float, levelTrimSteps>, 3> levelTrimDb { {\n");
    for (int mode = 0; mode < 3; ++mode)
    {
        std::printf ("    {");
        for (int step = 0; step < oneknob::levelTrimSteps; ++step)
        {
            const float drive = (float) step / (float) (oneknob::levelTrimSteps - 1);
            const float corrected = oneknob::levelTrimDb[(size_t) mode][(size_t) step] - levelChangeDb (mode, drive, input);
            std::printf ("%s%s%.2ff", step % 10 == 0 ? "\n      " : " ", step == 0 ? "" : "", corrected);
            if (step + 1 < oneknob::levelTrimSteps)
                std::printf (",");
        }
        std::printf (" },\n");
    }
    std::printf ("} };\n");
    return 0;
}
