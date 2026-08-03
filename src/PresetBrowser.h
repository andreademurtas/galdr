// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include "PluginProcessor.h"
#include "Presets.h"
#include "BlackMetalLookAndFeel.h"

// Full-page preset browser overlaid on the editor: factory presets by
// category, user presets from disk with name/author/description metadata,
// search, save form and prev/next stepping for the header arrows.
class PresetBrowser : public juce::Component
{
public:
    PresetBrowser(GaldrAudioProcessor& p, BlackMetalLookAndFeel& laf)
        : processorRef(p), lnf(laf)
    {
        for (auto* list : { &categoryList, &presetList })
        {
            list->setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
            list->setColour(juce::ListBox::outlineColourId, theme::outline);
            list->setOutlineThickness(1);
            addAndMakeVisible(list);
        }
        categoryList.setModel(&categoryModel);
        presetList.setModel(&presetModel);

        searchBox.setTextToShowWhenEmpty("Search...", theme::boneDim);
        searchBox.onTextChange = [this] { rebuildVisible(); };
        addAndMakeVisible(searchBox);

        closeButton.onClick = [this] { setVisible(false); };
        addAndMakeVisible(closeButton);

        saveToggle.onClick = [this] { enterSaveMode(); };
        addAndMakeVisible(saveToggle);

        folderButton.onClick = [] { presetDirectory().revealToUser(); };
        folderButton.setTooltip("Show the user preset folder in the file browser");
        addAndMakeVisible(folderButton);

        deleteButton.onClick = [this] { deleteSelected(); };
        deleteButton.setTooltip("Delete the selected user preset");
        addAndMakeVisible(deleteButton);

        for (auto* l : { &nameLabel, &authorLabel, &descLabel })
        {
            l->setColour(juce::Label::textColourId, theme::boneDim);
            addChildComponent(l);
        }
        descEdit.setMultiLine(true, true);
        descEdit.setReturnKeyStartsNewLine(true);
        for (auto* e : { &nameEdit, &authorEdit, &descEdit })
            addChildComponent(e);

        saveConfirm.onClick = [this] { doSave(); };
        addChildComponent(saveConfirm);
        cancelSave.onClick = [this] { leaveSaveMode(); };
        addChildComponent(cancelSave);

        refresh();
    }

    static juce::File presetDirectory()
    {
        auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                       .getChildFile("Galdr Presets");
        dir.createDirectory();
        return dir;
    }

    void open(bool withSaveForm)
    {
        refresh();
        if (withSaveForm)
            enterSaveMode();
        else
            leaveSaveMode();
        setVisible(true);
        toFront(true);
    }

    // Header prev/next arrows step through every preset, wrapping around.
    void step(int delta)
    {
        if (entries.empty())
            return;
        int cur = lastApplied;
        if (cur < 0)
        {
            const auto name = processorRef.apvts.state.getProperty("presetName").toString();
            for (int i = 0; i < (int) entries.size(); ++i)
                if (entries[(size_t) i].name == name)
                {
                    cur = i;
                    break;
                }
        }
        const int n = (int) entries.size();
        applyEntry(cur < 0 ? (delta > 0 ? 0 : n - 1) : ((cur + delta) % n + n) % n);
    }

