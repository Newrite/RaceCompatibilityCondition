#pragma once

#include "toml.hpp"
#include "xbyak/xbyak.h"

#include <print>

namespace RaceCompatibilityCondition {

namespace Serialization {

static constexpr auto MAX_PATH_FILE = 260;

static char race_file_name[MAX_PATH_FILE]{};
static RE::FormID race_form_id = 0x0;

static RE::TESRace* get_race()
{
  auto data_handler = RE::TESDataHandler::GetSingleton();
  if (data_handler) {
    return data_handler->LookupForm<RE::TESRace>(race_form_id, race_file_name);
  }
  return nullptr;
}

static constexpr uint32_t LABEL = 'RCCF';
static const int32_t SERIALIZATION_VERSION = 1;

static void reset()
{
  strcpy_s(race_file_name, "");
  race_form_id = 0x0;
}

static void load(const SKSE::SerializationInterface& a_interface)
{
  uint32_t type;
  uint32_t version;
  uint32_t length;

  logger::info("Start read save"sv);

  while (a_interface.GetNextRecordInfo(type, version, length)) {
    switch (type) {
      case LABEL: {
        int32_t serialization_version;

        if (!a_interface.ReadRecordData(serialization_version)) {
          logger::error("Fail load ser version, return"sv);
          reset();
          return;
        }

        if (serialization_version != SERIALIZATION_VERSION) {
          logger::warn("Serialization version mismatched, clear cache and return, "
                       "versions: {} | {}"sv,
                       serialization_version,
                       SERIALIZATION_VERSION);
          reset();
          return;
        }

        RE::FormID form_id;
        if (!a_interface.ReadRecordData(form_id)) {
          logger::error("Fail read form id"sv);
          break;
        }

        char file_name[MAX_PATH_FILE]{};
        if (!a_interface.ReadRecordData(file_name)) {
          logger::error("Fail read file name"sv);
          break;
        }

        logger::info("Success read record with FormID: {} FileName: {}"sv, form_id, file_name);
      }
      default: {
        logger::warn("Unrecognized signature type: {}"sv, type);
        break;
      }
    }
  }

  logger::info("Finish read save"sv);
}

static void save(const SKSE::SerializationInterface& a_interface)
{
  logger::info("Start write save"sv);

  if (!a_interface.OpenRecord(LABEL, 1)) {
    logger::error("Error when try open record {} on save"sv, LABEL);
    return;
  }

  if (!a_interface.WriteRecordData(&SERIALIZATION_VERSION, sizeof SERIALIZATION_VERSION)) {
    logger::error("Failed to write SERIALIZATION_VERSION"sv);
    return;
  }

  if (!a_interface.WriteRecordData(&race_form_id, sizeof race_form_id)) {
    logger::error("Failed to write form id"sv);
    return;
  }
  if (!a_interface.WriteRecordData(&race_file_name, sizeof race_file_name)) {
    logger::error("Failed to write file name"sv);
    return;
  }
  logger::info("Finish write save with FormID {} and FileName {}"sv, race_form_id, race_file_name);
}

static void skse_save_callback(SKSE::SerializationInterface* a_interface)
{
  if (!a_interface) {
    logger::error("Null skse serialization interface error when save"sv);
    return;
  }
  save(*a_interface);
}

static void skse_load_callback(SKSE::SerializationInterface* a_interface)
{
  if (!a_interface) {
    logger::error("Null skse serialization interface error when load"sv);
    return;
  }
  load(*a_interface);
}

}

struct Settings final
{
  Settings() = default;

  using SLOT = RE::BGSBipedObjectForm::BipedObjectSlot;

  bool first_load{true};
  bool ready_to_repatch_armor_addons{false};
  std::vector<SLOT> last_slots_array{};

  bool get_set_race_hook{true};
  // bool bDisableHookWhileRaceMenuOpen{false};
  bool runtime_patch_races_stat{true};
  bool disable_vanilla_races{false};
  bool runtime_patch_armor_addons{false};
  bool slots_array_as_white_list{true};
  std::vector<SLOT> slots_array{SLOT::kRing,
                                SLOT::kHair,
                                SLOT::kLongHair,
                                SLOT::kCirclet,
                                SLOT::kShield,
                                SLOT::kEars,
                                SLOT::kAmulet,
                                SLOT::kHead,
                                SLOT::kTail,
                                SLOT::kFX01};

  [[nodiscard]] static Settings& get_singleton() noexcept
  {
    static Settings instance;
    return instance;
  }

