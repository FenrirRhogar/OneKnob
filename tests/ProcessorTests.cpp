// One Knob unit tests: run the OneKnobTests target; it exits non-zero on any failure.

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 512;

    std::unique_ptr<OneKnobAudioProcessor> makeProcessor (int numChannels = 2)
    {
        auto* plugin = dynamic_cast<OneKnobAudioProcessor*> (createPluginFilter());
        jassert (plugin != nullptr);
        const auto set = numChannels == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
        plugin->setBusesLayout ({ { set }, { set } });
        plugin->setPlayConfigDetails (numChannels, numChannels, sampleRate, blockSize);
        plugin->prepareToPlay (sampleRate, blockSize);
        return std::unique_ptr<OneKnobAudioProcessor> (plugin);
    }

    void setParam (OneKnobAudioProcessor& p, const juce::String& id, float plainValue)
    {
        auto* param = p.apvts.getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
    }

    /// Runs `input` through the plugin block by block and returns the output.
    juce::AudioBuffer<float> run (OneKnobAudioProcessor& p, const juce::AudioBuffer<float>& input)
    {
        juce::AudioBuffer<float> out (input);
        juce::MidiBuffer midi;
        for (int start = 0; start < out.getNumSamples(); start += blockSize)
        {
            const int n = juce::jmin (blockSize, out.getNumSamples() - start);
            juce::AudioBuffer<float> block (out.getArrayOfWritePointers(), out.getNumChannels(), start, n);
            p.processBlock (block, midi);
        }
        return out;
    }

    juce::AudioBuffer<float> sine (float freq, float amplitude, int numSamples, int numChannels = 2)
    {
        juce::AudioBuffer<float> b (numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ++ch)
            for (int n = 0; n < numSamples; ++n)
                b.setSample (ch, n, amplitude * std::sin (juce::MathConstants<float>::twoPi * freq * (float) n / (float) sampleRate));
        return b;
    }

    /// Pink-ish noise (white noise through a gentle low-pass) at roughly -18 dBFS RMS.
    juce::AudioBuffer<float> music (int numSamples)
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

    float rmsDb (const juce::AudioBuffer<float>& b, int start)
    {
        float sum = 0.0f;
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            sum += juce::square (b.getRMSLevel (ch, start, b.getNumSamples() - start));
        return juce::Decibels::gainToDecibels (std::sqrt (sum / (float) b.getNumChannels()));
    }

    /// Level of `freq` in `b` (channel 0, from `start`), in dB relative to a full-scale sine.
    float toneDb (const juce::AudioBuffer<float>& b, int start, float freq)
    {
        double re = 0.0, im = 0.0;
        const int n = b.getNumSamples() - start;
        for (int i = 0; i < n; ++i)
        {
            const double w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / (n - 1));   // Hann
            const double phase = juce::MathConstants<double>::twoPi * freq * i / sampleRate;
            re += w * b.getSample (0, start + i) * std::cos (phase);
            im += w * b.getSample (0, start + i) * std::sin (phase);
        }
        return juce::Decibels::gainToDecibels ((float) (4.0 * std::sqrt (re * re + im * im) / n), -200.0f);
    }

    const char* modeName (int mode) { return mode == 0 ? "Tape" : mode == 1 ? "Console" : "Destroy"; }
}

class LatencyTests : public juce::UnitTest
{
public:
    LatencyTests() : juce::UnitTest ("Latency and transparency", "One Knob") {}

    void runTest() override
    {
        beginTest ("The oversampling latency is reported to the host");
        auto plugin = makeProcessor();
        const int latency = plugin->getLatencySamples();
        logMessage ("latency: " + juce::String (latency) + " samples");
        expect (latency > 0 && latency < 256, "latency " + juce::String (latency));

        beginTest ("The latency is known before the plugin is prepared");
        std::unique_ptr<juce::AudioProcessor> fresh (createPluginFilter());
        expectEquals (fresh->getLatencySamples(), latency);

        beginTest ("At 0 % the output is the input, delayed by exactly the reported latency");
        const auto input = sine (1000.0f, 0.5f, 9600);
        const auto output = run (*plugin, input);
        float worst = 0.0f;
        for (int n = 2000; n < output.getNumSamples(); ++n)
            worst = juce::jmax (worst, std::abs (output.getSample (0, n) - input.getSample (0, n - latency)));
        expectLessThan (worst, 0.002f, "max error vs delayed input");

        beginTest ("Bypass passes the delayed input through untouched, even at full drive");
        auto bypassed = makeProcessor();
        setParam (*bypassed, "macroDrive", 1.0f);
        setParam (*bypassed, "bypass", 1.0f);
        const auto through = run (*bypassed, input);
        worst = 0.0f;
        for (int n = 4800; n < through.getNumSamples(); ++n)
            worst = juce::jmax (worst, std::abs (through.getSample (0, n) - input.getSample (0, n - latency)));
        expectLessThan (worst, 1.0e-6f, "max error vs delayed input");
    }
};

class SoundTests : public juce::UnitTest
{
public:
    SoundTests() : juce::UnitTest ("Sound", "One Knob") {}

