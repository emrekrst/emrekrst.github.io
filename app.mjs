import {projects} from './projects.mjs?v=20261022-links';
import {solveSudoku,inferDecoder,RoverCore} from './lab-core.mjs';
const $=s=>document.querySelector(s), $$=s=>[...document.querySelectorAll(s)];
const projectDialog=$('#project-dialog'),fadecDialog=$('#fadec-dialog');
function openProject(id){
 const p=projects.find(p=>p.id===id);if(!p)return;
 const content=$('#dialog-content');content.replaceChildren();
 const add=(tag,text,className)=>{const e=document.createElement(tag);e.textContent=text;if(className)e.className=className;content.append(e);return e;};
 add('span',p.category.toUpperCase()+' / PROJECT STUDY','index');const title=add('h2',p.title);title.id='dialog-title';
 add('p',p.summary);if(p.image){const img=document.createElement('img');img.src=p.image;img.alt=p.title+' — recorded project evidence';img.className='dialog-image';content.append(img);}
 if(p.video){const video=document.createElement('video');video.src=p.video;video.poster=p.poster;video.controls=true;video.playsInline=true;video.preload='metadata';video.className='dialog-video';video.setAttribute('aria-label',p.title+' video demonstration');content.append(video);}
 add('p',p.detail);add('p',p.result,'result');add('p',p.stack,'tags');const actions=add('div','','actions');
 if(p.demo){const b=document.createElement('button');b.className='button';b.textContent='Try the demo ↗';b.dataset.demo=p.demo;actions.append(b);}
 const a=document.createElement('a');a.href=`source/${p.repo}.html${p.sourceFile?'#file-'+p.sourceFile:''}`;a.className='outline-button';a.textContent='Browse source →';actions.append(a);
 const zip=document.createElement('a');zip.href=`downloads/${p.repo}.zip`;zip.download='';zip.className='text-button';zip.textContent='Download ZIP ↓';actions.append(zip);
 projectDialog.showModal();
}
function selectTab(name,focus=false){
 $$('[data-tab]').forEach(b=>{const active=b.dataset.tab===name;b.setAttribute('aria-selected',active);b.tabIndex=active?0:-1;$('#panel-'+b.dataset.tab).hidden=!active;if(active&&focus)b.focus();});
}
function openDemo(name){
 if(projectDialog.open)projectDialog.close();
 if(name==='fadec'){const iframe=fadecDialog.querySelector('iframe');iframe.src=iframe.dataset.src;fadecDialog.showModal();}
 else{selectTab(name);$('#lab').scrollIntoView({behavior:matchMedia('(prefers-reduced-motion: reduce)').matches?'instant':'smooth'});$('#tab-'+name).focus({preventScroll:true});}
}
document.addEventListener('click',e=>{const demo=e.target.closest('[data-demo]'),project=e.target.closest('[data-project]');if(demo)openDemo(demo.dataset.demo);else if(project)openProject(project.dataset.project);});
$$('dialog').forEach(d=>{d.querySelector('.dialog-close').addEventListener('click',()=>d.close());d.addEventListener('click',e=>{if(e.target===d){const r=d.getBoundingClientRect();if(e.clientX<r.left||e.clientX>r.right||e.clientY<r.top||e.clientY>r.bottom)d.close();}});});
projectDialog.addEventListener('close',()=>projectDialog.querySelector('video')?.pause());
fadecDialog.addEventListener('close',()=>fadecDialog.querySelector('iframe').removeAttribute('src'));
$$('[data-tab]').forEach(b=>{b.addEventListener('click',()=>selectTab(b.dataset.tab));b.addEventListener('keydown',e=>{const tabs=$$('[data-tab]');let i=tabs.indexOf(b);if(e.key==='ArrowRight')i=(i+1)%tabs.length;else if(e.key==='ArrowLeft')i=(i+tabs.length-1)%tabs.length;else if(e.key==='Home')i=0;else if(e.key==='End')i=tabs.length-1;else return;e.preventDefault();selectTab(tabs[i].dataset.tab,true);});});
function renderProjects(filter='All'){$('#project-list').replaceChildren();const shown=projects.filter(p=>filter==='All'||p.category===filter);for(const p of shown){const row=document.createElement('article');row.className='project-row';const n=document.createElement('span');n.className='row-number';n.textContent=String(projects.indexOf(p)+1).padStart(2,'0');const desc=document.createElement('div'),b=document.createElement('button');b.className='project-title';b.textContent=p.title+' ↗';b.dataset.project=p.id;const summary=document.createElement('p');summary.textContent=p.summary;desc.append(b,summary);const stack=document.createElement('span');stack.className='row-stack';stack.textContent=p.stack;const link=document.createElement('a');link.className='row-source';link.href=`source/${p.repo}.html${p.sourceFile?'#file-'+p.sourceFile:''}`;link.textContent='Source ↗';link.setAttribute('aria-label',p.title+' source');row.append(n,desc,stack,link);$('#project-list').append(row);}$('#project-count').textContent=`${shown.length} projects`;}
$$('[data-filter]').forEach(b=>b.addEventListener('click',()=>{$$('[data-filter]').forEach(x=>x.setAttribute('aria-pressed',x===b));renderProjects(b.dataset.filter);}));renderProjects();
const rover=new RoverCore();let linked=true,lastFrame=performance.now();
function tick(time){const now=Math.round(time),dt=Math.max(.001,Math.min(.05,(time-lastFrame)/1000));lastFrame=time;if(linked)rover.accept(Number($('#forward').value),Number($('#turn').value),now);const state=rover.tick(now,dt);for(const side of ['left','right']){$('#'+side+'-meter').value=state[side];$('#'+side+'-value').textContent=state[side].toFixed(2);}$('#rover-status').textContent=state.stopped?'WATCHDOG / STOPPED':linked?'LINK ACTIVE':'LINK INTERRUPTED';requestAnimationFrame(tick);}requestAnimationFrame(tick);
for(const id of ['forward','turn'])$('#'+id).addEventListener('input',()=>$('#'+id+'-value').textContent=Number($('#'+id).value).toFixed(2));
$('#link-toggle').addEventListener('click',()=>{linked=!linked;$('#link-toggle').textContent=linked?'Interrupt link':'Restore link';});
$('#rover-stop').addEventListener('click',()=>{for(const id of ['forward','turn']){$('#'+id).value=0;$('#'+id+'-value').textContent='0.00';}rover.accept(0,0,Math.round(performance.now()));rover.emergencyStop();});
const puzzle='530070000600195000098000060800060003400803001700020006060000280000419005000080079';
for(let i=0;i<81;i++){const input=document.createElement('input');input.type='text';input.inputMode='numeric';input.maxLength=1;input.setAttribute('aria-label',`Row ${Math.floor(i/9)+1} column ${i%9+1}`);input.addEventListener('input',()=>{input.value=input.value.replace(/[^1-9]/g,'');input.classList.remove('given');});$('#sudoku-grid').append(input);}
const cells=$$('#sudoku-grid input');function resetSudoku(blank=false){cells.forEach((c,i)=>{c.value=blank||puzzle[i]==='0'?'':puzzle[i];c.classList.toggle('given',!blank&&puzzle[i]!=='0');});$('#sudoku-status').textContent=blank?'A blank grid is ready.':'A starting puzzle is ready. Make it your own.';}
$('#solve').addEventListener('click',()=>{const result=solveSudoku(cells.map(c=>c.value===''?0:Number(c.value)));if(!result){$('#sudoku-status').textContent='No solution: check for conflicting clues or an impossible board.';return;}cells.forEach((c,i)=>c.value=result[i]);$('#sudoku-status').textContent='Solved. Every row, column and box contains 1–9.';});$('#reset').addEventListener('click',()=>resetSudoku());$('#clear').addEventListener('click',()=>resetSudoku(true));resetSudoku();
let model,inputValue=5;for(let i=0;i<16;i++){const c=document.createElement('div');c.className='decoder-column';const bar=document.createElement('div'),label=document.createElement('span');label.textContent=i.toString(16).toUpperCase();c.append(bar,label);$('#decoder-bars').append(c);}
function updateDecoder(){if(!model)return;const result=inferDecoder(inputValue,model);$$('[data-bit]').forEach(b=>{const on=!!(inputValue&(1<<Number(b.dataset.bit)));b.setAttribute('aria-pressed',on);b.firstChild.textContent=on?'1':'0';});$$('.decoder-column').forEach((c,i)=>{c.firstChild.style.height=`${result.activations[2][i]*100}px`;c.classList.toggle('winner',i===result.prediction);c.setAttribute('aria-label',`Output ${i}: ${result.activations[2][i].toFixed(4)}`);});$('#decoder-value').textContent=`INPUT ${inputValue}`;$('#decoder-status').textContent=`Decoded output: ${result.prediction} / hex ${result.prediction.toString(16).toUpperCase()} · trained C++ weights`;}
$$('[data-bit]').forEach(b=>b.addEventListener('click',()=>{inputValue^=1<<Number(b.dataset.bit);updateDecoder();}));
fetch('assets/decoder-weights.json').then(r=>{if(!r.ok)throw Error('Weights unavailable');return r.json();}).then(m=>{model=m;updateDecoder();}).catch(()=>{$('#decoder-status').textContent='Trained weights could not load. Open the site through the local preview server.';});
function plots(turbo){$('#evidence-image').src=turbo?'assets/turbojet.png':'assets/controls.png';$('#evidence-open').href=$('#evidence-image').src;$('#evidence-image').alt=turbo?'Recorded turbojet speed-control simulation traces':'Four recorded MATLAB plots from mechanical control studies';$('#evidence-source').href=turbo?'source/turbojet-speed-control.html':'source/control-systems-coursework.html';$('#mechanical-plots').setAttribute('aria-pressed',!turbo);$('#turbojet-plots').setAttribute('aria-pressed',turbo);$('#mechanical-plots').className=turbo?'outline-button':'button';$('#turbojet-plots').className=turbo?'button':'outline-button';}$('#mechanical-plots').addEventListener('click',()=>plots(false));$('#turbojet-plots').addEventListener('click',()=>plots(true));

const trackerVideo=$('.video-demo video');
let trackerVisible=false;
function syncTrackerPlayback(){
 if(trackerVisible&&!document.hidden){trackerVideo.muted=true;trackerVideo.play().catch(()=>{});}
 else trackerVideo.pause();
}
new IntersectionObserver(([entry])=>{trackerVisible=entry.isIntersecting&&entry.intersectionRatio>=0.15;syncTrackerPlayback();},{threshold:[0,0.15]}).observe(trackerVideo);
document.addEventListener('visibilitychange',syncTrackerPlayback);
