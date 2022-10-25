/* MapPaletteModal.js
 */
 
import { Dom } from "/js/core.js";

export class MapPaletteModal {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.onChooseTile = (tileid) => {};
    
    this._buildUi();
  }
  
  setTilesheetName(name) {
    this.element.querySelector("img").src = `/src/data/image/${name}.png`;
  }
  
  _buildUi() {
    this.element.innerHTML = "";
    const img = this.dom.spawn(this.element, "IMG", { "on-click": (event) => this.onClick(event) });
  }
  
  onClick(event) {
    const img = this.element.querySelector("img");
    const bounds = img.getBoundingClientRect();
    const x = event.x - bounds.x;
    const y = event.y - bounds.y;
    const col = Math.floor((x * 16) / bounds.width);
    const row = Math.floor((y * 16) / bounds.height);
    if ((col < 0) || (col >= 16)) return this.dom.popModal(this);
    if ((row < 0) || (row >= 16)) return this.dom.popModal(this);
    this.onChooseTile((row << 4) | col);
    this.dom.popModal(this);
  }
}
