/*
  ==============================================================================

    PluginEditor.cpp
    One Knob — parchment GUI built on the PNG artwork.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// Required for binary assets
#include "BinaryData.h"

namespace
{
    // Ink on parchment
    const juce::Colour ink      { 0xFF1A1410 };
    const juce::Colour fadedInk { 0xFF5A4632 };

    const char* const modeNames[] { "DWARVEN", "ELVEN", "DRAGONFIRE" };
}

//==============================================================================
// Colour palette helpers
//==============================================================================
juce::Colour OneKnobLookAndFeel::getArcColour (float normalised)
{
    // Fiery gradient: gold (0.15) to deep red (0.0)
    float hue = 0.15f - (normalised * 0.15f);
    if (hue < 0.0f) hue += 1.0f;
    return juce::Colour::fromHSV (hue, 0.9f, 0.95f, 1.0f);
}

//==============================================================================
// Custom LookAndFeel
//==============================================================================
OneKnobLookAndFeel::OneKnobLookAndFeel()
{
    // Load PNG
    knobImg = juce::ImageCache::getFromMemory (BinaryData::Knob_png, BinaryData::Knob_pngSize);
}

void OneKnobLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                           int x, int y, int width, int height,
                                           float sliderPos,
                                           float rotaryStartAngle,
                                           float rotaryEndAngle,
                                           juce::Slider& /*slider*/)
{
    const auto bounds   = juce::Rectangle<float> (static_cast<float>(x), static_cast<float>(y),
                                                  static_cast<float>(width), static_cast<float>(height));
    const auto maxRadius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto radius   = maxRadius - 40.0f; // Increased padding to prevent clipping the outer glow
    const auto centreX  = bounds.getCentreX();
    const auto centreY  = bounds.getCentreY();
    const auto diameter = radius * 2.0f;
    const auto angle    = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto arcRadius = radius + 10.0f;

    const juce::Colour currentColour = getArcColour (sliderPos);

    // How hot the knob burns: mostly the drive, plus a breath of the output level.
    const float heat = juce::jlimit (0.0f, 1.0f, sliderPos * (0.75f + 0.45f * glowLevel));

    // ---- Dynamic Outer Glow -------------------------------------------------
    if (sliderPos > 0.001f)
    {
        const float glowRadius = radius + 14.0f + 26.0f * heat;

        juce::ColourGradient glow (currentColour.withAlpha (0.10f + 0.35f * heat), centreX, centreY,
                                   currentColour.withAlpha (0.0f), centreX - glowRadius, centreY - glowRadius,
                                   true);
        g.setGradientFill (glow);
        g.fillEllipse (centreX - glowRadius, centreY - glowRadius,
                       glowRadius * 2.0f, glowRadius * 2.0f);
    }

    // ---- Track arc (inked groove in the parchment) -------------------------
    {
        juce::Path trackArc;
        trackArc.addCentredArc (centreX, centreY, arcRadius, arcRadius,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (ink.withAlpha (0.16f));
        g.strokePath (trackArc, juce::PathStrokeType (12.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (ink.withAlpha (0.35f));
        g.strokePath (trackArc, juce::PathStrokeType (1.0f));
    }

    // ---- Filled arc (ember gradient with a glow that grows with heat) -------
    if (sliderPos > 0.001f)
    {
        juce::Path filledArc;
        filledArc.addCentredArc (centreX, centreY, arcRadius, arcRadius,
                                 0.0f, rotaryStartAngle, angle, true);

        // Soft halo: the same arc stroked wider and fainter.
        for (auto [haloWidth, alpha] : { std::pair { 30.0f, 0.10f }, std::pair { 22.0f, 0.16f }, std::pair { 16.0f, 0.24f } })
        {
            g.setColour (currentColour.withAlpha (alpha * heat));
            g.strokePath (filledArc, juce::PathStrokeType (haloWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        juce::ColourGradient grad (getArcColour (0.0f), centreX - radius, centreY + radius,
                                   currentColour, centreX + radius, centreY - radius,
                                   false);
        g.setGradientFill (grad);
        g.strokePath (filledArc, juce::PathStrokeType (12.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Bright tip at the current position.
        const auto tip = juce::Point<float> (centreX, centreY).getPointOnCircumference (arcRadius, angle);
        g.setColour (juce::Colours::white.withAlpha (0.35f + 0.4f * heat));
        g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (tip));
    }

    // ---- Draw Custom Knob PNG ----------------------------------------------
    juce::Rectangle<float> imgBounds (centreX - radius, centreY - radius, diameter, diameter);

    if (knobImg.isValid())
    {
        // Resample the artwork once per size and display scale; rotating that copy is cheap.
        const int pixels = juce::roundToInt (diameter * g.getInternalContext().getPhysicalPixelScaleFactor());
        if (scaledKnob.getWidth() != pixels && pixels > 0)
            scaledKnob = knobImg.rescaled (pixels, pixels, juce::Graphics::highResamplingQuality);

        g.saveState();
        g.setOpacity (1.0f);   // drawImage uses the current colour's alpha
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.addTransform (juce::AffineTransform::rotation (angle, centreX, centreY));
        g.drawImage (scaledKnob, imgBounds);
        g.restoreState();
    }
    else
    {
        g.setColour (juce::Colours::black);
        g.fillEllipse (imgBounds);
    }
}

//==============================================================================
// Mode selector
//==============================================================================
ModeSelector::ModeSelector (juce::RangedAudioParameter& parameter)
    : attachment (parameter, [this] (float value) { selected = juce::roundToInt (value); repaint(); })
{
    attachment.sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTitle ("Saturation flavour");
}

int ModeSelector::modeAt (juce::Point<int> position) const
{
    if (! getLocalBounds().contains (position))
        return -1;
    for (int i = 0; i < 3; ++i)
        if (slotBounds (i).expanded (8.0f, 0.0f).contains (position.toFloat()))
            return i;
    return -1;
}

void ModeSelector::paint (juce::Graphics& g)
{
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));

    for (int i = 0; i < 3; ++i)
    {
        auto area = slotBounds (i);
        const bool isSelected = i == selected;

        g.setColour (isSelected ? ink : fadedInk.withAlpha (i == hovered ? 0.85f : 0.55f));
        g.drawText (modeNames[i], area.removeFromTop (16.0f), juce::Justification::centred, false);

        if (isSelected)
        {
            auto line = juce::Rectangle<float> (area.getWidth() - 4.0f, 3.0f).withCentre ({ area.getCentreX(), area.getY() + 3.0f });
            g.setColour (OneKnobLookAndFeel::getArcColour (0.55f));
            g.fillRoundedRectangle (line, 1.5f);
        }
    }
}

juce::Rectangle<float> ModeSelector::slotBounds (int index) const
{
    // Each word gets room for its own width, with equal gaps between them.
    const juce::Font font (juce::FontOptions (10.5f, juce::Font::bold));
    float widths[3], total = 0.0f;
    for (int i = 0; i < 3; ++i)
        total += widths[i] = juce::GlyphArrangement::getStringWidth (font, modeNames[i]);

    const float gap = juce::jmax (0.0f, ((float) getWidth() - total) / 3.0f);
    float x = gap * 0.5f;
    for (int i = 0; i < index; ++i)
        x += widths[i] + gap;
    return { x, 0.0f, widths[index], (float) getHeight() };
}

void ModeSelector::mouseDown (const juce::MouseEvent& e)
{
    if (const int mode = modeAt (e.getPosition()); mode >= 0 && mode != selected)
        attachment.setValueAsCompleteGesture ((float) mode);
}

void ModeSelector::mouseMove (const juce::MouseEvent& e)
{
    if (const int mode = modeAt (e.getPosition()); mode != hovered)
    {
        hovered = mode;
        repaint();
    }
}

void ModeSelector::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

//==============================================================================
// Editor
//==============================================================================
OneKnobAudioProcessorEditor::OneKnobAudioProcessorEditor (
    OneKnobAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      modeSelector (*p.apvts.getParameter ("satFlavor"))
{
    setLookAndFeel (&oneKnobLookAndFeel);

    // Load Background PNG
    bgImg = juce::ImageCache::getFromMemory (BinaryData::Background_png, BinaryData::Background_pngSize);

    // ---- Drive slider -------------------------------------------------------
    driveSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    driveSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    driveSlider.setPopupDisplayEnabled (false, false, this);
    driveSlider.setDoubleClickReturnValue (true, 0.0);
    driveSlider.setTitle ("Drive");
    addAndMakeVisible (driveSlider);

    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "macroDrive", driveSlider);

    driveSlider.onValueChange = [this] { cachedDrive = static_cast<float>(driveSlider.getValue()); };
    cachedDrive = static_cast<float> (driveSlider.getValue());   // show the saved value when the window opens

    // ---- Flavor selector ----------------------------------------------------
    addAndMakeVisible (modeSelector);

    updateBadge = std::make_unique<fenrikos::UpdateBadge> (
        juce::String (juce::CharPointer_UTF8 ("FENRIKOS  \xc2\xb7  V" JucePlugin_VersionString)),
        juce::FontOptions (8.5f, juce::Font::bold).withKerningFactor (0.3f),
        fadedInk.withAlpha (0.6f), juce::Colour (0xFFE86028), juce::Justification::centred);
    addAndMakeVisible (*updateBadge);

    // ---- Window size --------------------------------------------------------
    setSize (360, 540);

    startTimerHz(30);
}

OneKnobAudioProcessorEditor::~OneKnobAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void OneKnobAudioProcessorEditor::timerCallback()
{
    // Fast attack, slow release, like a VU needle.
    const float peak = juce::jlimit (0.0f, 1.0f, audioProcessor.outputPeak.load());
    const float previousLevel = meterLevel;
    meterLevel = peak > meterLevel ? peak : meterLevel * 0.85f;

    const bool levelMoved = std::abs (meterLevel - previousLevel) > 0.002f;
    oneKnobLookAndFeel.glowLevel = meterLevel;

    if (std::abs(cachedDrive - lastDrive) > 0.001f)
    {
        lastDrive = cachedDrive;
        repaint();
    }
    else if (levelMoved)
    {
        // Only the glow and the meter move with the audio.
        repaint (driveSlider.getBounds());
        repaint (meterBounds().toNearestIntEdges().expanded (2));
    }
}

juce::String OneKnobAudioProcessorEditor::getDriveStatusText (float drive)
{
    if (drive < 0.05f) return "Sleeping Village (Bypass)";
    if (drive < 0.20f) return "Hearth Warmth";
    if (drive < 0.35f) return "Tavern Drive";
    if (drive < 0.50f) return "Forge Saturation";
    if (drive < 0.65f) return "Wizard's Distortion";
    if (drive < 0.80f) return "Volcano Clipping";
    if (drive < 0.95f) return "Dragon's Breath";
    return "Rules Them All";
}

//==============================================================================
void OneKnobAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Draw PNG Background if available, resampled once per display scale
    if (bgImg.isValid())
    {
        const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
        if (scaledBg.isNull() || ! juce::approximatelyEqual (scaledBgScale, scale))
        {
            scaledBgScale = scale;
            scaledBg = bgImg.rescaled (juce::roundToInt ((float) getWidth() * scale),
                                       juce::roundToInt ((float) getHeight() * scale),
                                       juce::Graphics::highResamplingQuality);
        }
        g.drawImage (scaledBg, getLocalBounds().toFloat());
    }
    else
    {
        // Fallback background
        juce::ColourGradient bg (juce::Colour (0xFF16171B), 0.0f, 0.0f, juce::Colour (0xFF0C0D0F), 0.0f, static_cast<float>(getHeight()), false);
        g.setGradientFill (bg);
        g.fillAll();
    }

    // ---- Title --------------------------------------------------------------
    g.setColour (ink);
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold)); // LARGER TITLE
    g.drawText ("ONE KNOB", 0, 50, getWidth(), 30, juce::Justification::centred, true);

    g.setColour (fadedInk);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold).withKerningFactor (0.12f));
    g.drawText ("TO RULE THEM ALL", 0, 80, getWidth(), 20, juce::Justification::centred, true);


    // ---- Dynamic Status Text ------------------------------------------------
    juce::Rectangle<int> statusArea (0, 404, getWidth(), 30);

    // Status label (e.g. "Forge Saturation")
    g.setColour (ink);
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold)); // LARGER STATUS
    g.drawText (getDriveStatusText(cachedDrive), statusArea, juce::Justification::centred, true);

    // Drive percentage
    g.setColour (fadedInk);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold)); // LARGER PERCENTAGE
    g.drawText (juce::String(juce::roundToInt(cachedDrive * 100.0f)) + "%",
                statusArea.translated(0, 22), juce::Justification::centred, true);

    // ---- Ember level meter --------------------------------------------------
    {
        const auto meter = meterBounds();
        g.setColour (ink.withAlpha (0.18f));
        g.fillRoundedRectangle (meter, 2.0f);

        const float db = juce::Decibels::gainToDecibels (meterLevel, -48.0f);
        const float fill = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
        if (fill > 0.0f)
        {
            auto lit = meter.withWidth (meter.getWidth() * fill);
            g.setGradientFill (juce::ColourGradient (OneKnobLookAndFeel::getArcColour (0.0f), meter.getX(), 0.0f,
                                                     OneKnobLookAndFeel::getArcColour (1.0f), meter.getRight(), 0.0f, false));
            g.fillRoundedRectangle (lit, 2.0f);
        }
    }
}

juce::Rectangle<float> OneKnobAudioProcessorEditor::meterBounds() const
{
    return juce::Rectangle<float> (120.0f, 4.0f).withCentre ({ (float) getWidth() * 0.5f, 462.0f });
}

void OneKnobAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // ---- Drive knob (TRUE CENTER) -------------------------------------------
    const int knobSize = 300; // Increased size so the box doesn't cut the track
    driveSlider.setBounds (area.withSizeKeepingCentre (knobSize, knobSize));

    // ---- Flavor selector ----------------------------------------------------
    modeSelector.setBounds (84, 474, 192, 24);

    if (updateBadge != nullptr)
        updateBadge->setBounds (area.withSizeKeepingCentre (220, 12).withY (102));
}
