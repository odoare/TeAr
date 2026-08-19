#pragma once

#include <JuceHeader.h>
#include <FxmeTools/components/AccentToggle.h>
#include <FxmeTools/components/PresetBarComponent.h>
#include <FxmeTools/components/PresetComponent.h>
#include <FxmeTools/components/SplashOverlay.h>
#include <FxmeTools/components/TextEntryFocusFixer.h>
#include <FxmeTools/components/TopBar.h>
#include "PluginProcessor.h"
#include "ArpLookAndFeel.h"
#include "ArpeggiatorComponent.h"
#include "KeyboardComponent.h"
#include "Theme.h"
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
    /** Opaque backdrop for the preset browser: fxme::PresetComponent paints
        only a translucent panel, so on its own the arpeggiator UI would show
        through it. */
    struct PresetOverlay : juce::Component
    {
        explicit PresetOverlay (fxme::PresetManager& manager) : browser (manager)
        {
            addAndMakeVisible (browser);
        }

        void paint (juce::Graphics& g) override { g.fillAll (Theme::background); }
        void resized() override { browser.setBounds (getLocalBounds().reduced (8)); }

        fxme::PresetComponent browser;
    };

    /** The glowing identity line under the header. A component of its own
        rather than a paint() call, because half the glow falls *inside* the
        top bar: only a sibling stacked above it can bleed over the header. */
    struct GlowLine : juce::Component
    {
        static constexpr int kGlow   = 16;
        static constexpr int kHeight = 2 * kGlow + 2;

        GlowLine() { setInterceptsMouseClicks (false, false); }

        void paint (juce::Graphics& g) override
        {
            const auto b = getLocalBounds().toFloat();
            Theme::paintTearLine (g, { 0.0f, b.getCentreY() - 1.0f, b.getWidth(), 2.0f },
                                  (float) kGlow);
        }
    };

    void rebuildArpUI();

    /** Greys out the controls the current mode ignores. Polled from the timer
        rather than driven from onChange, so that a parameter moved by host
        automation greys them too. */
    void updateControlEnablement();

    void selectArp (int index);
    void updateTabAppearance();
    void setPresetPanelVisible (bool shouldBeVisible);
    void styleMomentaryButton (fxme::AccentToggle& b, juce::Colour accent);

    TeArAudioProcessor& audioProcessor;

    ArpLookAndFeel        arpLAF;
    fxme::FxmeLookAndFeel fxmeLAF;

    // --- FX-Mechanics chrome ---
    fxme::TopBar topBar { "TeAr", "the text arpeggiator", JucePlugin_VersionString,
                          juce::ImageCache::getFromMemory (BinaryData::logo686_png,
                                                           BinaryData::logo686_pngSize) };
    fxme::PresetBarComponent presetBar { audioProcessor.getPresetManager() };
    juce::TextButton         presetsButton { juce::String::fromUTF8 ("\xe2\x96\xbe") };
    PresetOverlay            presetOverlay { audioProcessor.getPresetManager() };
    GlowLine                 glowLine;
    fxme::SplashOverlay      splash;

    // Persistent toolbar buttons
    fxme::AccentToggle addArpButton, removeArpButton, patternGenButton;

    // Per-arp UI (rebuilt on add/remove)
    std::vector<std::unique_ptr<fxme::AccentToggle>>   tabButtons;
    std::vector<std::unique_ptr<ArpeggiatorComponent>> arpComponents;
    int selectedArpIndex { 0 };

    // Note-on blink: the tab lights up whenever its arpeggiator fires, tracked
    // by polling the engine's monotonic note-on counter each frame.
    std::vector<juce::uint32> lastNoteOnCounts;
    std::vector<float>        blinkLevels;

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
    fxme::MidiTools::Scale currentDisplayScale { 0, fxme::MidiTools::Scale::Type::Major };
    juce::Array<int>       lastStepIndices;
    int                    lastPlayedArpNote { -1 };

    // Declared last: it walks the children already in place and keeps typed
    // input working in hosted plugin windows.
    fxme::TextEntryFocusFixer textEntryFixer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TeArAudioProcessorEditor)
};
