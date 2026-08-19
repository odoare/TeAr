#include "ArpeggiatorComponent.h"
#include "Theme.h"

static const juce::StringArray subdivisionChoices { "1/4", "1/4T", "1/8", "1/8T",
                                                     "1/16", "1/16T", "1/32", "1/32T",
                                                     "1/64", "1/64T" };

ArpeggiatorComponent::ArpeggiatorComponent()
{
    addAndMakeVisible (textEditor);
    textEditor.setLookAndFeel (&laf);
    textEditor.setMultiLine (true);
    textEditor.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(),
                                                       24.0f, juce::Font::plain)));
    textEditor.setColour (juce::TextEditor::backgroundColourId,     juce::Colours::transparentBlack);

    textEditor.onReturnKey = [this] {
        if (onPatternChanged) onPatternChanged (textEditor.getText());
        giveAwayKeyboardFocus();
    };
    textEditor.onFocusLost = [this] {
        if (onPatternChanged) onPatternChanged (textEditor.getText());
    };

    // On/off switch for this arpeggiator, immediately left of the step
    // duration selector.
    addAndMakeVisible (onOffButton);
    onOffButton.setButtonText ("ON");
    onOffButton.setTooltip ("Turn this arpeggiator on or off");
    onOffButton.onClick = [this] {
        if (onOnOffChanged) onOnOffChanged (onOffButton.getToggleState());
    };

    addAndMakeVisible (subdivisionBox);
    subdivisionBox.setLookAndFeel (&fxmeLAF);
    subdivisionBox.addItemList (subdivisionChoices, 1);
    subdivisionBox.setSelectedItemIndex (4, juce::dontSendNotification); // default 1/16
    subdivisionBox.onChange = [this] {
        if (onSubdivisionChanged) onSubdivisionChanged (subdivisionBox.getSelectedItemIndex());
    };

    addAndMakeVisible (midiChannelLabel);
    midiChannelLabel.setText ("Ch", juce::dontSendNotification);
    midiChannelLabel.setLookAndFeel (&fxmeLAF);
    midiChannelLabel.setFont (juce::Font (juce::FontOptions (15.0f)));
    midiChannelLabel.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (midiChannelBox);
    midiChannelBox.setLookAndFeel (&fxmeLAF);
    for (int ch = 1; ch <= 16; ++ch)
        midiChannelBox.addItem (juce::String (ch), ch);
    midiChannelBox.setSelectedId (1, juce::dontSendNotification);
    midiChannelBox.onChange = [this] {
        if (onMidiChannelChanged) onMidiChannelChanged (midiChannelBox.getSelectedId());
    };

    setArpColour (juce::Colours::lime);
}

ArpeggiatorComponent::~ArpeggiatorComponent()
{
    textEditor.setLookAndFeel (nullptr);
    subdivisionBox.setLookAndFeel (nullptr);
    midiChannelLabel.setLookAndFeel (nullptr);
    midiChannelBox.setLookAndFeel (nullptr);
}

void ArpeggiatorComponent::setArpColour (juce::Colour c)
{
    colour = c;
    textEditor.setColour (juce::TextEditor::textColourId,            colour);
    textEditor.setColour (juce::CaretComponent::caretColourId,       colour.brighter());
    textEditor.setColour (juce::TextEditor::highlightColourId,       colour);
    textEditor.setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::black);
    textEditor.setColour (juce::TextEditor::outlineColourId,         colour);

    // The body colour is left to FxmeLookAndFeel's dark default rather than
    // forced transparent: drawComboBox fills it as a rounded pill, which is the
    // same vocabulary as the buttons.
    subdivisionBox.setColour (juce::ComboBox::textColourId,    colour);
    subdivisionBox.setColour (juce::ComboBox::outlineColourId, colour);
    subdivisionBox.setColour (juce::ComboBox::arrowColourId,   colour);

    midiChannelLabel.setColour (juce::Label::textColourId, colour);

    midiChannelBox.setColour (juce::ComboBox::textColourId,    colour);
    midiChannelBox.setColour (juce::ComboBox::outlineColourId, colour);
    midiChannelBox.setColour (juce::ComboBox::arrowColourId,   colour);

    // A drop-down cannot see the box that opened it, so this is what makes this
    // arpeggiator's menus highlight in its own colour instead of neutral grey.
    fxmeLAF.setAccentColour (colour);

    Theme::accentToggle (onOffButton, colour);
}

void ArpeggiatorComponent::setOnState (bool shouldBeOn)
{
    onOffButton.setToggleState (shouldBeOn, juce::dontSendNotification);
}

bool ArpeggiatorComponent::getOnState() const
{
    return onOffButton.getToggleState();
}

void ArpeggiatorComponent::setText (const juce::String& text, bool sendNotification)
{
    juce::ignoreUnused (sendNotification);
    textEditor.setText (text, false);
}

juce::String ArpeggiatorComponent::getText() const
{
    return textEditor.getText();
}

void ArpeggiatorComponent::setSubdivisionIndex (int index)
{
    subdivisionBox.setSelectedItemIndex (index, juce::dontSendNotification);
}

int ArpeggiatorComponent::getSubdivisionIndex() const
{
    return subdivisionBox.getSelectedItemIndex();
}

void ArpeggiatorComponent::setMidiChannel (int channel)
{
    midiChannelBox.setSelectedId (channel, juce::dontSendNotification);
}

int ArpeggiatorComponent::getMidiChannel() const
{
    return midiChannelBox.getSelectedId();
}

void ArpeggiatorComponent::setHighlightedRegion (juce::Range<int> range)
{
    textEditor.setHighlightedRegion (range);
}

bool ArpeggiatorComponent::editorHasKeyboardFocus() const
{
    return textEditor.hasKeyboardFocus (true);
}

void ArpeggiatorComponent::resized()
{
    auto b = getLocalBounds().reduced (4);
    auto bottomRow = b.removeFromBottom (38);
    b.removeFromBottom (4);
    textEditor.setBounds (b);

    onOffButton.setBounds (bottomRow.removeFromLeft (Theme::onOffWidth).reduced (2, 4));
    bottomRow.removeFromLeft (4);

    int half = bottomRow.getWidth() / 2;
    subdivisionBox.setBounds (bottomRow.removeFromLeft (half).reduced (2, 0));
    bottomRow.removeFromLeft (4);
    midiChannelLabel.setBounds (bottomRow.removeFromLeft (32));
    midiChannelBox.setBounds (bottomRow.reduced (2, 0));
}
