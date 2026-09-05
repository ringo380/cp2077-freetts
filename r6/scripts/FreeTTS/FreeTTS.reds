// FreeTTS - speaks the highlighted quickhack or dialogue choice aloud as the
// player moves through the list.
//
// Quickhacks: every highlight change (keyboard, wheel, gamepad, and the game's
// own re-selection after a RAM or cooldown change) goes through
// QuickhacksListGameController.SelectData, so that single wrap covers combat,
// scanner and device hacks alike.
//
// Dialogue: the base class calls dialogWidgetGameController.OnInteractionsChanged
// on every hub-data, active-hub, and selected-index change, and that controller
// holds all four facts needed (the hubs, the active hub, the index, whether
// dialogue is open), so it is the single funnel for the choice list.

import FreeTTS.*

// Provided by red4ext/plugins/FreeTTS/FreeTTS.dll. rate is SAPI's -10..10;
// voice is 0 for the Windows default or a number from the plugin's voice list.
public native func FreeTTS_Speak(text: String, rate: Int32, voice: Int32) -> Bool;

// The voice every list shares; the rates are per list.
public func FreeTTS_Voice() -> Int32 {
  let settings: ref<FreeTTSSettings> = FreeTTSSettings.Get(GetGameInstance());
  return IsDefined(settings) ? EnumInt(settings.voice) : 0;
}
public native func FreeTTS_IsReady() -> Bool;

// The display name of voice n from the plugin's startup list ("Microsoft
// Zira Desktop - English (United States)"); 0 is the Windows default voice's
// name; "" past the end of the list or before the list exists.
public native func FreeTTS_VoiceName(voice: Int32) -> String;

// What the settings page says when the voice changes: the slot, then the
// name without its language tail. An unknown slot says so.
public func FreeTTS_VoicePreview(voice: Int32) -> String {
  let name: String = FreeTTS_VoiceName(voice);
  let cut: Int32 = StrFindFirst(name, " - ");
  if cut > 0 { name = StrLeft(name, cut); };
  if voice == 0 { return "Windows default" + (StrLen(name) > 0 ? ", " + name : ""); };
  if StrLen(name) == 0 { return "Voice " + IntToString(voice) + " is not installed"; };
  return "Voice " + IntToString(voice) + ", " + name;
}

// ---------------------------------------------------------------------------
// Quickhacks

// The last phrase spoken while the panel has been open. The game re-selects
// the current row on every RAM tick and cooldown change; without this the same
// hack would be announced over and over mid-combat.
@addField(QuickhacksListGameController)
private let m_freeTtsLastPhrase: String;

// A row highlighted while the panel is still hidden. The game selects row 0
// (PopulateData) before it shows the panel (SetVisibility), so the first row
// waits here and is spoken together with the RAM readout once the panel is
// actually on screen.
@addField(QuickhacksListGameController)
private let m_freeTtsPendingRow: String;

// Set when the panel became visible with no row waiting: the selection is
// arriving after the show instead of before it, so the next spoken row
// carries the RAM readout. The two wraps cover both orders.
@addField(QuickhacksListGameController)
private let m_freeTtsAnnounceRam: Bool;

// "RAM <current> of <max>", from the same stat and stat pool the panel's own
// memory bar reads.
public func FreeTTS_BuildRamPhrase(game: GameInstance, player: ref<GameObject>) -> String {
  let id: EntityID = player.GetEntityID();
  let max: Int32 = FloorF(GameInstance.GetStatsSystem(game).GetStatValue(Cast<StatsObjectID>(id), gamedataStatType.Memory));
  let current: Int32 = FloorF(GameInstance.GetStatPoolsSystem(game).GetStatPoolValue(Cast<StatsObjectID>(id), gamedataStatPoolType.Memory, false));
  return "RAM " + IntToString(current) + " of " + IntToString(max);
}

// "<title>, <cost> RAM[, locked[, <reason>]]". A row that only says there are
// no quickhacks is spoken as its title alone.
public func FreeTTS_BuildPhrase(data: ref<QuickhackData>) -> String {
  let phrase: String = GetLocalizedText(data.m_title);
  if data.m_noQuickhackData {
    return phrase;
  }
  phrase += ", " + IntToString(data.m_cost) + " RAM";
  if data.m_isLocked {
    phrase += ", locked";
    let reason: String = GetLocalizedText(data.m_inactiveReason);
    if NotEquals(reason, "") {
      phrase += ", " + reason;
    }
  }
  return phrase;
}

@wrapMethod(QuickhacksListGameController)
private final func SelectData(data: ref<QuickhackData>) -> Void {
  wrappedMethod(data);
  if !IsDefined(data) {
    return;
  }
  let settings: ref<FreeTTSSettings> = FreeTTSSettings.Get(GetGameInstance());
  if IsDefined(settings) && !settings.quickhacksEnabled {
    return;
  }
  let phrase: String = FreeTTS_BuildPhrase(data);
  if Equals(phrase, this.m_freeTtsLastPhrase) {
    return;
  }
  this.m_freeTtsLastPhrase = phrase;
  if !this.GetRootWidget().IsVisible() {
    this.m_freeTtsPendingRow = phrase;
    return;
  }
  if this.m_freeTtsAnnounceRam {
    this.m_freeTtsAnnounceRam = false;
    phrase = FreeTTS_BuildRamPhrase(GetGameInstance(), this.GetPlayerControlledObject()) + ", " + phrase;
  }
  let rate: Int32 = IsDefined(settings) ? settings.quickhackRate : 0;
  if !FreeTTS_Speak(phrase, rate, FreeTTS_Voice()) {
    FTLogWarning("[FreeTTS] no voice available, not spoken: " + phrase);
  }
}

