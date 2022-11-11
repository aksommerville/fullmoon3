export class AudioManager {
  constructor(wasmAdapter, window) {
    this.wasmAdapter = wasmAdapter;
    this.window = window;
    
    this.wasmAdapter.onConfigure = (serial) => this.onConfigure(serial);
    this.wasmAdapter.onPlaySong = (serial) => this.onPlaySong(serial);
    this.wasmAdapter.onPauseSong = (pause) => this.onPauseSong(pause);
    this.wasmAdapter.onNote = (programid, noteid, velocity, durationms) => this.onNote(programid, noteid, velocity, durationms);
    this.wasmAdapter.onSilence = () => this.onSilence();
    this.wasmAdapter.onReleaseAll = () => this.onReleaseAll();
    
    this.context = null;
    this.mixNode = null;
    this.heldNotes = [];
  }
  
  setup() {
    if (this.context) return;
    this.context = new AudioContext({
      sampleRate: 22050,
      latencyHint: "interactive",
    });
    this.wasmAdapter.whenReady(() => {
      this.mixNode = new GainNode(this.context);
      this.mixNode.connect(this.context.destination);
      if (this.context.state === "suspended") this.context.resume();
    });
  }
  
  /* Events from WasmAdapter.
   **********************************************************************/
   
  onConfigure(serial) {
    console.log(`AudioManager.onConfigure`, serial);
  }
  
  onPlaySong(serial) {
    console.log(`AudioManager.onPlaySong`, serial);
  }
  
  onPauseSong(pause) {
    console.log(`AudioManager.onPauseSong ${pause}`);
  }
  
  onNote(programid, noteid, velocity, durationms) {
    if (!this.context) return;
    if (durationms < 100) durationms = 100;
    if (velocity <= 0) velocity = 0;
    else if (velocity >= 127) velocity = 1;
    else velocity = velocity / 127.0;
    const oscillator = new OscillatorNode(this.context, {
      frequency: this.frequencyFromNoteid(noteid),
    });
    const envelope = new GainNode(this.context);
    envelope.gain.setValueAtTime(0, this.context.currentTime);
    envelope.gain.linearRampToValueAtTime(velocity, this.context.currentTime + 0.080);
    envelope.gain.linearRampToValueAtTime(0, this.context.currentTime + (durationms - 80) / 1000);
    oscillator.connect(envelope);
    envelope.connect(this.mixNode);
    oscillator.start();
    this.heldNotes.push(envelope);
    //TODO removal time. is there some event we can listen for?
    this.window.setTimeout(() => {
      envelope.disconnect();
      const p = this.heldNotes.indexOf(envelope);
      if (p >= 0) this.heldNotes.splice(p, 1);
    }, durationms);
  }
  
  onSilence() {
    for (const envelope of this.heldNotes) envelope.disconnect();
    this.heldNotes = [];
  }
  
  onReleaseAll() {
    for (const envelope of this.heldNotes) {
      envelope.gain.linearRampToValueAtTime(0, this.context.currentTime + 0.100);
      this.window.setTimeout(() => envelope.disconnect(), 110);
    }
    this.heldNotes = [];
  }
  
  /* MIDI
   *****************************************************************/
   
  frequencyFromNoteid(noteid) {
    //TODO
    return 440 + (noteid - 0x40) * 50;
  }
}
