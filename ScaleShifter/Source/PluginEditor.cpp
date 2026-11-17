#include "PluginEditor.h"
#include "MidiFileConverter.h"

//==============================================================================
const juce::Colour EfluneLookAndFeel::bg      { 0xfff5f4f0 };
const juce::Colour EfluneLookAndFeel::ink     { 0xff111111 };
const juce::Colour EfluneLookAndFeel::muted   { 0x8f111111 };
const juce::Colour EfluneLookAndFeel::line    { 0x24111111 };
const juce::Colour EfluneLookAndFeel::card    { 0xb0ffffff };
const juce::Colour EfluneLookAndFeel::accent  { 0xffc9c4ff };
const juce::Colour EfluneLookAndFeel::accent2 { 0xffffd3ea };
const juce::Colour EfluneLookAndFeel::accent3 { 0xffbfe6ff };
const juce::Colour EfluneLookAndFeel::accent4 { 0xffd6fbe9 };

namespace
{
    juce::Font uiFont (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    }

    juce::String asciiNoteName (int pc)
    {
        return ja (eflune::noteName (pc)).replace (ja (u8"♭"), "b");
    }

    bool isMajorLike (int scaleIndex)
    {
        const auto& steps = eflune::scales()[(size_t) scaleIndex].steps;
        return std::find (steps.begin(), steps.end(), 4) != steps.end();   // 長3度を含む
    }

    const char* romanNumerals[] = { "I", "II", "III", "IV", "V", "VI", "VII" };
}

EfluneLookAndFeel::EfluneLookAndFeel()
{
   #if JUCE_WINDOWS
    setDefaultSansSerifTypefaceName ("Yu Gothic UI");
   #elif JUCE_MAC
    setDefaultSansSerifTypefaceName ("Hiragino Sans");
   #else
    setDefaultSansSerifTypefaceName ("Noto Sans CJK JP");
   #endif

    setColour (juce::ResizableWindow::backgroundColourId, bg);
    setColour (juce::TextButton::buttonColourId, juce::Colours::white.withAlpha (0.8f));
    setColour (juce::TextButton::buttonOnColourId, ink);
    setColour (juce::TextButton::textColourOffId, ink);
    setColour (juce::TextButton::textColourOnId, bg);
    setColour (juce::ComboBox::backgroundColourId, juce::Colours::white.withAlpha (0.85f));
    setColour (juce::ComboBox::textColourId, ink);
    setColour (juce::ComboBox::outlineColourId, line);
    setColour (juce::ComboBox::arrowColourId, muted);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xfffbfaf7));
    setColour (juce::PopupMenu::textColourId, ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour (juce::PopupMenu::highlightedTextColourId, ink);
    setColour (juce::ToggleButton::textColourId, ink);
    setColour (juce::ToggleButton::tickColourId, ink);
    setColour (juce::ToggleButton::tickDisabledColourId, muted);
    setColour (juce::Slider::thumbColourId, ink);
    setColour (juce::Slider::trackColourId, accent);
    setColour (juce::Slider::backgroundColourId, line);
    setColour (juce::Label::textColourId, ink);
}

void EfluneLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& backgroundColour,
                                              bool isMouseOver, bool isButtonDown)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    auto c = b.getToggleState() ? findColour (juce::TextButton::buttonOnColourId) : backgroundColour;
    if (isButtonDown)      c = c.darker (0.12f);
    else if (isMouseOver)  c = b.getToggleState() ? c.brighter (0.25f) : c.interpolatedWith (accent, 0.35f);

    g.setColour (c);
    g.fillRoundedRectangle (r, 9.0f);
    g.setColour (b.getToggleState() ? ink : line);
    g.drawRoundedRectangle (r, 9.0f, 1.0f);
}

juce::Font EfluneLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return uiFont (juce::jmin (16.0f, (float) buttonHeight * 0.52f));
}

