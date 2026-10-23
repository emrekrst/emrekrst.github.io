import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {runInNewContext} from 'node:vm';

test('featured video plays only when visible in an active tab and handles autoplay refusal',async()=>{
 const app=await readFile(new URL('../app.mjs',import.meta.url),'utf8');
 let observe,visibility,plays=0,pauses=0;
 const video={muted:false,play(){plays++;return Promise.reject(new Error('Browser refused autoplay'));},pause(){pauses++;}};
 const document={hidden:false,addEventListener(name,fn){assert.equal(name,'visibilitychange');visibility=fn;}};
 runInNewContext(app.slice(app.indexOf("const trackerVideo=")),{
  $:selector=>{assert.equal(selector,'.video-demo video');return video;},document,
  IntersectionObserver:class{constructor(fn){observe=fn;}observe(target){assert.equal(target,video);}}
 });
 observe([{isIntersecting:false,intersectionRatio:0}]);assert.equal(plays,0);assert.equal(pauses,1);
 observe([{isIntersecting:true,intersectionRatio:1}]);assert.equal(plays,1);assert.equal(video.muted,true);
 document.hidden=true;visibility();assert.equal(pauses,2);
 document.hidden=false;visibility();assert.equal(plays,2);
 observe([{isIntersecting:true,intersectionRatio:.1}]);assert.equal(pauses,3);
 await Promise.resolve();
});
