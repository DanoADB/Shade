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
const document={hidden:false,getElementById:get,addEventListener(){},createElement(){
  const el=element();el.querySelector=selector=>buttons.get(Number(selector.match(/\d+/)[0]));created.push(el);return el;
}};
const canvas=element(),calls=[];let now=0,tick;
const restored=[],stored=new Map();
const Module={canvas,_shade_gameplay:(...args)=>calls.push(args),_shade_gameplay_restore:(...args)=>{restored.push(args);return 1;}};
vm.runInNewContext(fs.readFileSync(require.resolve('../gameplay.js'),'utf8'),{
  document,Module,localStorage:{getItem:key=>stored.get(key),setItem:(key,value)=>stored.set(key,value)},
  window:{addEventListener(){}},performance:{now:()=>now},setInterval(fn){tick=fn;},Map,Math,Number
});
const event=(pointerId,x=220,y=140)=>({pointerId,clientX:x,clientY:y,button:0});
const send=(name,event)=>canvas.listeners[name](event);
send('pointerdown',event(1));send('pointerup',event(1));
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
get('task-toggle').listeners.click();assert.equal(get('task-body').hidden,true,'Task can collapse to reveal the scene');
get('task-toggle').listeners.click();assert.equal(get('task-body').hidden,false);
get('status').textContent='Loading';tick();assert.equal(calls.length,0,'Do not call engine before startup');
const snapshot=JSON.stringify({version:1,objects:[[1,42,10,20,30,0],[2,7,11,22,33,110]]});
Module.shadeGameplaySave(snapshot);
get('status').textContent='Running...';tick();assert.deepEqual(calls.pop(),[0,0,0]);
assert.deepEqual(restored,[[0,0,0,0,0,0],[1,42,10,20,30,0],[2,7,11,22,33,110],[3,0,0,0,0,0]],'Restore is staged then committed after engine startup');
tick();calls.pop();assert.equal(restored.length,4,'Restore only runs once');
document.hidden=true;tick();assert.equal(calls.length,0,'Do not poll a hidden page');
console.log('Gameplay tap, drag, pinch, cancellation, startup, save/resume and action availability passed.');
