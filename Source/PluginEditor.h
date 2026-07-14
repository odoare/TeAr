#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ArpLookAndFeel.h"
#include "ArpeggiatorComponent.h"
#include "KeyboardComponent.h"
#include "FxmeLogo.h"
#include "popupWindow.h"

class TeArAudioProcessorEditor : public juce::AudioProcessorEditor,
                                 public juce::Timer,
                                 public juce::ChangeListener
{
public:
    TeArAudioProcessorEditor (TeArAudioProcessor&);
    ~TeArAudioProcessorEditor() override;

    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void timerCallback() override;
    void updateScaleDisplay();
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void rebuildArpUI();
    void selectArp (int index);
    void updateTabAppearance();

    TeArAudioProcessor& audioProcessor;

    ArpLookAndFeel        arpLAF;
    fxme::FxmeLookAndFeel fxmeLAF;

    // Persistent toolbar buttons
    juce::TextButton addArpButton    { "+" };
    juce::TextButton removeArpButton { "-" };
    juce::TextButton patternGenButton{ "?" };

    // Per-arp UI (rebuilt on add/remove)
    std::vector<std::unique_ptr<juce::TextButton>>        tabButtons;
    std::vector<std::unique_ptr<ArpeggiatorComponent>>    arpComponents;
    int selectedArpIndex { 0 };

    // Global controls
    juce::Label    chordMethodLabel;
    juce::ComboBox chordMethodBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> chordMethodAttachment;

    juce::Label    scaleRootLabel;
    juce::ComboBox scaleRootBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleRootAttachment;

    juce::Label    scaleTypeLabel;
    juce::ComboBox scaleTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleTypeAttachment;

    juce::ToggleButton followMidiInButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> followMidiInAttachment;

    KeyboardComponent      keyboardComponent;
    fxme::MidiTools::Scale       currentDisplayScale { 0, fxme::MidiTools::Scale::Type::Major };
    juce::Array<int>       lastStepIndices;
    int                    lastPlayedArpNote { -1 };

    FxmeLogo logo { "", false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TeArAudioProcessorEditor)
};
