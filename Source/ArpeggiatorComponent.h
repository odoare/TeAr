#pragma once
#include <JuceHeader.h>
#include <FxmeTools/components/AccentToggle.h>
#include <FxmeTools/lookandfeels/FxmeLookAndFeel.h>
#include "ArpLookAndFeel.h"

class ArpeggiatorComponent : public juce::Component
{
public:
    // TextEditor handling Return / Shift+Return.
    //
    // Keyboard focus is not this class's problem: the editor owns a
    // fxme::TextEntryFocusFixer, which claims and re-asserts the OS focus for
    // every TextEditor beneath it. Return is handled here rather than there
    // because this field is multiline, where the fixer deliberately leaves
    // Return alone, while a pattern wants Return to commit and Shift+Return to
    // insert a newline.
    class ArpTextEditor : public juce::TextEditor
    {
    public:
        std::function<void()> onReturnKey;

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
    // Both declared before the widgets that point at them, so they outlive
    // them under reverse-declaration-order destruction.
    //
    // Two look-and-feels rather than one: FxmeLookAndFeel draws the combo
    // boxes and their menus in the house style, but overrides neither
    // fillTextEditorBackground nor drawTextEditorOutline, which are what give
    // the pattern field its rounded dark box. ArpLookAndFeel stays for that.
    //
    // One FxmeLookAndFeel per ArpeggiatorComponent, not one shared: a menu is
    // its own window and takes its highlight from the look-and-feel that opened
    // it, so this is what lets each arpeggiator's drop-downs carry that
    // arpeggiator's own colour (see setArpColour).
    ArpLookAndFeel        laf;
    fxme::FxmeLookAndFeel fxmeLAF;
    ArpTextEditor      textEditor;
    fxme::AccentToggle onOffButton;
    juce::ComboBox subdivisionBox;
    juce::Label    midiChannelLabel;
    juce::ComboBox midiChannelBox;
    juce::Colour   colour { juce::Colours::white };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArpeggiatorComponent)
};
