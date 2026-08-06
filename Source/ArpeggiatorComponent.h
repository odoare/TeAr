#pragma once
#include <JuceHeader.h>
#include <FxmeTools/components/AccentToggle.h>
#include "ArpLookAndFeel.h"

class ArpeggiatorComponent : public juce::Component
{
public:
    // TextEditor that grabs focus on click and handles Return / Shift+Return
    class ArpTextEditor : public juce::TextEditor
    {
    public:
        std::function<void()> onReturnKey;

        void mouseDown (const juce::MouseEvent& event) override
        {
            juce::TextEditor::mouseDown (event);
            // Post the focus grab asynchronously so X11 processes it after the
            // current event is fully dispatched rather than during it.
            juce::Component::SafePointer<juce::Component> safeThis (this);
            juce::MessageManager::callAsync ([safeThis] {
                if (safeThis != nullptr && safeThis->isShowing())
                    safeThis->grabKeyboardFocus();
            });
        }

        bool keyPressed (const juce::KeyPress& key) override
        {
            if (key.getKeyCode() == juce::KeyPress::returnKey)
            {
                if (key.getModifiers().isShiftDown())
                    insertTextAtCaret ("\n");
                else if (onReturnKey)
                    onReturnKey();
                return true;
            }
            return juce::TextEditor::keyPressed (key);
        }
    };

    ArpeggiatorComponent();
    ~ArpeggiatorComponent() override;

    void setArpColour (juce::Colour c);
    void setText (const juce::String& text, bool sendNotification);
    juce::String getText() const;
    void setSubdivisionIndex (int index);
    int  getSubdivisionIndex() const;
    void setMidiChannel (int channel);
    int  getMidiChannel() const;
    void setHighlightedRegion (juce::Range<int> range);
    bool editorHasKeyboardFocus() const;

    /** The arpeggiator's own on/off switch, at the bottom left next to the
        step duration selector. Kept in sync with the processor by the editor;
        setting it never fires onOnOffChanged. */
    void setOnState (bool shouldBeOn);
    bool getOnState() const;

    std::function<void (const juce::String&)> onPatternChanged;
    std::function<void (int)>                 onSubdivisionChanged;
    std::function<void (int)>                 onMidiChannelChanged;
    std::function<void (bool)>                onOnOffChanged;

    void resized() override;

private:
    ArpLookAndFeel     laf;
    ArpTextEditor      textEditor;
    fxme::AccentToggle onOffButton;
    juce::ComboBox subdivisionBox;
    juce::Label    midiChannelLabel;
    juce::ComboBox midiChannelBox;
    juce::Colour   colour { juce::Colours::white };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArpeggiatorComponent)
};
