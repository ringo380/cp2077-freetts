// FreeTTS settings. Shown under Settings > Mods (and in Mod Configuration
// Menu, which bridges Mod Settings) when the Mod Settings plugin is
// installed. Without it the defaults below apply: everything on, rate 0.
//
// Rate is per list on purpose: dialogue choices are timed, quickhacks are not.
module FreeTTS

public class FreeTTSSettings extends ScriptableSystem {
  @runtimeProperty("ModSettings.mod", "FreeTTS")
  @runtimeProperty("ModSettings.category", "Quickhacks")
  @runtimeProperty("ModSettings.category.order", "0")
  @runtimeProperty("ModSettings.displayName", "Speak quickhacks")
  @runtimeProperty("ModSettings.description", "Speak the highlighted quickhack as you cycle the list.")
  public let quickhacksEnabled: Bool = true;

  @runtimeProperty("ModSettings.mod", "FreeTTS")
  @runtimeProperty("ModSettings.category", "Quickhacks")
  @runtimeProperty("ModSettings.category.order", "0")
  @runtimeProperty("ModSettings.displayName", "Quickhack speech rate")
  @runtimeProperty("ModSettings.description", "0 is the Windows voice's own speed. Negative is slower, positive is faster.")
  @runtimeProperty("ModSettings.step", "1")
  @runtimeProperty("ModSettings.min", "-10")
  @runtimeProperty("ModSettings.max", "10")
  @runtimeProperty("ModSettings.dependency", "quickhacksEnabled")
  public let quickhackRate: Int32 = 0;

  @runtimeProperty("ModSettings.mod", "FreeTTS")
  @runtimeProperty("ModSettings.category", "Dialogue")
  @runtimeProperty("ModSettings.category.order", "1")
  @runtimeProperty("ModSettings.displayName", "Speak dialogue choices")
  @runtimeProperty("ModSettings.description", "Speak the highlighted dialogue choice as you move through the options.")
  public let dialogueEnabled: Bool = true;

  @runtimeProperty("ModSettings.mod", "FreeTTS")
  @runtimeProperty("ModSettings.category", "Dialogue")
  @runtimeProperty("ModSettings.category.order", "1")
  @runtimeProperty("ModSettings.displayName", "Dialogue speech rate")
  @runtimeProperty("ModSettings.description", "Timed choices may need a faster voice. 0 is the Windows voice's own speed.")
  @runtimeProperty("ModSettings.step", "1")
  @runtimeProperty("ModSettings.min", "-10")
  @runtimeProperty("ModSettings.max", "10")
  @runtimeProperty("ModSettings.dependency", "dialogueEnabled")
  public let dialogueRate: Int32 = 0;

  @runtimeProperty("ModSettings.mod", "FreeTTS")
  @runtimeProperty("ModSettings.category", "Map")
  @runtimeProperty("ModSettings.category.order", "2")
  @runtimeProperty("ModSettings.displayName", "Speak map")
  @runtimeProperty("ModSettings.description", "Speak the highlighted map pin, and the district under the cursor when zoomed out.")
  public let mapEnabled: Bool = true;

  @runtimeProperty("ModSettings.mod", "FreeTTS")
  @runtimeProperty("ModSettings.category", "Map")
  @runtimeProperty("ModSettings.category.order", "2")
  @runtimeProperty("ModSettings.displayName", "Map speech rate")
  @runtimeProperty("ModSettings.description", "0 is the Windows voice's own speed. Negative is slower, positive is faster.")
  @runtimeProperty("ModSettings.step", "1")
  @runtimeProperty("ModSettings.min", "-10")
  @runtimeProperty("ModSettings.max", "10")
  @runtimeProperty("ModSettings.dependency", "mapEnabled")
  public let mapRate: Int32 = 0;

  @runtimeProperty("ModSettings.mod", "FreeTTS")
  @runtimeProperty("ModSettings.category", "Voice")
  @runtimeProperty("ModSettings.category.order", "3")
  @runtimeProperty("ModSettings.displayName", "Voice")
  @runtimeProperty("ModSettings.description", "0 uses the Windows default voice. 1 and up pick a voice by its number in the plugin's log (voice 1, voice 2, ...). Takes effect on the next thing spoken.")
  @runtimeProperty("ModSettings.step", "1")
  @runtimeProperty("ModSettings.min", "0")
  @runtimeProperty("ModSettings.max", "20")
  public let voice: Int32 = 0;

  // The live instance; the one Mod Settings writes into. Never `new` this.
  public static func Get(game: GameInstance) -> ref<FreeTTSSettings> {
    return GameInstance.GetScriptableSystemsContainer(game).Get(n"FreeTTS.FreeTTSSettings") as FreeTTSSettings;
  }

  private func OnAttach() -> Void {
    this.RegisterWithModSettings();
  }

  private func OnDetach() -> Void {
    this.UnregisterFromModSettings();
  }

  // Only one branch of each pair compiles, depending on whether the Mod
  // Settings scripts are present. check-reds must run both ways.
  @if(ModuleExists("ModSettingsModule"))
  private func RegisterWithModSettings() -> Void {
    ModSettings.RegisterListenerToClass(this);
  }

  @if(!ModuleExists("ModSettingsModule"))
  private func RegisterWithModSettings() -> Void {}

  @if(ModuleExists("ModSettingsModule"))
  private func UnregisterFromModSettings() -> Void {
    ModSettings.UnregisterListenerToClass(this);
  }

  @if(!ModuleExists("ModSettingsModule"))
  private func UnregisterFromModSettings() -> Void {}
}
