// FreeTTS - reads shards aloud.
//
// Shard popup: Read on the "shard collected" notification, or Read on a shard
// in the inventory, opens ShardNotificationController, which fills its title
// and text in OnInitialize and pauses the game. The wrap reads back what it
// displayed. The ways out (close, pause menu, crack) go through Close().
//
// Journal > Shards: moving through the list speaks each row's title (the
// item controller's OnSelected, mouse hover or gamepad). Opening a shard ends
// in ShardsMenuGameController.OnShardSelectedEvent, which fills the right
// pane through CodexEntryViewController.ShowEntry; the wrap reads the pane
// back, since the description can be an input-device override rather than
// the entry's own text.
//
// An encrypted shard's text is a hex dump, so only its title is read; the
// game's own title already ends in "(Encrypted)" in every language.
//
// Closing the popup or leaving the Shards tab stops the reading unless the
// "Keep reading after closing" setting is on. Stopping is an empty
// FreeTTS_Speak, which purges the Windows voice and cancels NVDA.

import FreeTTS.FreeTTSSettings

public func FreeTTS_ShardsEnabled() -> Bool {
  let settings: ref<FreeTTSSettings> = FreeTTSSettings.Get(GetGameInstance());
  return !IsDefined(settings) || settings.shardsEnabled;
}

public func FreeTTS_SpeakShard(phrase: String) -> Void {
  let settings: ref<FreeTTSSettings> = FreeTTSSettings.Get(GetGameInstance());
  let rate: Int32 = IsDefined(settings) ? settings.shardRate : 0;
  if !FreeTTS_Speak(phrase, rate, FreeTTS_Voice()) {
    FTLogWarning("[FreeTTS] no voice available, not spoken: " + phrase);
  }
}

// A shard screen closed: cut the reading off, unless the player chose to
// keep listening while they play.
public func FreeTTS_StopShard() -> Void {
  let settings: ref<FreeTTSSettings> = FreeTTSSettings.Get(GetGameInstance());
  if IsDefined(settings) && (!settings.shardsEnabled || settings.shardKeepReading) {
    return;
  }
  FreeTTS_Speak("", 0, FreeTTS_Voice());
}

// "<title>. <text>", or the title alone for an encrypted shard or one with
// no text.
public func FreeTTS_BuildShardPhrase(title: String, text: String, isCrypted: Bool) -> String {
  if isCrypted || Equals(text, "") {
    return title;
  }
  return title + ". " + text;
}

// ---------------------------------------------------------------------------
// Pickup popup

@wrapMethod(ShardNotificationController)
protected cb func OnInitialize() -> Bool {
  let result: Bool = wrappedMethod();
  if FreeTTS_ShardsEnabled() {
    FreeTTS_SpeakShard(FreeTTS_BuildShardPhrase(
      inkTextRef.GetText(this.m_titleRef), inkTextRef.GetText(this.m_longTextRef), this.m_data.isCrypted));
  }
  return result;
}

@wrapMethod(ShardNotificationController)
private final func Close() -> Void {
  wrappedMethod();
  FreeTTS_StopShard();
}

// Also on teardown, in case the popup is taken down without Close(). A
// second stop right after the first is harmless.
@wrapMethod(ShardNotificationController)
protected cb func OnUninitialize() -> Bool {
  let result: Bool = wrappedMethod();
  FreeTTS_StopShard();
  return result;
}

// ---------------------------------------------------------------------------
// Journal > Shards

// Hash of the shard last read in full. Opening a shard for the first time
// marks it visited, and the menu answers that by selecting and opening the
// same row again (OnEntryVisitedUpdate); this keeps that second open quiet.
// Highlighting a different row clears it, so moving away and back reads the
// shard again. Lives on the list's shared sync object, which both the menu
// and every row can reach.
@addField(CodexListSyncData)
public let m_freeTtsReadHash: Int32;

// The row last highlighted (hash and title, since group rows share a hash
// scheme with nothing). Re-selecting the same row, which the menu does after
// a first open and when a group folds, is neither re-announced nor allowed
// to cut off the shard being read.
@addField(CodexListSyncData)
public let m_freeTtsHoverKey: String;

// The shard the right pane now shows, as displayed. The title can be a bare
// localization key; GetLocalizedText resolves those and leaves text alone.
@addMethod(CodexEntryViewController)
public func FreeTTS_ShownPhrase() -> String {
  let shard: ref<ShardEntryData> = this.m_data as ShardEntryData;
  let isCrypted: Bool = IsDefined(shard) && shard.m_isCrypted;
  return FreeTTS_BuildShardPhrase(GetLocalizedText(inkTextRef.GetText(this.m_titleText)),
    GetLocalizedText(inkTextRef.GetText(this.m_descriptionText)), isCrypted);
}

@wrapMethod(ShardsMenuGameController)
protected cb func OnShardSelectedEvent(evt: ref<ShardSelectedEvent>) -> Bool {
  let result: Bool = wrappedMethod(evt);
  // A group row only folds or unfolds; its title was spoken on highlight.
  if evt.m_group || !FreeTTS_ShardsEnabled() {
    return result;
  }
  if evt.m_entryHash == this.m_activeData.m_freeTtsReadHash {
    return result;
  }
  this.m_activeData.m_freeTtsReadHash = evt.m_entryHash;
  FreeTTS_SpeakShard(this.m_entryViewController.FreeTTS_ShownPhrase());
  return result;
}

@wrapMethod(ShardsMenuGameController)
protected cb func OnUninitialize() -> Bool {
  let result: Bool = wrappedMethod();
  FreeTTS_StopShard();
  return result;
}

// A highlighted row: its title as the list shows it, and for a group the
// number of shards in it, as the list's "(n)".
@wrapMethod(ShardItemVirtualController)
protected cb func OnSelected(itemController: wref<inkVirtualCompoundItemController>, discreteNav: Bool) -> Bool {
  let result: Bool = wrappedMethod(itemController, discreteNav);
  if !FreeTTS_ShardsEnabled() || !IsDefined(this.m_entryData) {
    return result;
  }
  let phrase: String = GetLocalizedText(this.m_entryData.m_title);
  if IsDefined(this.m_activeItemSync) {
    let key: String = IntToString(this.m_entryData.m_hash) + ":" + phrase;
    if Equals(key, this.m_activeItemSync.m_freeTtsHoverKey) {
      return result;
    }
    this.m_activeItemSync.m_freeTtsHoverKey = key;
    this.m_activeItemSync.m_freeTtsReadHash = 0;
  }
  if this.m_nestedListData.m_isHeader {
    phrase += ", " + IntToString(this.m_entryData.m_counter);
  }
  FreeTTS_SpeakShard(phrase);
  return result;
}
