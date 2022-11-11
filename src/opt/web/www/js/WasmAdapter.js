/* WasmAdapter.js
 * Load, link, and communicate with the Wasm module.
 */
 
export class WasmAdapter {
  constructor() {
    this.instance = null;
    this.fbdirty = false;
    this.fb = null;
    this._input = 0;
    this.highscore = 0;
    this.waitingForReady = [];
    this.currentSong = null;
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
  
  /* Call loop() in Wasm code, with the input state (see InputManager.js)
   */
  update(input) {
    this._input = input;
    this.instance.instance.exports.loop();
  }
  
  /* If we have received a framebuffer dirty notification from Wasm,
   * return it as a Uint8Array (72 * 5) and mark clean.
   * Otherwise return null.
   */
  getFramebufferIfDirty() {
    if (this.fbdirty) {
      this.fbdirty = false;
      return this.fb;
    }
    return null;
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
  }

  _fmn_platform_update() {
    return this._input;
  }
  
  _fmn_platform_audio_configure(v, c) {
    if (!this.onConfigure) return;
    const buffer = this.instance.instance.exports.memory.buffer;
    if ((v < 0) || (c < 1) || (v > buffer.byteLength - c)) return;
    const serial = new Uint8Array(buffer, v, c);
    this.onConfigure(serial);
  }
  
  _fmn_platform_audio_play_song(v, c) {
    if (v === this.currentSong) return;
    this.currentSong = v;
    if (!this.onPlaySong) return;
    const buffer = this.instance.instance.exports.memory.buffer;
    if ((v < 0) || (c < 1) || (v > buffer.byteLength - c)) return;
    const serial = new Uint8Array(buffer, v, c);
    this.onPlaySong(serial);
  }
  
  _fmn_platform_audio_pause_song(pause) {
    if (!this.onPauseSong) return;
    this.onPauseSong(pause);
  }
  
  _fmn_platform_audio_note(programid, noteid, velocity, durationms) {
    if (!this.onNote) return;
    this.onNote(programid, noteid, velocity, durationms);
  }
  
  _fmn_platform_audio_silence() {
    if (!this.onSilence) return;
    this.onSilence();
  }
  
  _fmn_platform_audio_release_all() {
    if (!this.onReleaseAll) return;
    this.onReleaseAll();
  }
  
  _fmn_web_external_render(v,w,h) {
    const buffer = this.instance.instance.exports.memory.buffer;
    const fblen = w * h * 4;
    if ((v < 0) || (v + fblen > buffer.byteLength)) return;
    this.fb = new Uint8Array(buffer, v, fblen);
    this.fbdirty = true;
  }
  
  _fmn_web_external_console_log(p) {
    const buffer = this.instance.instance.exports.memory.buffer;
    let text = "";
    if ((p >= 0) && (p < buffer.byteLength)) {
      const src = new Uint8Array(buffer, p, buffer.byteLength - p);
      for (let i=0; p<buffer.byteLength; p++, i++) {
        if (!src[i]) break;
        text += String.fromCharCode(src[i]);
      }
    }
    console.log(text);
  }
}
