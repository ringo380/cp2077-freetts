// FreeTTS - speaks what is highlighted on the world map.
//
// Map pins: every highlighted pin, by mouse hover or gamepad cursor, ends in
// WorldMapMenuGameController.OnSelectedMappinChanged, which fills the tooltip.
// The tooltip controller already works out the display title for every pin
// type (fast travel point, quest, shop, vehicle offer, "Undiscovered" ...),
// so the wrap reads back the title and description it just set rather than
// re-deriving them. Hiding the tooltip forgets the last phrase, so hovering
// the same pin again speaks it again.
//
// Districts: OnUpdateHoveredDistricts fires when the hovered district or
// subdistrict changes while the map is zoomed out far enough to show them.

import FreeTTS.FreeTTSSettings

public func FreeTTS_MapEnabled() -> Bool {
  let settings: ref<FreeTTSSettings> = FreeTTSSettings.Get(GetGameInstance());
  return !IsDefined(settings) || settings.mapEnabled;
}

public func FreeTTS_SpeakMap(phrase: String) -> Void {
  let settings: ref<FreeTTSSettings> = FreeTTSSettings.Get(GetGameInstance());
  let rate: Int32 = IsDefined(settings) ? settings.mapRate : 0;
  if !FreeTTS_Speak(phrase, rate) {
    FTLogWarning("[FreeTTS] no voice available, not spoken: " + phrase);
  }
}

// ---------------------------------------------------------------------------
// Map pins

// Last pin phrase spoken while a tooltip has been showing. Lives on the base
// class so the Hide wraps below can clear it for every tooltip kind.
@addField(WorldMapTooltipBaseController)
private let m_freeTtsLastPhrase: String;

// "<title>[, <description>]" from the text the tooltip just displayed. The
// title can still be a bare localization key for some pin types; passing it
// through GetLocalizedText resolves those and leaves plain text alone.
@addMethod(WorldMapTooltipController)
private func FreeTTS_SpeakTooltip() -> Void {
  if !FreeTTS_MapEnabled() {
    return;
  }
  let phrase: String = GetLocalizedText(inkTextRef.GetText(this.m_titleText));
  if Equals(phrase, "") {
    return;
  }
  if inkTextRef.IsVisible(this.m_descText) {
    let desc: String = GetLocalizedText(inkTextRef.GetText(this.m_descText));
    if NotEquals(desc, "") && NotEquals(desc, "None") && NotEquals(desc, phrase) {
      phrase += ", " + desc;
    }
  }
  if Equals(phrase, this.m_freeTtsLastPhrase) {
    return;
  }
  this.m_freeTtsLastPhrase = phrase;
  FreeTTS_SpeakMap(phrase);
}

@wrapMethod(WorldMapTooltipController)
public func SetData(const data: script_ref<WorldMapTooltipData>, menu: ref<WorldMapMenuGameController>) -> Void {
  wrappedMethod(data, menu);
  this.FreeTTS_SpeakTooltip();
}

// The police tooltip overrides SetData without calling the base, so it needs
// its own wrap; the helper is inherited.
@wrapMethod(WorldMapPoliceTooltipController)
public func SetData(const data: script_ref<WorldMapTooltipData>, menu: ref<WorldMapMenuGameController>) -> Void {
  wrappedMethod(data, menu);
  this.FreeTTS_SpeakTooltip();
}

@wrapMethod(WorldMapTooltipBaseController)
public func Hide() -> Void {
  wrappedMethod();
  this.m_freeTtsLastPhrase = "";
}

@wrapMethod(WorldMapTooltipBaseController)
public func HideInstant(opt force: Bool) -> Void {
  wrappedMethod(force);
  this.m_freeTtsLastPhrase = "";
}

// ---------------------------------------------------------------------------
// Districts

@addField(WorldMapMenuGameController)
private let m_freeTtsLastDistrict: String;

// "<district>[, <subdistrict>]", with Dogtown named the way the map names it.
// Nothing for the Invalid district (cursor off the city).
public func FreeTTS_BuildDistrictPhrase(district: gamedataDistrict, subdistrict: gamedataDistrict) -> String {
  if Equals(district, gamedataDistrict.Invalid) {
    return "";
  }
  let phrase: String;
  if Equals(district, gamedataDistrict.Dogtown) {
    phrase = GetLocalizedText("LocKey#10946");
  } else {
    let record: wref<District_Record> = MappinUtils.GetDistrictRecord(district);
    if IsDefined(record) {
      phrase = GetLocalizedText(record.LocalizedName());
    }
  }
  let subRecord: wref<District_Record> = MappinUtils.GetDistrictRecord(subdistrict);
  if IsDefined(subRecord) {
    let subName: String = GetLocalizedText(subRecord.LocalizedName());
    if NotEquals(subName, "") && NotEquals(subName, phrase) {
      phrase += Equals(phrase, "") ? subName : ", " + subName;
    }
  }
  return phrase;
}

@wrapMethod(WorldMapMenuGameController)
protected cb func OnUpdateHoveredDistricts(district: gamedataDistrict, subdistrict: gamedataDistrict) -> Bool {
  let result: Bool = wrappedMethod(district, subdistrict);
  if !FreeTTS_MapEnabled() {
    return result;
  }
  let phrase: String = FreeTTS_BuildDistrictPhrase(district, subdistrict);
  if NotEquals(phrase, "") && NotEquals(phrase, this.m_freeTtsLastDistrict) {
    this.m_freeTtsLastDistrict = phrase;
    FreeTTS_SpeakMap(phrase);
  }
  return result;
}
