/* RootController.js
 * Top of our DOM tree.
 */
 
import { Dom } from "../core/Dom.js";
import { Runtime } from "../rt/Runtime.js";
import { InputManager } from "../rt/InputManager.js";
import { Audio } from "../audio/Audio.js";
import { Header } from "./Header.js";
import { Footer } from "./Footer.js";
import { Sidebar } from "./Sidebar.js";
import { GameView } from "./GameView.js";

export class RootController {
  static getDependencies() {
    return [HTMLElement, Dom, Runtime, Audio, InputManager];
  }
  constructor(element, dom, runtime, audio, inputManager) {
    this.element = element;
    this.dom = dom;
    this.runtime = runtime;
    this.audio = audio;
    this.inputManager = inputManager;
    
    this.header = null;
    this.footer = null;
    this.sidebar = null;
    this.gameView = null;
    
    this.inputListener = this.inputManager.listen((state) => this.runtime.setInput(state));
    
    this.buildUi();
  }
  
  onRemoveFromDom() {
    this.inputManager.unlisten(this.inputListener);
    this.inputListener = null;
  }
  
  /* Layout.
   **************************************************************/
  
  buildUi() {
    this.element.innerHTML = "";
    
    this.header = this.dom.spawnController(this.element, Header);
    this.header.element.addEventListener("playResume", () => this.onPlayResume());
    this.header.element.addEventListener("playSuspend", () => this.onPlaySuspend());
    this.header.element.addEventListener("audioResume", () => this.onAudioResume());
    this.header.element.addEventListener("audioSuspend", () => this.onAudioSuspend());
    this.header.element.addEventListener("enterFullscreen", () => this.onEnterFullscreen());
    this.header.element.addEventListener("editSettings", () => this.onEditSettings());

    const middleRow = this.dom.spawn(this.element, "DIV", ["middleRow"]);
    this.sidebar = this.dom.spawnController(middleRow, Sidebar);
    const innards = this.dom.spawn(middleRow, "DIV", ["innards"]);
    
    this.gameView = this.dom.spawnController(innards, GameView);
    this.runtime.onFramebufferReady = (fb, w, h) => this.gameView.refresh(fb, w, h);
    
    this.footer = this.dom.spawnController(this.element, Footer);
  }
  
  /* Events.
   **************************************************************/
   
  onPlayResume() {
    if (!this.runtime.resume()) {
      this.header.setRunning(false);
    }
  }
  
  onPlaySuspend() {
    this.runtime.suspend();
  }
  
  onAudioResume() {
    this.audio.resume();
  }
  
  onAudioSuspend() {
    this.audio.suspend();
  }
  
  onEnterFullscreen() {
    this.gameView.element.requestFullscreen({ navigationUI: "hide" })
      .then(() => {})
      .catch((e) => {/* whatever */});
  }
  
  onEditSettings() {
    //TODO
  }
}
