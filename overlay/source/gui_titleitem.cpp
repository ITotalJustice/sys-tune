#include "gui_titleitem.hpp"
#include "gui_browser.hpp"

#include "elm_volume.hpp"
#include "elm_overlayframe.hpp"
#include "tune.h"

namespace {

    constexpr const size_t num_steps = 20;

}

TitleItemGui::TitleItemGui(const std::string& title, u64 id) : m_id{id} {
    m_list = new tsl::elm::List();

    m_list->addItem(new tsl::elm::CategoryHeader(title, true));

    /* Load all title overrides. */
    bool has_tune_play{};
    bool tune_play{};
    tuneGetTunePlayOverride(id, &tune_play, &has_tune_play);

    bool has_tune_volume{};
    float tune_volume{1.f};
    tuneGetTuneVolumeOverride(id, &tune_volume, &has_tune_volume);

    bool has_title_volume{};
    float title_volume{1.f};
    tuneGetTitleVolumeOverride(id, &title_volume, &has_title_volume);

    bool has_title_music{};
    char title_music[256]{};
    tuneGetTitleMusicPathOverride(id, title_music, sizeof(title_music), &has_title_music);

    static const char* PLAY_PAUSE_STR[2] = { "Pause", "Play" };

    /* Buttons for all overrides. */
    auto play_button = new tsl::elm::ListItem("Tune");
    play_button->setValue(has_tune_play ? PLAY_PAUSE_STR[tune_play] : "Unset", !has_tune_play);
    play_button->setClickListener([play_button, id](u64 keys) {
        if (keys & HidNpadButton_A) {
            bool has_tune_play{};
            bool tune_play{};
            tuneGetTunePlayOverride(id, &tune_play, &has_tune_play);

            if (!has_tune_play) {
                tune_play = true;
            } else {
                tune_play ^= 1;
            }

            play_button->setValue(PLAY_PAUSE_STR[tune_play]);
            tuneSetTunePlayOverride(id, tune_play, false);
            return true;
        } else if (keys & HidNpadButton_X) {
            play_button->setValue("Unset", true);
            tuneSetTunePlayOverride(id, false, true);
            return true;
        }
        return false;
    });
    m_list->addItem(play_button);

    auto tune_volume_slider = new ElmVolume("\uE13C", "Tune Volume", num_steps);
    tune_volume_slider->setProgress(tune_volume * num_steps);
    tune_volume_slider->setClickListener([tune_volume_slider, id](u64 keys) -> bool {
        if (keys & HidNpadButton_X) {
            tune_volume_slider->setProgress(num_steps);
            tuneSetTuneVolumeOverride(id, 0, true);
            return true;
        }

        return false;
    });
    tune_volume_slider->setValueChangedListener([id](u8 value){
        const float volume = float(value) / float(num_steps);
        tuneSetTuneVolumeOverride(id, volume, false);
    });
    m_list->addItem(tune_volume_slider);

    auto title_volume_slider = new ElmVolume("\uE13C", "Game Volume", num_steps);
    title_volume_slider->setProgress(title_volume * num_steps);
    title_volume_slider->setClickListener([title_volume_slider, id](u64 keys) -> bool {
        if (keys & HidNpadButton_X) {
            title_volume_slider->setProgress(num_steps);
            tuneSetTitleVolumeOverride(id, 0, true);
            return true;
        }

        return false;
    });
    title_volume_slider->setValueChangedListener([this, id](u8 value){
        const float volume = float(value) / float(num_steps);
        tuneSetTitleVolumeOverride(id, volume, false);
    });
    m_list->addItem(title_volume_slider);

    auto picker_button = new tsl::elm::ListItem("Default music");
    picker_button->setValue(has_title_music ? "Set" : "Unset", !has_title_music);
    picker_button->setClickListener([this, picker_button, id](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<BrowserGui>([this, picker_button, id](const std::string& path){
                picker_button->setValue("Set");
                tuneSetTitleMusicPathOverride(id, path.c_str(), false);
            });
            return true;
        } else if (keys & HidNpadButton_X) {
            picker_button->setValue("Unset", true);
            tuneSetTitleMusicPathOverride(id, NULL, true);
            return true;
        }
        return false;
    });
    m_list->addItem(picker_button);
}

tsl::elm::Element *TitleItemGui::createUI() {
    auto rootFrame = new SysTuneOverlayFrame();

    rootFrame->setContent(this->m_list);
    rootFrame->setDescription("\uE0E1  Back     \uE0E0  OK     \uE0E2  Reset");

    return rootFrame;
}

void TitleItemGui::update()  {

}
