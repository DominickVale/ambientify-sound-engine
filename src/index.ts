import { NativeModules } from 'react-native';

export type AmbientifyChannelRandomizationSettings = {
  times: number;
  minutes: number;
  panRange?: [number, number];
  volumeRange?: [number, number];
  pitchRange?: [number, number];
};

export type AmbientifyChannelState = {
  currentFilePath: string | undefined;
  id: number;
  durationMs: number;
  isLoaded: boolean;
  isLoading: boolean;
  isPlaying: boolean;
  didJustFinish: boolean;
  crossfadeEnabled: boolean;
  isMuted: boolean;
  volume: number;
  pan: number;
  pitch: number;
  cfPercentageStart: number;
  noUnload: boolean;
  randomizationEnabled: boolean;
  rSettings: AmbientifyChannelRandomizationSettings;
}

export type AmbientifySoundState = {
  masterVolume: number;
  sounds: Array<AmbientifyChannelState>;
};

function install() {
  // @ts-ignore
  if (global._AmbientifySoundEngine?.isReady) {
    throw new Error('AmbientifySoundEngine already initialized.');
  }
  const res = NativeModules.ASoundEngine.install();
  if (!res) {
    throw new Error('Could not initialize AmbientifySoundEngine.');
  }
}
install();

function getInstance() {
  // @ts-ignore
  return global._AmbientifySoundEngine;
}

const AmbientifySoundEngine = {
  isReady(): boolean {
    return getInstance().isReady;
  },
  stopAll(): boolean {
    return getInstance().stopAll();
  },
  playAll(): boolean {
    return getInstance().playAll();
  },
  /*
    @return number the index of the newly created channel
  */
  async createChannelAsync(
    soundPath?: string,
    shouldPlay: boolean = false
  ): Promise<number> {
    return getInstance().createChannelAsync(soundPath, shouldPlay);
  },
  async loadChannelAsync(
    channelId: number,
    soundPath: string
  ): Promise<number> {
    return getInstance().loadChannelAsync(channelId, soundPath);
  },
  unloadChannel(channelId: number): number {
    return getInstance().unloadChannel(channelId);
  },
  resetChannel(channelId: number): number {
    return getInstance().resetChannel(channelId);
  },
  toggleChannelPlayback(channelID: number): boolean {
    return getInstance().toggleChannelPlayback(channelID);
  },
  toggleChannelMuted(channelID: number): boolean {
    return getInstance().toggleChannelMuted(channelID);
  },
  // setChannelMuted(channelID: number, muted: boolean) {
  //   return getInstance().setChannelMuted(channelID, muted);
  // },
  play(channelId: number) {
    return getInstance().playChannel(channelId);
  },
  stop(channelId: number) {
    return getInstance().stopChannel(channelId);
  },
  setRandomizationEnabled(channelId: number, enabled: boolean) {
    return getInstance().setRandomizationEnabled(channelId, enabled);
  },
  setRandomizationSettings(
    channelId: number,
    newSettings: Partial<AmbientifyChannelRandomizationSettings>
  ) {
    return getInstance().setRandomizationSettings(channelId, newSettings);
  },
  setCrossfadingAsync(channelId: number, enabled: boolean): Promise<boolean> {
    return getInstance().setCrossfadingAsync(channelId, enabled);
  },
  setChannelVolume(channelID: number, volume: number, pan?: number) {
    //@todo replace -2 with a different overridden method
    return getInstance().setChannelVolume(channelID, volume, pan || -2);
  },
  setMasterVolume(volume: number) {
    return getInstance().setMasterVolume(volume);
  },
  async getStatusAsync(): Promise<AmbientifySoundState> {
    return getInstance().getSerializedStateAsync();
  },
  async setStatusAsync(
    channelId: number,
    newStatus: Partial<Omit<AmbientifyChannelState, 'id'>>
  ): Promise<boolean> {
    return getInstance().setStatusAsync(channelId, newStatus);
  },
  test(): 'OK' | undefined {
    return getInstance().test();
  },
};

export default AmbientifySoundEngine;
