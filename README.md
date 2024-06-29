## Universal solution for Custom Races with SKSE Plugin!
### Support 1.5.97 \ 1.6.640 \ 1.6.1170 Steam only I think (not tested with GOG and other releases)

The mod allows you to avoid patching scripts and esp to add compatibility to any custom race just by writing the race config once (yes, you can use any vampire mod and everything will work automatically without patches). The mod also has some more additional features that I will write about next. Confugurations for UBE is included.  

It is recommended to disable **RaceCompatibility** from nexus and all patches for it, however if you have plugins that require it, I am attaching a light version that does not edit vanilla records. I'm also attaching the latest version of esp **UBE** from nexus which also does not edit vanilla records. And patch for **Unofficial Wet Redux UBE Patch** (yes, patch for patch!).

[Google Drive with releases and other files.](https://drive.google.com/drive/folders/1lwWp4bOrbEFSkp78Kwjq4BIy_9Hy1Cda)  

#### Fast Install Guide for UBE Users:
If you're playing a vampire or werewolf, you'll probably need a new game.
1. Disable **RaceCompatibility** from nexus and all patches for it.
2. Download and install  **RaceCompatibilityCondition** from google disk.
3. Download and install **RaceCompatibilityLight** from google disk.
4. Download and install **UBE_AllRace** from google disk, overwrite original UBE esp.
5. If you have installed **UnofficialWetReduxUBEPatch**, download and install **UnofficialWetReduxUBEPatch_RaceCompatibilityCondition** with overwrite original scripts.

### Requirements:
[Address Library](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

### How it works.

Here is the standard config of the plugin itself:
```toml
[RaceCompatilbility]
GetSetRaceHook = true # Patch papyrus SetRace\ GetRace functions and patch GetIsRace game condition
RuntimePatchRacesStat = true # Set gameplay stats to mod race from stat race on game boot
DisableVanillaRaces = false # Uncheck playble flags from vanilla races on game boot
RuntimePatchArmorAddons = true # Add mod race to armor addons with specific slot if not added
SlotsArrayAsWhiteList = true # If set to true, patches armor addons for which at least one of slots from SlotsArray is present, otherwise patches armor addons for which none of the slots from SlotsArray are present
SlotsArray = [ 30, 31, 35, 36, 39, 40, 41, 42, 43, 47 ] # slot indexes to add \ exlude

# default array 30, 31, 35, 36, 39, 40, 41, 42, 43, 47

# use this slots index
# kHead = 30
# kHair = 31
# kBody = 32
# kHands = 33
# kForearms = 34
# kAmulet = 35
# kRing = 36
# kFeet = 37
# kCalves = 38
# kShield = 39
# kTail = 40
# kLongHair = 41
# kCirclet = 42
# kEars = 43
# kModMouth = 44
# kModNeck = 45
# kModChestPrimary = 46
# kModBack = 47
# kModMisc1 = 48
# kModPelvisPrimary = 49
# kDecapitateHead = 50
# kDecapitate = 51
# kModPelvisSecondary = 52
# kModLegRight = 53
# kModLegLeft = 54
# kModFaceJewelry = 55
# kModChestSecondary = 56
# kModShoulder = 57
# kModArmLeft = 58
# kModArmRight = 59
# kModMisc2 = 60
# kFX01 = 61 
```

Here's the config used for UBE_Breton
```toml
[RaceInfo]
HeadType = 0
ProxyRaceFormId = 0x13741
ProxyAltRacesFormId = [ 0x8883C ]
ProxyRaceModName = "Skyrim.esm"
ProxyAltRacesModName = [ "Skyrim.esm" ]
ModRaceFormId = 0x5734
ModAltRacesFormId = [ 0x5735 ]
ModRaceModName = "UBE_AllRace.esp"
ModAltRacesModName = [ "UBE_AllRace.esp" ]
PatchBaseRaceStats = true
StatRaceFormId = 0x13741
StatRaceModName = "Skyrim.esm"
PatchAltRacesStats = true
StatAltRacesFormId = [ 0x8883C ]
StatAltRacesModName = [ "Skyrim.esm" ]
```

So we have 2 papyrus functions and a game condition that we need to patch.
Functions:
```
Actor.SetRace
Actor.GetRace
ActorBase.GetRace
```
Game Conditions:
```
GetIsRace
GetPCIsRace
```

Patching these functions and condition is activated when **GetSetRaceHook** is enabled, in this case if you are using a mod race that the config is added for, the plugin will substitute that race in the calls. For example when Papyrus requests **GetRace** for a player who plays as **UBE_Breton** , it will get **Breton** instead of **UBE_Breton** and works with it, based on this it may for example decide to assign BretonVampire race to the player, then **SetRace(BretonVampire)** will be called, but the player will get the race specified in the config, and it will be **UBE_BretonVampire** . With the game condition **GetIsRace** the same thing will happen, the game will get **Breton** instead of **UBE_Breton** and will check by this race (this, for example, automatically activates all racial dialogs and so on and so forth).

**GetRace** \ **SetRace** works only for a player. **GetIsRace** works for everyone.

### Let's break down the basic config
```toml
[RaceCompatilbility]
GetSetRaceHook = true # Patch papyrus SetRace\ GetRace functions and patch GetIsRace game condition
RuntimePatchRacesStat = true # Set gameplay stats to mod race from stat race on game boot
DisableVanillaRaces = false # Uncheck playble flags from vanilla races on game boot
RuntimePatchArmorAddons = true # Add mod race to armor addons with specific slot if not added
SlotsArrayAsWhiteList = true # If set to true, patches armor addons for which at least one one slot from SlotsArray is present, otherwise patches armor addons for which none of the slots from SlotsArray are present
SlotsArray = [ 30, 31, 35, 36, 39, 40, 41, 42, 43, 47 ] # slot indexes to add \ exlude
```
Basically everything is clear here by the name and comments, let's focus on the functionality of **RuntimePatchArmorAddons** and **RuntimePatchRacesStat**.  

**RuntimePatchRacesStat** - Transfers from main race such characteristics as skill values, health - magic - stamina, spells \ abilities, keywords (but it adds those keywords which are not present on the race from the mod, does not replace keywords which are already present on the race) and attack data of the race.  

**RuntimePatchArmorAddons** - automatically adds races from the config to those armor addons that do not have them. Fully compatible with **ArmoryDataManipulator** because it is executed after it. You can also change the slots in the array, and in the game to open close the main menu (that on ESC), will update armor addons, and they will be patched already based on the new values.  


### Now let's look at the **UBE_Breton** race config.
```toml
[RaceInfo]
HeadType = 0
ProxyRaceFormId = 0x13741
ProxyAltRacesFormId = [ 0x8883C ]
ProxyRaceModName = "Skyrim.esm"
ProxyAltRacesModName = [ "Skyrim.esm" ]
ModRaceFormId = 0x5734
ModAltRacesFormId = [ 0x5735 ]
ModRaceModName = "UBE_AllRace.esp"
ModAltRacesModName = [ "UBE_AllRace.esp" ]
PatchBaseRaceStats = true
StatRaceFormId = 0x13741
StatRaceModName = "Skyrim.esm"
PatchAltRacesStats = true
StatAltRacesFormId = [ 0x8883C ]
StatAltRacesModName = [ "Skyrim.esm" ]
```

**HeadType** - Can take values 0 (**Human**), 1 (**Mer\Elf**), 2 (**Khajit**), 3 (**Argonian**). This is necessary for more correct work of **RuntimePatchArmorAddons** , select value based on the modded race's head type.  

**ProxyRaceFormId** and **ProxyRaceModName** - FormID of the race that will substitute for the custom race, as well as the name of the mod in which to look for this FormID.  

**ProxyAltRacesFormId** and **ProxyAltRacesModName** - Same thing, but this specifies the vampire analog of the race, or other similar races that work on the same principle (for example, in one lich mod each race has its own lich race).  

**ModRaceFormId** and **ModRaceModName** - This is simple, the FormID of the custom race and the name of the mod it is in.  

**ModAltRacesFormId** and **ModAltRacesModName** - Same thing, but for the vampire analogs of the custom race and so on.  

**PatchBaseRaceStats** and **PatchAltRacesStats** - Works if **RuntimePatchRacesStat** is enabled in the main config. Enables or disables this patching of stats for the custom race and for the "vampire" custom race.  

**StatRaceFormId** and **StatRaceModName** - This is the actual race from which to take stats for the custom race when patching.  

**StatAltRacesFormId** and **StatAltRacesModName** - The same, but for the vampire race already.  


Configurations of custom races are located in **SKSE\Plugins\RaceCompatibilityCondition\ModRace**, the name does not matter, the main thing is **.toml** extension.

### Papyrus extension

The plugin also provides 2 papyrus functions to bypass the patch, in case you specifically need to know exactly what race a player is, etc.
```papyrus
Scriptname RaceCompatibilityCondition Hidden

Race Function GetRaceActorBase(ActorBase actor) global native

Race Function GetRaceActor(Actor actor) global native
```
