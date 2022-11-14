/* WasmAdapter.js
 * Interface to the game's Wasm context. Loading and all.
 */
 
import { Audio } from "../audio/Audio.js";
 
export class WasmAdapter {
  static getDependencies() {
    return [Audio, Window];
  }
  constructor(audio, window) {
    this.audio = audio;
    this.window = window;
    
    this.enableLogging = true; // Wasm code can log to JS console.
    
    this.instance = null;
    this.fbdirty = false;
    this.fb = null; // Uint8Array of RGBA.
    this.fbw = 0;
    this.fbh = 0;
    this._input = 0;
    this.waitingForReady = [];
    this.currentSong = null; // Address in wasm heap, if playing. We discard redundant play_song.
    this.terminated = false;
  }
  
  /* Download and instantiate.
   * Does not run any Wasm code.
   * Returns a Promise.
   */
  load(path) {
    const params = this._generateParams();
    return WebAssembly.instantiateStreaming(fetch(path), params).then((instance) => {
      this.instance = instance;
      for (const cb of this.waitingForReady) cb();
      this.waitingForReady = [];
      return instance;
    });
  }
  
  whenReady(cb) {
    if (this.instance) {
      cb();
    } else {
      this.waitingForReady.push(cb);
    }
  }
  
  /* Call setup() in Wasm code.
   */
  init() {
    this.instance.instance.exports.setup();
  }
  
  /* Call loop() in Wasm code, with the input state.
   */
  update(input) {
    if (!this.terminated) {
      this._input = input;
      this.instance.instance.exports.loop();
    }
  }
  
  /* If we have received a framebuffer dirty notification from Wasm,
   * return it as a Uint8Array (72 * 5) and mark clean.
   * Otherwise return null.
   */
  getFramebufferIfDirty() {
    if (this.fbdirty) {
      this.fbdirty = false;
      return { fb: this.fb, w: this.fbw, h: this.fbh };
    }
    return null;
  }
  
  /* Find some name exported by the wasm module.
   * Read it as an address in the module's memory, up to the first NUL.
   * Return as a Javascript string.
   * Exception if absent, not terminated, or longer than some sanity limit.
   */
  readStaticCString(exportedName) {
    const SANITY_LIMIT = 8 << 10;
    if (!this.instance) throw new Error(`Can't look up wasm member '${exportedName}', module not loaded.`);
    const g = this.instance.instance.exports[exportedName];
    if (!g) throw new Error(`Wasm export '${exportedName}' not found.`);
    const p = g.value;
    const buffer = this.instance.instance.exports.memory.buffer;
    if ((typeof(p) !== "number") || (p < 0) || (p >= buffer.byteLength)) {
      throw new Error(`Invalid address for wasm export '${exportedName}'`);
    }
    const fullView = new Uint8Array(buffer);
    let c = 0;
    for (;;) {
      if (p + c >= buffer.byteLength) {
        throw new Error(`Wasm export '${exportedName}' not terminated`);
      }
      if (!fullView[p + c]) break;
      c++;
      if (c > SANITY_LIMIT) {
        throw new Error(`Wasm export '${exportedName}' exceeded sanity limit ${SANITY_LIMIT} bytes.`);
      }
    }
    return new TextDecoder().decode(new Uint8Array(buffer, p, c));
  }
  
  /* Private.
   ***********************************************************************/
   
  _generateParams() {
    return {
      env: {
        millis: () => Date.now(),
        micros: () => Date.now() * 1000,
        srand: () => {},
        rand: () => Math.floor(Math.random() * 2147483647),
        abort: (...args) => {},
        sinf: (n) => Math.sin(n),
        
        fmn_platform_init: () => this._fmn_platform_init(),
        fmn_platform_terminate: (status) => this._fmn_platform_terminate(status),
        fmn_platform_update: () => this._fmn_platform_update(),
        fmn_platform_audio_configure: (v,c) => this._fmn_platform_audio_configure(v,c),
        fmn_platform_audio_play_song: (v,c) => this._fmn_platform_audio_play_song(v,c),
        fmn_platform_audio_pause_song: (pause) => this._fmn_platform_audio_pause_song(pause),
        fmn_platform_audio_note: (programid,noteid,velocity,durationms) => this._fmn_platform_audio_note(programid,noteid,velocity,durationms),
        fmn_platform_audio_silence: () => this._fmn_platform_audio_silence(),
        fmn_platform_audio_release_all: () => this._fmn_platform_audio_release_all(),
        
        fmn_web_external_render: (v,w,h) => this._fmn_web_external_render(v,w,h),
        fmn_web_external_console_log: (src) => this._fmn_web_external_console_log(src),
      },
    };
  }
  
  /* Platform hooks exposed to wasm app.
   ***************************************************************************/
   
  _fmn_platform_init() {
    console.log(`fmn_platform_init`);
    return 0;
  }
  
  _fmn_platform_terminate(status) {
    console.log(`fmn_platform_terminate(${status})`);
    this.terminated = true;
    this._input = 0;
  }

  _fmn_platform_update() {
    return this._input;
  }
  
  _fmn_platform_audio_configure(v, c) {
    const buffer = this.instance.instance.exports.memory.buffer;
    if ((v < 0) || (c < 1) || (v > buffer.byteLength - c)) return;
    const serial = new Uint8Array(buffer, v, c);
    this.audio.configure(serial);
  }
  
  _fmn_platform_audio_play_song(v, c) {
    if (v === this.currentSong) return;
    this.currentSong = v;
    const buffer = this.instance.instance.exports.memory.buffer;
    if ((v < 0) || (c < 1) || (v > buffer.byteLength - c)) return;
    const serial = new Uint8Array(buffer, v, c);
    this.audio.playSong(serial);
  }
  
  _fmn_platform_audio_pause_song(pause) {
    this.audio.pauseSong(pause);
  }
  
  _fmn_platform_audio_note(programid, noteid, velocity, durationms) {
    this.audio.note(programid, noteid, velocity, durationms);
  }
  
  _fmn_platform_audio_silence() {
    this.audio.silence();
  }
  
  _fmn_platform_audio_release_all() {
    this.audio.releaseAll();
  }
  
  _fmn_web_external_render(v, w, h) {
    const buffer = this.instance.instance.exports.memory.buffer;
    const fblen = w * h * 4;
    if ((v < 0) || (v + fblen > buffer.byteLength)) return;
    this.fbw = w;
    this.fbh = h;
    this.fb = new Uint8Array(buffer, v, fblen);
    this.fbdirty = true;
  }
  
  _fmn_web_external_console_log(p) {
    if (!this.enableLogging) return;
    const buffer = this.instance.instance.exports.memory.buffer;
    let text = "";
    if ((p >= 0) && (p < buffer.byteLength)) {
      const src = new Uint8Array(buffer, p, buffer.byteLength - p);
      for (let i=0; p<buffer.byteLength; p++, i++) {
        if (!src[i]) break;
        text += String.fromCharCode(src[i]);
      }
    }
    console.log(`[game] ${text}`);
  }
}

WasmAdapter.singleton = true;
