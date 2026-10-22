"use strict";
// Shade mechanics prototype — independent of the unfinished openblack WASM port.
// No external assets. This is a design test, not a claim of engine integration.
const hud=document.querySelector("#hud"),logEl=document.querySelector("#log");
const W=960,H=540, village={x:680,y:280}, creature={x:310,y:290,target:null,stamina:100,trust:55,compassion:35,learning:0,state:"Exploring"};
let spirit=65,food=42,belief=45,time=0,mode="observe",paused=false,events=[];
const rng=()=>Math.random(),clamp=(v,a,b)=>Math.min(b,Math.max(a,v));
function log(t){events.unshift(t);events=events.slice(0,6);logEl.replaceChildren(...events.map(e=>{const row=document.createElement("div");row.textContent=e;return row;}));}
function action(kind){
 if(kind==="rain"){if(spirit<35)return log("Not enough Spirit for rain.");spirit-=35;food=clamp(food+27,0,100);belief=clamp(belief+9,0,100);log("You summoned rain. The villagers credit their god.");}
 if(kind==="nudge"){if(spirit<7)return log("Not enough Spirit to persuade.");spirit-=7;const chance=clamp(.25+creature.trust/250+creature.compassion/350,0,.92);if(rng()<chance){creature.target={x:village.x-80,y:village.y+20};creature.state="Responding to your suggestion";log("Your creature considered the hungry village and approached.");}else{creature.trust=clamp(creature.trust-1,0,100);log("Your creature declined. It seems preoccupied.");}}
 if(kind==="comfort"){if(spirit<4)return log("Not enough Spirit.");spirit-=4;creature.trust=clamp(creature.trust+8,0,100);creature.stamina=clamp(creature.stamina+8,0,100);log("Your creature senses reassurance.");}
 if(kind==="wait"){for(let i=0;i<120;i++)tick(.1);log("You let the world unfold without interfering.");}
 update();
}
document.querySelectorAll("[data-action]").forEach(b=>b.addEventListener("click",()=>action(b.dataset.action)));
document.querySelector("#pause").onclick=()=>{paused=!paused;document.querySelector("#pause").textContent=paused?"Resume":"Pause";update();};
document.querySelector("#reset").onclick=()=>{
 spirit=65;food=42;belief=45;time=0;paused=false;events=[];
 Object.assign(creature,{x:310,y:290,target:null,stamina:100,trust:55,compassion:35,learning:0,state:"Exploring"});
 document.querySelector("#pause").textContent="Pause";
 log("A new world begins. Your saved world is still available.");update();
};
const SAVE_KEY="shade.prototype.save.v1";
const saveStatus=document.querySelector("#save-status");
function saveWorld(){
 try{
  localStorage.setItem(SAVE_KEY,JSON.stringify({version:1,savedAt:new Date().toISOString(),spirit,food,belief,time,creature:{...creature},events:[...events]}));
  saveStatus.textContent="Saved on this browser. Use Load world to continue later.";
 }catch(error){saveStatus.textContent="Could not save: browser storage is unavailable or full.";}
}
function loadWorld(){
 try{
  const raw=localStorage.getItem(SAVE_KEY);
  if(!raw){saveStatus.textContent="No saved world yet. Choose Save world first.";return;}
  const data=JSON.parse(raw), c=data.creature;
  const bounded=(v,min,max)=>Number.isFinite(v)&&v>=min&&v<=max;
  if(data.version!==1 || !c || ![data.spirit,data.food,data.belief,c.stamina,c.trust,c.compassion].every(v=>bounded(v,0,100)) ||
     !bounded(data.time,0,Number.MAX_SAFE_INTEGER) || !bounded(c.x,0,W) || !bounded(c.y,0,H) ||
     !Number.isSafeInteger(c.learning) || c.learning<0 || typeof c.state!=="string" || c.state.length>150 ||
     !Array.isArray(data.events) || data.events.length>6 || !data.events.every(e=>typeof e==="string"&&e.length<500) ||
     (c.target!==null && (!c.target || !bounded(c.target.x,0,W) || !bounded(c.target.y,0,H)))) throw new Error("Invalid save");
  spirit=data.spirit;food=data.food;belief=data.belief;time=data.time;
  Object.assign(creature,{x:c.x,y:c.y,target:c.target?{x:c.target.x,y:c.target.y}:null,stamina:c.stamina,trust:c.trust,compassion:c.compassion,learning:c.learning,state:c.state});
  events=data.events;paused=true;document.querySelector("#pause").textContent="Resume";
  log("Saved world restored. Resume when ready.");update();
  saveStatus.textContent="Loaded saved world · paused. No offline time has elapsed.";
 }catch(error){saveStatus.textContent="Could not load this save. Your current world has been kept.";}
}
document.querySelector("#save").onclick=saveWorld;
document.querySelector("#load").onclick=loadWorld;
function tick(dt){
 time+=dt;food=clamp(food-dt*.14,0,100);
 spirit=clamp(spirit+dt*(belief/100)*.055,0,100);
 creature.stamina=clamp(creature.stamina+dt*.14,0,100);
 if(!creature.target && rng()<dt*.006){if(food<62 && rng()<(.2+creature.compassion/130)){creature.target={x:600,y:315};creature.state="Noticed hungry villagers";log("Your creature noticed the villagers' hunger on its own.");}else{creature.target={x:180+rng()*450,y:150+rng()*270};creature.state="Wandering";}}
 if(creature.target){let dx=creature.target.x-creature.x,dy=creature.target.y-creature.y,d=Math.hypot(dx,dy);if(d<8){const atVillage=Math.hypot(creature.target.x-600,creature.target.y-315)<30;if(atVillage&&creature.stamina>15){food=clamp(food+14,0,100);belief=clamp(belief+5,0,100);creature.stamina-=16;creature.compassion=clamp(creature.compassion+3,0,100);creature.learning++;log("The creature fetched food. The villagers cheered.");}creature.target=null;creature.state="Resting";}else{const v=Math.min(d,dt*12);creature.x+=dx/d*v;creature.y+=dy/d*v;creature.stamina=clamp(creature.stamina-dt*.09,0,100);}}
 if(food<18)belief=clamp(belief-dt*.15,0,100);
 update();
}
function update(){window.shadeSnapshot={spirit,food,belief,time,paused,creature:{...creature,target:creature.target?{...creature.target}:null}};hud.innerHTML=[["Spirit",spirit],["Village food",food],["Worship",belief],["Creature trust",creature.trust],["Compassion",creature.compassion],["Stamina",creature.stamina]].map(([k,v])=>'<div class="stat"><span>'+k+'</span><strong>'+Math.round(v)+'</strong><meter min="0" max="100" value="'+v+'"></meter></div>').join("")+'<div class="state">Creature: '+creature.state.replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))+' · Helpful actions learned: '+creature.learning+'</div>';}
let previous=performance.now();function frame(now){const dt=Math.min((now-previous)/1000*3,1.5);previous=now;if(!paused)tick(dt);setTimeout(()=>requestAnimationFrame(frame),80);}
log("The villagers are running short of food. Your creature is free to act.");update();requestAnimationFrame(frame);
