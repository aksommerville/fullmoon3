/* Footer.js
 */
 
import { Dom } from "../core/Dom.js";

export class Footer {
  static getDependencies() {
    return [HTMLUListElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "LI", "\u00a9 2022 AK Sommerville");
    this.spawnLink("GitHub", "https://github.com/aksommerville/fullmoon3");
    this.spawnLink("Sommerville", "http://aksommerville.com/");
  }
  
  spawnLink(text, url) {
    const li = this.dom.spawn(this.element, "LI");
    const a = this.dom.spawn(li, "A", text, { href: url });
  }
}
