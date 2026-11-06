/*
  ==============================================================================

    PluginEditor.h
    One Knob — parchment GUI.
    Inscribed knob over an ember arc whose glow grows with drive and breathes
    with the output level, dynamic status text, an ember level meter and an
    inked three-way mode selector.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UpdateCheck.h"

//==============================================================================
/**
    Custom LookAndFeel with the inscribed knob and its ember arc.
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

    /** Helper: returns a colour along the knob gradient for a given 0-1 position. */
    static juce::Colour getArcColour (float normalised);

    /** Output level (0-1, smoothed) that makes the glow breathe with the audio. */
    float glowLevel = 0.0f;

    // PNG Assets, and the knob resampled to its drawn size
    juce::Image knobImg, scaledKnob;
};

//==============================================================================
/**
    The three saturation flavours as inked words on the parchment. The selected one is
    drawn in full ink with an ember underline; click a word to switch.
*/
class ModeSelector : public juce::Component
{
public:
    ModeSelector (juce::RangedAudioParameter& parameter);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

private:
    int modeAt (juce::Point<int> position) const;
    juce::Rectangle<float> slotBounds (int index) const;

    juce::ParameterAttachment attachment;
    int selected = 0;
    int hovered = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModeSelector)
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

    juce::Rectangle<float> meterBounds() const;

    OneKnobAudioProcessor& audioProcessor;

    // Custom look and feel
    OneKnobLookAndFeel oneKnobLookAndFeel;

    // The big knob
    juce::Slider driveSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;

    // ---- Flavor selector ----------------------------------------------------
    ModeSelector modeSelector;

    // "FENRIKOS · V1.1.0" under the title, or "UPDATE ... AVAILABLE" in ember (UpdateCheck.h)
    std::unique_ptr<fenrikos::UpdateBadge> updateBadge;

    // Cached drive value for background painting
    float cachedDrive = 0.0f;
    float lastDrive = -1.0f;   // last value painted, per editor

    // Output level, smoothed for the glow and the meter
    float meterLevel = 0.0f;

    // PNG Background, and a copy resampled to the window at the current display scale
    juce::Image bgImg, scaledBg;
    float scaledBgScale = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OneKnobAudioProcessorEditor)
};
