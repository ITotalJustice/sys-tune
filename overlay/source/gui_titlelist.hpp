#pragma once

#include <tesla.hpp>
#include <vector>

class TitlelistGui final : public tsl::Gui {
  private:
    tsl::elm::List *m_list;
    std::vector<u64> m_ids;

  public:
    TitlelistGui(bool applet_list);

    tsl::elm::Element *createUI() override;
    void update() override;
};