juce::Font EfluneLookAndFeel::getComboBoxFont (juce::ComboBox&) { return uiFont (15.0f); }
juce::Font EfluneLookAndFeel::getPopupMenuFont()                 { return uiFont (15.0f); }

void EfluneLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (box.hasKeyboardFocus (true) ? ink.withAlpha (0.5f) : line);
    g.drawRoundedRectangle (r, 8.0f, 1.0f);

    juce::Path arrow;
    const float cx = (float) width - 14.0f, cy = (float) height * 0.5f;
    arrow.startNewSubPath (cx - 4.0f, cy - 2.0f);
    arrow.lineTo (cx, cy + 2.5f);
    arrow.lineTo (cx + 4.0f, cy - 2.0f);
    g.setColour (muted);
    g.strokePath (arrow, juce::PathStrokeType (1.5f));
}

//==============================================================================
void DragOutTile::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const bool hot = enabled && isMouseOver();

    g.setColour (enabled ? (hot ? EfluneLookAndFeel::accent : EfluneLookAndFeel::accent4) : juce::Colours::white.withAlpha (0.5f));
    g.fillRoundedRectangle (r, 10.0f);

    const float dash[] = { 5.0f, 4.0f };
    juce::Path border;
    border.addRoundedRectangle (r, 10.0f);
    juce::Path dashed;
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, border, dash, 2);
    g.setColour (enabled ? EfluneLookAndFeel::ink.withAlpha (0.5f) : EfluneLookAndFeel::line);
    g.fillPath (dashed);

    g.setColour (enabled ? EfluneLookAndFeel::ink : EfluneLookAndFeel::muted);
    g.setFont (uiFont (14.0f, enabled));
    g.drawFittedText (caption, getLocalBounds().reduced (10, 4), juce::Justification::centred, 2);
}

