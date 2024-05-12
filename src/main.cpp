#include "MenuHandler.hpp"
#include "RaceCompatibilityCondition.hpp"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

void init_logging()
{
  auto path = logger::log_directory();
  if (!path)
    return;

  const auto plugin = SKSE::PluginDeclaration::GetSingleton();
  *path /= std::format("{}.log", plugin->GetName());

  std::vector<spdlog::sink_ptr> sinks{std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true),
                                      std::make_shared<spdlog::sinks::msvc_sink_mt>()};

  auto logger = std::make_shared<spdlog::logger>("global", sinks.begin(), sinks.end());
  logger->set_level(spdlog::level::info);
  logger->flush_on(spdlog::level::info);

  spdlog::set_default_logger(std::move(logger));
  spdlog::set_pattern("[%^%L%$] %v");
}

static void message_handler(SKSE::MessagingInterface::Message* message)
{
  switch (message->type) {
    case SKSE::MessagingInterface::kDataLoaded: {
      MenuHandler::register_();
      auto& settings = RaceCompatibilityCondition::Settings::get_singleton();
      settings.load();
      logger::info("Settings: \nbGetSetRaceHook = {}\nbRuntimePatchRacesStat = {}\nbDisableVanillaRaces = "
                   "{}\nbRuntimePatchArmorAddons = {}\nbSlotsArrayAsWhiteList = {}",
                   settings.get_set_race_hook,
                   settings.runtime_patch_races_stat,
                   settings.disable_vanilla_races,
                   settings.runtime_patch_armor_addons,
                   settings.slots_array_as_white_list);
      RaceCompatibilityCondition::load_data();
      if (settings.runtime_patch_races_stat) {
        RaceCompatibilityCondition::runtime_race_patch();
      }
      if (settings.disable_vanilla_races) {
        RaceCompatibilityCondition::runtime_disable_vanilla_race();
      }
      if (settings.runtime_patch_armor_addons) {
        RaceCompatibilityCondition::runtime_patch_armor(settings);
      }
      if (settings.get_set_race_hook) {
        RaceCompatibilityCondition::install_hooks();
        RaceCompatibilityCondition::OnPlayerUpdate::install_hook();
      }
      break;
    }
  }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
  init_logging();

  const auto plugin = SKSE::PluginDeclaration::GetSingleton();
  logger::info("{} v{} is loading...", plugin->GetName(), plugin->GetVersion());

  SKSE::Init(a_skse);
  if (!SKSE::GetMessagingInterface()->RegisterListener(message_handler)) {
    logger::critical("Failed to load messaging interface! This error is fatal, plugin will not load.");
    return false;
  }

  const auto serialization = SKSE::GetSerializationInterface();
  if (!serialization) {
    logger::info("Serialization interface is null"sv);
    return false;
  }

  serialization->SetUniqueID(RaceCompatibilityCondition::Serialization::LABEL);
  serialization->SetSaveCallback(RaceCompatibilityCondition::Serialization::skse_save_callback);
  serialization->SetLoadCallback(RaceCompatibilityCondition::Serialization::skse_load_callback);

  logger::info("{} loaded.", plugin->GetName());

  return true;
}
