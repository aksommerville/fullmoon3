/* MapToolbox.js
 * UI controller for the palette and paint tools.
 */
 
import { Dom } from "/js/core.js";
import { MapPaintService } from "./MapPaintService.js";
import { ResService } from "/js/ResService.js";
import { MapPaletteModal } from "./MapPaletteModal.js";

export class MapToolbox {
  static getDependencies() {
    return [HTMLElement, Dom, MapPaintService, ResService];
  }
  constructor(element, dom, mapPaintService, resService) {
    this.element = element;
    this.dom = dom;
    this.mapPaintService = mapPaintService;
    this.resService = resService;
    
    this.selectedTile = mapPaintService.selectedTile;
    this._map = null;
    
    this._buildUi();
  }
  
  setMap(map) {
    this._map = map;
    this._renderSelectedTile();
  }
  
  setSelectedTile(tileid) {
    if (tileid === this.selectedTile) return;
    this.selectedTile = tileid;
    this._renderSelectedTile();
  }
  
  setTool(name) {
    for (const element of this.element.querySelectorAll(".tool input:checked")) element.checked = false;
    const element = this.element.querySelector(`.tool input[value='${name}'`);
    if (!element) return;
    element.checked = true;
    this.onToolChanged();
  }
  
  _buildUi() {
    this.element.innerHTML = "";
    
    this.dom.spawn(this.element, "CANVAS", ["selectedTile"], { "on-click": () => this.openPalette() });
    
    const toolsRow = this.dom.spawn(this.element, "DIV", ["toolsRow"], { "on-change": () => this.onToolChanged() });
    for (const name of this.mapPaintService.toolNames) {
      this._spawnToolButton(toolsRow, name);
    }
    
    this._renderSelectedTile();
  }
  
  _spawnToolButton(row, name) {
    const label = this.dom.spawn(row, "LABEL", ["tool"]);
    const input = this.dom.spawn(label, "INPUT", ["hidden"], { type: "radio", name: "tool", value: name });
    const icon = this.dom.spawn(label, "IMG", ["tool"], { src: `/img/tool_${name}.png` });
    if (name === this.mapPaintService.toolLeft) input.checked = true;
  }
  
  _renderSelectedTile(retry = true) {
    const canvas = this.element.querySelector(".selectedTile");
    canvas.width = canvas.offsetWidth;
    canvas.height = canvas.offsetHeight;
    const context = canvas.getContext("2d");
    context.imageSmoothingEnabled = false;
    try {
      const src = this.resService.getImageSync(this._map.getTilesheetName());
      const tilesize = src.naturalWidth >> 4;
      const srcx = (this.selectedTile & 15) * tilesize;
      const srcy = (this.selectedTile >> 4) * tilesize;
      context.fillStyle = "#fff";
      context.fillRect(0, 0, canvas.width, canvas.height);
      context.drawImage(src, srcx, srcy, tilesize, tilesize, 0, 0, canvas.width, canvas.height);
    } catch (e) {
      context.fillStyle = "#000";
      context.fillRect(0, 0, canvas.width, canvas.height);
      if (retry) {
        const name = this._map?.getTilesheetName();
        if (name) this.resService.getImage(name).then(() => this._renderSelectedTile(false));
      }
    }
  }
  
  onToolChanged() {
    const input = this.element.querySelector(".tool input:checked");
    if (!input) return;
    this.mapPaintService.setToolLeft(input.value);
  }
  
  openPalette() {
    const name = this._map?.getTilesheetName();
    if (!name) return;
    const controller = this.dom.spawnModal(MapPaletteModal);
    controller.setTilesheetName(name);
    controller.onChooseTile = (tileid) => this.mapPaintService.setSelectedTile(tileid);
  }
}
