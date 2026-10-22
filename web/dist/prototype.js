"use strict";
const canvas=document.querySelector("#world"),ctx=canvas.getContext("2d");
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

draw();
