/* MapController.js
 * Top level of map editor, we are scoped to a single resource.
 */
 
import { Dom } from "/js/core.js";
import { MapService } from "/js/map/MapService.js";
import { FullmoonMap } from "/js/map/FullmoonMap.js";
import { ResService } from "/js/ResService.js";
import { MapCanvas } from "./MapCanvas.js";
import { RootController } from "/js/root/RootController.js";
import { MapPaintService } from "./MapPaintService.js";
import { MapToolbox } from "./MapToolbox.js";
import { MapCommands } from "./MapCommands.js";
import { NewMapCommandModal } from "./NewMapCommandModal.js";
import { Tileprops } from "/js/image/Tileprops.js";

export class MapController {
  static getDependencies() {
    return [HTMLElement, Dom, MapService, ResService, Window, RootController, MapPaintService];
  }
  constructor(element, dom, mapService, resService, window, rootController, mapPaintService) {
    this.element = element;
    this.dom = dom;
    this.mapService = mapService;
    this.resService = resService;
    this.window = window;
    this.rootController = rootController;
    this.mapPaintService = mapPaintService;
    
    this.res = null;
    this.mapCanvas = null;
    this.mapToolbox = null;
    this.mapCommands = null;
    
    this.mapPaintService.tileprops = null;
    this.mapPaintService.onDirty = (map) => {
      this.onDirty(map);
      this.mapCanvas?._renderSoon();
    };
    this.mapPaintService.onSelectedTileChanged = (tileid) => this.mapToolbox?.setSelectedTile(tileid);
    this.mapPaintService.onNavigate = (nameOrObject) => this.onNavigate(nameOrObject);
    this.mapPaintService.onAddCommand = (x, y) => this.onAddCommand(x, y);
    this.mapPaintService.onEditCommand = (command, map) => this.onEditCommand(command, map);
    
    this._buildUi();
  }
  
  onRemoveFromDom() {
  }
  
  setResource(res) {
    this.res = res;
    if (!this.res.object) {
      this.res.object = this.mapService.decode(this.res.text || this.res.serial);
    }
    this.mapPaintService.setMap(this.res.object);
    this.mapCanvas.setMap(this.res.object, this.res.name);
    this.mapToolbox.setMap(this.res.object);
    this.mapCommands.setMap(this.res.object);
    this.resService.getTocEntry("tileprops", this.res.object.getTilesheetName()).then(tp => {
      if (!tp.object) tp.object = new Tileprops(tp.text);
      this.mapPaintService.tileprops = tp.object;
    }).catch(e => { console.log(e); });
  }
  
  /* Build UI.
   ********************************************************************/
  
  _buildUi() {
    this.element.innerHTML = "";
    
    this.mapCanvas = this.dom.spawnController(this.element, MapCanvas);
    this.mapCanvas.onNavigate = (name) => this.onNavigate(name);
    this.mapCanvas.onCreateNeighbor = (dir, name) => this.onCreateNeighbor(dir, name);
    this.mapCanvas.onDirty = () => this.onDirty();
    
    const toolbar = this.dom.spawn(this.element, "DIV", ["toolbar"]);
    
    this.mapToolbox = this.dom.spawnController(toolbar, MapToolbox);

    this.mapCommands = this.dom.spawnController(toolbar, MapCommands);
  }
  
  /* Event handlers.
   ****************************************************************/
   
  onNavigate(name) {
    if (typeof(name) === "string") {
      this.rootController.onShowResource("map", name);
    } else if ((name?.type === "map") && name?.name) {
      this.rootController.onShowResource("map", name.name);
    } else if (name instanceof FullmoonMap) {
      this.resService.getNameForObject(name)
        .then(name => this.rootController.onShowResource("map", name))
        .catch(e => {});
    }
  }
  
  onCreateNeighbor(dir, name) {
    if (!this.res?.object) return;
    this.mapService.promptForNewMap(name)
      .then(() => {
        this.res.object.setNeighborName(dir, name);
        this.onDirty();
      })
      .then(() => this.resService.getTocEntry("map", name))
      .then((neighbor) => {
        if (!neighbor.object) neighbor.object = this.mapService.decode(neighbor.text);
        neighbor.object.setNeighborName(this.mapService.oppositeDir(dir), this.res.name);
        this.mapService.discoverNeighbors(neighbor);
        this.resService.dirty("map", name, () => neighbor.object.encode());
      })
      .then(() => this.rootController.onShowResource("map", name))
      .catch(e => { console.error(e); });
  }
  
  onDirty(map) {
    if (!this.res) return;
    if (map && (map instanceof FullmoonMap) && (map !== this.res.object)) {
      this.resService.dirtyObject(map, () => map.encode());
    } else if (map && (map.type === "map") && map.object) {
      this.resService.dirty("map", map.name, () => map.object.encode());
    } else {
      this.resService.dirty("map", this.res.name, () => this.res.object.encode());
    }
  }
  
  onEditCommand(command, map) {
    const modal = this.dom.spawnModal(NewMapCommandModal);
    modal.setupExisting(command, map);
    modal.onAddCommand = (incoming) => {
      command.splice(0, command.length);
      for (const arg of incoming) command.push(arg);
      this.resService.dirtyObject(map, () => map.encode());
      this.mapCanvas?._rebuildBadges();
    };
  }
  
  onAddCommand(x, y) {
    if (!this.res?.object) return;
    const modal = this.dom.spawnModal(NewMapCommandModal);
    modal.setup(this.res.object, x, y);
    modal.onAddCommand = (command) => {
      this.res.object.commands.push(command);
      this.resService.dirty("map", this.res.name, () => this.res.object.encode());
      this.mapCanvas?._rebuildBadges();
    };
  }
   
}
