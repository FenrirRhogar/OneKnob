/*
  ==============================================================================

    PluginProcessor.cpp
    One Knob — asymmetric distortion plugin.
    Maps a single "macroDrive" knob to multiple DSP parameters with
    exponential / linear curves, and applies inverse output compensation.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
// Parameter layout
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
    OneKnobAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("macroDrive", 1),
        "Macro Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID ("satFlavor", 1),
        "Saturation Flavor",
        juce::StringArray { "Dwarven (Tape)", "Elven (Console)", "Dragonfire (Destroy)" },
        0));

    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID ("bypass", 1), "Bypass", false));

    return layout;
}

//==============================================================================
OneKnobAudioProcessor::OneKnobAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
    // The oversampling filters fix the latency up front, so hosts that ask before preparing see it too.
    oversampling.initProcessing (512);
    setLatencySamples (static_cast<int> (oversampling.getLatencyInSamples()));
}

OneKnobAudioProcessor::~OneKnobAudioProcessor()
{
}

//==============================================================================
const juce::String OneKnobAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OneKnobAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool OneKnobAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool OneKnobAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double OneKnobAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OneKnobAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int OneKnobAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OneKnobAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String OneKnobAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void OneKnobAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void OneKnobAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    oversampling.reset();
    oversampling.initProcessing (static_cast<size_t> (samplesPerBlock));
    setLatencySamples (static_cast<int> (oversampling.getLatencyInSamples()));

    const auto factor = static_cast<int> (oversampling.getOversamplingFactor());
    dspEngine.prepare (sampleRate * factor, samplesPerBlock * factor, getTotalNumInputChannels());

    bypassFader.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels(), getLatencySamples(),
                         apvts.getRawParameterValue ("bypass")->load() > 0.5f);

    smoothedDrive.reset (sampleRate, 0.02);  // 20 ms smoothing
    smoothedDrive.setCurrentAndTargetValue (
        apvts.getRawParameterValue ("macroDrive")->load());

    smoothedTrimDb.reset (sampleRate, 0.02);
    smoothedTrimDb.setCurrentAndTargetValue (
        oneknob::levelTrim (static_cast<int> (apvts.getRawParameterValue ("satFlavor")->load()),
                            smoothedDrive.getCurrentValue()));
}

void OneKnobAudioProcessor::releaseResources()
{
    dspEngine.reset();
    oversampling.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool OneKnobAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // Support mono or stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input must match output
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

//==============================================================================
void OneKnobAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // ---- Retrieve parameters ------------------------------------------------
    const float rawDrive = apvts.getRawParameterValue ("macroDrive")->load();
    smoothedDrive.setTargetValue (rawDrive);

    const int flavorIdx = static_cast<int> (apvts.getRawParameterValue ("satFlavor")->load());
    dspEngine.setSaturationMode (static_cast<SaturationMode> (flavorIdx));

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (totalNumInputChannels, 2);
    if (numChannels == 0)
        return;

    bypassFader.capture (buffer, numChannels);

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(),
                                        static_cast<size_t> (numChannels),
                                        static_cast<size_t> (numSamples));
    auto up = oversampling.processSamplesUp (block);
    const int factor = static_cast<int> (oversampling.getOversamplingFactor());

    // ---- Per-sample processing with smoothed macro drive --------------------
    // The drive mapping runs once per host sample; the waveshaping runs at the oversampled rate.
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float drive = smoothedDrive.getNextValue();

        // ── Map macro drive (0-1) to DSP parameters ──

        // 1. Input Gain: exponential 0 dB → +36 dB
        //    dB = drive * 36   →   linear = 10^(dB/20)
        const float inputGainDb = drive * 36.0f;
        const float inputGainLin = std::pow (10.0f, inputGainDb / 20.0f);
        dspEngine.setInputGain (inputGainLin);

        // 2. Pre-Filter HPF: 20 Hz → 150 Hz (exponential sweep)
        const float hpfFreq = 20.0f * std::pow (150.0f / 20.0f, drive);
        dspEngine.setPreHPFCutoff (hpfFreq);

        // 3. Post-Filter LPF: 22000 Hz → 6000 Hz (exponential sweep downward)
        const float lpfFreq = 22000.0f * std::pow (6000.0f / 22000.0f, drive);
        dspEngine.setPostLPFCutoff (lpfFreq);

        // 4. DC Bias Depth: 0 % → 40 % of envelope
        const float biasDepth = drive * 0.4f;
        dspEngine.setBiasDepth (biasDepth);

        // 5. Output Gain Compensation (inverse of input gain)
        //    Reduced from 0.85f to 0.25f because the waveshaper naturally crushes the amplitude.
        //    Pulling it back too much causes a severe volume drop. The measured per-mode trim
        //    on top keeps the output level with the input across the whole knob.
        smoothedTrimDb.setTargetValue (oneknob::levelTrim (flavorIdx, drive));
        const float outputGainDb  = -(inputGainDb * 0.25f) + smoothedTrimDb.getNextValue();
        const float outputGainLin = std::pow (10.0f, outputGainDb / 20.0f);
        dspEngine.setOutputGain (outputGainLin);

        // Fade in the DSP effect over the first 2% of the knob to ensure 0% is true bypass.
        // Mixing before downsampling keeps the dry and processed signals time-aligned.
        const float mix = juce::jlimit (0.0f, 1.0f, drive * 50.0f);

        // ── Process each channel ──
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* channelData = up.getChannelPointer (static_cast<size_t> (ch));
            for (int k = sample * factor; k < (sample + 1) * factor; ++k)
            {
                const float inputSample = channelData[k];
                const float processedSample = dspEngine.processSample (ch, inputSample);
                channelData[k] = inputSample + (processedSample - inputSample) * mix;
            }
        }
    }

    oversampling.processSamplesDown (block);
    bypassFader.apply (buffer, numChannels, apvts.getRawParameterValue ("bypass")->load() > 0.5f);
    outputPeak.store (buffer.getMagnitude (0, numSamples));
}

//==============================================================================
bool OneKnobAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OneKnobAudioProcessor::createEditor()
{
    return new OneKnobAudioProcessorEditor (*this);
}

//==============================================================================
void OneKnobAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void OneKnobAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OneKnobAudioProcessor();
}
