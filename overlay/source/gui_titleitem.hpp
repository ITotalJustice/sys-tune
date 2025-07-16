#pragma once

#include <tesla.hpp>
#include <vector>

class TitleItemGui final : public tsl::Gui {
  private:
    tsl::elm::List *m_list{};

    const u64 m_id;
    u8 m_value{};
    float m_tune_volume{1.f};
    float m_title_volume{1.f};

  public:
    TitleItemGui(const std::string& title, u64 id);

    tsl::elm::Element *createUI() override;
    void update() override;
};
