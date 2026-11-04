/*
  ==============================================================================

    PluginEditor.h
    One Knob — colorful, fun, premium GUI.
    Gradient knob arc that shifts hue with drive, animated glow,
    dynamic status text, and mode-colored combo box.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
    Custom LookAndFeel with colorful gradient knob and styled combo box.
*/
class OneKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OneKnobLookAndFeel();

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    /** Helper: returns a colour along the knob gradient for a given 0-1 position. */
    static juce::Colour getArcColour (float normalised);
    
    // PNG Assets
    juce::Image knobImg;
};

//==============================================================================
class OneKnobAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                               private juce::Timer
{
public:
    explicit OneKnobAudioProcessorEditor (OneKnobAudioProcessor&);
    ~OneKnobAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    /** Returns a fun status string based on the current drive value. */
    static juce::String getDriveStatusText (float drive);

    OneKnobAudioProcessor& audioProcessor;

    // Custom look and feel
    OneKnobLookAndFeel oneKnobLookAndFeel;

    // The big knob
    juce::Slider driveSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;

    // ---- Flavor switch ------------------------------------------------------
    juce::Slider modeSwitch;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modeAttachment;

    // Cached drive value for background painting
    float cachedDrive = 0.0f;

    // PNG Background
    juce::Image bgImg;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OneKnobAudioProcessorEditor)
};
