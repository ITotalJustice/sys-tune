#include "gui_main.hpp"

#include "elm_overlayframe.hpp"
#include "elm_volume.hpp"
#include "gui_browser.hpp"
#include "gui_playlist.hpp"
#include "gui_titlelist.hpp"
#include "pm/pm.hpp"

namespace {
    constexpr const size_t num_steps = 20;
}

MainGui::MainGui() {
    m_status_bar    = new StatusBar();
}

tsl::elm::Element *MainGui::createUI() {
    auto frame = new SysTuneOverlayFrame();
    auto list  = new tsl::elm::List();

    /* Current track. */
    list->addItem(this->m_status_bar, tsl::style::ListItemDefaultHeight * 2);

    /* Playlist. */
    auto queue_button = new tsl::elm::ListItem("Playlist");
    queue_button->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<PlaylistGui>();
            return true;
        }
        return false;
    });
    list->addItem(queue_button);

    /* Browser. */
    auto browser_button = new tsl::elm::ListItem("Music browser");
    browser_button->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<BrowserGui>();
            return true;
        }
        return false;
    });
    list->addItem(browser_button);

    /* Volume indicator */
    list->addItem(new tsl::elm::CategoryHeader("Playback Control"));

    float tune_volume = 1.f;
    bool default_title_play = true;
    float default_title_volume = 1.f;

    tuneGetVolume(&tune_volume);
    tuneGetDefaultTitlePlay(&default_title_play);
    tuneGetDefaultTitleVolume(&default_title_volume);

    /* Default title tune toggle. */
    auto tune_default_play = new tsl::elm::ToggleListItem("Tune", default_title_play, "Play", "Pause");
    tune_default_play->setStateChangedListener([](bool new_value) {
        tuneSetDefaultTitlePlay(new_value);
        // todo: should we do the below?
        if (new_value) {
            tunePlay();
        } else {
            tunePause();
        }
    });
    list->addItem(tune_default_play);

    auto tune_volume_slider = new ElmVolume("\uE13C", "Tune Volume", num_steps);
    tune_volume_slider->setProgress(tune_volume * num_steps);
    tune_volume_slider->setValueChangedListener([](u8 value){
        const float volume = float(value) / float(num_steps);
        tuneSetVolume(volume);
    });
    list->addItem(tune_volume_slider);

    auto default_title_volume_slider = new ElmVolume("\uE13C", "Game Volume", num_steps);
    default_title_volume_slider->setProgress(default_title_volume * num_steps);
    default_title_volume_slider->setValueChangedListener([](u8 value){
        const float volume = float(value) / float(num_steps);
        tuneSetDefaultTitleVolume(volume);
    });
    list->addItem(default_title_volume_slider);

    /* Title Overrides. */
    auto title_button = new tsl::elm::ListItem("Title Overrides");
    title_button->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<TitlelistGui>(false);
            return true;
        }
        return false;
    });
    list->addItem(title_button);

    list->addItem(new tsl::elm::CategoryHeader("Misc"));

    auto startup_button = new tsl::elm::ListItem("Remove start up file");
    startup_button->setClickListener([frame](u64 keys) {
        if (keys & HidNpadButton_A) {
            char path[512];
            tuneGetAutoPlayPath(path, sizeof(path));
            if (strlen(path)) {
                tuneSetAutoPlayPath("");
                const auto* p = path;
                if (auto ext = std::strrchr(path, '/')) {
                    p = ext + 1;
                }

                frame->setToast("Removed start up file", p);
            } else {
                frame->setToast("Failed to remove start up file", "No start up file set in config");
            }
            return true;
        }
        return false;
    });
    list->addItem(startup_button);

    auto exit_button = new tsl::elm::ListItem("Close sys-tune");
    exit_button->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tuneQuit();
            tsl::goBack();
            return true;
        }
        return false;
    });
    list->addItem(exit_button);

    frame->setContent(list);

    return frame;
}

void MainGui::update() {
    static u8 tick = 0;
    /* Update status 4 times per second. */
    if ((tick % 15) == 0)
        this->m_status_bar->update();
    tick++;
}
