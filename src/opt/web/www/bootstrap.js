import { WasmAdapter } from "./js/WasmAdapter.js";
import { VideoOut } from "./js/VideoOut.js";
import { InputManager } from "./js/InputManager.js";
import { AudioManager } from "./js/AudioManager.js";

const wasmAdapter = new WasmAdapter();
const videoOut = new VideoOut();
const inputManager = new InputManager();
const audioManager = new AudioManager(wasmAdapter);

function render() {
  const fb = wasmAdapter.getFramebufferIfDirty();
  if (fb) {
    videoOut.render(fb);
  }
  window.requestAnimationFrame(render);
}

window.addEventListener("load", () => {
  wasmAdapter.load("fullmoon.wasm").then(() => {
    videoOut.setup(document.getElementById("game-container"));
    wasmAdapter.init();
    wasmAdapter.update(0);
    render();
    window.setInterval(() => wasmAdapter.update(inputManager.update()), 1000 / 60);
  }).catch((error) => {
    console.log(`Failed to fetch or load WebAssembly module.`, error);
  });
});
