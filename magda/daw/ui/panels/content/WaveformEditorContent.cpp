#include "WaveformEditorContent.hpp"

#include "../../themes/DarkTheme.hpp"
#include "../../themes/FontManager.hpp"

namespace magda::daw::ui {

WaveformEditorContent::WaveformEditorContent() {
    setName("WaveformEditor");

    // Register as ClipManager listener
    magda::ClipManager::getInstance().addListener(this);

    // Check if there's already a selected audio clip
    magda::ClipId selectedClip = magda::ClipManager::getInstance().getSelectedClip();
    if (selectedClip != magda::INVALID_CLIP_ID) {
        const auto* clip = magda::ClipManager::getInstance().getClip(selectedClip);
        if (clip && clip->type == magda::ClipType::Audio) {
            editingClipId_ = selectedClip;
        }
    }
}

WaveformEditorContent::~WaveformEditorContent() {
    magda::ClipManager::getInstance().removeListener(this);
}

void WaveformEditorContent::paint(juce::Graphics& g) {
    g.fillAll(DarkTheme::getPanelBackgroundColour());

    auto bounds = getLocalBounds();

    // Header area (time axis)
    auto headerArea = bounds.removeFromTop(HEADER_HEIGHT);
    paintHeader(g, headerArea);

    // Waveform area
    auto waveformArea = bounds.reduced(SIDE_MARGIN, 10);

    if (editingClipId_ != magda::INVALID_CLIP_ID) {
        const auto* clip = magda::ClipManager::getInstance().getClip(editingClipId_);
        if (clip && clip->type == magda::ClipType::Audio) {
            paintWaveform(g, waveformArea, *clip);
        } else {
            paintNoClipMessage(g, waveformArea);
        }
    } else {
        paintNoClipMessage(g, waveformArea);
    }
}

void WaveformEditorContent::paintHeader(juce::Graphics& g, juce::Rectangle<int> area) {
    g.setColour(DarkTheme::getColour(DarkTheme::SURFACE));
    g.fillRect(area);

    // Draw time markers
    g.setColour(DarkTheme::getSecondaryTextColour());
    g.setFont(FontManager::getInstance().getUIFont(9.0f));

    const auto* clip = editingClipId_ != magda::INVALID_CLIP_ID
                           ? magda::ClipManager::getInstance().getClip(editingClipId_)
                           : nullptr;

    double lengthSeconds = clip ? clip->length : 10.0;

    // Draw markers every second
    for (int sec = 0; sec <= static_cast<int>(lengthSeconds) + 1; sec++) {
        int x = SIDE_MARGIN + static_cast<int>(sec * horizontalZoom_);
        if (x < area.getRight() - SIDE_MARGIN) {
            g.drawVerticalLine(x, area.getY(), area.getBottom());

            // Format time as MM:SS
            int minutes = sec / 60;
            int seconds = sec % 60;
            juce::String timeStr = juce::String::formatted("%d:%02d", minutes, seconds);
            g.drawText(timeStr, x + 2, area.getY(), 40, area.getHeight(),
                       juce::Justification::centredLeft, false);
        }
    }

    // Border
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.drawRect(area);
}

void WaveformEditorContent::paintWaveform(juce::Graphics& g, juce::Rectangle<int> area,
                                          const magda::ClipInfo& clip) {
    // Background
    g.setColour(DarkTheme::getColour(DarkTheme::TRACK_BACKGROUND));
    g.fillRoundedRectangle(area.toFloat(), 4.0f);

    // Since we don't have actual audio data loaded, draw a placeholder waveform
    // In a real implementation, this would read from an AudioThumbnail or similar

    g.setColour(clip.colour);

    float centerY = area.getCentreY();
    int width = juce::jmin(area.getWidth(), static_cast<int>(clip.length * horizontalZoom_));

    // Draw a simulated waveform using sine waves of varying frequencies
    juce::Path waveform;
    bool started = false;

    for (int x = 0; x < width; x++) {
        float time = static_cast<float>(x) / horizontalZoom_;
        float amplitude = 0.0f;

        // Combine multiple frequencies for a more realistic look
        amplitude += std::sin(time * 5.0f) * 0.3f;
        amplitude += std::sin(time * 13.0f) * 0.2f;
        amplitude += std::sin(time * 27.0f) * 0.15f;
        amplitude += std::sin(time * 53.0f) * 0.1f;

        // Add some envelope variation
        float envelope = 0.7f + 0.3f * std::sin(time * 0.5f);
        amplitude *= envelope;

        float y = centerY + amplitude * area.getHeight() * 0.4f;

        if (!started) {
            waveform.startNewSubPath(area.getX() + x, y);
            started = true;
        } else {
            waveform.lineTo(area.getX() + x, y);
        }
    }

    g.strokePath(waveform, juce::PathStrokeType(1.5f));

    // Draw center line
    g.setColour(DarkTheme::getColour(DarkTheme::BORDER));
    g.drawHorizontalLine(static_cast<int>(centerY), area.getX(), area.getX() + width);

    // Clip info overlay
    g.setColour(clip.colour);
    g.setFont(FontManager::getInstance().getUIFont(12.0f));
    g.drawText(clip.name, area.reduced(8, 4), juce::Justification::topLeft, true);

    // File path (if available)
    if (clip.audioFilePath.isNotEmpty()) {
        g.setColour(DarkTheme::getSecondaryTextColour());
        g.setFont(FontManager::getInstance().getUIFont(10.0f));
        g.drawText(clip.audioFilePath, area.reduced(8, 4).translated(0, 16),
                   juce::Justification::topLeft, true);
    }

    // Border
    g.setColour(clip.colour.withAlpha(0.5f));
    g.drawRoundedRectangle(area.toFloat(), 4.0f, 1.0f);
}

void WaveformEditorContent::paintNoClipMessage(juce::Graphics& g, juce::Rectangle<int> area) {
    g.setColour(DarkTheme::getSecondaryTextColour());
    g.setFont(FontManager::getInstance().getUIFont(14.0f));
    g.drawText("No audio clip selected", area, juce::Justification::centred, false);

    g.setFont(FontManager::getInstance().getUIFont(11.0f));
    g.drawText("Select an audio clip to view its waveform", area.translated(0, 24),
               juce::Justification::centred, false);
}

void WaveformEditorContent::resized() {
    // Nothing special to layout
}

void WaveformEditorContent::onActivated() {
    // Check for selected audio clip
    magda::ClipId selectedClip = magda::ClipManager::getInstance().getSelectedClip();
    if (selectedClip != magda::INVALID_CLIP_ID) {
        const auto* clip = magda::ClipManager::getInstance().getClip(selectedClip);
        if (clip && clip->type == magda::ClipType::Audio) {
            editingClipId_ = selectedClip;
        }
    }
    repaint();
}

void WaveformEditorContent::onDeactivated() {
    // Nothing to do
}

// ============================================================================
// ClipManagerListener
// ============================================================================

void WaveformEditorContent::clipsChanged() {
    // Check if our clip was deleted
    if (editingClipId_ != magda::INVALID_CLIP_ID) {
        const auto* clip = magda::ClipManager::getInstance().getClip(editingClipId_);
        if (!clip) {
            editingClipId_ = magda::INVALID_CLIP_ID;
        }
    }
    repaint();
}

void WaveformEditorContent::clipPropertyChanged(magda::ClipId clipId) {
    if (clipId == editingClipId_) {
        repaint();
    }
}

void WaveformEditorContent::clipSelectionChanged(magda::ClipId clipId) {
    // Auto-switch to the selected clip if it's an audio clip
    if (clipId != magda::INVALID_CLIP_ID) {
        const auto* clip = magda::ClipManager::getInstance().getClip(clipId);
        if (clip && clip->type == magda::ClipType::Audio) {
            editingClipId_ = clipId;
            repaint();
        }
    }
}

// ============================================================================
// Public Methods
// ============================================================================

void WaveformEditorContent::setClip(magda::ClipId clipId) {
    if (editingClipId_ != clipId) {
        editingClipId_ = clipId;
        repaint();
    }
}

}  // namespace magda::daw::ui
