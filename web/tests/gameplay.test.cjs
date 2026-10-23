const assert = require('node:assert/strict');
const vm = require('node:vm');
const fs = require('node:fs');
function element() {
  return {textContent:'',style:{},listeners:{},hidden:false,disabled:false,
    setAttribute(){},append(){},addEventListener(name,fn){this.listeners[name]=fn;},
    getBoundingClientRect(){return {left:20,top:40,width:400,height:200};}};
}
const elements=new Map();
const get=id=>{if(!elements.has(id))elements.set(id,element());return elements.get(id);};
const buttons=new Map([2,3,7,8].map(code=>[code,element()]));
const created=[];
const cameraButtons=[element()];
const document={hidden:false,getElementById:get,addEventListener(){},querySelectorAll(){return cameraButtons;},createElement(){
  const el=element();el.querySelector=selector=>buttons.get(Number(selector.match(/\d+/)[0]));created.push(el);return el;
}};
const canvas=element(),calls=[];let now=0,tick;
const restored=[],stored=new Map();
const frames=[];
let paused=0,stopped=0,reloaded=0;
const Module={canvas,mobileControls:{stop(){stopped++;}},_shade_pause_loop(){paused++;},_shade_gameplay:(...args)=>calls.push(args),_shade_gameplay_restore:(...args)=>{restored.push(args);return 1;}};
vm.runInNewContext(fs.readFileSync(require.resolve('../gameplay.js'),'utf8'),{
  document,Module,location:{reload(){reloaded++;}},localStorage:{getItem:key=>stored.get(key),setItem:(key,value)=>stored.set(key,value)},
  window:{addEventListener(){}},requestAnimationFrame:fn=>frames.push(fn),performance:{now:()=>now},setInterval(fn){tick=fn;},Map,Math,Number
});
const event=(pointerId,x=220,y=140)=>({pointerId,clientX:x,clientY:y,button:0});
const send=(name,event)=>canvas.listeners[name](event);
send('pointerdown',event(1));send('pointerup',event(1));
assert.equal(calls.length,0,'Wait for native menu input capture before selection');
while(frames.length)frames.shift()();
assert.deepEqual(calls.pop(),[1,.5,.5],'Selection uses canvas-relative coordinates');
send('pointerdown',event(1));send('pointermove',event(1,250));send('pointerup',event(1,220));
assert.equal(calls.length,0,'A drag returning to its start must not select');
send('pointerdown',event(1));send('pointerdown',event(2));send('pointerup',event(2));send('pointerup',event(1));
assert.equal(calls.length,0,'Pinch release must not select');
send('pointerdown',event(1));send('pointercancel',event(1));send('pointerup',event(1));
assert.equal(calls.length,0,'Cancelled input must not select');
send('pointerdown',event(1));now=700;send('pointerup',event(1));
assert.equal(calls.length,0,'Long camera holds must not select');
Module.shadeGameplayUpdate({kind:1,gathered:false,carrying:false,delivered:0,x:.5,y:.5,message:'Gather'});
assert.equal(buttons.get(2).disabled,false);assert.equal(buttons.get(3).disabled,true);
Module.shadeGameplayUpdate({kind:2,carrying:true,delivered:20,wood:12,food:3,people:2,x:.5,y:.5,message:'Deliver'});
assert.equal(buttons.get(2).disabled,true);assert.equal(buttons.get(3).disabled,false);
assert.equal(get('selected-info').textContent,'Wood: 12 · Food: 3 · Residents: 2');
Module.shadeGameplayUpdate({kind:4,gathered:false,carrying:false,delivered:20,fed:0,x:.5,y:.5,message:'Harvest'});
assert.equal(buttons.get(2).textContent,'Harvest food');assert.equal(buttons.get(2).disabled,false);
Module.shadeGameplayUpdate({kind:3,foodHeld:true,carrying:true,hunger:10,delivered:20,fed:0,x:.5,y:.5,message:'Feed'});
assert.equal(buttons.get(3).textContent,'Feed villager');assert.equal(buttons.get(3).disabled,false);
Module.shadeGameplayUpdate({kind:3,foodHeld:true,carrying:true,hunger:0,delivered:20,fed:10,x:.5,y:.5,message:'Full'});
assert.equal(buttons.get(3).disabled,true,'A full villager cannot consume another bundle');
assert.equal(get('food-progress').textContent,'Food eaten: 10 / 10 · Complete · Carrying 10 food');
Module.shadeGameplayUpdate({kind:3,foodHeld:false,carrying:true,hunger:100,delivered:20,x:.5,y:.5,message:'Wood'});
assert.equal(buttons.get(3).disabled,true,'Wood cannot feed a villager');
get('task-toggle').listeners.click();assert.equal(get('task-body').hidden,true,'Task can collapse to reveal the scene');
get('task-toggle').listeners.click();assert.equal(get('task-body').hidden,false);
get('status').textContent='Loading';tick();assert.equal(calls.length,0,'Do not call engine before startup');
const snapshot=JSON.stringify({version:1,objects:[[1,42,10,20,30,0],[2,7,11,22,33,110],[4,8,12,22,33,0],[5,7,11,22,33,10],[6,9,12,21,32,90]]});
Module.shadeGameplaySave(snapshot);
get('status').textContent='Running...';tick();assert.deepEqual(calls.pop(),[0,0,0]);
assert.deepEqual(restored,[[0,0,0,0,0,0],[1,42,10,20,30,0],[2,7,11,22,33,110],[4,8,12,22,33,0],[5,7,11,22,33,10],[6,9,12,21,32,90],[3,0,0,0,0,0]],'Wood, fields, food stocks and hunger restore in one staged transaction');
tick();calls.pop();assert.equal(restored.length,7,'Restore only runs once');
Module.shadeTempleChanged(true);tick();assert.equal(calls.length,0,'Citadel must suspend island task polling');
assert.equal(created[0].hidden,true);assert.equal(get('village').disabled,true);
Module.shadeTempleChanged(false);tick();assert.deepEqual(calls.pop(),[0,0,0]);
assert.equal(created[0].hidden,false);assert.equal(get('village').disabled,false);
document.hidden=true;tick();assert.equal(calls.length,0,'Do not poll a hidden page');
document.hidden=false;Module.shadeMapChanged();tick();assert.equal(calls.length,0,'Do not apply Land1 task state after native island switching');
assert.equal(created[0].hidden,true);assert.equal(get('village').disabled,true);
Module.shadeTempleChanged(true);Module.shadeTempleChanged(false);tick();
assert.equal(calls.length,0,'Leaving Citadel must not reactivate a different island task');
assert.equal(created[0].hidden,true);assert.equal(get('village').disabled,true);
let prevented=false;
send('webglcontextlost',{preventDefault(){prevented=true;}});
assert.equal(prevented,true);assert.equal(paused,1);assert.equal(stopped,1);
assert.equal(cameraButtons[0].disabled,true);assert.equal(get('restart').hidden,false);
send('webglcontextrestored',{});assert.match(get('status').textContent,/Graphics restored/);
get('restart').onclick();assert.equal(reloaded,1);
tick();assert.equal(calls.length,0,'Stopped engine must not receive task polling');
console.log('Gameplay tap, drag, pinch, cancellation, startup, save/resume and action availability passed.');
