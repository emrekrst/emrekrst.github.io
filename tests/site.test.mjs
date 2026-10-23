import {test} from 'node:test';
import assert from 'node:assert/strict';
import {readFile,readdir,stat} from 'node:fs/promises';
import path from 'node:path';
const root=path.resolve(import.meta.dirname,'..');
test('portfolio supplies a real home page and all fifteen projects',async()=>{
 const html=await readFile(path.join(root,'index.html'),'utf8');
 assert.match(html,/<title>Emre Karausta/);
 const {projects}=await import('../projects.mjs');
 assert.equal(projects.length,15); assert.equal(new Set(projects.map(p=>p.id)).size,15);
 for(const p of projects){assert.ok(p.title&&p.summary&&p.repo&&p.result);assert.ok((await stat(path.join(root,`source/${p.repo}.html`))).isFile());assert.ok((await stat(path.join(root,`downloads/${p.repo}.zip`))).size>0);}
});
test('every local HTML image, script and stylesheet exists at its relative path',async()=>{
 const walk=async d=>(await Promise.all((await readdir(d,{withFileTypes:true})).map(async e=>e.isDirectory()?walk(path.join(d,e.name)):[path.join(d,e.name)]))).flat();
 for(const f of (await walk(root)).filter(x=>x.endsWith('.html'))){
 const html=await readFile(f,'utf8');
 for(const [,raw] of html.matchAll(/(?:src|href)=["']([^"']+)["']/g)){
  if(/^(?:https?:|mailto:|data:|#)/.test(raw))continue;
  const target=decodeURIComponent(raw.split('#')[0].split('?')[0]);
  if(!target)continue;
  assert.ok(!target.startsWith('/'),`root-relative URL prevents subpath use: ${f} ${raw}`);
  await stat(path.resolve(path.dirname(f),target));
 }
 }
});
test('source packages provide readable code views and study-specific file destinations',async()=>{
 const {projects}=await import('../projects.mjs');
 for(const repo of new Set(projects.map(p=>p.repo))){
  const code=await readFile(path.join(root,`source/${repo}-code.html`),'utf8');assert.match(code,/<pre>/);
 }
 for(const p of projects.filter(p=>p.repo==='control-systems-coursework')){
  assert.ok(p.sourceFile,`missing study file for ${p.id}`);
  const index=await readFile(path.join(root,`source/${p.repo}.html`),'utf8');
  assert.ok(index.includes(`id="file-${p.sourceFile}"`));
  await stat(path.join(root,'source',p.repo,p.sourceFile));
 }
});
test('local clickable links point to existing files and real HTML anchors',async()=>{
 const walk=async d=>(await Promise.all((await readdir(d,{withFileTypes:true})).map(async e=>e.isDirectory()?walk(path.join(d,e.name)):[path.join(d,e.name)]))).flat();
 const htmlFiles=(await walk(root)).filter(f=>f.endsWith('.html'));
 const documents=new Map(await Promise.all(htmlFiles.map(async f=>[f,await readFile(f,'utf8')])));
 for(const [file,html] of documents)for(const [,href] of html.matchAll(/<a\b[^>]*\bhref="([^"]+)"/g)){
  if(/^(https?:|mailto:|data:)/.test(href))continue;
  const [url,hash]=href.split('#');const target=url?path.resolve(path.dirname(file),decodeURIComponent(url.split('?')[0])):file;
  await stat(target);
  if(hash&&documents.has(target))assert.ok(documents.get(target).includes(`id="${decodeURIComponent(hash)}"`),`${file}: ${href}`);
 }
});
