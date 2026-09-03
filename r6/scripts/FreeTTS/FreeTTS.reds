// FreeTTS - speaks the highlighted quickhack aloud as the list is cycled.
//
// Every highlight change in the quickhack list (keyboard, wheel, gamepad, and
// the game's own re-selection after a RAM or cooldown change) goes through
// QuickhacksListGameController.SelectData, so that single wrap covers combat,
// scanner and device hacks alike.

// Provided by red4ext/plugins/FreeTTS/FreeTTS.dll.
public native func FreeTTS_Speak(text: String) -> Bool;
public native func FreeTTS_IsReady() -> Bool;

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
  let phrase: String = FreeTTS_BuildPhrase(data);
  if Equals(phrase, this.m_freeTtsLastPhrase) {
    return;
  }
  this.m_freeTtsLastPhrase = phrase;
  if !FreeTTS_Speak(phrase) {
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
