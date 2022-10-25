/* RootController.js
 * Top level of our DOM. This gets created at load, and lives forever.
 */

import { Dom } from "/js/core.js";
import { ToolbarController } from "./ToolbarController.js";
import { ResService } from "/js/ResService.js";
import { MapService } from "/js/map/MapService.js";
import { MapController } from "/js/map/MapController.js";
import { LaunchService } from "/js/LaunchService.js";
import { ImageController } from "/js/image/ImageController.js";

export class RootController {
  static getDependencies() {
    return [HTMLDivElement, Dom, ResService, MapService, LaunchService, Window];
  }
  constructor(element, dom, resService, mapService, launchService, window) {
    this.element = element;
    this.dom = dom;
    this.resService = resService;
    this.mapService = mapService;
    this.launchService = launchService;
    this.window = window;
    
    this.toolbar = null;
    this.contentController = null;
    
    this._buildUi();
    this.resService.listen("toc", toc => this.toolbar.setResourceToc(toc));
    this._setupFragmentHandler();
  }
  
  _setupFragmentHandler() {
    this.onHashChange(this.window.location.hash);
    this.window.addEventListener("hashchange", event => this.onHashChange(event.newURL));
  }
  onHashChange(url) {
    const fragment = (url || "").split("#")[1] || "";
    const fragmentComponents = fragment.split("/").filter(a => a);
    switch (fragmentComponents[0]) {
      case "res": return this.setBodyViewRes(fragmentComponents[1], fragmentComponents[2], fragmentComponents.slice(3));
    }
    this.setBodyViewDefault();
  }
  
  _buildUi() {
    this.element.innerHTML = "";
    
    this.toolbar = this.dom.spawnController(this.element, ToolbarController);
    this.toolbar.setSaveStatus("clean");
    this.toolbar.onSave = () => this.onSave();
    this.toolbar.onShowSpecialView = (name) => this.onShowSpecialView(name);
    this.toolbar.onCommand = (name) => this.onCommand(name);
    this.toolbar.onShowResource = (type, name, path) => this.onShowResource(type, name, path);
    this.toolbar.setSpecialViewNames([]);
    this.toolbar.setCommandNames(["Launch", "New map"]);
    this.toolbar.setResourceToc(this.resService.toc);
    
    this.dom.spawn(this.element, "DIV", ["content"]);
  }
  
  setBodyViewDefault() {
    const contentPanel = this.element.querySelector(".content");
    contentPanel.innerHTML = "";
    this.contentController = null;
  }
  
  setBodyViewError(error) {
    console.error(`Error reported to RootController`, error);
    const contentPanel = this.element.querySelector(".content");
    contentPanel.innerHTML = "";
    this.contentController = null;
    if (error) {
      this.dom.spawn(contentPanel, "DIV", ["error"], error.message || error.toString());
    }
  }
  
  setBodyViewRes(type, name, args) {
    this.resService.getTocEntry(type, name)
      .then(r => {
        switch (r.type) {
          case "map": return this.setBodyViewRes_map(r);
          case "image": return this.setBodyViewRes_image(r);
          case "song": return this.setBodyViewRes_song(r);
          case "waves": return this.setBodyViewRes_waves(r);
          default: return this.setBodyViewError(`No view handler for type '${r.type}'`);
        }
      })
      .catch(e => {
        if ((type === "map") && e?.message?.includes("not found")) {
          this.setBodyViewDefault();
          this.promptForNewMap(name, args)
            .catch(e => {}); // eg cancelled
        } else {
          this.setBodyViewError(e);
        }
      });
  }
  
  /* Instantiate resource editors.
   **********************************************************************/
  
  setBodyViewRes_map(r) {
    const contentPanel = this.element.querySelector(".content");
    contentPanel.innerHTML = "";
    this.contentController = this.dom.spawnController(contentPanel, MapController, [this]);
    this.contentController.setResource(r);
  }
  
  setBodyViewRes_image(r) {
    const contentPanel = this.element.querySelector(".content");
    contentPanel.innerHTML = "";
    this.contentController = this.dom.spawnController(contentPanel, ImageController);
    this.contentController.setResource(r);
  }
  
  setBodyViewRes_song(r) {
    console.log(`RootController.setBodyViewRes_song`, r);
    const contentPanel = this.element.querySelector(".content");
    contentPanel.innerHTML = "";
    this.contentController = null;
    //TODO
  }
  
  setBodyViewRes_waves(r) {
    console.log(`RootController.setBodyViewRes_waves`, r);
    const contentPanel = this.element.querySelector(".content");
    contentPanel.innerHTML = "";
    this.contentController = null;
    //TODO
  }
  
  /* Event handlers.
   *****************************************************************************/
  
  onSave() {
    console.log(`TODO RootController.onSave`);
    //this.toolbar.setSaveStatus("pending");
    //window.setTimeout(() => this.toolbar.setSaveStatus("clean"), 2000);
  }
  
  onShowSpecialView(name) {
    switch (name) {
      default: console.log(`RootController.onShowSpecialView '${name}'`);
    }
  }
  
  onCommand(name) {
    switch (name) {
      case "Launch": this.launchService.launch(); break;
      case "New map": this.mapService.promptForNewMap().then(name => this.onShowResource("map", name)).catch(e => {}); break;
      default: console.log(`RootController.onCommand '${name}'`);
    }
  }
  
  onShowResource(type, name, path) {
    const url = `${this.window.location.pathname}#/res/${type}/${name}`;
    this.window.location = url;
  }
  
  promptForNewMap(name, args) {
    if (!this.window.confirm(`Map '${name}' does not exist. Proceeding will create it. Proceed?`)) return Promise.reject();
    return this.mapService.promptForNewMap(name).then(name => {
      // Can't use onShowResource() here because our URL may already be at it.
      this.setBodyViewRes("map", name, args);
    });
  }
}