void DragOutTile::mouseDrag (const juce::MouseEvent& e)
{
    if (! enabled || dragStarted || e.getDistanceFromDragStart() < 5 || ! prepareFile)
        return;

    dragStarted = true;
    const auto file = prepareFile();
    if (file.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
}

//==============================================================================
void ScaleShifterEditor::setupCombo (juce::ComboBox& box)
{
    box.setJustificationType (juce::Justification::centredLeft);
}

ScaleShifterEditor::ScaleShifterEditor (ScaleShifterProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    using APVTS = juce::AudioProcessorValueTreeState;

    // --- キー / スケール選択 ---
    for (auto* box : { &srcRootBox, &dstRootBox })
    {
        for (int i = 0; i < 12; ++i)
            box->addItem (ja (eflune::noteName (i)), i + 1);
        setupCombo (*box);
        addAndMakeVisible (*box);
    }
    for (auto* box : { &srcScaleBox, &dstScaleBox })
    {
        for (int i = 0; i < eflune::numScales(); ++i)
            box->addItem (ja (eflune::scales()[(size_t) i].nameJa), i + 1);
        setupCombo (*box);
        addAndMakeVisible (*box);
    }
    octaveBox.addItemList ({ ja (u8"オクターブ -2"), ja (u8"オクターブ -1"), ja (u8"オクターブ ±0"),
                             ja (u8"オクターブ +1"), ja (u8"オクターブ +2") }, 1);
    setupCombo (octaveBox);
    addAndMakeVisible (octaveBox);

    srcRootAtt  = std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, ParamIDs::srcRoot,  srcRootBox);
    srcScaleAtt = std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, ParamIDs::srcScale, srcScaleBox);
    dstRootAtt  = std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, ParamIDs::dstRoot,  dstRootBox);
    dstScaleAtt = std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, ParamIDs::dstScale, dstScaleBox);
    octaveAtt   = std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, ParamIDs::octave,   octaveBox);

    detectButton.setButtonText (ja (u8"キー自動検出"));
    detectButton.setTooltip (ja (u8"演奏されたノート、または読み込んだMIDIファイルから元のキーを推定します"));
    detectButton.onClick = [this] { detectKey(); };
    addAndMakeVisible (detectButton);

    clearHistButton.setButtonText (ja (u8"リセット"));
    clearHistButton.onClick = [this]
    {
        proc.clearLiveHistogram();
        setStatus (ja (u8"検出用のノート履歴をクリアしました"));
    };
    addAndMakeVisible (clearHistButton);

    // --- ワンクリック・ボタン ---
    parallelButton.setButtonText (ja (u8"メジャー ⇄ マイナー"));
    parallelButton.onClick = [this] { applyParallel(); };
    relativeButton.setButtonText (ja (u8"平行調へ"));
    relativeButton.onClick = [this] { applyRelative(); };
    resetButton.setButtonText (ja (u8"元に戻す"));
    resetButton.onClick = [this] { resetToSource(); };
    for (auto* b : { &parallelButton, &relativeButton, &resetButton })
    {
        b->setColour (juce::TextButton::buttonColourId, EfluneLookAndFeel::accent2.withAlpha (0.9f));
        addAndMakeVisible (*b);
    }

    for (int i = 0; i < eflune::numScales(); ++i)
    {
        auto* b = scaleButtons.add (new juce::TextButton (ja (eflune::scales()[(size_t) i].nameJa)));
        b->onClick = [this, i] { proc.setParameterIndex (ParamIDs::dstScale, i); };
        addAndMakeVisible (b);
    }

    // --- MIDI ファイル ---
    openButton.setButtonText (ja (u8"MIDIファイルを開く…"));
    openButton.onClick = [this] { openMidiFile(); };
    addAndMakeVisible (openButton);

    saveButton.setButtonText (ja (u8"変換して保存…"));
    saveButton.onClick = [this] { saveConvertedFile(); };
    saveButton.setEnabled (false);
    addAndMakeVisible (saveButton);

    dragTile.caption = ja (u8"MIDIファイルをここにドロップ");
    dragTile.prepareFile = [this] { return convertToTemp(); };
    addAndMakeVisible (dragTile);

    // --- 内蔵音源 / バイパス ---
    bypassToggle.setButtonText (ja (u8"変換オフ（比較用）"));
    synthToggle.setButtonText (ja (u8"内蔵音源で試聴"));
    addAndMakeVisible (bypassToggle);
    addAndMakeVisible (synthToggle);
    bypassAtt = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, ParamIDs::bypass, bypassToggle);
    synthAtt  = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, ParamIDs::synth,  synthToggle);

    volumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (volumeSlider);
    volumeAtt = std::make_unique<APVTS::SliderAttachment> (proc.apvts, ParamIDs::volume, volumeSlider);

    setStatus (ja (u8"変換先のスケールボタンを押すだけで、メロディーもコードも即座に変わります"));

    setSize (880, 760);
    startTimerHz (20);
}

ScaleShifterEditor::~ScaleShifterEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void ScaleShifterEditor::timerCallback()
{
    const int dst = proc.getParameterIndex (ParamIDs::dstScale);
    for (int i = 0; i < scaleButtons.size(); ++i)
        scaleButtons[i]->setToggleState (i == dst, juce::dontSendNotification);

    const auto s = proc.getSettings();
    if (s.src.root != lastSettings.src.root || s.src.scale != lastSettings.src.scale
        || s.dst.root != lastSettings.dst.root || s.dst.scale != lastSettings.dst.scale
        || s.octaveShift != lastSettings.octaveShift)
    {
        lastSettings = s;
        if (loadedFile.existsAsFile())
            dragTile.caption = ja (u8"↗ ここをドラッグしてCubaseへ\n") + convertedFileName();
        repaint();
    }
}

void ScaleShifterEditor::setStatus (const juce::String& text, bool isError)
{
    status = text;
    statusIsError = isError;
    repaint (footerArea);
}

