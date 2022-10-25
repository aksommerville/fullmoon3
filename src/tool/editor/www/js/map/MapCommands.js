/* MapCommands.js
 * UI controller for the map's command list.
 */
 
import { Dom } from "/js/core.js";

export class MapCommands {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.map = null;
    
    this._buildUi();
  }
  
  setMap(map) {
    this.map = map;
    this._populateUi();
  }
  
  _buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "UL", ["commands"]);
  }
  
  _populateUi() {
    const ul = this.element.querySelector("ul.commands");
    ul.innerHTML = "";
    if (this.map?.commands) {
      for (const command of this.map.commands) {
        this.dom.spawn(ul, "LI", command.join(" "));
      }
    }
  }
}
