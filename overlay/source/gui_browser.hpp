#pragma once

#include <tesla.hpp>

#include "elm_overlayframe.hpp"

class BrowserGui final : public tsl::Gui {
  public:
    using FilePickerCallback = std::function<void(const std::string& path)>;

  private:
    SysTuneOverlayFrame* m_frame;
    tsl::elm::List *m_list;
    FsFileSystem m_fs;
    bool has_music;
    char cwd[FS_MAX_PATH];
    const FilePickerCallback m_picker_callback;

  public:
    BrowserGui(FilePickerCallback&& cb = nullptr);
    ~BrowserGui();

    tsl::elm::Element *createUI() override;
    bool handleInput(u64 keysDown, u64, const HidTouchState&, HidAnalogStickState, HidAnalogStickState) override;

    bool IsPicker() const {
      return m_picker_callback != nullptr;
    }

  private:
    void scanCwd();
    void upCwd();
    void addAllToPlaylist();
    void infoAlert(const std::string &title, const std::string &text);
};
