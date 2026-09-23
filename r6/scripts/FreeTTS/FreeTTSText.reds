// The few words FreeTTS adds around the game's own text ("locked", "RAM 8
// of 12"), in the language the game shows on screen. Everything else FreeTTS
// speaks is the game's own localized text already.
//
// Each language is one row of templates in FreeTTSTextKey order; {0} and {1}
// are filled by FreeTTS_Text. A language not listed here speaks English.
// To add or correct a language, edit its row: keep the {0}/{1} slots and the
// order, and use the game's own language code (the Text Language setting,
// e.g. "de-de"; only zh needs the region).

enum FreeTTSTextKey {
  RamOf = 0,          // quickhack panel opened: current RAM, then max RAM
  RamCost = 1,        // a quickhack's RAM cost
  Locked = 2,         // a quickhack that cannot be used
  Unavailable = 3,    // a greyed-out dialogue choice
  WindowsDefault = 4, // the Voice setting's first entry
  VoiceN = 5,         // the Voice setting's numbered entries
  VoiceMissing = 6,   // a Voice number past the end of the installed list
}

// The game's on-screen text language as the game spells it ("en-us",
// "de-de"). It can only change from the main menu, so a loaded game keeps
// one value.
public func FreeTTS_GameLanguage() -> String {
  let setting: ref<ConfigVarListName> = GameInstance.GetSettingsSystem(GetGameInstance())
    .GetVar(n"/language", n"OnScreen") as ConfigVarListName;
  if !IsDefined(setting) { return "en-us"; };
  return NameToString(setting.GetValue());
}

public func FreeTTS_Text(key: FreeTTSTextKey, opt a: String, opt b: String) -> String {
  let row: array<String> = FreeTTS_TextRow(FreeTTS_GameLanguage());
  let text: String = row[EnumInt(key)];
  text = StrReplace(text, "{0}", a);
  return StrReplace(text, "{1}", b);
}

private func FreeTTS_TextRow(language: String) -> array<String> {
  let code: String = StrLower(language);
  let base: String = StrLeft(code, 2);
  if Equals(base, "de") {
    return ["RAM {0} von {1}", "{0} RAM", "gesperrt", "nicht verfügbar",
            "Windows-Standard", "Stimme {0}", "Stimme {0} ist nicht installiert"];
  }
  if Equals(base, "fr") {
    return ["RAM {0} sur {1}", "{0} RAM", "verrouillé", "indisponible",
            "Voix Windows par défaut", "Voix {0}", "La voix {0} n'est pas installée"];
  }
  if Equals(base, "es") {
    return ["RAM {0} de {1}", "{0} de RAM", "bloqueado", "no disponible",
            "Voz predeterminada de Windows", "Voz {0}", "La voz {0} no está instalada"];
  }
  if Equals(base, "it") {
    return ["RAM {0} su {1}", "{0} RAM", "bloccato", "non disponibile",
            "Voce predefinita di Windows", "Voce {0}", "La voce {0} non è installata"];
  }
  if Equals(base, "pl") {
    return ["RAM {0} z {1}", "{0} RAM", "zablokowane", "niedostępne",
            "Domyślny głos Windows", "Głos {0}", "Głos {0} nie jest zainstalowany"];
  }
  if Equals(base, "pt") {
    return ["RAM {0} de {1}", "{0} de RAM", "bloqueado", "indisponível",
            "Voz padrão do Windows", "Voz {0}", "A voz {0} não está instalada"];
  }
  if Equals(base, "ru") {
    return ["ОЗУ {0} из {1}", "{0} ОЗУ", "заблокировано", "недоступно",
            "Голос Windows по умолчанию", "Голос {0}", "Голос {0} не установлен"];
  }
  if Equals(base, "uk") || Equals(base, "ua") {
    return ["ОЗП {0} з {1}", "{0} ОЗП", "заблоковано", "недоступно",
            "Типовий голос Windows", "Голос {0}", "Голос {0} не встановлено"];
  }
  if Equals(base, "cs") || Equals(base, "cz") {
    return ["RAM {0} z {1}", "{0} RAM", "zamčeno", "nedostupné",
            "Výchozí hlas Windows", "Hlas {0}", "Hlas {0} není nainstalován"];
  }
  if Equals(base, "hu") {
    return ["RAM {0} / {1}", "{0} RAM", "zárolva", "nem elérhető",
            "Windows alapértelmezett hang", "{0}. hang", "A(z) {0}. hang nincs telepítve"];
  }
  if Equals(base, "tr") {
    return ["RAM {0} / {1}", "{0} RAM", "kilitli", "kullanılamaz",
            "Windows varsayılanı", "Ses {0}", "Ses {0} yüklü değil"];
  }
  if Equals(base, "ja") || Equals(base, "jp") {
    return ["RAM {0} / {1}", "{0} RAM", "ロック中", "選択不可",
            "Windowsの既定の音声", "音声 {0}", "音声 {0} はインストールされていません"];
  }
  if Equals(base, "ko") || Equals(base, "kr") {
    return ["RAM {0} / {1}", "{0} RAM", "잠김", "선택 불가",
            "Windows 기본 음성", "음성 {0}", "음성 {0}이(가) 설치되지 않았습니다"];
  }
  if Equals(code, "zh-tw") {
    return ["記憶體 {0} / {1}", "{0} 記憶體", "已鎖定", "無法使用",
            "Windows 預設語音", "語音 {0}", "語音 {0} 未安裝"];
  }
  if Equals(base, "zh") {
    return ["内存 {0} / {1}", "{0} 内存", "已锁定", "不可用",
            "Windows 默认语音", "语音 {0}", "语音 {0} 未安装"];
  }
  if Equals(base, "ar") {
    return ["RAM {0} من {1}", "{0} RAM", "مقفل", "غير متاح",
            "صوت Windows الافتراضي", "الصوت {0}", "الصوت {0} غير مثبت"];
  }
  if Equals(base, "th") {
    return ["RAM {0} จาก {1}", "{0} RAM", "ล็อกอยู่", "ไม่พร้อมใช้งาน",
            "เสียงเริ่มต้นของ Windows", "เสียง {0}", "ไม่ได้ติดตั้งเสียง {0}"];
  }
  return ["RAM {0} of {1}", "{0} RAM", "locked", "unavailable",
          "Windows default", "Voice {0}", "Voice {0} is not installed"];
}
