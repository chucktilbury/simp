const $ = selector => document.querySelector(selector);
const docsList = $('#docs-list'), threadsList = $('#threads-list');
const searchDialog = $('#search-dialog'), detailDialog = $('#detail-dialog'), threadDialog = $('#thread-dialog'), accountDialog = $('#account-dialog');
let activeThreadId = null;
let currentUser = null;
let csrfToken = '';
let accountMode = 'login';

async function api(url, options = {}) {
  const method = String(options.method || 'GET').toUpperCase();
  const headers = { ...(options.body ? {'Content-Type':'application/json'} : {}), ...(options.headers || {}) };
  if (method !== 'GET' && method !== 'HEAD') {
    if (!csrfToken && url !== '/api/auth/me') await loadAuth();
    if (csrfToken) headers['X-CSRF-Token'] = csrfToken;
  }
  const response = await fetch(url, {...options, method, headers, credentials:'same-origin'});
  const data = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(data.error || `Request failed (${response.status})`);
  return data;
}

function renderMarkdown(source) {
    if (typeof marked === 'undefined' ||
        typeof DOMPurify === 'undefined') {
        throw new Error('Markdown renderer or sanitizer failed to load.');
    }

    return DOMPurify.sanitize(
        marked.parse(String(source ?? ''), {
            gfm: true,
            breaks: false
        }),
        { USE_PROFILES: { html: true } }
    );
}