// Opening the panel speaks the RAM readout and the row that was waiting for
// it as one phrase. Closing it forgets the last phrase, so reopening it
// announces the first row again even when it is the same hack as before.
// The vanilla SetVisibility(true) can decline to open (no target, panel
// blocked); the visibility of the root widget is what says it really did.
@wrapMethod(QuickhacksListGameController)
private final func SetVisibility(value: Bool) -> Void {
  let wasVisible: Bool = this.GetRootWidget().IsVisible();
  wrappedMethod(value);
  if !value {
    this.m_freeTtsLastPhrase = "";
    this.m_freeTtsPendingRow = "";
    this.m_freeTtsAnnounceRam = false;
    return;
  }
  if wasVisible || !this.GetRootWidget().IsVisible() {
    return;
  }
  let settings: ref<FreeTTSSettings> = FreeTTSSettings.Get(GetGameInstance());
  if IsDefined(settings) && !settings.quickhacksEnabled {
    this.m_freeTtsPendingRow = "";
    return;
  }
  if Equals(this.m_freeTtsPendingRow, "") {
    // No row yet: the selection follows the show. SelectData adds the RAM.
    this.m_freeTtsAnnounceRam = true;
    return;
  }
  let phrase: String = FreeTTS_BuildRamPhrase(GetGameInstance(), this.GetPlayerControlledObject())
    + ", " + this.m_freeTtsPendingRow;
  this.m_freeTtsPendingRow = "";
  let rate: Int32 = IsDefined(settings) ? settings.quickhackRate : 0;
  if !FreeTTS_Speak(phrase, rate, FreeTTS_Voice()) {
    FTLogWarning("[FreeTTS] no voice available, not spoken: " + phrase);
  }
}

// ---------------------------------------------------------------------------
// Dialogue choices

// Hub id, index and phrase of the last choice spoken. The controller rebuilds
// every hub on each change, and the same text can sit at the same index in a
// new hub, so the key carries all three.
@addField(dialogWidgetGameController)
private let m_freeTtsLastChoiceKey: String;

// What the player sees for one choice: "[Tag] text", with ", unavailable" for
// a greyed-out option. Mirrors the tag split in
// DialogHubLogicController.UpdateDialogHubData; localizedName is already
// localized.
public func FreeTTS_BuildChoicePhrase(choice: ListChoiceData) -> String {
  let tags: String = GetCaptionTagsFromArray(choice.captionParts.parts);
  let text: String = choice.localizedName;
  if Equals(tags, "") && StrBeginsWith(text, "[") {
    if StrSplitFirst(text, "]", tags, text) {
      tags = StrFrontToUpper(StrAfterFirst(tags, "["));
      if StrBeginsWith(text, " ") {
        text = StrAfterFirst(text, " ");
      }
    }
  }
  // "[Leave]" splits into a tag and no text; speak the tag alone then,
  // as SetChoiceText shows it alone.
  let phrase: String;
  if Equals(text, "") {
    phrase = tags;
  } else if Equals(tags, "") {
    phrase = text;
  } else {
    phrase = tags + ", " + text;
  }
  if ChoiceTypeWrapper.IsType(choice.type, gameinteractionsChoiceType.Inactive)
    || ChoiceTypeWrapper.IsType(choice.type, gameinteractionsChoiceType.CheckFailed) {
    phrase += ", unavailable";
  }
  return phrase;
}

@wrapMethod(dialogWidgetGameController)
protected func OnInteractionsChanged() -> Void {
  wrappedMethod();
  if !this.m_AreDialogsOpen {
    this.m_freeTtsLastChoiceKey = "";
    return;
  }
  let settings: ref<FreeTTSSettings> = FreeTTSSettings.Get(GetGameInstance());
  if IsDefined(settings) && !settings.dialogueEnabled {
    return;
  }
  // The hub the highlight lives in. A single-option prompt runs with no
  // active hub (id -1) and only one hub, so that one is the target then.
  let hubs: array<ListChoiceHubData> = this.m_data.choiceHubs;
  let hubIndex: Int32 = -1;
  let i: Int32 = 0;
  while i < ArraySize(hubs) {
    if hubs[i].id == this.m_activeHubID {
      hubIndex = i;
    }
    i += 1;
  }
  if hubIndex < 0 && ArraySize(hubs) == 1 {
    hubIndex = 0;
  }
  if hubIndex < 0 {
    return;
  }
  let hub: ListChoiceHubData = hubs[hubIndex];
  if this.m_selectedIndex < 0 || this.m_selectedIndex >= ArraySize(hub.choices) {
    return;
  }
  let phrase: String = FreeTTS_BuildChoicePhrase(hub.choices[this.m_selectedIndex]);
  let key: String = IntToString(hub.id) + ":" + IntToString(this.m_selectedIndex) + ":" + phrase;
  if Equals(key, this.m_freeTtsLastChoiceKey) {
    return;
  }
  this.m_freeTtsLastChoiceKey = key;
  let rate: Int32 = IsDefined(settings) ? settings.dialogueRate : 0;
  if !FreeTTS_Speak(phrase, rate, FreeTTS_Voice()) {
    FTLogWarning("[FreeTTS] no voice available, not spoken: " + phrase);
  }
}
