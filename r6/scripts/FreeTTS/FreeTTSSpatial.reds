// Marker for the positional playback natives (FreeTTS_PlayAt and friends,
// declared in FreeTTS.reds). Another mod tests ModuleExists("FreeTTS.Spatial")
// to know the installed FreeTTS has them; an older FreeTTS has no such module.
module FreeTTS.Spatial

public abstract class FreeTTSSpatial {
  // 1 = PlayAt, SetListener, IsPlaying, StopSound (FreeTTS 0.7.0).
  public static func Version() -> Int32 { return 1; }
}
