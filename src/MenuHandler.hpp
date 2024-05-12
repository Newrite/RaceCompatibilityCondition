#pragma once

#include "RaceCompatibilityCondition.hpp"

struct MenuHandler final : RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
  [[nodiscard]] static auto get_singleton() noexcept -> MenuHandler*
  {
    static MenuHandler instance;
    return std::addressof(instance);
  }

  static auto register_() -> void
  {
    if (const auto ui = RE::UI::GetSingleton()) {
      ui->AddEventSink(get_singleton());
    }
  }

  auto ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
      -> RE::BSEventNotifyControl override
  {

    if (event) {
      // logger::info("Menu {} is {}", event->menuName.c_str(), event->opening ? "open" : "close");
      if (event->menuName == RE::JournalMenu::MENU_NAME && !event->opening) {
        auto& settings = RaceCompatibilityCondition::Settings::get_singleton();
        settings.load();
        if (settings.ready_to_repatch_armor_addons) {
          settings.ready_to_repatch_armor_addons = false;
          logger::info("Start repatch armor");
          RaceCompatibilityCondition::runtime_patch_armor(settings);
        }
      }
      if (event->menuName == RE::RaceSexMenu::MENU_NAME) {
        // RaceCompatibilityCondition::race_menu_is_open = event->opening;
      }
    }
    return RE::BSEventNotifyControl::kContinue;
  }
};
