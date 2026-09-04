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

import FreeTTS.FreeTTSSettings

// Provided by red4ext/plugins/FreeTTS/FreeTTS.dll. rate is SAPI's -10..10.
public native func FreeTTS_Speak(text: String, rate: Int32) -> Bool;
public native func FreeTTS_IsReady() -> Bool;

// ---------------------------------------------------------------------------
// Quickhacks

// The last phrase spoken while the panel has been open. The game re-selects
// the current row on every RAM tick and cooldown change; without this the same
// hack would be announced over and over mid-combat.
@addField(QuickhacksListGameController)
private let m_freeTtsLastPhrase: String;

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
  let rate: Int32 = IsDefined(settings) ? settings.quickhackRate : 0;
  if !FreeTTS_Speak(phrase, rate) {
    FTLogWarning("[FreeTTS] no voice available, not spoken: " + phrase);
  }
}

// Closing the panel forgets the last phrase, so reopening it announces the
// first row again even when it is the same hack as before.
@wrapMethod(QuickhacksListGameController)
private final func SetVisibility(value: Bool) -> Void {
  wrappedMethod(value);
  if !value {
    this.m_freeTtsLastPhrase = "";
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
  let phrase: String = NotEquals(tags, "") ? tags + ", " + text : text;
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
  if !FreeTTS_Speak(phrase, rate) {
    FTLogWarning("[FreeTTS] no voice available, not spoken: " + phrase);
  }
}
