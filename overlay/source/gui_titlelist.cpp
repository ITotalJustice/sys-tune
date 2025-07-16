#include "gui_titlelist.hpp"

#include "gui_titleitem.hpp"
#include "elm_overlayframe.hpp"
#include "pm/pm.hpp"
#include "tune.h"

namespace {

    class TitleList final : public tsl::elm::List {
        private:
            s64 m_timestamp;
            u32 m_nacp_load_count;

        public:
            using List::List;

            void draw(tsl::gfx::Renderer *renderer) override {
                m_timestamp = armTicksToNs(armGetSystemTick()) / 1000000;
                m_nacp_load_count = 0;

                List::draw(renderer);
            }

            bool HasTimeRemaining() {
                bool result = false;
                const s64 now = armTicksToNs(armGetSystemTick()) / 1000000;
                const s64 timeout_ms = 10;

                // always load at least 1 entry per frame.
                if (m_nacp_load_count < 1) {
                    result = true;
                } else if ((now - m_timestamp) < timeout_ms) {
                    result = true;
                }

                if (result) {
                    m_nacp_load_count++;
                }

                return result;
            }
    };

    class TitleListItem final : public tsl::elm::ListItem {
      private:
        const u64 m_id;
        bool m_nacp_loaded{};
        bool m_has_overriden_loaded{};

      private:
        bool LoadedNacp() const {
            return m_nacp_loaded;
        }

        void FetchNacp() {
            if (LoadedNacp()) {
                return;
            }

            // prevent from attempting to load again, even if it fails.
            m_nacp_loaded = true;

            static NsApplicationControlData control; // ~140k
            u64 actual_size;
            if (R_FAILED(nsGetApplicationControlData(NsApplicationControlSource_Storage, m_id, &control, sizeof(control), &actual_size))) {
                return;
            }

            NacpLanguageEntry* lang;
            if (R_FAILED(nsGetApplicationDesiredLanguage(&control.nacp, &lang))) {
                return;
            }

            this->setText(lang->name);
        }

      public:
        explicit TitleListItem(const std::string& name, u64 id, bool applet_list) : ListItem(name), m_id{id}, m_nacp_loaded{applet_list} {
            this->setClickListener([this](u64 keys) -> bool {
                if (keys & HidNpadButton_A) {
                    tsl::changeTo<TitleItemGui>(this->getText(), m_id);
                    // mark dirty to force checking again for changes.
                    m_has_overriden_loaded = false;
                    return true;
                }

                return false;
            });
        }

        explicit TitleListItem(u64 id) : TitleListItem{"[Loading...]", id, false} {

        }

        void draw(tsl::gfx::Renderer *renderer) override {
            // only fetch nacp if we have enough time left on this frame.
            if (!LoadedNacp()) {
                auto list = dynamic_cast<TitleList*>(getParent());
                if (list && list->HasTimeRemaining()) {
                    FetchNacp();
                }
            }

            if (!m_has_overriden_loaded) {
                bool has{};
                if (R_SUCCEEDED(tuneHasOverride(m_id, &has)) && has) {
                    setValue("\uE140");
                } else {
                    setValue("");
                }
                m_has_overriden_loaded = true;
            }

            ListItem::draw(renderer);
        }

        void Reset() {
            tuneResetOverride(m_id);
            m_has_overriden_loaded = false;
        }
    };

}

TitlelistGui::TitlelistGui(bool applet_list) {
    m_list = new TitleList();

    m_list->setClickListener([this](u64 keys) {
        auto item = dynamic_cast<TitleListItem*>(getFocusedElement());
        if (item) {
            if (keys & HidNpadButton_X) {
                item->Reset();
                return true;
            }
        }
        return false;
    });

    if (applet_list) {
        for (auto& e : pm::GetSystemAppletList()) {
            auto item = new TitleListItem(e.name, e.id, applet_list);
            m_list->addItem(item);
        }

        return;
    }

    std::vector<NsApplicationRecord> record_list(300);

    Result rc;
    s32 offset{};
    s32 record_count{};
    if (R_FAILED(rc = nsListApplicationRecord(record_list.data(), record_list.size(), offset, &record_count))) {
        char result_buffer[0x10];
        std::snprintf(result_buffer, sizeof(result_buffer), "2%03X-%04X", R_MODULE(rc), R_DESCRIPTION(rc));
        this->m_list->addItem(new tsl::elm::ListItem("Failed nsListApplicationRecord()"));
        this->m_list->addItem(new tsl::elm::ListItem(result_buffer));
        return;
    }

    if (!record_count) {
        m_list->addItem(new tsl::elm::ListItem("Titlelist empty."));
        return;
    }

    m_list->addItem(new tsl::elm::CategoryHeader("System applets"));
    auto applet_list_button = new tsl::elm::ListItem("Applet list");
    applet_list_button->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<TitlelistGui>(true);
            return true;
        }
        return false;
    });
    m_list->addItem(applet_list_button);

    m_list->addItem(new tsl::elm::CategoryHeader("Games"));
    for (s32 i = 0; i < record_count; i++) {
        const auto id = record_list[i].application_id;
        m_list->addItem(new TitleListItem(id));
    }
}

tsl::elm::Element *TitlelistGui::createUI() {
    auto rootFrame = new SysTuneOverlayFrame();

    rootFrame->setContent(this->m_list);
    rootFrame->setDescription("\uE0E1  Back     \uE0E0  OK     \uE0E2  Reset");

    return rootFrame;
}

void TitlelistGui::update()  {

}
