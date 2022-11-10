export const containamajig = {};

export class AudioManager {
  constructor(wasmAdapter) {
    this.wasmAdapter = wasmAdapter;
  }
  
  _old_setup() {
    /* The AudioContext was not allowed to start. It must be resumed (or created) after a user gesture on the page.
     * ^ TODO figure out what to do about this.
     * We need some kind of logic, force the user to click somewhere before audio can begin.
     */
    window.setTimeout(() => {
      console.log(`create AudioContext`);
      this.context = new AudioContext({
        sampleRate: 22050,
        latencyHint: "interactive",
      });
      this.wasmAdapter.whenReady(() => {
      
        /* The recommended approach: AudioWorklet.
         * I can't figure out how to have the worklet call into my wasm program.
         * Is that even possible? It might be the design of worklets, that that is simply not allowed.
        console.log(`addModule`);
        this.context.audioWorklet.addModule("/js/MyAudioWorklet.js").then(() => {
          console.log(`addModule ok rate=${this.context.sampleRate}`);
          
          this.wasmAdapter.instance.instance.exports.minisyni_init(this.context.sampleRate, 1);
          console.log(`minisyni_init ok...?`);
          containamajig.minisyni_update = this.wasmAdapter.instance.instance.exports.minisyni_update;
          console.log(`AudioManager: minisyni_update`, containamajig.minisyni_update);
          
          this.node = new AudioWorkletNode(this.context, "MyAudioWorklet", {
            processorOptions: {
              foo: "bar",
            },
          });
          this.node.connect(this.context.destination);
        }).catch((error) => {
          console.error(`Failed to add MyAudioWorklet module`, error);
        });
        /**/
        
        /* Using ScriptProcessorNode instead.
         * Beware, this is deprecated. https://developer.mozilla.org/en-US/docs/Web/API/ScriptProcessorNode
         * But MDN says all the browsers still support it.
         *
        this.wasmAdapter.instance.instance.exports.minisyni_init(this.context.sampleRate, 1);
        this.node = this.context.createScriptProcessor(512, 0, 1);
        this.node.onaudioprocess = (event) => {
          const dst = event.outputBuffer.getChannelData(0);
          const buffer = this.wasmAdapter.instance.instance.exports.storage_for_wasi;
          const result = this.wasmAdapter.instance.instance.exports.minisyni_update(buffer.value, 512);
          const readback = new Int16Array(this.wasmAdapter.instance.instance.exports.memory.buffer, buffer.value, 1024);
          for (let i=0; i<512; i++) dst[i] = readback[i] / 32768.0;
        };
        this.node.connect(this.context.destination);
        /**/
        
        /* This time around, don't use the game's synthesizer, we'll build without one.
         * Implement an equivalent synthesizer using the piecemeal WebAudio API.
         * Should be much better quality that way, and no more weird how-to-call-wasm problems.
         */
        console.log(`TODO: AudioManager`);
      });
    }, 2000);
  }
}
