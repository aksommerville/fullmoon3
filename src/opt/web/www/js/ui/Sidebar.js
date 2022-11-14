/* Sidebar.js
 * Left of the main content, between Header and Footer.
 * I'm not sure we actually want something here, but it's ready if we are.
 */
 
import { Dom } from "../core/Dom.js";

export class Sidebar {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
  }
}
