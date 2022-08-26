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
  isRunning(): boolean {
    return getInstance().isRunning;
  },
  shutDown(): void {
    return getInstance().shutDown();
  },
  async stopAllAsync(): Promise<AmbientifySoundState> {
    await getInstance().stopAllAsync();
    return getInstance().getSerializedStateAsync();
  },
  async playAllAsync(): Promise<AmbientifySoundState> {
    await getInstance().playAllAsync();
    return getInstance().getSerializedStateAsync();
  },
  /*
    @return number the index of the newly created channel
  */
  async createChannelAsync(
    soundPath?: string,
    shouldPlay: boolean = false
  ): Promise<void> {
    return getInstance().createChannelAsync(soundPath, shouldPlay);
  },
  async loadChannelAsync(
    channelId: number,
    soundPath: string
  ): Promise<AmbientifyChannelState> {
    return getInstance().loadChannelAsync(channelId, soundPath);
  },
  async unloadChannelAsync(channelId: number): Promise<AmbientifyChannelState> {
    return getInstance().unloadChannelAsync(channelId);
  },
  async resetChannelAsync(channelId: number): Promise<AmbientifyChannelState> {
    return getInstance().resetChannelAsync(channelId);
  },
  async toggleChannelPlaybackAsync(channelID: number): Promise<boolean> {
    return getInstance().toggleChannelPlaybackAsync(channelID);
  },
  async toggleChannelMutedAsync(channelID: number): Promise<boolean> {
    return getInstance().toggleChannelMutedAsync(channelID);
  },
  // setChannelMuted(channelID: number, muted: boolean) {
  //   return getInstance().setChannelMuted(channelID, muted);
  // },
  async playAsync(channelId: number): Promise<boolean> {
    return getInstance().playChannelAsync(channelId);
  },
  async stopAsync(channelId: number): Promise<boolean> {
    return getInstance().stopChannelAsync(channelId);
  },
  async setRandomizationEnabledAsync(channelId: number, enabled: boolean): Promise<boolean> {
    return getInstance().setRandomizationEnabledAsync(channelId, enabled);
  },
  async setRandomizationSettingsAsync(
    channelId: number,
    newSettings: Partial<AmbientifyChannelRandomizationSettings>
  ): Promise<AmbientifyChannelState> {
    return getInstance().setRandomizationSettingsAsync(channelId, newSettings);
  },
  async setCrossfadingAsync(channelId: number, enabled: boolean): Promise<boolean> {
    return getInstance().setCrossfadingAsync(channelId, enabled);
  },
  async setChannelVolumeAsync(channelID: number, volume: number, pan?: number): Promise<AmbientifyChannelState> {
    //@todo replace -2 with a different overridden method
    return getInstance().setChannelVolumeAsync(channelID, volume, pan || -2);
  },
  async setMasterVolumeAsync(volume: number): Promise<number> {
    return getInstance().setMasterVolumeAsync(volume);
  },
  async getSerializedStateAsync(): Promise<AmbientifySoundState> {
    return getInstance().getSerializedStateAsync();
  },
  async setStatusAsync(
    channelId: number,
    newStatus: Partial<Omit<AmbientifyChannelState, 'id'>>
  ): Promise<AmbientifyChannelState> {
    return getInstance().setStatusAsync(channelId, newStatus);
  },
  test(): 'OK' | undefined {
    return getInstance().test();
  },
};

export default AmbientifySoundEngine;
