const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const shell = fs.readFileSync(require.resolve('../engine-shell.html'),'utf8');
const config = shell.slice(shell.indexOf('const launchOptions'),shell.indexOf('var Module'));
function launch(search) {
 return vm.runInNewContext(config+';launchOptions',{URLSearchParams,location:{search}});
}
assert.equal(launch('').native,false);
assert.equal(launch('?native=1').native,true);
assert.equal(launch('?level=land4.txt').level,'Land4.txt');
assert.equal(launch('?level=LandT.txt').native,true);
assert.equal(launch('?level=../../bad').level,'Land1.txt');
assert.equal(launch('?level=../../bad').native,false);
const elements=new Map();
function element(){return {hidden:true,disabled:true,style:{},textContent:'',listeners:{},setAttribute(){},append(){},addEventListener(name,fn){this.listeners[name]=fn;}};}
const get=id=>{if(!elements.has(id))elements.set(id,element());return elements.get(id);};
let tick,calls=0,saves=0;
const canvas=element();
const Module={shadeNative:true,canvas,_shade_gameplay(){calls++;},_shade_gameplay_restore(){throw Error('Native mode cannot restore Shade save');}};
vm.runInNewContext(fs.readFileSync(require.resolve('../gameplay.js'),'utf8'),{
 Module,document:{getElementById:get,createElement:element,addEventListener(){},querySelectorAll(){return [];}},
 window:{addEventListener(){}},setInterval(fn){tick=fn;},localStorage:{setItem(){saves++;},getItem(){throw Error('Native mode cannot read Shade save');}},Map,Math,Number
});
get('status').textContent='Running...';tick();
Module.shadeGameplaySave('{}');Module.shadeGameplayUpdate({});
Module.shadeTempleChanged(true);Module.shadeTempleChanged(false);tick();
assert.equal(calls,0);assert.equal(saves,0);assert.equal(get('village').disabled,true);
console.log('Native launch allowlist and isolation from Shade task/save/camera passed.');
