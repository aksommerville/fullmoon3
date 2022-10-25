/* TileModal.js
 * Detailed view for one tile via ImageController.
 */
 
import { Dom } from "/js/core.js";
import { Tileprops } from "./Tileprops.js";

export class TileModal {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    // We modify (tileprops) in place, then call this.
    this.onDirty = () => {};
    
    this.image = null;
    this.tileprops = null;
    this.tileid = 0;
    
    this.element.addEventListener("change", () => this.onChange());
    this._buildUi();
  }
  
  setup(image, tileprops, tileid) {
    this.image = image;
    this.tileprops = tileprops;
    this.tileid = tileid;
    this._populateUi();
  }
  
  _buildUi() {
    this.element.innerHTML = "";
    
    const idRow = this.dom.spawn(this.element, "DIV", ["idRow"]);
    this.dom.spawn(idRow, "CANVAS", ["thumbnail"]);
    this.dom.spawn(idRow, "DIV", ["tileid"]);
    
    const navigation = this.dom.spawn(idRow, "DIV", ["navigation"]);
    this.dom.spawn(navigation, "INPUT", { type: "button", value: "<", "on-click": () => this.navigate(-1, 0) });
    const vertNav = this.dom.spawn(navigation, "DIV", ["vertNav"]);
    this.dom.spawn(vertNav, "INPUT", { type: "button", value: "^", "on-click": () => this.navigate(0, -1) });
    this.dom.spawn(vertNav, "INPUT", { type: "button", value: "v", "on-click": () => this.navigate(0, 1) });
    this.dom.spawn(navigation, "INPUT", { type: "button", value: ">", "on-click": () => this.navigate(1, 0) });
    
    const propsRow = this.dom.spawn(this.element, "DIV", ["propsRow"]);
    const leftColumn = this.dom.spawn(propsRow, "DIV", ["leftColumn"]);
    
    const physicsMenu = this.dom.spawn(leftColumn, "SELECT", { name: "physics" });
    this.dom.spawn(physicsMenu, "OPTION", { value: "0" }, "Vacant");
    this.dom.spawn(physicsMenu, "OPTION", { value: "1" }, "Solid");
    this.dom.spawn(physicsMenu, "OPTION", { value: "2" }, "Hole");
    this.dom.spawn(physicsMenu, "OPTION", { value: "3" }, "Reserved");
    
    this.dom.spawn(
      this.dom.spawn(leftColumn, "LABEL", "Family"),
      "INPUT", { name: "family", type: "number", min: 0, max: 255 }
    );
    this.dom.spawn(
      this.dom.spawn(leftColumn, "LABEL", "Probability"),
      "INPUT", { name: "probability", type: "number", min: 0, max: 255 }
    );
    
    const neighbors = this.dom.spawn(propsRow, "TABLE", ["neighbors"]);
    this._spawnNeighborsRow(neighbors, [0x80, 0x40, 0x20]);
    this._spawnNeighborsRow(neighbors, [0x10, 0x00, 0x08]);
    this._spawnNeighborsRow(neighbors, [0x04, 0x02, 0x01]);
    
    this.dom.spawn(this.element, "DIV", "----- Presets -----");
    this.dom.spawn(this.element, "DIV", ["presets"]);
  }
  
  _spawnNeighborsRow(table, masks) {
    const tr = this.dom.spawn(table, "TR");
    for (const mask of masks) {
      const td = this.dom.spawn(tr, "TD");
      if (mask) {
        const button = this.dom.spawn(td, "INPUT", ["neighbor"], { type: "checkbox", value: mask });
      }
    }
  }
  
  _populateUi() {
    
    if (this.image) {
      const colw = this.image.naturalWidth >> 4;
      const rowh = this.image.naturalHeight >> 4;
      const canvas = this.element.querySelector(".thumbnail");
      canvas.width = colw;
      canvas.height = rowh;
      const context = canvas.getContext("2d");
      const srcx = (this.tileid & 15) * colw;
      const srcy = (this.tileid >> 4) * rowh;
      context.drawImage(this.image, srcx, srcy, colw, rowh, 0, 0, colw, rowh);
    }
    
    this.element.querySelector(".tileid").innerText = `Tile 0x${this.tileid.toString(16).padStart('0', 2)}`;
    
    this.element.querySelector("select[name='physics']").value = this.tileprops.physics[this.tileid];
    this.element.querySelector("input[name='family']").value = this.tileprops.family[this.tileid];
    this.element.querySelector("input[name='probability']").value = this.tileprops.probability[this.tileid];
    
    const neighbors = this.tileprops.neighbors[this.tileid];
    for (let mask = 0x80; mask; mask >>= 1) {
      this.element.querySelector(`input.neighbor[value='${mask}']`).checked = !!(mask & neighbors);
    }
    
    const presets = this.element.querySelector(".presets");
    presets.innerHTML = "";
    const col = this.tileid & 15;
    const row = this.tileid >> 4;
    for (const preset of Tileprops.PRESETS) {
      const attr = { "on-click": () => this.applyPreset(preset) };
      if ((col + preset.w > 16) || (row + preset.h > 16)) {
        attr.disabled = "disabled";
      }
      const button = this.dom.spawn(presets, "BUTTON", attr);
      if (this.image) {
        const tileSize = this.image.naturalWidth >> 4;
        const canvas = this.dom.spawn(button, "CANVAS");
        canvas.width = preset.w * tileSize;
        canvas.height = preset.h * tileSize;
        const context = canvas.getContext("2d");
        context.drawImage(this.image, col * tileSize, row * tileSize, canvas.width, canvas.height, 0, 0, canvas.width, canvas.height);
      } else {
        this.dom.spawn(button, "DIV", preset.name);
      }
    }
  }
  
  onChange() {
    this.tileprops.physics[this.tileid] = +this.element.querySelector("select[name='physics']").value;
    this.tileprops.family[this.tileid] = +this.element.querySelector("input[name='family']").value;
    this.tileprops.probability[this.tileid] = +this.element.querySelector("input[name='probability']").value;
    
    let neighbors = 0;
    for (const element of this.element.querySelectorAll(".neighbor:checked")) neighbors |= +element.value;
    this.tileprops.neighbors[this.tileid] = neighbors;
    
    this.onDirty();
  }
  
  navigate(dx, dy) {
    let x = this.tileid & 15;
    let y = this.tileid >> 4;
    x += dx;
    if ((x < 0) || (x >= 16)) return;
    y += dy;
    if ((y < 0) || (y >= 16)) return;
    this.tileid = (y << 4) | x;
    this._populateUi();
  }
  
  applyPreset(preset) {
    const col = this.tileid & 15;
    const row = this.tileid >> 4;
    if (col + preset.w > 16) return;
    if (row + preset.h > 16) return;
    
    const physics = +this.element.querySelector("select[name='physics']").value;
    const family = +this.element.querySelector("input[name='family']").value;
    const probability = +this.element.querySelector("input[name='probability']").value;
    
    for (let y=0, mp=0; y<preset.h; y++) {
      for (let x=0; x<preset.w; x++, mp++) {
        const tileid = this.tileid + (y << 4) + x;
        this.tileprops.physics[tileid] = physics;
        this.tileprops.family[tileid] = family;
        this.tileprops.probability[tileid] = probability;
        this.tileprops.neighbors[tileid] = preset.masks[mp];
      }
    }
    
    this._populateUi();
    this.onDirty();
  }
}
