/* Header.js
 */
 
import { Dom } from "../core/Dom.js";

/* Owner should listen for custom events on my element:
 *  - playSuspend
 *  - playResume
 *  - audioSuspend
 *  - audioResume
 *  - enterFullscreen (no corresponding "exit"; we aren't visible when fullscreen)
 *  - editSettings
 */
export class Header {
  static getDependencies() {
    return [HTMLElement, Dom, "discriminator"];
  }
  constructor(element, dom, discriminator) {
    this.element = element;
    this.dom = dom;
    this.discriminator = discriminator;
    
    this.buildUi();
  }
  
  setRunning(running) {
    const input = this.element.querySelector("input[name='Play']");
    if (input) {
      input.checked = !!running;
    }
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.addToggle("Play", "play");
    this.addToggle("Sound", "audio");
    this.addAction("Fullscreen", "enterFullscreen");
    this.addAction("Settings", "editSettings");
  }
  
  addToggle(text, eventPrefix) {
    const id = `Header-${this.discriminator}-toggle-${text}`;
    const input = this.dom.spawn(this.element, "INPUT", ["hidden", "toggle"], { type: "checkbox", id, name: text, tabindex: 0 });
    const label = this.dom.spawn(this.element, "LABEL", ["toggle"], { for: id }, text);
    if (eventPrefix) {
      input.addEventListener("change", () => {
        if (input.checked) this.element.dispatchEvent(new Event(eventPrefix + "Resume"));
        else this.element.dispatchEvent(new Event(eventPrefix + "Suspend"));
      });
    }
  }
  
  addAction(text, eventName) {
    const input = this.dom.spawn(this.element, "INPUT", { type: "button", value: text });
    if (eventName) {
      input.addEventListener("click", () => this.element.dispatchEvent(new Event(eventName)));
    }
  }
}
