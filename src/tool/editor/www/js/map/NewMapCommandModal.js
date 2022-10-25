/* NewMapCommandModal.js
 */
 
import { Dom } from "/js/core.js";
import { ResService } from "/js/ResService.js";
import { FullmoonMap } from "./FullmoonMap.js";
import { MapService } from "./MapService.js";

export class NewMapCommandModal {
  static getDependencies() {
    return [HTMLElement, Dom, ResService, MapService, "discriminator"];
  }
  constructor(element, dom, resService, mapService, discriminator) {
    this.element = element;
    this.dom = dom;
    this.resService = resService;
    this.mapService = mapService;
    this.discriminator = discriminator;
    
    this.onAddCommand = (command) => {};
    
    this.map = null;
    this.x = 0;
    this.y = 0;
    this.command = null;
  }
  
  setup(map, x, y) {
    this.map = map;
    this.x = x;
    this.y = y;
    this.command = null;
    this._buildUi();
  }
  
  setupExisting(command, map) {
    this.map = map;
    this.command = command;
    this._buildUi();
  }
  
  _buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "H2", this.command ? 'Edit command' : `New command at ${this.x},${this.y}`);
    const form = this.dom.spawn(this.element, "FORM");
    
    const keywordMenu = this.dom.spawn(form, "SELECT", { name: "keyword", value: "", "on-change": () => this.onKeywordChanged() });
    this.dom.spawn(keywordMenu, "OPTION", "Select command...", { value: "", disabled: "disabled", selected: "selected" });
    for (const command of FullmoonMap.COMMANDS_SCHEMA) {
      this.dom.spawn(keywordMenu, "OPTION", command.keyword, { value: command.keyword });
    }
    
    const volatileSection = this.dom.spawn(form, "TABLE", ["volatile"]);
    
    this.dom.spawn(form, "INPUT", { type: "submit", value: this.command ? "Save" : "Add", "on-click": () => this.onAddOrSave() });
    
    if (this.command) {
      keywordMenu.value = this.command[0];
      this.onKeywordChanged();
    }
  }
  
  onKeywordChanged() {
    const keyword = this.element.querySelector("select[name='keyword']")?.value;
    const schema = FullmoonMap.COMMANDS_SCHEMA.find(s => s.keyword === keyword);
    const volatileSection = this.element.querySelector(".volatile");
    volatileSection.innerHTML = "";
    if (schema) this._rebuildVolatileSection(volatileSection, schema);
  }
  
  _rebuildVolatileSection(parent, schema) {
    let wordp = 1;
    for (const arg of schema.args || []) {
      const tr = this.dom.spawn(parent, "TR");
      const tdk = this.dom.spawn(tr, "TD", ["key"], arg);
      const tdv = this.dom.spawn(tr, "TD", ["value"]);
      this._buildValueInputForField(tdv, arg, schema.keyword, wordp);
      wordp++;
    }
  }
  
  _buildValueInputForField(parent, arg, keyword, wordp) {
    const attr = { name: arg };
    let datalistResType = "";
    switch (arg) {
      case "x": attr.type = "number"; attr.min = 0; attr.max = FullmoonMap.COLC - 1; attr.value = this.x; break;
      case "y": attr.type = "number"; attr.min = 0; attr.max = FullmoonMap.ROWC - 1; attr.value = this.y; break;
      // (dstx,dsty) aren't really related to the (x,y) we have, but might as well start somewhere...
      case "dstx": attr.type = "number"; attr.min = 0; attr.max = FullmoonMap.COLC - 1; attr.value = this.x; break;
      case "dsty": attr.type = "number"; attr.min = 0; attr.max = FullmoonMap.ROWC - 1; attr.value = this.y; break;
      case "tile": attr.type = "number"; attr.min = 0; attr.max = 0xff; break;
      case "image": {
          attr.type = "text";
          attr.list = `NewMapCommandModal-${this.discriminator}-image`;
          datalistResType = "image";
        } break;
      case "song": {
          attr.type = "text";
          attr.list = `NewMapCommandModal-${this.discriminator}-song`;
          datalistResType = "song";
        } break;
      case "map": {
          attr.type = "text";
          attr.list = `NewMapCommandModal-${this.discriminator}-map`;
          datalistResType = "map";
        } break;
      case "flags": {
          attr.type = "text";
          //TODO datalist with whatever these things are
        } break;
      case "controller": {
          attr.type = "text";
          //TODO datalist with sprite controller names -- new REST call
        } break;
      case "args": {
          attr.type = "text";
          //TODO not sure how we want to handle these
        } break;
      case "event": {
          attr.type = "text";
          //TODO datalist with event enums -- where do these come from?
        } break;
      case "hook": {
          attr.type = "text";
          //TODO datalist with hook names -- new REST call
        } break;
      default: {
          attr.type = "text";
        }
    }
    if (this.command && (wordp >= 0) && (wordp < this.command.length)) {
      attr.value = this.command[wordp];
    }
    const input = this.dom.spawn(parent, "INPUT", attr);
    
    if (datalistResType) {
      const datalist = this.dom.spawn(parent, "DATALIST", { id: attr.list });
      for (const value of this.resService.toc.filter(r => r.type === datalistResType).map(r => r.name)) {
        this.dom.spawn(datalist, "OPTION", { value });
      }
    }
  }
  
  _composeAndSubmitValue() {
    const keyword = this.element.querySelector("select[name='keyword']")?.value;
    const schema = FullmoonMap.COMMANDS_SCHEMA.find(s => s.keyword === keyword);
    if (!schema) return false;
    const command = [keyword];
    for (const arg of schema.args) {
      const element = this.element.querySelector(`.volatile *[name='${arg}']`);
      if (!element) {
        console.log(`Field '${arg}' not found in NewMapCommandModal for keyword '${keyword}'`);
        command.push("");
      } else {
        command.push(element.value || "");
      }
    }
    this.onAddCommand(command);
    return true;
  }
  
  onAddOrSave() {
    // (!!this.command) tells us "save" vs "add", but actually we don't need to distinguish them.
    if (!this._composeAndSubmitValue()) return;
    this.dom.popModal(this);
  }
}
