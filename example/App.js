import * as React from 'react';
import {
  StyleSheet,
  View,
  Text,
  TouchableOpacity,
  TextInput,
  ScrollView,
  Switch,
} from 'react-native';
import Slider from '@react-native-community/slider';
import AmbientifySoundEngine, {AmbientifySoundState} from '../src';
import {Picker} from '@react-native-picker/picker';
import MultiSlider, {
  MultiSliderProps,
} from '@ptomasroos/react-native-multi-slider';
import {useRef} from 'react';
import useDebounceFn from 'ahooks/lib/useDebounceFn';

const soundFiles = [
  {
    label: 'celtic loop (wav)',
    value: 'file:///android_asset/sounds/celtic_loop.wav',
  },
  {
    label: 'Bathroom fan (wav)',
    value: 'file:///android_asset/sounds/Bathroom_fan.wav',
  },
  {
    label: 'Bathroom fan (ogg)',
    value: 'file:///android_asset/sounds/Bathroom_fan.ogg',
  },
  {label: 'drumloop (wav)', value: 'file:///android_asset/sounds/drumloop.wav'},
  {
    label: 'Sing_bowl_meditation (ogg)',
    value: 'file:///android_asset/sounds/Sing_bowl_meditation.ogg',
  },
];

export default function App() {
  const [localVolume, setLocalVolume] = React.useState(1);
  const [localVolumeRange, setLocalVolumeRange] = React.useState([1, 1]);
  const [localPanRange, setLocalPanRange] = React.useState([0, 0]);
  const [localPitchRange, setLocalPitchRange] = React.useState([1, 1]);
  const [localPan, setLocalPan] = React.useState(0);
  const [localCfStart, setLocalCfStart] = React.useState(0.75);
  const [file, setFile] = React.useState(soundFiles[0].value);
  const [serializedState, setSerializedState] = React.useState(null);
  const [soundsCount, setSoundsCount] = React.useState(0);
  const [multiMode, setMultiMode] = React.useState(false);
  const firstSoundState = serializedState?.[0];

  const playedRef = React.useRef([]);
  const multiModeRef = React.useRef(false);
  const fileRef = React.useRef(file);
  multiModeRef.current = multiMode;

  React.useEffect(() => {
    const interval = setInterval(async () => {
      if (AmbientifySoundEngine.isReady()) {
        const newState = await AmbientifySoundEngine.getStatusAsync();
        setSerializedState(newState);
        setSoundsCount(newState?.length || 0);
      }
    }, 100);
    return () => clearInterval(interval);
  }, []);

  React.useEffect(() => {
    setLocalVolume(firstSoundState?.volume);
    setLocalPan(firstSoundState?.pan);
  }, [firstSoundState?.volume, firstSoundState?.pan]);

  const onMultiSwitch = () => {
    multiModeRef.current = !multiMode;
    setMultiMode(!multiMode);
    if (serializedState?.length > 0) {
      serializedState.forEach(async (ch, id) => {
        if (ch.isLoaded) {
          AmbientifySoundEngine.unloadChannel(id);
        }
      });
    }
  };

  const {run: setRandomSettings} = useDebounceFn(
    (v, rangeType) => {
      AmbientifySoundEngine.setRandomizationSettings(0, {
        ...serializedState[0].rSettings,
        times: 6,
        minutes: 1,
        [rangeType]: [...v],
      });
    },
    {
      wait: 500,
    },
  );

  return (
    <ScrollView>
      <View style={styles.container}>
        <ScrollView style={{height: 200}} nestedScrollEnabled>
          <Text style={{fontSize: 12}}>
            Status:
            {serializedState !== null
              ? JSON.stringify(serializedState, null, 2)
              : 'null'}
          </Text>
        </ScrollView>
        <Text>Next op. sound file: {file}</Text>
        <Picker
          onValueChange={value => setFile(value)}
          selectedValue={file}
          style={{flex: 1, width: '100%'}}
          //@ts-ignore
          options={soundFiles}>
          {soundFiles.map(({value, label}) => (
            <Picker.Item key={value} label={label} value={value} />
          ))}
        </Picker>
        <View style={{flex: 1, flexDirection: 'row', alignItems: 'center'}}>
          <Text>Multi mode?</Text>
          <Switch value={multiMode} onChange={onMultiSwitch} />
        </View>
        {!multiMode ? (
          <>
            <TouchableOpacity
              onPress={async () => {
                if (!firstSoundState) {
                  const newIdx = AmbientifySoundEngine.createChannelAsync();
                } else {
                  await AmbientifySoundEngine.loadChannelAsync(0, file);
                }
              }}
              style={styles.button}>
              <Text style={styles.buttonTxt}>
                {serializedState?.[0]
                  ? firstSoundState.current?.currentFilePath
                    ? 'Reload sound'
                    : ' Load sound'
                  : 'Create channel'}
              </Text>
            </TouchableOpacity>
            {firstSoundState && (
              <TouchableOpacity
                onPress={async () => {
                  if (firstSoundState) {
                    AmbientifySoundEngine.unloadChannel(0);
                  }
                }}
                style={styles.button}>
                <Text style={styles.buttonTxt}>Unload</Text>
              </TouchableOpacity>
            )}

            <TouchableOpacity
              onPress={() => {
                AmbientifySoundEngine.toggleChannelPlayback(0);
              }}
              style={styles.button}>
              <Text style={styles.buttonTxt}>
                Toggle sound
                {firstSoundState?.isPlaying ? ' (now playing)' : ' (stopped)'}
              </Text>
            </TouchableOpacity>
            <TouchableOpacity
              onPress={() => {
                AmbientifySoundEngine.setCrossfadingAsync(
                  0,
                  !serializedState?.[0].crossfadeEnabled,
                );
              }}
              style={styles.button}>
              <Text style={styles.buttonTxt}>
                Toggle Crossfade (
                {serializedState?.[0].crossfadeEnabled ? 'on' : ' off'})
              </Text>
            </TouchableOpacity>
            <TouchableOpacity
              onPress={() => {
                AmbientifySoundEngine.resetChannel(0);
              }}
              style={styles.button}>
              <Text style={styles.buttonTxt}>Reset channel</Text>
            </TouchableOpacity>

            <Text>
              Cf Percentage Start: {firstSoundState?.cfPercentageStart}
            </Text>
            <Slider
              style={{width: 200, height: 40}}
              minimumValue={0}
              maximumValue={1}
              value={localVolume}
              step={0.01}
              onValueChange={value => {
                setLocalCfStart(value);
                if (serializedState?.[0]) {
                  serializedState.forEach((ch, id) => {
                    AmbientifySoundEngine.setStatusAsync(id, {
                      cfPercentageStart: value,
                    });
                  });
                }
              }}
            />
            <TouchableOpacity
              onPress={() => {
                AmbientifySoundEngine.setRandomizationEnabled(
                  0,
                  !serializedState?.[0]?.randomizationEnabled,
                );
              }}
              style={styles.button}>
              <Text style={styles.buttonTxt}>Toggle randomization</Text>
            </TouchableOpacity>
            <Text>Volume range</Text>
            <MultiSlider
              min={0}
              max={1}
              values={localVolumeRange}
              step={0.1}
              onValuesChange={v => {
                setLocalVolumeRange([v[0], v[1]]);
                setRandomSettings(v, 'volumeRange');
              }}
              containerStyle={{width: '100%'}}
              snapped
            />
            <Text>Pan range</Text>
            <MultiSlider
              min={-1}
              max={1}
              values={localPanRange}
              step={0.1}
              onValuesChange={v => {
                setLocalPanRange([v[0], v[1]]);
                setRandomSettings(v, 'panRange');
              }}
              containerStyle={{width: '100%'}}
              snapped
            />
            <Text>Pitch range</Text>
            <MultiSlider
              min={0}
              max={2}
              values={localPitchRange}
              step={0.1}
              onValuesChange={v => {
                setLocalPitchRange([v[0], v[1]]);
                setRandomSettings(v, 'pitchRange');
              }}
              containerStyle={{width: '100%'}}
              snapped
            />
          </>
        ) : (
          <>
            <Text>N. of additional sounds: {soundsCount}</Text>
            <TouchableOpacity
              onPress={async () => {
                const newIdx = await AmbientifySoundEngine.createChannelAsync(
                  file,
                );
                await AmbientifySoundEngine.setCrossfadingAsync(newIdx, true);
                await AmbientifySoundEngine.setStatusAsync(newIdx, {
                  volume: localVolume,
                  pan: localPan,
                  cfPercentageStart: localCfStart,
                });
                AmbientifySoundEngine.play(newIdx);
                setSoundsCount(soundsCount + 1);
                playedRef.current.push(newIdx);
              }}
              style={styles.button}>
              <Text style={styles.buttonTxt}>Add sound</Text>
            </TouchableOpacity>
          </>
        )}
        <Text>Volume: {firstSoundState?.volume}</Text>
        <Slider
          style={{width: 200, height: 40}}
          minimumValue={0}
          maximumValue={1}
          value={localVolume}
          step={0.01}
          onValueChange={value => {
            setLocalVolume(value);
            if (serializedState?.[0]) {
              serializedState.forEach((ch, id) => {
                AmbientifySoundEngine.setChannelVolume(
                  id,
                  value,
                  firstSoundState?.pan || 0,
                );
              });
            }
          }}
        />
        <Text>Pan: {firstSoundState?.pan}</Text>
        <Slider
          style={{width: 200, height: 40}}
          minimumValue={-1}
          maximumValue={1}
          value={localPan}
          step={0.1}
          onValueChange={value => {
            setLocalPan(value);
            serializedState.forEach((_, id) => {
              AmbientifySoundEngine.setChannelVolume(
                id,
                firstSoundState?.volume || 1,
                value,
              );
            });
          }}
        />
        <TouchableOpacity
          onPress={() => {
            serializedState.forEach((_, id) => {
              AmbientifySoundEngine.toggleChannelMuted(id);
            });
          }}
          style={styles.button}>
          <Text style={styles.buttonTxt}>Toggle mute all</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
  box: {
    width: 60,
    height: 60,
    marginVertical: 20,
  },
  button: {
    width: '95%',
    alignSelf: 'center',
    height: 40,
    backgroundColor: 'green',
    alignItems: 'center',
    justifyContent: 'center',
    borderRadius: 5,
    marginVertical: 10,
  },
  buttonTxt: {
    color: 'white',
  },
});