    void resized() override
    {
        const float scale = lnf.uiScale;
        auto sc = [scale](int v) { return juce::roundToInt((float) v * scale); };

        auto inner = innerRect().reduced(sc(14), sc(12));
        auto header = inner.removeFromTop(sc(38));
        closeButton.setBounds(header.removeFromRight(sc(28)).withSizeKeepingCentre(sc(26), sc(26)));
        header.removeFromRight(sc(8));
        searchBox.setBounds(header.removeFromRight(sc(220)).reduced(0, sc(5)));
        inner.removeFromTop(sc(6));

        auto left = inner.removeFromLeft(sc(180));
        categoryList.setBounds(left);
        categoryList.setRowHeight(sc(30));

        detailArea = inner.removeFromRight(sc(270));
        presetList.setBounds(inner.reduced(sc(10), 0));
        presetList.setRowHeight(sc(26));

        // action buttons at the bottom of the detail column
        auto actions = detailArea;
        auto rowOf = [&](int h) { return actions.removeFromBottom(sc(h)); };
        folderButton.setBounds(rowOf(28));
        actions.removeFromBottom(sc(6));
        deleteButton.setBounds(rowOf(28));
        actions.removeFromBottom(sc(6));
        saveToggle.setBounds(rowOf(28));

        // save form fills the same column
        auto form = detailArea;
        nameLabel.setBounds(form.removeFromTop(sc(18)));
        nameEdit.setBounds(form.removeFromTop(sc(26)));
        form.removeFromTop(sc(8));
        authorLabel.setBounds(form.removeFromTop(sc(18)));
        authorEdit.setBounds(form.removeFromTop(sc(26)));
        form.removeFromTop(sc(8));
        descLabel.setBounds(form.removeFromTop(sc(18)));
        descEdit.setBounds(form.removeFromTop(sc(96)));
        form.removeFromTop(sc(10));
        saveConfirm.setBounds(form.removeFromTop(sc(28)));
        form.removeFromTop(sc(6));
        cancelSave.setBounds(form.removeFromTop(sc(28)));

        const auto body = lnf.getBodyFont(15.0f * scale);
        for (auto* e : { &searchBox, &nameEdit, &authorEdit, &descEdit })
        {
            e->setFont(body);
            e->applyFontToAllText(body);
        }
    }

    void paint(juce::Graphics& g) override
    {
        const float scale = lnf.uiScale;
        auto sc = [scale](int v) { return juce::roundToInt((float) v * scale); };

        g.fillAll(juce::Colour(0xcc06060a));

        auto box = innerRect();
        g.setColour(theme::panel);
        g.fillRect(box);
        g.setColour(theme::outline);
        g.drawRect(box, 2);

        g.setFont(lnf.getTitleFont(30.0f * scale));
        g.setColour(theme::bone);
        g.drawText("Presets", box.getX() + sc(16), box.getY() + sc(10), sc(220), sc(34),
                   juce::Justification::centredLeft);

        if (! saveMode)
            paintDetails(g, scale);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (! innerRect().contains(e.getPosition()))
            setVisible(false);
    }

private:
    struct Entry
    {
        juce::String name, category, author, description;
        int factoryIndex = -1; // -1: user preset loaded from `file`
        juce::File file;
    };

    struct Model : juce::ListBoxModel
    {
        PresetBrowser& owner;
        const bool categories;
        Model(PresetBrowser& o, bool c) : owner(o), categories(c) {}

        int getNumRows() override
        {
            return categories ? owner.categoryNames.size() : (int) owner.visible.size();
        }

        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override
        {
            owner.paintRow(g, row, w, h, selected, categories);
        }

        void listBoxItemClicked(int row, const juce::MouseEvent&) override
        {
            if (categories)
                owner.selectCategory(row);
            else if (row >= 0 && row < (int) owner.visible.size())
                owner.applyEntry(owner.visible[(size_t) row]);
        }
    };

    juce::Rectangle<int> innerRect() const
    {
        auto b = getLocalBounds();
        return b.withSizeKeepingCentre(juce::roundToInt((float) b.getWidth() * 0.78f),
                                       juce::roundToInt((float) b.getHeight() * 0.72f));
    }

    void refresh()
    {
        entries.clear();
        categoryNames.clearQuick();
        categoryNames.add("All");

        const auto& factory = presets::all();
        for (int i = 0; i < (int) factory.size(); ++i)
        {
            const auto& fp = factory[(size_t) i];
            entries.push_back({ fp.name, fp.category, "Galdr", "", i, {} });
            categoryNames.addIfNotAlreadyThere(fp.category);
        }

        for (const auto& file : presetDirectory().findChildFiles(juce::File::findFiles, false, "*.galdr"))
        {
            Entry e;
            e.file = file;
            e.category = "User";
            e.name = file.getFileNameWithoutExtension();
            if (auto xml = juce::XmlDocument::parse(file))
            {
                const auto tree = juce::ValueTree::fromXml(*xml);
                if (tree.getProperty("presetName").toString().isNotEmpty())
                    e.name = tree.getProperty("presetName").toString();
                e.author = tree.getProperty("presetAuthor").toString();
                e.description = tree.getProperty("presetDescription").toString();
            }
            entries.push_back(std::move(e));
        }
        categoryNames.add("User");

        // resync the applied marker by name; the vector was rebuilt
        lastApplied = -1;
        const auto current = processorRef.apvts.state.getProperty("presetName").toString();
        for (int i = 0; i < (int) entries.size(); ++i)
            if (entries[(size_t) i].name == current)
            {
                lastApplied = i;
                break;
            }

        if (currentCategory >= categoryNames.size())
            currentCategory = 0;
        categoryList.updateContent();
        categoryList.selectRow(currentCategory, juce::dontSendNotification);
        rebuildVisible();
    }

