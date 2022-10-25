/* LaunchService.js
 */
 
import { Comm } from "/js/core.js";
 
export class LaunchService {
  static getDependencies() {
    return [Comm];
  }
  constructor(comm) {
    this.comm = comm;
  }
  
  launch() {
    console.log(`TODO LaunchService.launch`);
  }
}

LaunchService.singleton = true;
