/* MapCommands.js
 * UI controller for the map's command list.
 */
 
import { Dom } from "/js/core.js";

export class MapCommands {
  static getDependencies() {
    return [HTMLElement, Dom, Window];
  }
  constructor(element, dom, window) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    
    this.onDirty = () => {};
    this.onAddCommand = () => {};
    this.onEditCommand = (command) => {};
    
    this.map = null;
    
    this._buildUi();
  }
  
  setMap(map) {
    this.map = map;
    this._populateUi();
  }
  
  _buildUi() {
    this.element.innerHTML = "";
    
    const buttonsRow = this.dom.spawn(this.element, "DIV", ["buttonsRow"]);
    this.dom.spawn(buttonsRow, "BUTTON", { "on-click": () => this.onAddCommand() }, "+");
    this.dom.spawn(buttonsRow, "BUTTON", { "on-click": () => this._populateUi() }, "Refresh");
    
    this.dom.spawn(this.element, "UL", ["commands"]);
  }
  
  _populateUi() {
    const ul = this.element.querySelector("ul.commands");
    ul.innerHTML = "";
    if (this.map?.commands) {
      for (const command of this.map.commands) {
        const li = this.dom.spawn(ul, "LI", ["command"]);
        this._populateCommandRow(li, command);
      }
    }
  }
  
  _populateCommandRow(parent, command) {
    this.dom.spawn(parent, "BUTTON", "X", ["danger"], { "on-click": () => this.deleteCommand(command) });
    this.dom.spawn(parent, "BUTTON", "^", { "on-click": () => this.moveCommand(command, -1) });
    this.dom.spawn(parent, "BUTTON", "v", { "on-click": () => this.moveCommand(command, 1) });
    this.dom.spawn(parent, "BUTTON", command.join(" "), ["edit"], { "on-click": () => this.onEditCommand(command) });
  }
  
  moveCommand(command, d) {
    const op = this.map.commands.indexOf(command);
    if (op < 0) return;
    const np = op + d;
    if (np < 0) return;
    if (np >= this.map.commands.length) return;
    this.map.commands.splice(op, 1);
    this.map.commands.splice(np, 0, command);
    this._populateUi();
    this.onDirty();
  }
  
  deleteCommand(command) {
    const p = this.map.commands.indexOf(command);
    if (p < 0) return;
    if (!this.window.confirm(`Really delete command '${command.join(" ")}'?`)) return;
    this.map.commands.splice(p, 1);
    this._populateUi();
    this.onDirty();
  }
}