  void load()
  {

    logger::info("Load settings");

    constexpr auto path_to_config = "Data/SKSE/Plugins/RaceCompatibilityCondition.toml";

    constexpr std::string_view section = "RaceCompatilbility"sv;
    constexpr std::string_view GetSetRaceHookKey = "GetSetRaceHook"sv;
    // const auto bDisableHookWhileRaceMenuOpenVar = "bDisableHookWhileRaceMenuOpen"s;
    constexpr std::string_view RuntimePatchRacesStatKey = "RuntimePatchRacesStat"sv;
    constexpr std::string_view DisableVanillaRacesKey = "DisableVanillaRaces"sv;
    constexpr std::string_view RuntimePatchArmorAddonsKey = "RuntimePatchArmorAddons"sv;
    constexpr std::string_view SlotsArrayAsWhiteListKey = "SlotsArrayAsWhiteList"sv;
    constexpr std::string_view SlotsArrayKey = "SlotsArray"sv;

    auto tbl = toml::parse_file(path_to_config);
    get_set_race_hook = tbl[section][GetSetRaceHookKey].value_or(get_set_race_hook);
    runtime_patch_races_stat = tbl[section][RuntimePatchRacesStatKey].value_or(runtime_patch_races_stat);
    disable_vanilla_races = tbl[section][DisableVanillaRacesKey].value_or(disable_vanilla_races);
    runtime_patch_armor_addons = tbl[section][RuntimePatchArmorAddonsKey].value_or(runtime_patch_armor_addons);
    slots_array_as_white_list = tbl[section][SlotsArrayAsWhiteListKey].value_or(slots_array_as_white_list);
    if (const toml::array* slots_node_array = tbl[section][SlotsArrayKey].as_array()) {
      logger::info("Size of toml array elements: {}", slots_node_array->size());
      slots_array.clear();
      if (!slots_node_array->empty()) {
        logger::info("Toml array not empty");

        slots_node_array->for_each([this](auto&& slot) {
          if constexpr (toml::is_number<decltype(slot)>) {
            logger::info("Toml raed array value: {}", *slot);
            slots_array.push_back(static_cast<SLOT>(1 << static_cast<std::int64_t>(*slot - 30)));
          }
        });
      }
    }

    std::sort(slots_array.begin(), slots_array.end());
    slots_array.erase(std::unique(slots_array.begin(), slots_array.end()), slots_array.end());

    if (first_load) {
      logger::info("First load...");
      first_load = false;
      last_slots_array.clear();
      for (const auto slot : slots_array) {
        last_slots_array.push_back(slot);
      }
    }

    for (const auto slot : last_slots_array) {
      logger::info("Defined slot in last array: {}", static_cast<std::int64_t>(slot));
    }

    if (last_slots_array != slots_array) {
      logger::info("Last slots != Slots, repatch");
      ready_to_repatch_armor_addons = true;
      last_slots_array.clear();
      for (const auto slot : slots_array) {
        logger::info("Add slot {} to last array", static_cast<std::int64_t>(slot));
        last_slots_array.push_back(slot);
      }
    }

    for (const auto slot : slots_array) {
      logger::info("Defined slot in array: {}", static_cast<std::int64_t>(slot));
    }

    logger::info("Finish load settings");
  }
};

struct RaceInfo final
{
  RE::TESRace* proxy_race;
  std::vector<RE::TESRace*> proxy_alt_races;
  RE::TESRace* mod_race;
  std::vector<RE::TESRace*> mod_alt_races;
  RE::TESRace* stat_race;
  std::vector<RE::TESRace*> stat_alt_races;
  bool patch_base_race_stat;
  bool patch_alt_races_stat;
  static constexpr std::string_view SECTION_NAME = "RaceInfo"sv;
  static constexpr std::string_view PROXY_RACE_FORM_ID_KEY = "ProxyRaceFormId"sv;
  static constexpr std::string_view STAT_RACE_FORM_ID_KEY = "StatRaceFormId"sv;
  static constexpr std::string_view PROXY_ALT_RACES_FORM_ID_KEY = "ProxyAltRacesFormId"sv;
  static constexpr std::string_view STAT_ALT_RACES_FORM_ID_KEY = "StatAltRacesFormId"sv;
  static constexpr std::string_view PROXY_RACE_MOD_NAME_KEY = "ProxyRaceModName"sv;
  static constexpr std::string_view STAT_RACE_MOD_NAME_KEY = "StatRaceModName"sv;
  static constexpr std::string_view PROXY_ALT_RACES_MOD_NAME_KEY = "ProxyAltRacesModName"sv;
  static constexpr std::string_view STAT_ALT_RACES_MOD_NAME_KEY = "StatAltRacesModName"sv;
  static constexpr std::string_view MOD_RACE_FORM_ID_KEY = "ModRaceFormId"sv;
  static constexpr std::string_view MOD_ALT_RACES_FORM_ID_KEY = "ModAltRacesFormId"sv;
  static constexpr std::string_view MOD_RACE_MOD_NAME_KEY = "ModRaceModName"sv;
  static constexpr std::string_view MOD_ALT_RACES_MOD_NAME_KEY = "ModAltRacesModName"sv;
  static constexpr std::string_view VANILLA_RACE_FORM_ID_KEY = "RaceFormId"sv;
  static constexpr std::string_view VANILLA_RACE_MOD_NAME_KEY = "RaceModName"sv;
  static constexpr std::string_view PATCH_BASE_RACE_STATS = "PatchBaseRaceStats"sv;
  static constexpr std::string_view PATCH_ALT_RACES_STATS = "PatchAltRacesStats"sv;
};

struct ArmorAddonInfo final
{
  RE::TESObjectARMA* armor_addon;
  std::uint16_t write_count;
};

/*
 * Входит в сет рейс - Подменять расу только тогда, если запомнена модовая раса (определить путем перебора всех мод
 * рас), а выставляется раса которая ее прокси { return RaceInfo.mod_race; } Входит в гет рейс - Подменять расу только
 * тогда, если запомнена модовая раса (определить путем перебора всех мод рас), и актор имеет эту модовую расу { return
 * RaceInfo.proxy_race; } входит в гет ис рейс - Подменять расу только тогда, если в аргументе укахана модовая раса
 * (определить путем перебора для мод рас) { return RaceInfo.proxy_race; }
 */

static std::string path_to_mod_race_tomls = "Data/SKSE/Plugins/RaceCompatibilityCondition/ModRace";
static std::string path_to_vanilla_race_tomls = "Data/SKSE/Plugins/RaceCompatibilityCondition/VanillaRace";

// Mod esp with formlists
static const std::string RCC_ESP = "RaceCompatibilityCondition.esp"s;

static std::vector<RE::TESRace*> vanilla_races{};
static std::vector<RaceInfo> race_infos{};
static std::vector<ArmorAddonInfo> armor_addon_infos{};

static auto get_is_race_address_call_01 = REL::RelocationID(21691, 22173).address() + REL::Relocate(0x68, 0x68);
static auto set_race_papyrus_address_call_01 = REL::RelocationID(53934, 54758).address() + REL::Relocate(0x23, 0x23);
static auto get_is_race_address_call_02 = REL::RelocationID(21691, 22179).address() + REL::Relocate(0x66, 0x66);
static auto get_is_race_address_jmp_01 = REL::ID(21034).address() + 0x7;
static REL::Relocation<uintptr_t> get_is_race_address_table{REL::RelocationID(668606, 361561)};
static REL::Relocation<uintptr_t> get_pc_is_race_address_table{REL::RelocationID(668982, 361937)};
static REL::Relocation<uintptr_t> get_race_base_papyrus_address{REL::RelocationID(55449, 56004)};
static REL::Relocation<uintptr_t> get_race_papyrus_address{REL::RelocationID(54134, 54930)};

static bool get_is_race_call_01(RE::Actor* actor, RE::TESRace* race, void* a3, double* ans);
static bool get_is_race_call_02(RE::Actor* actor, RE::TESRace* race, void* a3, double* ans);
static bool get_is_race_jmp_01(RE::Actor* actor, RE::TESRace* race, void* a3, double* ans);
static bool get_is_race_table(RE::Actor* actor, RE::TESRace* race, void* a3, double* ans);
static bool get_pc_is_race_table(RE::Actor* actor, RE::TESRace* race, void* a3, double* ans);
static void set_race_papyrus_call_01(RE::Actor* actor, RE::TESRace* race, bool is_player);

static inline REL::Relocation<decltype(get_is_race_call_01)> get_is_race_call_01_;
static inline REL::Relocation<decltype(get_is_race_call_02)> get_is_race_call_02_;
static inline REL::Relocation<decltype(get_is_race_jmp_01)> get_is_race_jmp_01_;
static inline REL::Relocation<decltype(get_is_race_table)> get_is_race_table_;
static inline REL::Relocation<decltype(get_pc_is_race_table)> get_pc_is_race_table_;
static inline REL::Relocation<decltype(set_race_papyrus_call_01)> set_race_papyrus_call_01_;

// god bless Nukem for this
template<typename Func>
static auto write_hook_generic(uintptr_t ID, size_t ByteCopyCount, Func Dest)
{
  auto& trampoline = SKSE::GetTrampoline();

  const REL::Relocation<std::uintptr_t> target{ID};
  const auto opcode = *reinterpret_cast<uint8_t*>(target.address());

  if (opcode == 0xE8) {
    return trampoline.write_call<5>(target.address(), Dest);
  } else if (opcode == 0xE9) {
    return trampoline.write_branch<5>(target.address(), Dest);
  } else {
    struct Patch : Xbyak::CodeGenerator
    {
      explicit Patch(uintptr_t OriginalFuncAddr, size_t OriginalByteLength)
      {
        // Hook returns here. Execute the restored bytes and jump back to the
        // original function.
        for (size_t i = 0; i < OriginalByteLength; i++)
          db(*reinterpret_cast<uint8_t*>(OriginalFuncAddr + i));

        jmp(qword[rip]);
        dq(OriginalFuncAddr + OriginalByteLength);
      }
    };

    Patch p(target.address(), ByteCopyCount);
    p.ready();

    trampoline.write_branch<5>(target.address(), Dest);

    auto alloc = trampoline.allocate(p.getSize());
    memcpy(alloc, p.getCode(), p.getSize());

    return reinterpret_cast<uintptr_t>(alloc);
  }
}

static bool race_is_in_vector(const RE::TESRace* comparer_race, const std::vector<RE::TESRace*> races)
{
  if (!comparer_race) {
    return false;
  }

  for (const auto race_to_comparer : races) {
    if (comparer_race == race_to_comparer) {
      return true;
    }
  }

  return false;
}

static std::tuple<RE::TESRace*, int> get_alt_race(const RE::TESRace* comparer_race,
                                                  const std::vector<RE::TESRace*>& races)
{
  if (!comparer_race) {
    return {nullptr, -1};
  }

  for (int i = 0; i < races.size(); i++) {
    if (comparer_race == races.at(i)) {
      return {races.at(i), i};
    }
  }
  return {nullptr, -1};
};

// Actor::SetRace call in papyrus SetRace function hook
static void set_race_papyrus_call_01(RE::Actor* actor, RE::TESRace* race, bool is_player)
{
  // get and set only for player
  if (is_player && actor && race) {

    if (const auto memo_race = Serialization::get_race()) {

      logger::info("set_race_papyrus_call_01 races actor {} race {}",
                   actor->GetActorRuntimeData().race->GetFormEditorID(),
                   race->GetFormEditorID());

      for (const auto& race_info : race_infos) {
        if ((race_info.mod_race == memo_race || race_is_in_vector(memo_race, race_info.mod_alt_races)) &&
            (race_info.proxy_race == race || race_is_in_vector(race, race_info.proxy_alt_races))) {
          auto [alt_race, alt_race_index] = get_alt_race(race, race_info.proxy_alt_races);
          logger::info("SetRace Swap race: Proxy {} Mod {}  Mem {}",
                       race_info.proxy_race->GetFormEditorID(),
                       race_info.mod_race->GetFormEditorID(),
                       memo_race->GetFormEditorID());
          if (alt_race_index != -1) {
            logger::info("SetRace AltRace: {}", alt_race->GetFormEditorID());
          }
          // swap race here
          auto race_for_swap =
              (race_info.proxy_race == race) ? race_info.proxy_race : race_info.mod_alt_races.at(alt_race_index);
          logger::info("SetRace race_for_swap: {}", race_for_swap->GetFormEditorID());
          return set_race_papyrus_call_01_(actor, race_for_swap, is_player);
        }
      }
    }
  }
  return set_race_papyrus_call_01_(actor, race, is_player);
}

// Actor.GetRace papyrus function hook
static RE::TESRace* get_race_papyrus(void*, void*, RE::Actor* actor)
{
  // get and set only for player
  if (actor && actor->IsPlayerRef()) {

    if (const auto race = actor->GetActorRuntimeData().race) {

      logger::info("get_race_papyrus races actor race {}", race->GetFormEditorID());

      if (const auto memo_race = Serialization::get_race()) {

        for (const auto& race_info : race_infos) {
          if ((race_info.mod_race == memo_race && race_info.mod_race == race) ||
              (race_is_in_vector(memo_race, race_info.mod_alt_races) &&
               race_is_in_vector(race, race_info.mod_alt_races))) {
            auto [alt_race, alt_race_index] = get_alt_race(memo_race, race_info.mod_alt_races);
            logger::info("GetRace Swap race: Proxy {} Mod {}  Mem {}",
                         race_info.proxy_race->GetFormEditorID(),
                         race_info.mod_race->GetFormEditorID(),
                         memo_race->GetFormEditorID());
            if (alt_race_index != -1) {
              logger::info("GetRace AltRace: {}", alt_race->GetFormEditorID());
            }
            // swap race here
            auto race_for_swap =
                (race_info.mod_race == race) ? race_info.proxy_race : race_info.proxy_alt_races.at(alt_race_index);
            logger::info("GetRace race_for_swap: {}", race_for_swap->GetFormEditorID());
            return race_for_swap;
          }
        }
      }
    }
  }

  if (actor) {
    return actor->GetRace();
  }
  return nullptr;
}

// ActorBase.GetRace papyrus function hook
static RE::TESRace* get_race_base_papyrus(void*, void*, RE::TESNPC* actor)
{
  if (actor && actor->IsPlayer()) {

    if (const auto race = actor->race) {

      logger::info("get_race_base_papyrus races actor race {}", actor->race->GetFormEditorID());

      if (const auto memo_race = Serialization::get_race()) {

        for (const auto& race_info : race_infos) {
          if ((race_info.mod_race == memo_race && race_info.mod_race == race) ||
              (race_is_in_vector(memo_race, race_info.mod_alt_races) &&
               race_is_in_vector(race, race_info.mod_alt_races))) {
            auto [alt_race, alt_race_index] = get_alt_race(memo_race, race_info.mod_alt_races);
            logger::info("GetBaseRace Swap race: Proxy {} Mod {}  Mem {}",
                         race_info.proxy_race->GetFormEditorID(),
                         race_info.mod_race->GetFormEditorID(),
                         memo_race->GetFormEditorID());
            if (alt_race_index != -1) {
              logger::info("GetBaseRace AltRace: {}", alt_race->GetFormEditorID());
            }
            // swap race here
            auto race_for_swap =
                (race_info.mod_race == race) ? race_info.proxy_race : race_info.proxy_alt_races.at(alt_race_index);
            logger::info("GetBaseRace race_for_swap: {}", race_for_swap->GetFormEditorID());
            return race_for_swap;
          }
        }
      }
    }
  }
  if (actor) {
    return actor->GetRace();
  }
  return nullptr;
}

// GetIsRace game condition patch
static bool get_is_race_call_01(RE::Actor* actor, RE::TESRace* race, void* a3, double* ans)
{
  if (race && actor && actor->GetActorRuntimeData().race) {

    const auto actor_race = actor->GetActorRuntimeData().race;
    // logger::info("Check races actor {} race {}", actor_race->GetFormEditorID(), race->GetFormEditorID());

    for (const auto& race_info : race_infos) {
      if ((race_info.mod_race == actor_race && race_info.proxy_race == race) ||
          (race_is_in_vector(actor_race, race_info.mod_alt_races) &&
           race_is_in_vector(race, race_info.proxy_alt_races))) {
        auto [alt_race, alt_race_index] = get_alt_race(actor_race, race_info.mod_alt_races);
        // swap function argument here
        auto race_for_swap = (race_info.proxy_race == race) ? race_info.mod_race : alt_race;
        return get_is_race_call_01_(actor, race_for_swap, a3, ans);
      }
    }
  }
  return get_is_race_call_01_(actor, race, a3, ans);
}

static bool get_is_race_call_02(RE::Actor* actor, RE::TESRace* race, void* a3, double* ans)
{
  if (race && actor && actor->GetActorRuntimeData().race) {

    const auto actor_race = actor->GetActorRuntimeData().race;
    // logger::info("Check races actor {} race {}", actor_race->GetFormEditorID(), race->GetFormEditorID());

    for (const auto& race_info : race_infos) {
      if ((race_info.mod_race == actor_race && race_info.proxy_race == race) ||
          (race_is_in_vector(actor_race, race_info.mod_alt_races) &&
           race_is_in_vector(race, race_info.proxy_alt_races))) {
        auto [alt_race, alt_race_index] = get_alt_race(actor_race, race_info.mod_alt_races);
        // swap function argument here
        auto race_for_swap = (race_info.proxy_race == race) ? race_info.mod_race : alt_race;
        return get_is_race_call_02_(actor, race_for_swap, a3, ans);
      }
    }
  }
  return get_is_race_call_02_(actor, race, a3, ans);
}

static bool get_is_race_jmp_01(RE::Actor* actor, RE::TESRace* race, void* a3, double* ans)
{
  if (race && actor && actor->GetActorRuntimeData().race) {

    const auto actor_race = actor->GetActorRuntimeData().race;
    // logger::info("Check races actor {} race {}", actor_race->GetFormEditorID(), race->GetFormEditorID());

    for (const auto& race_info : race_infos) {
      if ((race_info.mod_race == actor_race && race_info.proxy_race == race) ||
          (race_is_in_vector(actor_race, race_info.mod_alt_races) &&
           race_is_in_vector(race, race_info.proxy_alt_races))) {
        auto [alt_race, alt_race_index] = get_alt_race(actor_race, race_info.mod_alt_races);
        // swap function argument here
        auto race_for_swap = (race_info.proxy_race == race) ? race_info.mod_race : alt_race;
        return get_is_race_jmp_01_(actor, race_for_swap, a3, ans);
      }
    }
  }
  return get_is_race_jmp_01_(actor, race, a3, ans);
}

static bool get_is_race_table(RE::Actor* actor, RE::TESRace* race, void* a3, double* ans)
{
  if (race && actor && actor->GetActorRuntimeData().race) {

    const auto actor_race = actor->GetActorRuntimeData().race;
    // logger::info("Check races actor {} race {}", actor_race->GetFormEditorID(), race->GetFormEditorID());

    for (const auto& race_info : race_infos) {
      if ((race_info.mod_race == actor_race && race_info.proxy_race == race) ||
          (race_is_in_vector(actor_race, race_info.mod_alt_races) &&
           race_is_in_vector(race, race_info.proxy_alt_races))) {
        auto [alt_race, alt_race_index] = get_alt_race(actor_race, race_info.mod_alt_races);
        logger::info("GetIsRace Swap race: Proxy {} Mod {}",
                     race_info.proxy_race->GetFormEditorID(),
                     race_info.mod_race->GetFormEditorID());
        if (alt_race_index != -1) {
          logger::info("GetIsRace AltRace: {}", alt_race->GetFormEditorID());
        }
        // swap function argument here
        auto race_for_swap = (race_info.proxy_race == race) ? race_info.mod_race : alt_race;
        logger::info("GetIsRace race_for_swap: {}", race_for_swap->GetFormEditorID());
        return get_is_race_table_(actor, race_for_swap, a3, ans);
      }
    }
  }
  return get_is_race_table_(actor, race, a3, ans);
}

static bool get_pc_is_race_table(RE::Actor* actor, RE::TESRace* race, void* a3, double* ans)
{
  if (race && actor && actor->GetActorRuntimeData().race) {

    const auto actor_race = actor->GetActorRuntimeData().race;
    // logger::info("Check races actor {} race {}", actor_race->GetFormEditorID(), race->GetFormEditorID());

    for (const auto& race_info : race_infos) {
      if ((race_info.mod_race == actor_race && race_info.proxy_race == race) ||
          (race_is_in_vector(actor_race, race_info.mod_alt_races) &&
           race_is_in_vector(race, race_info.proxy_alt_races))) {
        auto [alt_race, alt_race_index] = get_alt_race(actor_race, race_info.mod_alt_races);
        logger::info("GetPCIsRace Swap race: Proxy {} Mod {}",
                     race_info.proxy_race->GetFormEditorID(),
                     race_info.mod_race->GetFormEditorID());
        if (alt_race_index != -1) {
          logger::info("GetPCIsRace AltRace: {}", alt_race->GetFormEditorID());
        }
        // swap function argument here
        auto race_for_swap = (race_info.proxy_race == race) ? race_info.mod_race : alt_race;
        logger::info("GetPCIsRace race_for_swap: {}", race_for_swap->GetFormEditorID());
        return get_pc_is_race_table_(actor, race_for_swap, a3, ans);
      }
    }
  }
  return get_pc_is_race_table_(actor, race, a3, ans);
}

static void runtime_patch_armor(const Settings& settings)
{

  const auto check_slots = [](const RE::TESObjectARMO* check_armor, const Settings& settings) -> bool {
    for (const auto slot : settings.slots_array) {
      // logger::info("Check slot {} for armor: {}", static_cast<std::int64_t>(slot), armor->GetFormEditorID());
      if (settings.slots_array_as_white_list && check_armor->bipedModelData.bipedObjectSlots.any(slot)) {
        return true;
      }
      if (!settings.slots_array_as_white_list && check_armor->bipedModelData.bipedObjectSlots.any(slot)) {
        return false;
      }
    }
    return false;
  };

  logger::info("Start runtime patch armor");

  auto data_handler = RE::TESDataHandler::GetSingleton();
  auto [forms_map, lock] = RE::TESForm::GetAllForms();

  if (!data_handler || !forms_map) {
    return;
  }

  for (const auto& armor_addon_info : armor_addon_infos) {
    if (!armor_addon_info.armor_addon) {
      continue;
    }

    for (std::uint16_t index = 0; index < armor_addon_info.write_count; index++) {
      logger::info("REMOVED RACES FROM: {} INDEX {}", armor_addon_info.armor_addon->GetFormEditorID(), index);
      armor_addon_info.armor_addon->additionalRaces.pop_back();
    }
  }

  armor_addon_infos.clear();

  const auto nord_race = data_handler->LookupForm<RE::TESRace>(0x13746, "Skyrim.esm");

  for (auto [key, form] : *forms_map) {

    if (!form) {
      continue;
    }

    const auto armor = form->As<RE::TESObjectARMO>();
    if (!armor) {
      continue;
    }

    if (armor->templateArmor) {
      continue;
    }

    if (check_slots(armor, settings)) {

      auto armor_addons = armor->armorAddons;
      for (const auto& race_info : race_infos) {

        RE::TESObjectARMA* armor_addon_to_add = nullptr;
        bool can_add = true;
        for (auto armor_addon : armor_addons) {

          if (!armor_addon) {
            continue;
          }

          for (auto armor_race : armor_addon->additionalRaces) {

            if (!armor_race) {
              continue;
            }

            if (armor_race == nord_race) {
              armor_addon_to_add = armor_addon;
            }

            if (race_info.mod_race == armor_race) {
              can_add = false;
            }

            if (!can_add) {
              break;
            }
          }

          if (!can_add) {
            break;
          }
        }

        if (can_add && armor_addon_to_add) {

          auto armor_info_opt =
              [](const RE::TESObjectARMA* check_armor_addon) -> std::optional<std::reference_wrapper<ArmorAddonInfo>> {
            for (auto& armor_info : armor_addon_infos) {
              if (armor_info.armor_addon == check_armor_addon) {
                return std::reference_wrapper{armor_info};
              }
            }
            return std::nullopt;
          }(armor_addon_to_add);

          if (armor_info_opt.has_value()) {
            auto& armor_info_ref = armor_info_opt.value().get();
            logger::info(
                "FOUND WITH INFO: Add races to: {} Count {}", armor->GetFullName(), armor_info_ref.write_count);
            armor_info_ref.armor_addon->additionalRaces.push_back(race_info.mod_race);
            for (auto alt_race : race_info.mod_alt_races) {
              armor_info_ref.armor_addon->additionalRaces.push_back(alt_race);
              armor_info_ref.write_count = armor_info_ref.write_count + 1;
            }
            armor_info_ref.write_count = armor_info_ref.write_count + 1;
          } else {
            logger::info("FOUND NEW INFO: Add races to: {}", armor->GetFullName());
            armor_addon_to_add->additionalRaces.push_back(race_info.mod_race);
            std::uint16_t count = 1;
            for (auto alt_race : race_info.mod_alt_races) {
              armor_addon_to_add->additionalRaces.push_back(alt_race);
              count = count + 1;
            }
            armor_addon_infos.push_back(ArmorAddonInfo{armor_addon_to_add, count});
          }
        }
      }
    }
  }

  logger::info("Finish runtime patch armor");
}

static void load_data()
{
  logger::info("Start load data");
  auto data_handler = RE::TESDataHandler::GetSingleton();
  if (data_handler) {

    namespace fs = std::filesystem;

    const auto parse_toml_to_race_info = [](const std::string& path_to_toml,
                                            RE::TESDataHandler* tes_data_handler) -> std::optional<RaceInfo> {
      logger::info("Read toml: {}", path_to_toml);

      auto tbl = toml::parse_file(path_to_toml);

      const auto proxy_race_form_id = tbl[RaceInfo::SECTION_NAME][RaceInfo::PROXY_RACE_FORM_ID_KEY].value_or(0x799);
      const auto proxy_race_mod_name =
          tbl[RaceInfo::SECTION_NAME][RaceInfo::PROXY_RACE_MOD_NAME_KEY].value_or("Skyrim.esm"s);
      const auto mod_race_form_id = tbl[RaceInfo::SECTION_NAME][RaceInfo::MOD_RACE_FORM_ID_KEY].value_or(0x799);
      const auto mod_race_mod_name =
          tbl[RaceInfo::SECTION_NAME][RaceInfo::MOD_RACE_MOD_NAME_KEY].value_or("Skyrim.esm"s);
      const auto patch_base_race_stats = tbl[RaceInfo::SECTION_NAME][RaceInfo::PATCH_BASE_RACE_STATS].value_or(false);
      const auto patch_alt_race_stats =
          tbl[RaceInfo::SECTION_NAME][RaceInfo::PATCH_ALT_RACES_STATS].value_or(false);

      logger::info("Read proxy race from data Id {} Mod {}", proxy_race_form_id, proxy_race_mod_name);
      RE::TESRace* proxy_race = tes_data_handler->LookupForm<RE::TESRace>(proxy_race_form_id, proxy_race_mod_name);

      logger::info("Read mod race from data Id {} Mod {}", mod_race_form_id, mod_race_mod_name);
      RE::TESRace* mod_race = tes_data_handler->LookupForm<RE::TESRace>(mod_race_form_id, mod_race_mod_name);

      RE::TESRace* stat_base_race;

      if (patch_base_race_stats) {
        const auto patch_race_form_id = tbl[RaceInfo::SECTION_NAME][RaceInfo::STAT_RACE_FORM_ID_KEY].value_or(0x799);
        const auto patch_race_mod_name =
            tbl[RaceInfo::SECTION_NAME][RaceInfo::STAT_RACE_MOD_NAME_KEY].value_or("Skyrim.esm"s);
        logger::info("Read patch race from data Id {} Mod {}", patch_race_form_id, patch_race_mod_name);
        stat_base_race = tes_data_handler->LookupForm<RE::TESRace>(patch_race_form_id, patch_race_mod_name);
      } else {
        stat_base_race = proxy_race;
      }

      const auto get_form_id_vector = [&tbl](const std::string_view key) -> std::vector<RE::FormID> {
        auto alt_races_ids = std::vector<RE::FormID>{};
        if (const toml::array* alt_races_id_array = tbl[RaceInfo::SECTION_NAME][key].as_array()) {
          logger::info("Size of toml array {}: {}",key, alt_races_id_array->size());
          if (!alt_races_id_array->empty()) {
            logger::info("Toml array {} not empty",key);

            alt_races_id_array->for_each([&alt_races_ids, key](auto&& form_id) {
              if constexpr (toml::is_number<decltype(form_id)>) {
                logger::info("Toml raed array value {}: {}",key, *form_id);
                alt_races_ids.push_back(static_cast<RE::FormID>(*form_id));
              }
            });
          }
        }
        return alt_races_ids;
      };
      
      const auto get_mod_name_vector = [&tbl](const std::string_view key) -> std::vector<std::string> {
        auto alt_races_mods = std::vector<std::string>{};
        if (const toml::array* alt_races_mods_array = tbl[RaceInfo::SECTION_NAME][key].as_array()) {
          logger::info("Size of toml array {}: {}",key, alt_races_mods_array->size());
          if (!alt_races_mods_array->empty()) {
            logger::info("Toml array {} not empty",key);

            alt_races_mods_array->for_each([&alt_races_mods, key](auto&& mod_name) {
              if constexpr (toml::is_string<decltype(mod_name)>) {
                logger::info("Toml raed array value {}: {}",key, *mod_name);
                alt_races_mods.push_back(static_cast<std::string>(*mod_name));
              }
            });
          }
        }
        return alt_races_mods;
      };

      const auto get_races_vector = [](RE::TESDataHandler* data, const std::vector<RE::FormID>& form_ids, const std::vector<std::string>& mod_names) -> std::vector<RE::TESRace*> {
        auto races = std::vector<RE::TESRace*>{};
        if (form_ids.empty() || mod_names.empty() || (form_ids.size() != mod_names.size())) {
          logger::info("mod_names and form_ids array mismatch");
          return races;
        }
        for (int index = 0; index < mod_names.size(); index++) {
          logger::info("Read alt race from data Id {} Mod {}", form_ids.at(index), mod_names.at(index));
          auto race = data->LookupForm<RE::TESRace>(form_ids.at(index), mod_names.at(index));
          if (race) {
            races.push_back(race);
          }
        }
        return races;
      };

      auto proxy_alt_races_id = get_form_id_vector(RaceInfo::PROXY_ALT_RACES_FORM_ID_KEY);
      auto proxy_alt_races_mod_name = get_mod_name_vector(RaceInfo::PROXY_ALT_RACES_MOD_NAME_KEY);
      auto proxy_alt_races = get_races_vector(tes_data_handler, proxy_alt_races_id, proxy_alt_races_mod_name);

      auto mod_alt_races_id = get_form_id_vector(RaceInfo::MOD_ALT_RACES_FORM_ID_KEY);
      auto mod_alt_races_mod_name = get_mod_name_vector(RaceInfo::MOD_ALT_RACES_MOD_NAME_KEY);
      auto mod_alt_races = get_races_vector(tes_data_handler, mod_alt_races_id, mod_alt_races_mod_name);
      
      std::vector<RE::TESRace*> stat_alt_races{};

      if (patch_alt_race_stats) {
        auto stat_alt_races_id = get_form_id_vector(RaceInfo::STAT_ALT_RACES_FORM_ID_KEY);
        auto stat_alt_races_mod_name = get_mod_name_vector(RaceInfo::STAT_ALT_RACES_MOD_NAME_KEY);
        stat_alt_races = get_races_vector(tes_data_handler, stat_alt_races_id, stat_alt_races_mod_name);
      } else {
        std::ranges::copy(proxy_alt_races.begin(), proxy_alt_races.end(), std::back_inserter(stat_alt_races));
      }

      const auto is_vectors_size_equals = proxy_alt_races.size() == mod_alt_races.size() == stat_alt_races.size();
      if (!is_vectors_size_equals) {
        logger::info("Vectors of races is not equals");
      }

      if (proxy_race && mod_race && stat_base_race && is_vectors_size_equals) {
        return RaceInfo{proxy_race,
                        proxy_alt_races,
                        mod_race,
                        mod_alt_races,
                        stat_base_race,
                        stat_alt_races,
                        patch_base_race_stats,
                        patch_alt_race_stats};
      }

      logger::info("Proxy or mod race is null");
      return std::nullopt;
    };

    for (const auto& toml_entry : fs::directory_iterator(path_to_vanilla_race_tomls)) {
      const auto toml_path = toml_entry.path().string();

      if (toml_path.ends_with(".toml")) {

        logger::info("Read toml: {}", toml_path);

        auto tbl = toml::parse_file(toml_path);
        const auto vanilla_race_form_id =
            tbl[RaceInfo::SECTION_NAME][RaceInfo::VANILLA_RACE_FORM_ID_KEY].value_or(0x799);
        const auto vanilla_race_mod_name =
            tbl[RaceInfo::SECTION_NAME][RaceInfo::VANILLA_RACE_MOD_NAME_KEY].value_or("Skyrim.esm"s);

        logger::info("Read vanilla race from data Id {} Mod {}", vanilla_race_form_id, vanilla_race_mod_name);
        auto vanilla_race = data_handler->LookupForm<RE::TESRace>(vanilla_race_form_id, vanilla_race_mod_name);

        if (vanilla_race) {
          logger::info("Race {} added to vanilla races", vanilla_race->GetFormEditorID());
          vanilla_races.push_back(vanilla_race);
        }
      }
    }

    const auto race_is_mod_race = [](const RE::TESRace* race) -> bool {
      if (!race) {
        return false;
      }
      for (const auto& race_info : race_infos) {
        if (race_info.mod_race == race) {
          return true;
        }
      }
      return false;
    };

    for (const auto& toml_entry : fs::directory_iterator(path_to_mod_race_tomls)) {
      const auto toml_path = toml_entry.path().string();

      if (toml_path.ends_with(".toml")) {
        auto race_info = parse_toml_to_race_info(toml_path, data_handler);

        if (race_info.has_value()) {
          if (!race_is_mod_race(race_info.value().mod_race)) {
            logger::info("Add races: Proxy {} Mod {} Stat {} ProxyAltSize {} ModAltSize {} StatAltSize {}",
                         race_info.value().proxy_race->GetFormEditorID(),
                         race_info.value().mod_race->GetFormEditorID(),
                         race_info.value().stat_race->GetFormEditorID(),
                         race_info.value().proxy_alt_races.size(),
                         race_info.value().mod_alt_races.size(),
                         race_info.value().stat_alt_races.size());
            race_infos.push_back(race_info.value());
          } else {
            logger::info("Race: {} is already exist in array", race_info.value().mod_race->GetFormEditorID());
          }
        }
      }
    }
  }
  logger::info("Finish load data");
}

static void install_hooks()
{
  logger::info("Start install hooks");
  auto& trampoline = SKSE::GetTrampoline();
  trampoline.create(1024);
  get_is_race_call_01_ = SKSE::GetTrampoline().write_call<5>(get_is_race_address_call_01, get_is_race_call_01);
  get_is_race_call_02_ = SKSE::GetTrampoline().write_call<5>(get_is_race_address_call_02, get_is_race_call_02);

  // only skyrim se has
  if (REL::Module::GetRuntime() == REL::Module::Runtime::SE) {
    get_is_race_jmp_01_ = SKSE::GetTrampoline().write_branch<5>(get_is_race_address_jmp_01, get_is_race_jmp_01);
  }

  set_race_papyrus_call_01_ =
      SKSE::GetTrampoline().write_call<5>(set_race_papyrus_address_call_01, set_race_papyrus_call_01);
  get_is_race_table_ = get_is_race_address_table.write_vfunc(0x0, get_is_race_table);
  get_pc_is_race_table_ = get_pc_is_race_address_table.write_vfunc(0x0, get_pc_is_race_table);
  write_hook_generic(get_race_base_papyrus_address.address(), 8, get_race_base_papyrus);
  write_hook_generic(get_race_papyrus_address.address(), 8, get_race_papyrus);

  logger::info("Finish install hooks");
}

static void runtime_race_patch()
{

  const auto patch_race = [](const RE::TESRace* based_race, RE::TESRace* patched_race) {
    logger::info("Patch races: {} = {}", patched_race->GetFormEditorID(), based_race->GetFormEditorID());
    patched_race->attackAnimationArrayMap[RE::SEXES::kFemale] = based_race->attackAnimationArrayMap[RE::SEXES::kFemale];
    patched_race->attackAnimationArrayMap[RE::SEXES::kMale] = based_race->attackAnimationArrayMap[RE::SEXES::kMale];
    patched_race->attackDataMap = based_race->attackDataMap;
    patched_race->data = based_race->data;
    based_race->ForEachKeyword([patched_race](RE::BGSKeyword* keyword) {
      if (keyword && !patched_race->HasKeyword(keyword)) {
        patched_race->AddKeyword(keyword);
      }
      return RE::BSContainer::ForEachResult::kContinue;
    });
    patched_race->actorEffects = based_race->actorEffects;
    patched_race->validEquipTypes = based_race->validEquipTypes;
  };

  logger::info("Start runtime patch races");
  for (const auto& race_info : race_infos) {

    if (race_info.patch_base_race_stat) {
      patch_race(race_info.stat_race, race_info.mod_race);
    }

    if (race_info.patch_alt_races_stat) {
      for (int race_index = 0; race_index < race_info.mod_alt_races.size(); race_index++) {
        patch_race(race_info.stat_alt_races.at(race_index), race_info.mod_alt_races.at(race_index));
      }
    }
  }
  logger::info("Finish runtime patch races");
}

static void runtime_disable_vanilla_race()
{
  logger::info("Start runtime disable vanilla races");
  for (auto vanilla_race : vanilla_races) {
    if (vanilla_race->GetPlayable()) {
      logger::info("Disable race: {}", vanilla_race->GetFormEditorID());
      vanilla_race->data.flags.reset(RE::RACE_DATA::Flag::kPlayable);
    }
  }
  logger::info("Finish runtime disable vanilla races");
}

struct OnPlayerUpdate final
{

