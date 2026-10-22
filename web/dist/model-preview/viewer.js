import {renderScene} from './renderer.js';
import {createScene} from './scene.js';
const canvas=document.querySelector('#view'),status=document.querySelector('#status');
let renderer,contextLost=false;
canvas.addEventListener('webglcontextlost',event=>{event.preventDefault();contextLost=true;renderer?.stop();status.textContent='Graphics context lost. Reload the page to restart the scene.';});
canvas.addEventListener('webglcontextrestored',()=>{status.textContent='Graphics context restored. Reload the page to restart.';});
async function start(){
 let cow=null,reason='';
 try{const response=await fetch('/local-cow.json',{signal:AbortSignal.timeout(10000)});if(!response.ok)throw Error('Local cow export is not enabled.');cow=await response.json();if(!Array.isArray(cow.primitives)||!cow.primitives.length||!Array.isArray(cow.textures))throw Error('Cow export is invalid.');}
 catch(error){reason=error.message;}
 if(contextLost)return;
 try{renderer=renderScene(canvas,createScene(cow),[0,1,0],8,()=>window.shadeSnapshot);status.textContent=cow?`Scene ready · original cow${cow.village?' and Celtic village models':''}${cow.village?.oak?' · original trees and grass':''} · simulation connected`:'Scene ready with placeholder creature · '+reason+' Enable the local cow export to use the original model.';}
 catch(error){status.textContent='Scene error: '+error.message;console.error(error);}
}
start();
