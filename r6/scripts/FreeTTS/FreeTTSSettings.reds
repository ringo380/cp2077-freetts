// FreeTTS settings. Shown under Settings > Mods (and in Mod Configuration
// Menu, which bridges Mod Settings) when the Mod Settings plugin is
// installed. Without it the defaults below apply: everything on, rate 0.
//
// Rate is per list on purpose: dialogue choices are timed, quickhacks are not.
module FreeTTS

// The Voice selector. Mod Settings can only show fixed names, and Windows
// voices are only known at runtime, so the entries are slots: Voice n is the
// nth line of the plugin's startup voice list. Changing it speaks the
// slot's real name (OnModSettingsChange below), which is the preview.
//
// The slot is only the pick. The plugin remembers the chosen voice by name
// (FreeTTS_SelectVoice) and every list asks it for the voice to use
// (FreeTTS_SavedVoice), because Windows renumbers the list whenever a voice
// is added, removed, or made the default; a stored number would silently
// become a different voice. After such a change the number shown here is
// stale until the next pick, but the voice heard is still the one chosen.
enum FreeTTSVoiceSlot {
  Default = 0,
  Voice1 = 1,
  Voice2 = 2,
  Voice3 = 3,
  Voice4 = 4,
  Voice5 = 5,
  Voice6 = 6,
  Voice7 = 7,
  Voice8 = 8,
  Voice9 = 9,
  Voice10 = 10,
  Voice11 = 11,
  Voice12 = 12,
}

// Where speech goes. Automatic hands it to NVDA whenever NVDA is running, so
// a screen reader user hears it in their own reader voice and speed and it
// does not talk over the reader; otherwise the Windows voice speaks.
enum FreeTTSOutput {
  Automatic = 0,
  WindowsVoice = 1,
}

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
  @runtimeProperty("ModSettings.description", "Which installed voice speaks. Voice n is the nth voice in the plugin's startup log; on apply the mod says the chosen voice's name in that voice and remembers it by name, so the number shown can go stale after Windows adds or removes voices.")
  @runtimeProperty("ModSettings.displayValues.Default", "Windows default")
  @runtimeProperty("ModSettings.displayValues.Voice1", "Voice 1")
  @runtimeProperty("ModSettings.displayValues.Voice2", "Voice 2")
  @runtimeProperty("ModSettings.displayValues.Voice3", "Voice 3")
  @runtimeProperty("ModSettings.displayValues.Voice4", "Voice 4")
  @runtimeProperty("ModSettings.displayValues.Voice5", "Voice 5")
  @runtimeProperty("ModSettings.displayValues.Voice6", "Voice 6")
  @runtimeProperty("ModSettings.displayValues.Voice7", "Voice 7")
  @runtimeProperty("ModSettings.displayValues.Voice8", "Voice 8")
  @runtimeProperty("ModSettings.displayValues.Voice9", "Voice 9")
  @runtimeProperty("ModSettings.displayValues.Voice10", "Voice 10")
  @runtimeProperty("ModSettings.displayValues.Voice11", "Voice 11")
  @runtimeProperty("ModSettings.displayValues.Voice12", "Voice 12")
  public let voice: FreeTTSVoiceSlot = FreeTTSVoiceSlot.Default;

  @runtimeProperty("ModSettings.mod", "FreeTTS")
  @runtimeProperty("ModSettings.category", "Voice")
  @runtimeProperty("ModSettings.category.order", "3")
  @runtimeProperty("ModSettings.displayName", "Output")
  @runtimeProperty("ModSettings.description", "Automatic speaks through NVDA whenever it is running (in your NVDA voice and speed, and on a braille display), otherwise through the Windows voice. Windows voice ignores NVDA.")
  @runtimeProperty("ModSettings.displayValues.Automatic", "Automatic")
  @runtimeProperty("ModSettings.displayValues.WindowsVoice", "Windows voice")
  public let output: FreeTTSOutput = FreeTTSOutput.Automatic;

  // The voice number last seen, so the preview speaks only on a change and
  // never at load. Not a setting: no ModSettings properties.
  private let m_previewedVoice: Int32 = 0;

  // The live instance; the one Mod Settings writes into. Never `new` this.
  public static func Get(game: GameInstance) -> ref<FreeTTSSettings> {
    return GameInstance.GetScriptableSystemsContainer(game).Get(n"FreeTTS.FreeTTSSettings") as FreeTTSSettings;
  }

  private func OnAttach() -> Void {
    // Register first: the class listener writes the stored value into the
    // field, and the baseline must be that value. Taken before it, the
    // baseline is the compiled default, and the next apply from any mod
    // would read the stored number as a change and re-save it, even when
    // the menu's number is stale after a renumber (ACCEPTANCE 48).
    this.RegisterWithModSettings();
    this.m_previewedVoice = EnumInt(this.voice);
    // Windows default then means a voice for the game's text language, when
    // one is installed.
    FreeTTS_SetLanguage(FreeTTS_GameLanguage());
    FreeTTS_SetOutput(EnumInt(this.output));
  }

  // Mod Settings calls this on apply, for any mod's settings. A changed
  // voice is handed to the plugin to remember by name, then previewed by
  // saying its slot and name in that voice, so a player who cannot read
  // the list can pick by ear. Rate 0, the voice's own speed. A slot past
  // the end is not remembered (SelectVoice declines) and the preview says
  // so; the plugin keeps whatever it had.
  public func OnModSettingsChange() -> Void {
    FreeTTS_SetOutput(EnumInt(this.output));
    let n: Int32 = EnumInt(this.voice);
    if n == this.m_previewedVoice { return; };
    this.m_previewedVoice = n;
    FreeTTS_SelectVoice(n);
    FreeTTS_PreviewVoice(FreeTTS_VoicePreview(n), n);
  }

  private func OnDetach() -> Void {
    this.UnregisterFromModSettings();
  }

  // Only one branch of each pair compiles, depending on whether the Mod
  // Settings scripts are present. check-reds must run both ways.
  @if(ModuleExists("ModSettingsModule"))
  private func RegisterWithModSettings() -> Void {
    // The class listener writes accepted values into the fields; the
    // modifications listener is what fires OnModSettingsChange.
    ModSettings.RegisterListenerToClass(this);
    ModSettings.RegisterListenerToModifications(this);
  }

  @if(!ModuleExists("ModSettingsModule"))
  private func RegisterWithModSettings() -> Void {}

  @if(ModuleExists("ModSettingsModule"))
  private func UnregisterFromModSettings() -> Void {
    ModSettings.UnregisterListenerToModifications(this);
    ModSettings.UnregisterListenerToClass(this);
  }

  @if(!ModuleExists("ModSettingsModule"))
  private func UnregisterFromModSettings() -> Void {}
}
