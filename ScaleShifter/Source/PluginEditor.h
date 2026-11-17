#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class EfluneLookAndFeel : public juce::LookAndFeel_V4
{
public:
    EfluneLookAndFeel();

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isMouseOver, bool isButtonDown) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;

    static const juce::Colour bg, ink, muted, line, card, accent, accent2, accent3, accent4;
};

/** 変換済み MIDI を Cubase のプロジェクトへドラッグ＆ドロップするためのタイル */
class DragOutTile : public juce::Component
{
public:
    std::function<juce::File()> prepareFile;   // ドラッグ開始時に変換を実行してファイルを返す
    juce::String caption;
    bool enabled = false;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { dragStarted = false; }
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }

private:
    bool dragStarted = false;
};

class ScaleShifterEditor : public juce::AudioProcessorEditor,
                           public juce::DragAndDropContainer,
                           public juce::FileDragAndDropTarget,
                           private juce::Timer
{
public:
    explicit ScaleShifterEditor (ScaleShifterProcessor&);
    ~ScaleShifterEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { fileHover = true;  repaint(); }
    void fileDragExit (const juce::StringArray&) override             { fileHover = false; repaint(); }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void timerCallback() override;

    void detectKey();
    void applyParallel();
    void applyRelative();
    void resetToSource();
    void loadMidiFile (const juce::File&);
    void openMidiFile();
    void saveConvertedFile();
    juce::File convertToTemp();
    juce::String convertedFileName() const;
    void setStatus (const juce::String&, bool isError = false);
    void paintMapping (juce::Graphics&, juce::Rectangle<int> area);
    static void setupCombo (juce::ComboBox&);

    ScaleShifterProcessor& proc;
    EfluneLookAndFeel lnf;

    juce::ComboBox srcRootBox, srcScaleBox, dstRootBox, dstScaleBox, octaveBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> srcRootAtt, srcScaleAtt, dstRootAtt, dstScaleAtt, octaveAtt;

    juce::ToggleButton bypassToggle, synthToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAtt, synthAtt;

    juce::Slider volumeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAtt;

    juce::TextButton detectButton, clearHistButton, parallelButton, relativeButton, resetButton;
    juce::OwnedArray<juce::TextButton> scaleButtons;

    juce::TextButton openButton, saveButton;
    DragOutTile dragTile;
    juce::File loadedFile;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::String status;
    bool statusIsError = false;
    bool fileHover = false;

    juce::Rectangle<int> srcCard, dstCard, buttonsCard, mappingArea, fileCard, footerArea;
    eflune::MappingSettings lastSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScaleShifterEditor)
};
