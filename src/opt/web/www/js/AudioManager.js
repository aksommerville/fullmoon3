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
    
    this._initializeFrequencyTable();
    this.context = null;
    this.mixNode = null;
    this.heldNotes = [];
    this.song = null;
    this.songTempo = 0; // ms/tick
    this.songPosition = 0;
    this.songLoopPosition = 0;
    this.songTimeout = null;
    this.songRunning = false;
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
      this.songRunning = true;
      this._updateSong();
    });
  }
  
  suspendOrResume(run) {
    if (!this.context) return;
    if (run) {
      this.context.resume();
      if (!this.songRunning) {
        this.songRunning = true;
        this._updateSong();
      }
    } else {
      this.context.suspend();
      this.songRunning = false;
      if (this.songTimeout) {
        this.window.clearTimeout(this.songTimeout);
        this.songTimeout = null;
      }
    }
  }
  
  /* Events from WasmAdapter.
   **********************************************************************/
   
  onConfigure(serial) {
    console.log(`AudioManager.onConfigure`, serial);
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
    velocity *= 0.3;
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
   
  _initializeFrequencyTable() {
    const refNoteid = 0x45;
    const refFreq = 440;
    this.freqv = [];
    for (let noteid=0; noteid<0x80; noteid++) {
      this.freqv.push(refFreq * Math.pow(2, (noteid - refNoteid) / 12));
    }
  }
   
  frequencyFromNoteid(noteid) {
    return this.freqv[noteid & 0x7f];
  }
  
  onSongConfig(chid, k, v) {
    //console.log(`AudioManager.onSongConfig ${chid}.${k}=${v}`);//TODO
  }
  
  onPlaySong(serial) {
    this.song = serial;
    if (this.songTimeout) {
      this.window.clearTimeout(this.songTimeout);
      this.songTimeout = null;
    }
    if (this.song && (this.song.length >= 4)) {
      this.songTempo = this.song[0];
      this.songPosition = this.song[1];
      this.songLoopPosition = (this.song[2] << 8) | this.song[3];
      if (!this.songTempo || (this.songPosition < 4) || (this.songLoopPosition < 4)) {
        this.song = null;
        this.songTempo = 0;
        this.songPosition = 0;
        this.songLoopPosition = 0;
      } else {
      }
    } else {
      this.songTempo = 0;
      this.songPosition = 0;
      this.songLoopPosition = 0;
    }
    this._updateSong();
  }
  
  _updateSong() {
    if (!this.song) return;
    if (!this.songRunning) return;
    while (this.songPosition < this.song.length) {
      const lead = this.song[this.songPosition];
      
      // 0: EOF (repeat)
      if (!lead) {
        this.songPosition = this.songLoopPosition;
        this.songTimeout = this.window.setTimeout(() => this._updateSong(), 1);
        return;
      }
      
      // High bit unset: Delay.
      if (!(lead & 0x80)) {
        this.songPosition++;
        this.songTimeout = this.window.setTimeout(() => this._updateSong(), lead * this.songTempo);
        return;
      }
      
      // Note?
      if (((lead & 0xe0) === 0x80) && (this.songPosition <= this.song.length - 3)) {
        const velocity = (lead & 0x1f) << 2;
        const noteid = (this.song[this.songPosition + 1] >> 2) + 0x20;
        let duration = ((this.song[this.songPosition + 1] & 3) << 4) | (this.song[this.songPosition + 2] >> 4);
        duration *= this.songTempo;
        const chid = this.song[this.songPosition + 2] & 15;
        this.songPosition += 3;
        this.onNote(chid, noteid, velocity, duration);
        
      // Config?
      } else if (((lead & 0xf0) === 0xa0) && (this.songPosition <= this.song.length - 3)) {
        const chid = lead & 0x0f;
        const k = this.song[this.songPosition + 1];
        const v = this.song[this.songPosition + 2];
        this.songPosition += 3;
        this.onSongConfig(chid, k, v);
        
      // Note On?
      } else if (((lead & 0xf0) === 0xb0) && (this.songPosition <= this.song.length - 3)) {
        const chid = lead & 0x0f;
        const noteid = this.song[this.songPosition + 1];
        const velocity = this.song[this.songPosition + 2];
        this.songPosition += 3;
        this.onNote(chid, noteid, velocity, 1000); // TODO sustainable note
        
      // Note Off?
      } else if (((lead & 0xf0) === 0xc0) && (this.songPosition <= this.song.length - 2)) {
        const chid = lead & 0x0f;
        const noteid = this.song[this.songPosition + 1];
        this.songPosition += 2;
        //TODO release sustainable note
        
      // Anything else is illegal. Die.
      } else {
        console.log(`illegal byte ${lead} at ${this.songPosition}/${this.song.length} in song`);
        this.song = null;
        this.songTempo = 0;
        this.songPosition = 0;
        this.songLoopPosition = 0;
        return;
      }
    }
    // Falling off the end is equivalent to EOF. But it's never supposed to happen.
    this.songPosition = this.songLoopPosition;
    this.songTimeout = this.window.setTimeout(() => this._updateSong(), 1);
  }
}
