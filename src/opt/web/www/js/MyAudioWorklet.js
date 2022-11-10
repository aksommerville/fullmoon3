import { containamajig } from "./AudioManager.js";

export class MyAudioWorklet extends AudioWorkletProcessor {
  constructor() {
    super();
    this.init = false;
    this.buffer = new Int16Array(4096);
    this.mslogstate = null;
  }
  
  process(inputs, outputs, parameters) {
  
    if (!this.init) {
      console.log(`MyAudioWorklet.process`, { inputs, outputs, parameters });
      this.init = true;
    }
    
    if (containamajig.minisyni_update) {
      if (this.mslogstate !== true) {
        console.log(`minisyni present`);
        this.mslogstate = true;
      }
      containamajig.minisyni_update(this.buffer, outputs[0][0].length);
    } else {
      if (this.mslogstate !== false) {
        console.log(`minisyni absent`);
        this.mslogstate = false;
      }
    }
  
    const output = outputs[0]
    output.forEach((channel) => {
      for (let i = 0; i < channel.length; i++) {
        channel[i] = this.buffer[i] / 32768.0;
      }
    })
    return true;
  }
}

registerProcessor('MyAudioWorklet', MyAudioWorklet)