void ScaleShifterEditor::detectKey()
{
    std::array<double, 12> h {};
    juce::String source;
    if (loadedFile.existsAsFile())
    {
        h = MidiFileConverter::histogram (loadedFile);
        source = loadedFile.getFileName();
    }
    else
    {
        h = proc.getLiveHistogram();
        source = ja (u8"演奏されたノート");
    }

    double total = 0;
    for (auto v : h) total += v;
    if (total <= 0)
    {
        setStatus (ja (u8"検出するノートがありません。Cubaseで再生するか、MIDIファイルを読み込んでください"), true);
        return;
    }

    const auto key = eflune::detectKey (h);
    const int oldSrcRoot = proc.getParameterIndex (ParamIDs::srcRoot);
    const int oldDstRoot = proc.getParameterIndex (ParamIDs::dstRoot);

    proc.setParameterIndex (ParamIDs::srcRoot, key.root);
    proc.setParameterIndex (ParamIDs::srcScale, key.scale);
    if (oldDstRoot == oldSrcRoot)
        proc.setParameterIndex (ParamIDs::dstRoot, key.root);

    setStatus (source + ja (u8" → 推定キー: ") + ja (eflune::noteName (key.root)) + " "
               + ja (eflune::scales()[(size_t) key.scale].nameJa));
}

void ScaleShifterEditor::applyParallel()
{
    const int dst = proc.getParameterIndex (ParamIDs::dstScale);
    proc.setParameterIndex (ParamIDs::dstScale, isMajorLike (dst) ? 1 : 0);
}

void ScaleShifterEditor::applyRelative()
{
    const int srcRoot  = proc.getParameterIndex (ParamIDs::srcRoot);
    const int srcScale = proc.getParameterIndex (ParamIDs::srcScale);
    if (isMajorLike (srcScale))
    {
        proc.setParameterIndex (ParamIDs::dstRoot, eflune::mod12 (srcRoot + 9));
        proc.setParameterIndex (ParamIDs::dstScale, 1);
    }
    else
    {
        proc.setParameterIndex (ParamIDs::dstRoot, eflune::mod12 (srcRoot + 3));
        proc.setParameterIndex (ParamIDs::dstScale, 0);
    }
}

void ScaleShifterEditor::resetToSource()
{
    proc.setParameterIndex (ParamIDs::dstRoot, proc.getParameterIndex (ParamIDs::srcRoot));
    proc.setParameterIndex (ParamIDs::dstScale, proc.getParameterIndex (ParamIDs::srcScale));
    proc.setParameterIndex (ParamIDs::octave, 2);
}

//==============================================================================
bool ScaleShifterEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (f.endsWithIgnoreCase (".mid") || f.endsWithIgnoreCase (".midi"))
            return true;
    return false;
}

void ScaleShifterEditor::filesDropped (const juce::StringArray& files, int, int)
{
    fileHover = false;
    for (const auto& f : files)
        if (f.endsWithIgnoreCase (".mid") || f.endsWithIgnoreCase (".midi"))
        {
            loadMidiFile (juce::File (f));
            break;
        }
    repaint();
}

void ScaleShifterEditor::openMidiFile()
{
    chooser = std::make_unique<juce::FileChooser> (ja (u8"変換するMIDIファイルを選択"), juce::File(), "*.mid;*.midi");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (f.existsAsFile())
                                  loadMidiFile (f);
                          });
}

void ScaleShifterEditor::loadMidiFile (const juce::File& f)
{
    juce::MidiFile test;
    juce::FileInputStream stream (f);
    if (! stream.openedOk() || ! test.readFrom (stream))
    {
        setStatus (ja (u8"MIDIファイルとして読み込めませんでした: ") + f.getFileName(), true);
        return;
    }

    loadedFile = f;
    dragTile.enabled = true;
    dragTile.caption = ja (u8"↗ ここをドラッグしてCubaseへ\n") + convertedFileName();
    saveButton.setEnabled (true);
    setStatus (ja (u8"読み込みました: ") + f.getFileName() + ja (u8"（「キー自動検出」で元のキーを推定できます）"));
    dragTile.repaint();
}