  OnPlayerUpdate() = delete;
  ~OnPlayerUpdate() = delete;
  OnPlayerUpdate(const OnPlayerUpdate& other) = delete;
  OnPlayerUpdate(OnPlayerUpdate&& other) noexcept = delete;
  OnPlayerUpdate& operator=(const OnPlayerUpdate& other) = delete;
  OnPlayerUpdate& operator=(OnPlayerUpdate&& other) noexcept = delete;

  private:
  static inline REL::Relocation<uintptr_t> update_pc_address_{REL::RelocationID(261916, 208040)};
  static inline auto update_pc_offset_{REL::Relocate(0xAD, 0xAD)};

  static auto update_pc(RE::PlayerCharacter* this_, float delta) -> void
  {

    static float timer_100_ms = 0.100f;

    if (this_ && timer_100_ms <= 0.f) {

      timer_100_ms = 0.100f;
      const auto race = this_->GetRace();
      if (race && race->GetFile(0)) {
        for (const auto& race_info : race_infos) {
          if (race_info.mod_race == race || race_is_in_vector(race, race_info.mod_alt_races)) {
            Serialization::race_form_id = race->GetLocalFormID();
            strcpy_s(Serialization::race_file_name, race->GetFile(0)->fileName);
            return update_pc_(this_, delta);
          }
        }
        for (const auto vanilla_race : vanilla_races) {
          if (vanilla_race == race) {
            Serialization::race_form_id = race->GetLocalFormID();
            strcpy_s(Serialization::race_file_name, race->GetFile(0)->fileName);
            return update_pc_(this_, delta);
          }
        }
      }
    }

    timer_100_ms -= delta;

    return update_pc_(this_, delta);
  }

  static inline REL::Relocation<decltype(update_pc)> update_pc_;

  public:
  static auto install_hook() -> void
  {
    logger::info("start hook update pc"sv);
    update_pc_ = update_pc_address_.write_vfunc(update_pc_offset_, update_pc);
    logger::info("finish hook update pc"sv);
  }
};

} // namespace RaceCompatibilityCondition
