import { WasmAdapter } from "./js/WasmAdapter.js";
import { VideoOut } from "./js/VideoOut.js";
import { InputManager } from "./js/InputManager.js";
import { AudioManager } from "./js/AudioManager.js";

//TODO Make a Runtime class or something, this bootstrap should be like 3 lines.

const wasmAdapter = new WasmAdapter();
const videoOut = new VideoOut();
const inputManager = new InputManager();
const audioManager = new AudioManager(wasmAdapter, window);
let interval = null;

function render() {
  const fb = wasmAdapter.getFramebufferIfDirty();
  if (fb) {
    videoOut.render(fb);
  }
  if (interval) {
    window.requestAnimationFrame(render);
  }
}

function suspendOrResume(run) {
  audioManager.suspendOrResume(run);
  if (run) {
    if (!interval) {
      interval = window.setInterval(() => wasmAdapter.update(inputManager.update()), 1000 / 60);
      window.requestAnimationFrame(render);
    }
  } else {
    if (interval) {
      window.clearInterval(interval);
      interval = null;
    }
  }
}

function pauseChanged() {
  const input = document.getElementById("pause");
  suspendOrResume(!input?.checked);
}

function firstUserGesture() {
  try {
    audioManager.setup();
  } catch (e) {
    console.error(`error setting up audio`, e);
  }
  window.removeEventListener("keydown", firstUserGesture);
  window.removeEventListener("click", firstUserGesture);
}

window.addEventListener("load", () => {

  const pauseInput = document.getElementById("pause");
  pauseInput.checked = false;
  pauseInput.addEventListener("change", pauseChanged);
  
  window.addEventListener("keydown", firstUserGesture);
  window.addEventListener("click", firstUserGesture);

  wasmAdapter.load("fullmoon.wasm").then(() => {
    videoOut.setup(document.getElementById("game-container"));
    wasmAdapter.init();
    wasmAdapter.update(0);
    suspendOrResume(true);
  }).catch((error) => {
    console.log(`Failed to fetch or load WebAssembly module.`, error);
  });
});