    void rebuildVisible()
    {
        visible.clear();
        const auto needle = searchBox.getText().trim();
        const auto category = categoryNames[currentCategory];
        for (int i = 0; i < (int) entries.size(); ++i)
        {
            const auto& e = entries[(size_t) i];
            if (currentCategory != 0 && e.category != category)
                continue;
            if (needle.isNotEmpty() && ! e.name.containsIgnoreCase(needle)
                && ! e.author.containsIgnoreCase(needle))
                continue;
            visible.push_back(i);
        }
        presetList.updateContent();
        presetList.deselectAllRows();
        repaint();
    }

    void selectCategory(int row)
    {
        if (row < 0 || row >= categoryNames.size())
            return;
        currentCategory = row;
        rebuildVisible();
    }

    void applyEntry(int index)
    {
        if (index < 0 || index >= (int) entries.size())
            return;
        const auto& e = entries[(size_t) index];
        if (e.factoryIndex >= 0)
        {
            presets::apply(processorRef.apvts, e.factoryIndex);
            processorRef.apvts.state.setProperty("presetName", e.name, nullptr);
            processorRef.presetDirty.store(false);
            processorRef.undoManager.clearUndoHistory();
        }
        else if (auto xml = juce::XmlDocument::parse(e.file))
        {
            processorRef.applyStateTree(juce::ValueTree::fromXml(*xml));
            if (processorRef.apvts.state.getProperty("presetName").toString().isEmpty())
                processorRef.apvts.state.setProperty("presetName", e.name, nullptr);
        }
        lastApplied = index;
        repaint();
    }

    void enterSaveMode()
    {
        saveMode = true;
        auto current = processorRef.apvts.state.getProperty("presetName").toString();
        nameEdit.setText(current.isNotEmpty() ? current : "My Preset",
                         juce::dontSendNotification);
        authorEdit.setText(processorRef.apvts.state.getProperty("presetAuthor").toString(),
                           juce::dontSendNotification);
        updateFormVisibility();
    }

    void leaveSaveMode()
    {
        saveMode = false;
        updateFormVisibility();
    }

    void updateFormVisibility()
    {
        for (auto* c : std::initializer_list<juce::Component*> {
                 &nameLabel, &nameEdit, &authorLabel, &authorEdit,
                 &descLabel, &descEdit, &saveConfirm, &cancelSave })
            c->setVisible(saveMode);
        for (auto* c : std::initializer_list<juce::Component*> {
                 &saveToggle, &deleteButton, &folderButton })
            c->setVisible(! saveMode);
        repaint();
    }

    void doSave()
    {
        const auto name = nameEdit.getText().trim();
        if (name.isEmpty())
            return;

        auto tree = processorRef.capturePresetState();
        tree.setProperty("presetName", name, nullptr);
        tree.setProperty("presetAuthor", authorEdit.getText().trim(), nullptr);
        tree.setProperty("presetDescription", descEdit.getText().trim(), nullptr);

        const auto file = presetDirectory()
                              .getChildFile(juce::File::createLegalFileName(name) + ".galdr");
        if (auto xml = tree.createXml())
            xml->writeTo(file);

        processorRef.apvts.state.setProperty("presetName", name, nullptr);
        processorRef.apvts.state.setProperty("presetAuthor", authorEdit.getText().trim(), nullptr);
        processorRef.presetDirty.store(false);

        saveMode = false;
        refresh();
        updateFormVisibility();
    }

    void deleteSelected()
    {
        const int row = presetList.getSelectedRow();
        if (row < 0 || row >= (int) visible.size())
            return;
        const auto& e = entries[(size_t) visible[(size_t) row]];
        if (e.factoryIndex >= 0)
            return; // factory presets are immortal
        juce::Component::SafePointer<PresetBrowser> safeThis(this);
        juce::AlertWindow::showOkCancelBox(
            juce::MessageBoxIconType::WarningIcon, "Delete preset",
            "Delete \"" + e.name + "\"?", "Delete", "Cancel", this,
            juce::ModalCallbackFunction::create([safeThis, file = e.file](int result)
            {
                if (result == 1 && safeThis != nullptr)
                {
                    file.deleteFile();
                    safeThis->refresh();
                }
            }));
    }

