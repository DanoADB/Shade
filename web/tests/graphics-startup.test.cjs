const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const html=fs.readFileSync(require.resolve('../engine-shell.html'),'utf8');
const script=html.slice(html.indexOf('const launchOptions'),html.indexOf('</script>',html.indexOf('const launchOptions')));
const elements=new Map();function get(id){if(!elements.has(id))elements.set(id,{textContent:'',value:'',disabled:true,hidden:true,listeners:{},addEventListener(name,fn){this.listeners[name]=fn;}});return elements.get(id);}
let reloads=0,ready=0,stopped=[];
const context={document:{getElementById:get},window:{addEventListener(){}},location:{search:'',reload(){reloads++;}},URLSearchParams};
vm.createContext(context);vm.runInContext(script,context);const m=context.Module;
m.mobileControls={ready(){ready++;}};m.shadeStop=message=>{stopped.push(message);get('status').textContent=message;};
m.canvas.listeners.webglcontextcreationerror({statusMessage:'GPU context blocked'});
m.shadeGraphicsFailure('WebGL2 context creation failed');assert(m.shadeGraphicsFailed);assert.match(get('status').textContent,/Graphics could not start/);assert.match(get('output').value,/GPU context blocked/);assert.equal(get('restart').textContent,'Retry graphics');assert(!get('restart').hidden);
m.setStatus('Running...');m.setStatus('Engine exited (1)');assert.equal(ready,0);assert.match(get('status').textContent,/Graphics could not start/);assert.equal(stopped.length,1);
get('restart').onclick();assert.equal(reloads,1);
console.log('Graphics startup errors preserve browser diagnostics, expose retry, and cannot be overwritten by loader/gameplay status.');
