"use strict";
// Shade mechanics prototype — independent of the unfinished openblack WASM port.
// No external assets. This is a design test, not a claim of engine integration.
const canvas=document.querySelector("#world"),ctx=canvas.getContext("2d");
const hud=document.querySelector("#hud"),logEl=document.querySelector("#log");
const W=960,H=540, village={x:680,y:280}, creature={x:310,y:290,target:null,stamina:100,trust:55,compassion:35,learning:0,state:"Exploring"};
let spirit=65,food=42,belief=45,time=0,mode="observe",paused=false,events=[];
const rng=()=>Math.random(),clamp=(v,a,b)=>Math.min(b,Math.max(a,v));
function log(t){events.unshift(t);events=events.slice(0,6);logEl.innerHTML=events.map(e=>"<div>"+e+"</div>").join("");}
function action(kind){
 if(kind==="rain"){if(spirit<35)return log("Not enough Spirit for rain.");spirit-=35;food=clamp(food+27,0,100);belief=clamp(belief+9,0,100);log("You summoned rain. The villagers credit their god.");}
 if(kind==="nudge"){if(spirit<7)return log("Not enough Spirit to persuade.");spirit-=7;const chance=clamp(.25+creature.trust/250+creature.compassion/350,0,.92);if(rng()<chance){creature.target={x:village.x-80,y:village.y+20};creature.state="Responding to your suggestion";log("Your creature considered the hungry village and approached.");}else{creature.trust=clamp(creature.trust-1,0,100);log("Your creature declined. It seems preoccupied.");}}
 if(kind==="comfort"){if(spirit<4)return log("Not enough Spirit.");spirit-=4;creature.trust=clamp(creature.trust+8,0,100);creature.stamina=clamp(creature.stamina+8,0,100);log("Your creature senses reassurance.");}
 if(kind==="wait"){time+=12;tick(12);log("You let the world unfold without interfering.");}
 update();
}
document.querySelectorAll("[data-action]").forEach(b=>b.addEventListener("click",()=>action(b.dataset.action)));
document.querySelector("#pause").onclick=()=>{paused=!paused;document.querySelector("#pause").textContent=paused?"Resume":"Pause";};
document.querySelector("#reset").onclick=()=>location.reload();
function tick(dt){
 time+=dt;food=clamp(food-dt*.14,0,100);
 spirit=clamp(spirit+dt*(belief/100)*.055,0,100);
 creature.stamina=clamp(creature.stamina+dt*.14,0,100);
 if(!creature.target && rng()<dt*.006){if(food<62 && rng()<(.2+creature.compassion/130)){creature.target={x:600,y:315};creature.state="Noticed hungry villagers";log("Your creature noticed the villagers' hunger on its own.");}else{creature.target={x:180+rng()*450,y:150+rng()*270};creature.state="Wandering";}}
 if(creature.target){let dx=creature.target.x-creature.x,dy=creature.target.y-creature.y,d=Math.hypot(dx,dy);if(d<8){const atVillage=creature.x>490;if(atVillage&&creature.stamina>15){food=clamp(food+14,0,100);belief=clamp(belief+5,0,100);creature.stamina-=16;creature.compassion=clamp(creature.compassion+3,0,100);creature.learning++;log("The creature fetched food. The villagers cheered.");}creature.target=null;creature.state="Resting";}else{const v=Math.min(d,dt*2.2);creature.x+=dx/d*v;creature.y+=dy/d*v;creature.stamina=clamp(creature.stamina-dt*.09,0,100);}}
 if(food<18)belief=clamp(belief-dt*.15,0,100);
 update();
}
function update(){hud.innerHTML=[["Spirit",spirit],["Village food",food],["Worship",belief],["Creature trust",creature.trust],["Compassion",creature.compassion],["Stamina",creature.stamina]].map(([k,v])=>'<div class="stat"><span>'+k+'</span><strong>'+Math.round(v)+'</strong><meter min="0" max="100" value="'+v+'"></meter></div>').join("")+'<div class="state">Creature: '+creature.state+' · Helpful actions learned: '+creature.learning+'</div>';}
function draw(){
 ctx.fillStyle="#203b37";ctx.fillRect(0,0,W,H);
 ctx.fillStyle="#315249";for(let i=0;i<35;i++){let x=(i*197+47)%W,y=(i*131+29)%H;ctx.beginPath();ctx.arc(x,y,8+i%7,0,Math.PI*2);ctx.fill();}
 ctx.fillStyle="#487b8a";ctx.beginPath();ctx.ellipse(490,510,550,85,-.18,0,Math.PI*2);ctx.fill();
 ctx.fillStyle="#d5bc8c";for(let i=0;i<8;i++){let x=village.x+(i%4)*39-60,y=village.y+Math.floor(i/4)*49-10;ctx.fillRect(x-13,y-8,28,22);ctx.fillStyle="#a7654f";ctx.beginPath();ctx.moveTo(x-18,y-8);ctx.lineTo(x,y-24);ctx.lineTo(x+18,y-8);ctx.fill();ctx.fillStyle="#d5bc8c";}
 ctx.fillStyle="#e8ca96";for(let i=0;i<9;i++){let x=village.x-70+i*19,y=village.y+92+(i%2)*8;ctx.beginPath();ctx.arc(x,y,4,0,Math.PI*2);ctx.fill();}
 ctx.save();ctx.translate(creature.x,creature.y);ctx.fillStyle="#b3adbd";ctx.beginPath();ctx.ellipse(0,0,63,46,0,0,Math.PI*2);ctx.fill();ctx.beginPath();ctx.arc(47,-25,29,0,Math.PI*2);ctx.fill();ctx.fillStyle="#e9d9ac";ctx.beginPath();ctx.arc(60,-23,5,0,Math.PI*2);ctx.fill();ctx.fillStyle="#222";ctx.beginPath();ctx.arc(61,-23,2,0,Math.PI*2);ctx.fill();ctx.fillStyle="#a697b5";ctx.beginPath();ctx.moveTo(28,-42);ctx.lineTo(31,-78);ctx.lineTo(43,-49);ctx.fill();ctx.restore();
 ctx.font="16px system-ui";ctx.textAlign="center";ctx.fillStyle="#f9eace";ctx.fillText("THE KNOWN GOD",creature.x,creature.y-94);ctx.fillText("THE BELIEVERS",village.x,village.y-75);
 ctx.textAlign="left";ctx.fillStyle="#d3e4de";ctx.fillText("Time: "+Math.floor(time),20,30);
 requestAnimationFrame(draw);
}
let previous=performance.now();function frame(now){const dt=Math.min((now-previous)/1000*3,1.5);previous=now;if(!paused)tick(dt);setTimeout(()=>requestAnimationFrame(frame),80);}
log("The villagers are running short of food. Your creature is free to act.");update();draw();requestAnimationFrame(frame);