    int detailIndex() const
    {
        const int row = presetList.getSelectedRow();
        if (row >= 0 && row < (int) visible.size())
            return visible[(size_t) row];
        return lastApplied;
    }

    void paintDetails(juce::Graphics& g, float scale)
    {
        auto sc = [scale](int v) { return juce::roundToInt((float) v * scale); };
        auto area = detailArea.withTrimmedBottom(sc(110));
        g.setColour(theme::outline);
        g.drawRect(area, 1);
        area.reduce(sc(10), sc(8));

        const int idx = detailIndex();
        if (idx < 0 || idx >= (int) entries.size())
        {
            g.setColour(theme::boneDim);
            g.setFont(lnf.getBodyFont(15.0f * scale));
            g.drawText("Select a preset", area, juce::Justification::centred);
            return;
        }

        const auto& e = entries[(size_t) idx];
        g.setColour(theme::bone);
        g.setFont(lnf.getBodyFont(19.0f * scale));
        g.drawText(e.name, area.removeFromTop(sc(24)), juce::Justification::centredLeft);

        g.setColour(theme::boneDim);
        g.setFont(lnf.getBodyFont(14.0f * scale));
        auto byline = e.category + (e.author.isNotEmpty() ? " - " + e.author : juce::String());
        g.drawText(byline, area.removeFromTop(sc(18)), juce::Justification::centredLeft);
        area.removeFromTop(sc(8));

        if (e.description.isNotEmpty())
        {
            g.setColour(theme::bone.withAlpha(0.8f));
            g.drawFittedText(e.description, area, juce::Justification::topLeft, 8);
        }
    }

    void paintRow(juce::Graphics& g, int row, int w, int h, bool selected, bool categories)
    {
        const float scale = lnf.uiScale;
        auto sc = [scale](int v) { return juce::roundToInt((float) v * scale); };

        juce::String text;
        bool isApplied = false;
        if (categories)
        {
            if (row < 0 || row >= categoryNames.size())
                return;
            text = categoryNames[row];
            selected = row == currentCategory;
        }
        else
        {
            if (row < 0 || row >= (int) visible.size())
                return;
            const int idx = visible[(size_t) row];
            text = entries[(size_t) idx].name;
            isApplied = idx == lastApplied;
        }

        if (selected)
        {
            g.setColour(theme::blood.withAlpha(0.35f));
            g.fillRect(0, 0, w, h);
        }

        auto textArea = juce::Rectangle<int>(sc(10), 0, w - sc(16), h);
        if (isApplied)
        {
            g.setColour(theme::bloodBright);
            g.fillEllipse((float) sc(3), (float) h * 0.5f - 2.0f, 4.0f, 4.0f);
        }
        g.setColour(selected || isApplied ? theme::bone : theme::bone.withAlpha(0.75f));
        g.setFont(lnf.getBodyFont((categories ? 16.0f : 15.0f) * scale));
        g.drawText(text, textArea, juce::Justification::centredLeft);
    }

    GaldrAudioProcessor& processorRef;
    BlackMetalLookAndFeel& lnf;

    std::vector<Entry> entries;
    juce::StringArray categoryNames;
    std::vector<int> visible;
    int currentCategory = 0;
    int lastApplied = -1;
    bool saveMode = false;

    Model categoryModel { *this, true }, presetModel { *this, false };
    juce::ListBox categoryList, presetList;
    juce::TextEditor searchBox;
    juce::Rectangle<int> detailArea;

    juce::TextButton closeButton { "X" };
    juce::TextButton saveToggle { "Save current..." };
    juce::TextButton deleteButton { "Delete" };
    juce::TextButton folderButton { "Open folder" };

    juce::Label nameLabel { {}, "Name" }, authorLabel { {}, "Author" }, descLabel { {}, "Description" };
    juce::TextEditor nameEdit, authorEdit, descEdit;
    juce::TextButton saveConfirm { "Save" }, cancelSave { "Cancel" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};