    void runTest() override
    {
        const auto input = music (48000);
        const float inDb = rmsDb (input, 0);

        for (int mode = 0; mode < 3; ++mode)
        {
            beginTest (juce::String ("Level stays close to the input across the knob: ") + modeName (mode));
            for (float drive : { 0.01f, 0.1f, 0.25f, 0.5f, 0.75f, 1.0f })
            {
                auto plugin = makeProcessor();
                setParam (*plugin, "satFlavor", (float) mode);
                setParam (*plugin, "macroDrive", drive);
                const auto out = run (*plugin, input);
                const float change = rmsDb (out, 4800) - inDb;
                logMessage (juce::String (modeName (mode)) + " at " + juce::String (juce::roundToInt (drive * 100))
                            + " %: " + juce::String (change, 1) + " dB");
                expect (std::isfinite (change));
                expectWithinAbsoluteError (change, 0.0f, 1.0f, "level change in dB");
            }
        }

        beginTest ("Hot input never produces runaway peaks, NaNs or DC");
        for (int mode = 0; mode < 3; ++mode)
        {
            auto plugin = makeProcessor();
            setParam (*plugin, "satFlavor", (float) mode);
            setParam (*plugin, "macroDrive", 1.0f);
            const auto out = run (*plugin, sine (60.0f, 1.0f, 48000));
            float peak = 0.0f;
            double mean = 0.0;
            for (int n = 9600; n < out.getNumSamples(); ++n)
            {
                const float x = out.getSample (0, n);
                expect (std::isfinite (x));
                peak = juce::jmax (peak, std::abs (x));
                mean += x;
            }
            mean /= (out.getNumSamples() - 9600);
            expectLessThan (peak, 2.0f, juce::String (modeName (mode)) + " peak");
            expectLessThan ((float) std::abs (mean), 0.01f, juce::String (modeName (mode)) + " DC offset");
        }

        beginTest ("Oversampling keeps aliasing low (Dragonfire at full drive, 5 kHz tone)");
        {
            auto plugin = makeProcessor();
            setParam (*plugin, "satFlavor", 2.0f);
            setParam (*plugin, "macroDrive", 1.0f);
            const auto out = run (*plugin, sine (5000.0f, 0.5f, 48000));
            const float fundamental = toneDb (out, 9600, 5000.0f);
            // 9th and 8th harmonics (45 and 40 kHz) fold back to 3 and 8 kHz without oversampling.
            for (float alias : { 3000.0f, 8000.0f })
            {
                const float rel = toneDb (out, 9600, alias) - fundamental;
                logMessage ("alias at " + juce::String (alias / 1000.0f) + " kHz: " + juce::String (rel, 1) + " dB");
                expectLessThan (rel, -45.0f, "alias level relative to the fundamental");
            }
        }
    }
};

class SwitchingTests : public juce::UnitTest
{
public:
    SwitchingTests() : juce::UnitTest ("Switching and state", "One Knob") {}

    void runTest() override
    {
        beginTest ("Changing mode mid-signal does not click");
        {
            const auto input = sine (220.0f, 0.5f, 24000);
            auto steadyA = makeProcessor(), steadyB = makeProcessor(), switching = makeProcessor();
            for (auto* p : { steadyA.get(), steadyB.get(), switching.get() })
                setParam (*p, "macroDrive", 0.7f);
            setParam (*steadyB, "satFlavor", 2.0f);

            const float maxStep = juce::jmax (maxStepOf (run (*steadyA, input)), maxStepOf (run (*steadyB, input)));

            juce::AudioBuffer<float> out (input);
            juce::MidiBuffer midi;
            float worst = 0.0f, last = 0.0f;
            for (int start = 0; start < out.getNumSamples(); start += blockSize)
            {
                if (start == 12288)
                    setParam (*switching, "satFlavor", 2.0f);
                const int n = juce::jmin (blockSize, out.getNumSamples() - start);
                juce::AudioBuffer<float> block (out.getArrayOfWritePointers(), 2, start, n);
                switching->processBlock (block, midi);
                for (int i = 0; i < n; ++i)
                {
                    const float x = block.getSample (0, i);
                    if (start + i > 4800)
                        worst = juce::jmax (worst, std::abs (x - last));
                    last = x;
                }
            }
            expectLessThan (worst, maxStep * 1.25f, "largest sample step while switching vs steady modes");
        }

        beginTest ("Settings survive a save and reload");
        {
            auto plugin = makeProcessor();
            setParam (*plugin, "macroDrive", 0.42f);
            setParam (*plugin, "satFlavor", 1.0f);
            juce::MemoryBlock state;
            plugin->getStateInformation (state);

            auto reopened = makeProcessor();
            reopened->setStateInformation (state.getData(), (int) state.getSize());
            expectWithinAbsoluteError (reopened->apvts.getRawParameterValue ("macroDrive")->load(), 0.42f, 0.001f);
            expectEquals ((int) reopened->apvts.getRawParameterValue ("satFlavor")->load(), 1);
        }

        beginTest ("Mono tracks work");
        {
            auto plugin = makeProcessor (1);
            setParam (*plugin, "macroDrive", 0.6f);
            const auto out = run (*plugin, sine (440.0f, 0.5f, 9600, 1));
            expect (out.getMagnitude (0, 4800, 4800) > 0.05f);
        }
    }

private:
    static float maxStepOf (const juce::AudioBuffer<float>& b)
    {
        float worst = 0.0f;
        for (int n = 4801; n < b.getNumSamples(); ++n)
            worst = juce::jmax (worst, std::abs (b.getSample (0, n) - b.getSample (0, n - 1)));
        return worst;
    }
};

static LatencyTests latencyTests;
static SoundTests soundTests;
static SwitchingTests switchingTests;

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runTestsInCategory ("One Knob");

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult (i)->failures;
    return failures == 0 ? 0 : 1;
}
