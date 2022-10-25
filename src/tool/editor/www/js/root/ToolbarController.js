/* ToolbarController.js
 * Left side of the top level UI, owned by RootController.
 */
 
import { Dom } from "/js/core.js";

export class ToolbarController {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    // Parent should replace:
    this.onSave = () => {};
    this.onShowSpecialView = (name) => {};
    this.onCommand = (name) => {};
    this.onShowResource = (type, name, path) => {};
    
    this.buildUi();
  }
  
  buildUi() {
    this.innerHTML = "";
    
    const statusRow = this.dom.spawn(this.element, "DIV", ["status"]);
    this.dom.spawn(statusRow, "INPUT", { type: "button", "on-click": () => this.onSave(), value: "Save" });
    this.dom.spawn(statusRow, "DIV", ["indicator"]);
    
    const menuRow = this.dom.spawn(this.element, "DIV", ["viewsMenu"]);
    this.dom.spawn(menuRow, "SELECT", ["views"], { "on-change": (event) => this.onViewSelected(event) });
    
    const commandsRow = this.dom.spawn(this.element, "DIV", ["commandsMenu"]);
    this.dom.spawn(commandsRow, "SELECT", ["commands"], { "on-change": (event) => this.onCommandSelected(event) });
    
    const resourcesRow = this.dom.spawn(this.element, "UL", ["resources"], { "on-click": (event) => this.onResourceSelected(event) });
    
    this.setSpecialViewNames([]);
  }
  
  setSaveStatus(status) {
    const statuses = ["clean", "dirty", "pending"];
    if (statuses.indexOf(status) < 0) return;
    const indicator = this.element.querySelector(".status > .indicator");
    for (const s of statuses) indicator.classList.remove(s);
    indicator.classList.add(status);
  }
  
  setSpecialViewNames(names) {
    const menu = this.element.querySelector(".viewsMenu > .views");
    menu.innerHTML = "";
    this.dom.spawn(menu, "OPTION", "Special views", { value: "", disabled: "disabled", selected: "selected" });
    for (const name of names) {
      this.dom.spawn(menu, "OPTION", name, { value: name });
    }
  }
  
  setCommandNames(names) {
    const menu = this.element.querySelector(".commandsMenu > .commands");
    menu.innerHTML = "";
    this.dom.spawn(menu, "OPTION", "Commands", { value: "", disabled: "disabled", selected: "selected" });
    for (const name of names) {
      this.dom.spawn(menu, "OPTION", name, { value: name });
    }
  }
  
  setResourceToc(toc) {
    const tocElement = this.element.querySelector(".resources");
    tocElement.innerHTML = "";
    const listsByRestype = {};
    for (const res of toc) {
    
      // Don't show appendix types.
      if (res.type === "tileprops") continue;
      if (res.type === "adjust") continue;
    
      let list = listsByRestype[res.type];
      if (!list) {
        const header = this.dom.spawn(tocElement, "LI", ["restype-header"], { "data-restype": res.type });
        this.dom.spawn(header, "DIV", ["restype-label"], { "data-restype": res.type }, res.type);
        list = this.dom.spawn(header, "UL", ["restype", "hidden"], { "data-restype": res.type });
        listsByRestype[res.type] = list;
      }
      const line = this.dom.spawn(list, "LI", ["res"], {
        "data-name": res.name,
        "data-path": res.path,
        "data-restype": res.type,
      }, res.name);
    }
  }
  
  onViewSelected(event) {
    const viewName = event.target.value;
    const menu = this.element.querySelector(".viewsMenu > .views");
    menu.value = "";
    this.onShowSpecialView(viewName);
  }
  
  onCommandSelected(event) {
    const commandName = event.target.value;
    const menu = this.element.querySelector(".commandsMenu > .commands");
    menu.value = "";
    this.onCommand(commandName);
  }
  
  // Could also be clicking a group header, or in undefined space.
  onResourceSelected(event) {
    if (!event.target) return;
    if (event.target.classList.contains("res")) {
      const type = event.target.getAttribute("data-restype");
      const name = event.target.getAttribute("data-name");
      const path = event.target.getAttribute("data-path");
      if (type && name && path) {
        this.onShowResource(type, name, path);
      }
      return;
    }
    if (event.target.classList.contains("restype-label")) {
      const list = event.target.parentNode?.querySelector?.(".restype");
      if (list) {
        if (list.classList.contains("hidden")) list.classList.remove("hidden");
        else list.classList.add("hidden");
      }
      return;
    }
  }
}