juce::String ScaleShifterEditor::convertedFileName() const
{
    const auto s = proc.getSettings();
    return loadedFile.getFileNameWithoutExtension() + "_" + asciiNoteName (s.dst.root)
           + juce::String (eflune::scales()[(size_t) s.dst.scale].name).removeCharacters (" -") + ".mid";
}

juce::File ScaleShifterEditor::convertToTemp()
{
    if (! loadedFile.existsAsFile())
        return {};

    const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("EfluneScaleShifter");
    dir.createDirectory();
    const auto out = dir.getChildFile (convertedFileName());

    juce::String error;
    if (! MidiFileConverter::convert (loadedFile, out, proc.getSettings(), error))
    {
        setStatus (error, true);
        return {};
    }
    setStatus (ja (u8"変換しました: ") + out.getFileName());
    return out;
}

void ScaleShifterEditor::saveConvertedFile()
{
    if (! loadedFile.existsAsFile())
        return;

    chooser = std::make_unique<juce::FileChooser> (ja (u8"変換したMIDIファイルの保存先"),
                                                   loadedFile.getSiblingFile (convertedFileName()), "*.mid");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == juce::File())
                                  return;
                              if (! f.hasFileExtension ("mid;midi"))
                                  f = f.withFileExtension ("mid");

                              juce::String error;
                              if (MidiFileConverter::convert (loadedFile, f, proc.getSettings(), error))
                                  setStatus (ja (u8"保存しました: ") + f.getFullPathName());
                              else
                                  setStatus (error, true);
                          });
}

//==============================================================================
static void drawCard (juce::Graphics& g, juce::Rectangle<int> r, juce::Colour tint = juce::Colours::transparentBlack)
{
    auto rf = r.toFloat();
    g.setColour (EfluneLookAndFeel::card);
    g.fillRoundedRectangle (rf, 14.0f);
    if (! tint.isTransparent())
    {
        g.setColour (tint.withAlpha (0.35f));
        g.fillRoundedRectangle (rf, 14.0f);
    }
    g.setColour (EfluneLookAndFeel::line);
    g.drawRoundedRectangle (rf.reduced (0.5f), 14.0f, 1.0f);
}

static void drawHeading (juce::Graphics& g, juce::Rectangle<int> card, const juce::String& text)
{
    g.setColour (EfluneLookAndFeel::muted);
    g.setFont (uiFont (12.5f, true));
    g.drawText (text, card.getX() + 16, card.getY() + 10, card.getWidth() - 32, 18, juce::Justification::centredLeft);
}