function escapeHtml(s='') { return String(s).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c])); }
function formatDate(s) { const d = new Date(s); return Number.isNaN(d.getTime()) ? '' : d.toLocaleDateString(undefined, {year:'numeric',month:'short',day:'numeric'}); }
function initials(name='Cwhip') { return name.trim().split(/\s+/).slice(0,2).map(x=>x[0]||'').join('').toUpperCase(); }
function renderDocs(items) {
  docsList.innerHTML = items.length ? items.map(d => `<a class="doc-card" href="#" data-doc="${escapeHtml(d.slug)}"><span class="tag">${escapeHtml(d.category.toUpperCase())}</span><h3>${escapeHtml(d.title)}</h3><p>${escapeHtml(d.summary)}</p><footer><span>Updated ${escapeHtml(formatDate(d.updated_at))}</span><span>Read →</span></footer></a>`).join('') : '<p class="muted">No documents match that search.</p>';
  docsList.querySelectorAll('[data-doc]').forEach(el => el.addEventListener('click', async e => { e.preventDefault(); await openDoc(el.dataset.doc); }));
}
function renderThreads(items) {
  threadsList.innerHTML = items.length ? items.map(t => `<article class="thread-card" tabindex="0" role="button" data-thread="${escapeHtml(t.id)}"><div class="thread-avatar">${escapeHtml(initials(t.author))}</div><div class="thread-main"><h3>${escapeHtml(t.title)}</h3><p>${escapeHtml(t.body.length > 180 ? t.body.slice(0,177)+'…' : t.body)}</p><div class="thread-meta"><span>${escapeHtml(t.category)}</span><span>·</span><span>${escapeHtml(t.author)}</span><span>·</span><span>${Number(t.reply_count)||0} replies</span><span>·</span><span>${escapeHtml(formatDate(t.updated_at))}</span></div></div></article>`).join('') : '<p class="muted">No discussions yet. Sign in to start the first conversation.</p>';
  threadsList.querySelectorAll('[data-thread]').forEach(el => { el.addEventListener('click', () => openThread(el.dataset.thread)); el.addEventListener('keydown', e => { if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); openThread(el.dataset.thread); } }); });
}
async function loadDocs(q='') {
  try { const data = await api('/api/docs'+(q ? '?q='+encodeURIComponent(q):'')); renderDocs(data.items); }
  catch (e) { docsList.innerHTML = `<p class="muted">Could not load documents: ${escapeHtml(e.message)}</p>`; }
}
async function loadThreads(q='') {
  try { const data = await api('/api/threads'+(q ? '?q='+encodeURIComponent(q):'')); renderThreads(data.items); }
  catch (e) { threadsList.innerHTML = `<p class="muted">Could not load discussions: ${escapeHtml(e.message)}</p>`; }
}
async function loadAuth() {
  try { const data = await api('/api/auth/me'); currentUser = data.user || null; csrfToken = data.csrf_token || ''; updateAuthUI(); }
  catch (_) { currentUser = null; csrfToken = ''; updateAuthUI(); }
}
function updateAuthUI() {
  const btn = $('#account-open');
  btn.textContent = currentUser ? `${currentUser.display_name} · Sign out` : 'Sign in';
  $('#thread-author-name').textContent = currentUser ? currentUser.display_name : 'contributor';
  $('#reply-form').hidden = !currentUser;
  $('#reply-signin-prompt').hidden = !!currentUser;
}
function setAccountMode(mode) {
  accountMode = mode;
  const registering = mode === 'register';
  $('#account-title').textContent = registering ? 'Create your account' : 'Sign in';
  $('#account-description').textContent = registering ? 'Create an account to start discussions and post replies.' : 'Sign in to start discussions and post replies.';
  $('#register-name-wrap').hidden = !registering;
  $('#account-form [name="name"]').required = registering;
  $('#account-form [name="password"]').autocomplete = registering ? 'new-password' : 'current-password';
  $('#account-form [name="password"]').placeholder = registering ? 'At least 12 characters' : 'Your password';
  $('#account-submit').textContent = registering ? 'Create account →' : 'Sign in →';
  $('#account-switch-copy').textContent = registering ? 'Already have an account?' : 'New to Cwhip?';
  $('#account-switch').textContent = registering ? 'Sign in' : 'Create an account';
  $('#account-message').textContent = '';
}
async function openDoc(slug) {
  try {
    const {item:d} = await api('/api/docs/'+encodeURIComponent(slug)); activeThreadId = null;
    $('#detail-category').textContent = 'DOCUMENTATION / '+d.category.toUpperCase(); $('#detail-title').textContent = d.title; $('#detail-summary').textContent = d.summary; $('#detail-body').innerHTML = renderMarkdown(d.body);
    $('#replies').innerHTML = ''; $('#reply-form').hidden = true; $('#reply-signin-prompt').hidden = true; detailDialog.showModal();
  } catch(e) { alert(e.message); }
}
async function openThread(id) {
  try {
    const {item:t} = await api('/api/threads/'+encodeURIComponent(id)); activeThreadId = t.id;
    $('#detail-category').textContent = 'DISCUSSION / '+t.category.toUpperCase(); $('#detail-title').textContent = t.title; $('#detail-summary').textContent = `Started by ${t.author} · ${formatDate(t.created_at)}`; $('#detail-body').textContent = t.body;
    $('#replies').innerHTML = `<h3>${t.replies.length} ${t.replies.length===1?'reply':'replies'}</h3>` + (t.replies.length ? t.replies.map(r => `<div class="reply"><strong>${escapeHtml(r.author)}</strong><small>${escapeHtml(formatDate(r.created_at))}</small><p>${escapeHtml(r.body)}</p></div>`).join('') : '<p class="muted">No replies yet. Add a useful thought or question.</p>');
    $('#reply-form').hidden = Number(t.locked) === 1 || !currentUser; $('#reply-signin-prompt').hidden = Number(t.locked) === 1 || !!currentUser; $('#reply-message').textContent = ''; detailDialog.showModal();
  } catch(e) { alert(e.message); }
}
function openAccount(mode='login') { setAccountMode(mode); accountDialog.showModal(); }
$('#doc-search').addEventListener('input', debounce(e => loadDocs(e.target.value.trim()), 220));
$('#search-open').addEventListener('click', () => { searchDialog.showModal(); $('#global-search').focus(); });
$('#global-search').addEventListener('input', debounce(async e => {
  const q=e.target.value.trim(), results=$('#global-results');
  if (!q) { results.innerHTML='<p class="muted">Search documentation and discussions.</p>'; return; }
  try {
    const [docs,threads]=await Promise.all([api('/api/docs?q='+encodeURIComponent(q)),api('/api/threads?q='+encodeURIComponent(q))]);
    const html=[...docs.items.map(d=>`<a href="#" class="result" data-search-doc="${escapeHtml(d.slug)}"><strong>▤ ${escapeHtml(d.title)}</strong><small>Documentation · ${escapeHtml(d.summary)}</small></a>`),...threads.items.map(t=>`<a href="#" class="result" data-search-thread="${escapeHtml(t.id)}"><strong>↗ ${escapeHtml(t.title)}</strong><small>Discussion · ${escapeHtml(t.body.slice(0,140))}</small></a>`)].join('');
    results.innerHTML=html||'<p class="muted">No results found.</p>';
    results.querySelectorAll('[data-search-doc]').forEach(a=>a.addEventListener('click',e=>{e.preventDefault();searchDialog.close();openDoc(a.dataset.searchDoc)}));
    results.querySelectorAll('[data-search-thread]').forEach(a=>a.addEventListener('click',e=>{e.preventDefault();searchDialog.close();openThread(a.dataset.searchThread)}));
  } catch(err) { results.innerHTML=`<p class="muted">${escapeHtml(err.message)}</p>`; }
},200));
$('#account-open').addEventListener('click', async () => {
  if (currentUser) {
    try { await api('/api/auth/logout',{method:'POST',body:JSON.stringify({})}); currentUser=null; await loadAuth(); await loadThreads(); }
    catch(e) { alert(e.message); }
  } else openAccount('login');
});
$('#account-switch').addEventListener('click',()=>setAccountMode(accountMode==='login'?'register':'login'));
$('#account-form').addEventListener('submit', async e => {
  e.preventDefault(); const form=new FormData(e.currentTarget), message=$('#account-message');
  message.className='form-message'; message.textContent=accountMode==='register'?'Creating account…':'Signing in…';
  try {
    const registering=accountMode==='register';
    const data=await api(registering?'/api/auth/register':'/api/auth/login',{method:'POST',body:JSON.stringify({name:form.get('name'),email:form.get('email'),password:form.get('password')})});
    currentUser=data.user; csrfToken=data.csrf_token; updateAuthUI(); accountDialog.close(); e.currentTarget.reset(); await loadThreads();
  } catch(err) { message.className='form-message'; message.textContent=err.message; }
});
$('#new-thread-open').addEventListener('click',()=>{ if(!currentUser){openAccount('login');return;} $('#thread-message').textContent='';threadDialog.showModal(); });
$('#thread-form').addEventListener('submit', async e => {
  e.preventDefault(); const form=new FormData(e.currentTarget),message=$('#thread-message');message.className='form-message';message.textContent='Creating discussion…';
  try { const result=await api('/api/threads',{method:'POST',body:JSON.stringify({title:form.get('title'),category:form.get('category'),body:form.get('body')})});message.className='form-message success';message.textContent='Discussion created.';e.currentTarget.reset();threadDialog.close();await loadThreads();await openThread(result.item.id); }
  catch(err){message.textContent=err.message;}
});
$('#reply-form').addEventListener('submit', async e => {
  e.preventDefault();if(!activeThreadId)return;const form=new FormData(e.currentTarget),message=$('#reply-message');message.className='form-message';message.textContent='Posting reply…';
  try { await api(`/api/threads/${encodeURIComponent(activeThreadId)}/replies`,{method:'POST',body:JSON.stringify({body:form.get('body')})});message.className='form-message success';message.textContent='Reply posted.';e.currentTarget.reset();await loadThreads();await openThread(activeThreadId); }
  catch(err){message.textContent=err.message;}
});
$('#reply-signin').addEventListener('click',()=>{detailDialog.close();openAccount('login')});
document.querySelectorAll('[data-close]').forEach(b=>b.addEventListener('click',()=>$('#'+b.dataset.close).close()));
$('#mobile-menu').addEventListener('click',()=>document.querySelector('.topbar nav').classList.toggle('open'));
document.querySelectorAll('.topbar nav a').forEach(a=>a.addEventListener('click',()=>document.querySelector('.topbar nav').classList.remove('open')));
for(const dialog of [searchDialog,detailDialog,threadDialog,accountDialog]) dialog.addEventListener('click',e=>{if(e.target===dialog)dialog.close()});
document.addEventListener('keydown',e=>{if(e.key==='/'&&!['INPUT','TEXTAREA'].includes(document.activeElement.tagName)&&!searchDialog.open&&!detailDialog.open&&!threadDialog.open&&!accountDialog.open){e.preventDefault();searchDialog.showModal();$('#global-search').focus();}if(e.key==='Escape'){for(const d of [detailDialog,threadDialog,accountDialog])if(d.open)d.close();}});
function debounce(fn,delay){let timer;return(...args)=>{clearTimeout(timer);timer=setTimeout(()=>fn(...args),delay)}}
loadAuth(); loadDocs(); loadThreads();
