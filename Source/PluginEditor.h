/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class TeArAudioProcessorEditor  : public juce::AudioProcessorEditor, // NOLINT
                                  public juce::Timer,
                                  public juce::ChangeListener
{
public:
    TeArAudioProcessorEditor (TeArAudioProcessor&);
    ~TeArAudioProcessorEditor() override;

    //==============================================================================
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void timerCallback() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    TeArAudioProcessor& audioProcessor;

    // Custom LookAndFeel for the arpeggiator editor to give it rounded corners.
    class ArpLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor& editor) override;
        void drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& editor) override;
    };

    ArpLookAndFeel arpLookAndFeel;

    // A custom TextEditor to handle Return and Shift+Return key presses.
    class ArpeggiatorTextEditor : public juce::TextEditor
    {
    public:
        // This lambda will be called when the user presses Return without Shift.
        std::function<void()> onReturnKey;

        bool keyPressed (const juce::KeyPress& key) override
        {
            // We only care about the Return key.
            if (key.getKeyCode() == juce::KeyPress::returnKey)
            {
                // If Shift is held, insert a newline.
                if (key.getModifiers().isShiftDown())
                {
                    insertTextAtCaret ("\n");
                }
                // Otherwise (no Shift), trigger our validation action.
                else if (onReturnKey)
                {
                    onReturnKey();
                }
                return true; // We've handled the Return key, so stop further processing.
            }
            // For all other keys, use the default TextEditor behavior.
            return juce::TextEditor::keyPressed(key);
        }
    };

    ArpeggiatorTextEditor arpeggiatorEditor;
    
    juce::Label chordMethodLabel;
    juce::ComboBox chordMethodBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> chordMethodAttachment;

    juce::Label subdivisionLabel;
    juce::ComboBox subdivisionBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subdivisionAttachment;

    juce::Label scaleRootLabel;
    juce::ComboBox scaleRootBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleRootAttachment;

    juce::Label scaleTypeLabel;
    juce::ComboBox scaleTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleTypeAttachment;

    juce::ToggleButton followMidiInButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> followMidiInAttachment;

    int lastStepIndex = -1;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TeArAudioProcessorEditor)
};