void ScaleShifterEditor::paint (juce::Graphics& g)
{
    using L = EfluneLookAndFeel;
    g.fillAll (L::bg);

    // 背景のやわらかいグラデーション
    juce::ColourGradient glow (L::accent.withAlpha (0.45f), 0.0f, 0.0f,
                               L::accent3.withAlpha (0.0f), (float) getWidth() * 0.6f, (float) getHeight() * 0.5f, true);
    g.setGradientFill (glow);
    g.fillRect (getLocalBounds());
    juce::ColourGradient glow2 (L::accent2.withAlpha (0.4f), (float) getWidth(), (float) getHeight(),
                                L::accent4.withAlpha (0.0f), (float) getWidth() * 0.4f, (float) getHeight() * 0.4f, true);
    g.setGradientFill (glow2);
    g.fillRect (getLocalBounds());

    // ヘッダー
    g.setColour (L::ink);
    g.setFont (uiFont (22.0f, true));
    g.drawText ("Eflune Scale Shifter", 24, 16, 400, 28, juce::Justification::centredLeft);
    g.setColour (L::muted);
    g.setFont (uiFont (13.0f));
    g.drawText (ja (u8"メロディー・コード進行のスケールをワンクリックで変換"), 24, 44, 500, 18, juce::Justification::centredLeft);

    drawCard (g, srcCard);
    drawHeading (g, srcCard, ja (u8"元のキー / スケール"));
    drawCard (g, dstCard, L::accent);
    drawHeading (g, dstCard, ja (u8"変換先"));

    // 矢印
    {
        const float cx = (float) (srcCard.getRight() + dstCard.getX()) * 0.5f;
        const float cy = (float) srcCard.getCentreY();
        juce::Path arrow;
        arrow.addArrow ({ cx - 12.0f, cy, cx + 12.0f, cy }, 2.0f, 10.0f, 8.0f);
        g.setColour (L::ink);
        g.fillPath (arrow);
    }

    drawCard (g, buttonsCard);
    drawHeading (g, buttonsCard, ja (u8"ワンクリックでスケール変更"));

    paintMapping (g, mappingArea);

    drawCard (g, fileCard, fileHover ? L::accent4 : juce::Colours::transparentBlack);
    drawHeading (g, fileCard, ja (u8"MIDIファイルを丸ごと変換（Cubaseからドラッグで書き出した .mid を読み込み → 変換結果をドラッグで戻す）"));

    // フッター
    auto f = footerArea;
    g.setColour (statusIsError ? juce::Colour (0xffb3261e) : L::ink);
    g.setFont (uiFont (13.5f, true));
    g.drawFittedText (status, f.removeFromTop (24).withTrimmedRight (320), juce::Justification::centredLeft, 1);
    f.removeFromTop (4);
    g.setColour (L::muted);
    g.setFont (uiFont (12.0f));
    g.drawFittedText (ja (u8"Cubaseでの使い方: このプラグインをインストゥルメントトラックに挿し、MIDIパートを再生 → 別のインストゥルメントトラックの"
                          u8"「MIDI入力」を「Eflune Scale Shifter」にすると、変換後のMIDIで好きな音源を鳴らしたり録音したりできます。"),
                      f, juce::Justification::topLeft, 3, 1.0f);
}

