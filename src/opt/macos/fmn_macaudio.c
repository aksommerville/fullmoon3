#include <string.h>
#include <pthread.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#include "fmn_macaudio.h"

/* Globals.
 */

static struct {
  int init;
  int state;
  AudioComponent component;
  AudioComponentInstance instance;
  void (*cb)(int16_t *dst,int dstc);
  pthread_mutex_t cbmtx;
} fmn_macaudio={0};

/* AudioUnit callback.
 */

static OSStatus fmn_macaudio_cb(
  void *userdata,
  AudioUnitRenderActionFlags *flags,
  const AudioTimeStamp *time,
  UInt32 bus,
  UInt32 framec,
  AudioBufferList *data
) {
  if (pthread_mutex_lock(&fmn_macaudio.cbmtx)) return 0;
  fmn_macaudio.cb(data->mBuffers[0].mData,data->mBuffers[0].mDataByteSize>>1);
  pthread_mutex_unlock(&fmn_macaudio.cbmtx);
  return 0;
}

/* Init.
 */
 
int fmn_macaudio_init(
  int rate,int chanc,
  void (*cb)(int16_t *dst,int dstc)
) {
  int err;
  if (fmn_macaudio.init||!cb) return -1;
  if ((rate<1000)||(rate>1000000)) return -1;
  if ((chanc<1)||(chanc>16)) return -1;
  memset(&fmn_macaudio,0,sizeof(fmn_macaudio));

  pthread_mutex_init(&fmn_macaudio.cbmtx,0);

  fmn_macaudio.init=1;
  fmn_macaudio.cb=cb;

  AudioComponentDescription desc={0};
  desc.componentType=kAudioUnitType_Output;
  desc.componentSubType=kAudioUnitSubType_DefaultOutput;

  fmn_macaudio.component=AudioComponentFindNext(0,&desc);
  if (!fmn_macaudio.component) {
    return -1;
  }

  if (AudioComponentInstanceNew(fmn_macaudio.component,&fmn_macaudio.instance)) {
    return -1;
  }

  if (AudioUnitInitialize(fmn_macaudio.instance)) {
    return -1;
  }

  AudioStreamBasicDescription fmt={0};
  fmt.mSampleRate=rate;
  fmt.mFormatID=kAudioFormatLinearPCM;
  fmt.mFormatFlags=kAudioFormatFlagIsSignedInteger; // implies little-endian
  fmt.mFramesPerPacket=1;
  fmt.mChannelsPerFrame=chanc;
  fmt.mBitsPerChannel=16;
  fmt.mBytesPerPacket=chanc*2;
  fmt.mBytesPerFrame=chanc*2;

  if (err=AudioUnitSetProperty(fmn_macaudio.instance,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Input,0,&fmt,sizeof(fmt))) {
    return -1;
  }

  AURenderCallbackStruct aucb={0};
  aucb.inputProc=fmn_macaudio_cb;
  aucb.inputProcRefCon=0;

  if (AudioUnitSetProperty(fmn_macaudio.instance,kAudioUnitProperty_SetRenderCallback,kAudioUnitScope_Input,0,&aucb,sizeof(aucb))) {
    return -1;
  }

  if (AudioOutputUnitStart(fmn_macaudio.instance)) {
    return -1;
  }

  fmn_macaudio.state=1;

  return 0;
}

/* Quit.
 */

void fmn_macaudio_quit() {
  if (fmn_macaudio.state) {
    AudioOutputUnitStop(fmn_macaudio.instance);
  }
  if (fmn_macaudio.instance) {
    AudioComponentInstanceDispose(fmn_macaudio.instance);
  }
  pthread_mutex_destroy(&fmn_macaudio.cbmtx);
  memset(&fmn_macaudio,0,sizeof(fmn_macaudio));
}

/* Callback lock.
 */
 
int fmn_macaudio_lock() {
  if (pthread_mutex_lock(&fmn_macaudio.cbmtx)) return -1;
  return 0;
}

int fmn_macaudio_unlock() {
  if (pthread_mutex_unlock(&fmn_macaudio.cbmtx)) return -1;
  return 0;
}