void ScaleShifterEditor::paintMapping (juce::Graphics& g, juce::Rectangle<int> area)
{
    using L = EfluneLookAndFeel;
    drawCard (g, area);
    drawHeading (g, area, ja (u8"音の対応（元 → 変換後）"));

    const auto s = proc.getSettings();
    const bool bypassed = proc.getParameterIndex (ParamIDs::bypass) != 0;
    const auto& srcScale = eflune::scales()[(size_t) s.src.scale];
    const int n = (int) srcScale.steps.size();

    auto r = area.reduced (16, 0).withTrimmedTop (32).withTrimmedBottom (12);
    const int cellW = juce::jmin (100, r.getWidth() / juce::jmax (1, n));
    r = r.withSizeKeepingCentre (cellW * n, r.getHeight());

    for (int i = 0; i < n; ++i)
    {
        auto cell = r.removeFromLeft (cellW).reduced (5, 0);
        const int srcNote = 60 + s.src.root + srcScale.steps[(size_t) i];
        const int dstNote = bypassed ? srcNote : eflune::mapNote (srcNote, s);
        const bool changed = eflune::mod12 (dstNote) != eflune::mod12 (srcNote);

        auto top = cell.removeFromTop (16);
        g.setColour (L::muted);
        g.setFont (uiFont (11.0f));
        g.drawText (romanNumerals[juce::jmin (6, srcScale.letters[(size_t) i])], top, juce::Justification::centred);

        auto srcR = cell.removeFromTop (cell.getHeight() / 2 - 6).toFloat().reduced (2.0f, 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillRoundedRectangle (srcR, 7.0f);
        g.setColour (L::line);
        g.drawRoundedRectangle (srcR, 7.0f, 1.0f);
        g.setColour (L::ink);
        g.setFont (uiFont (15.0f, true));
        g.drawText (ja (eflune::noteName (srcNote)), srcR, juce::Justification::centred);

        auto arrowR = cell.removeFromTop (12).toFloat();
        g.setColour (L::muted);
        juce::Path a;
        a.addArrow ({ arrowR.getCentreX(), arrowR.getY(), arrowR.getCentreX(), arrowR.getBottom() }, 1.2f, 6.0f, 5.0f);
        g.fillPath (a);

        auto dstR = cell.toFloat().reduced (2.0f, 2.0f);
        g.setColour (changed ? L::ink : L::accent4);
        g.fillRoundedRectangle (dstR, 7.0f);
        g.setColour (changed ? L::bg : L::ink);
        g.drawText (ja (eflune::noteName (dstNote)), dstR, juce::Justification::centred);
    }
}

void ScaleShifterEditor::resized()
{
    auto area = getLocalBounds().reduced (20, 0);

    // ヘッダー右側
    auto header = area.removeFromTop (72);
    bypassToggle.setBounds (header.removeFromRight (190).withSizeKeepingCentre (190, 28));

    // キー選択
    auto keyRow = area.removeFromTop (132);
    srcCard = keyRow.removeFromLeft ((keyRow.getWidth() - 44) / 2);
    keyRow.removeFromLeft (44);
    dstCard = keyRow;

    {
        auto r = srcCard.reduced (16).withTrimmedTop (22);
        auto row1 = r.removeFromTop (34);
        srcRootBox.setBounds (row1.removeFromLeft (80));
        row1.removeFromLeft (8);
        srcScaleBox.setBounds (row1);
        r.removeFromTop (8);
        auto row2 = r.removeFromTop (30);
        detectButton.setBounds (row2.removeFromLeft (row2.getWidth() - 96));
        row2.removeFromLeft (8);
        clearHistButton.setBounds (row2);
    }
    {
        auto r = dstCard.reduced (16).withTrimmedTop (22);
        auto row1 = r.removeFromTop (34);
        dstRootBox.setBounds (row1.removeFromLeft (80));
        row1.removeFromLeft (8);
        dstScaleBox.setBounds (row1);
        r.removeFromTop (8);
        octaveBox.setBounds (r.removeFromTop (30));
    }

    area.removeFromTop (12);

    // スケールボタン
    buttonsCard = area.removeFromTop (212);
    {
        auto r = buttonsCard.reduced (16).withTrimmedTop (22);
        auto quick = r.removeFromTop (32);
        const int qw = (quick.getWidth() - 16) / 3;
        parallelButton.setBounds (quick.removeFromLeft (qw));
        quick.removeFromLeft (8);
        relativeButton.setBounds (quick.removeFromLeft (qw));
        quick.removeFromLeft (8);
        resetButton.setBounds (quick);
        r.removeFromTop (10);

        const int cols = 5;
        const int rows = (scaleButtons.size() + cols - 1) / cols;
        const int bw = (r.getWidth() - (cols - 1) * 6) / cols;
        const int bh = (r.getHeight() - (rows - 1) * 6) / rows;
        for (int i = 0; i < scaleButtons.size(); ++i)
            scaleButtons[i]->setBounds (r.getX() + (i % cols) * (bw + 6), r.getY() + (i / cols) * (bh + 6), bw, bh);
    }

    area.removeFromTop (12);
    mappingArea = area.removeFromTop (130);

    area.removeFromTop (12);
    fileCard = area.removeFromTop (84);
    {
        auto r = fileCard.reduced (16).withTrimmedTop (20);
        openButton.setBounds (r.removeFromLeft (170).withSizeKeepingCentre (170, 32));
        r.removeFromLeft (10);
        saveButton.setBounds (r.removeFromRight (150).withSizeKeepingCentre (150, 32));
        r.removeFromRight (10);
        dragTile.setBounds (r);
    }

    area.removeFromTop (10);
    footerArea = area.withTrimmedBottom (10);
    {
        auto f = footerArea;
        auto synthRow = f.removeFromRight (300).removeFromTop (24);
        synthToggle.setBounds (synthRow.removeFromLeft (150));
        volumeSlider.setBounds (synthRow);
    }
}
