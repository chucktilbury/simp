<?php
declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');
header('X-Content-Type-Options: nosniff');
header('Referrer-Policy: strict-origin-when-cross-origin');
header('Cache-Control: no-store');

#$configFile = dirname(__DIR__) . '/home1/jhgfrgmy/public_http/cwhip-config.php';
$configFile = '/home1/jhgfrgmy/cwhip-config.php';
if (!is_file($configFile)) {
    http_response_code(503);
    echo json_encode(['error' => 'Cwhip is not configured yet. Follow the deployment guide.']);
    exit;
}
$config = require $configFile;

$isHttps = (!empty($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off') || (($_SERVER['HTTP_X_FORWARDED_PROTO'] ?? '') === 'https');
session_name('cwhip_session');
session_set_cookie_params([
    'lifetime' => 0,
    'path' => '/',
    'secure' => $isHttps,
    'httponly' => true,
    'samesite' => 'Strict',
]);
session_start();

try {
    $dsn = sprintf('mysql:host=%s;dbname=%s;charset=utf8mb4', $config['db_host'], $config['db_name']);
    $pdo = new PDO($dsn, $config['db_user'], $config['db_pass'], [
        PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
        PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
        PDO::ATTR_EMULATE_PREPARES => false,
    ]);
} catch (Throwable $e) {
    http_response_code(503);
    echo json_encode(['error' => 'Database connection failed. Check the private Cwhip configuration.']);
    exit;
}

function respond(array $data, int $status = 200): never {
    http_response_code($status);
    echo json_encode($data, JSON_UNESCAPED_SLASHES | JSON_INVALID_UTF8_SUBSTITUTE);
    exit;
}
function bodyJson(): array {
    $raw = file_get_contents('php://input');
    $data = json_decode($raw ?: '{}', true);
    if (!is_array($data)) respond(['error' => 'Request body must be a JSON object.'], 400);
    return $data;
}
function cleanText(mixed $value, int $max, string $field, bool $required = true): string {
    if (!is_string($value)) respond(['error' => "$field must be text."], 400);
    $value = trim($value);
    if ($required && $value === '') respond(['error' => "$field is required."], 400);
    $length = function_exists('mb_strlen') ? mb_strlen($value, 'UTF-8') : strlen($value);
    if ($length > $max) respond(['error' => "$field must be $max characters or fewer."], 400);
    return $value;
}
function csrfToken(): string {
    if (empty($_SESSION['csrf_token'])) $_SESSION['csrf_token'] = bin2hex(random_bytes(32));
    return (string)$_SESSION['csrf_token'];
}
function requireCsrf(): void {
    $sent = $_SERVER['HTTP_X_CSRF_TOKEN'] ?? '';
    $known = $_SESSION['csrf_token'] ?? '';
    if (!is_string($sent) || !is_string($known) || $known === '' || !hash_equals($known, $sent)) {
        respond(['error' => 'Your session expired or the security token is missing. Refresh the page and try again.'], 403);
    }
}
function currentUser(PDO $pdo): ?array {
    if (empty($_SESSION['user_id'])) return null;
    $stmt = $pdo->prepare('SELECT id, display_name, email, role, created_at FROM users WHERE id = ?');
    $stmt->execute([$_SESSION['user_id']]);
    $user = $stmt->fetch();
    if (!$user) { unset($_SESSION['user_id']); return null; }
    return $user;
}
function requireUser(PDO $pdo): array {
    $user = currentUser($pdo);
    if (!$user) respond(['error' => 'Please sign in to contribute.'], 401);
    return $user;
}
function requireAdmin(PDO $pdo): array {
    $user = requireUser($pdo);
    if ($user['role'] !== 'admin') respond(['error' => 'Administrator permission is required.'], 403);
    return $user;
}
function rateLimit(PDO $pdo, string $action, int $limit, int $seconds): void {
    $ip = (string)($_SERVER['REMOTE_ADDR'] ?? 'unknown');
    $ipHash = hash('sha256', $ip);
    $now = time();
    $stmt = $pdo->prepare('SELECT window_started, hits FROM auth_rate_limits WHERE ip_hash = ? AND action_name = ?');
    $stmt->execute([$ipHash, $action]);
    $row = $stmt->fetch();
    if (!$row || ($now - (int)$row['window_started']) >= $seconds) {
        $upsert = $pdo->prepare('INSERT INTO auth_rate_limits (ip_hash, action_name, window_started, hits) VALUES (?, ?, ?, 1) ON DUPLICATE KEY UPDATE window_started = VALUES(window_started), hits = 1');
        $upsert->execute([$ipHash, $action, $now]);
        return;
    }
    if ((int)$row['hits'] >= $limit) respond(['error' => 'Too many attempts. Please wait a little while and try again.'], 429);
    $update = $pdo->prepare('UPDATE auth_rate_limits SET hits = hits + 1 WHERE ip_hash = ? AND action_name = ?');
    $update->execute([$ipHash, $action]);
}
function uuid(): string {
    $data = random_bytes(16);
    $data[6] = chr((ord($data[6]) & 0x0f) | 0x40);
    $data[8] = chr((ord($data[8]) & 0x3f) | 0x80);
    $hex = bin2hex($data);
    return substr($hex,0,8).'-'.substr($hex,8,4).'-'.substr($hex,12,4).'-'.substr($hex,16,4).'-'.substr($hex,20);
}
function slugify(string $value): string {
    $value = iconv('UTF-8', 'ASCII//TRANSLIT//IGNORE', $value) ?: $value;
    $value = strtolower((string)preg_replace('/[^a-zA-Z0-9]+/', '-', $value));
    return trim(substr($value, 0, 70), '-') ?: 'document';
}
function seedDocs(PDO $pdo): void {
    $count = (int)$pdo->query('SELECT COUNT(*) FROM documents')->fetchColumn();
    if ($count > 0) return;
    $docs = [
        ['project-overview','Project overview','What Cwhip is, what it aims to solve, and the principles guiding its design.','Foundations',"# Project overview\n\nCwhip is a shared home for technical knowledge: documentation, design rationale, and focused discussions that help a project make progress.\n\n## What belongs here?\n\n- Guides that help people get oriented.\n- Reference material that describes current behavior.\n- Design notes that explain important trade-offs.\n- Discussions that can be distilled into durable knowledge.\n\n## Working principle\n\nKeep the source of truth easy to find, easy to improve, and connected to the conversations that shape it."],
        ['architecture','Architecture','A map of the system components, responsibilities, and contracts.','Engineering',"# Architecture\n\nThis document is a starting point for the Cwhip system architecture.\n\n## Current application\n\n- **Web interface:** static HTML, CSS, and JavaScript.\n- **Application API:** PHP JSON endpoints for documents and discussions.\n- **Persistence:** MySQL database hosted by the site provider.\n- **Identity:** individual accounts for contributors."],
        ['design-decisions','Design decisions','A durable index of decisions, alternatives, and their rationale.','Reference',"# Design decisions\n\nRecord consequential decisions with enough context that future contributors can understand them.\n\n## Decision record template\n\n- **Status:** proposed / accepted / superseded\n- **Context:** what problem needs a decision?\n- **Options:** what alternatives were considered?\n- **Decision:** what was chosen and why?\n- **Consequences:** what becomes easier, harder, or different?"]
    ];
    #$stmt = $pdo->prepare('INSERT INTO documents (id, slug, title, summary, body, category, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, NOW(), NOW())');
    $stmt = $pdo->prepare('INSERT INTO documents (id, slug, title, summary, category, body, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, NOW(), NOW())');
    
    foreach ($docs as $d) {
        array_unshift($d, uuid());
        $stmt->execute($d);
    }
}

#seedDocs($pdo);
try {
    seedDocs($pdo);
} catch (Throwable $e) {
    error_log('Cwhip seedDocs failed: ' . $e->getMessage());
    http_response_code(500);
    echo json_encode(['error' => 'Document seeding failed']);
    exit;
}

$path = trim((string)($_GET['path'] ?? ''), '/');
$method = strtoupper((string)($_SERVER['REQUEST_METHOD'] ?? 'GET'));

try {
    if ($path === 'health' && $method === 'GET') respond(['status'=>'ok','service'=>'cwhip','version'=>'0.3.0']);

    if ($path === 'auth/me' && $method === 'GET') {
        respond(['user'=>currentUser($pdo), 'csrf_token'=>csrfToken()]);
    }
    if ($path === 'auth/register' && $method === 'POST') {
        rateLimit($pdo, 'register', 5, 3600);
        requireCsrf();
        $data = bodyJson();
        $name = cleanText($data['name'] ?? null, 80, 'Display name');
        $email = strtolower(cleanText($data['email'] ?? null, 254, 'Email'));
        if (!filter_var($email, FILTER_VALIDATE_EMAIL)) respond(['error'=>'Enter a valid email address.'], 400);
        $password = $data['password'] ?? null;
        if (!is_string($password) || strlen($password) < 12 || strlen($password) > 200) respond(['error'=>'Password must be between 12 and 200 characters.'], 400);
        $check = $pdo->prepare('SELECT id FROM users WHERE email = ?'); $check->execute([$email]);
        if ($check->fetch()) respond(['error'=>'An account with that email already exists.'], 409);
        $userId = uuid();
        $insert = $pdo->prepare('INSERT INTO users (id, display_name, email, password_hash, role, created_at) VALUES (?, ?, ?, ?, \'member\', NOW())');
        $insert->execute([$userId, $name, $email, password_hash($password, PASSWORD_DEFAULT)]);
        session_regenerate_id(true); $_SESSION['user_id'] = $userId; $_SESSION['csrf_token'] = bin2hex(random_bytes(32));
        respond(['user'=>currentUser($pdo), 'csrf_token'=>csrfToken()], 201);
    }
    if ($path === 'auth/login' && $method === 'POST') {
        rateLimit($pdo, 'login', 10, 900);
        requireCsrf();
        $data = bodyJson(); $email = strtolower(cleanText($data['email'] ?? null, 254, 'Email'));
        $password = $data['password'] ?? null;
        $stmt = $pdo->prepare('SELECT id, password_hash FROM users WHERE email = ?'); $stmt->execute([$email]); $row = $stmt->fetch();
        if (!$row || !is_string($password) || !password_verify($password, $row['password_hash'])) respond(['error'=>'Email or password is incorrect.'], 401);
        session_regenerate_id(true); $_SESSION['user_id'] = $row['id']; $_SESSION['csrf_token'] = bin2hex(random_bytes(32));
        respond(['user'=>currentUser($pdo), 'csrf_token'=>csrfToken()]);
    }
    if ($path === 'auth/logout' && $method === 'POST') {
        requireCsrf(); $_SESSION = [];
        if (ini_get('session.use_cookies')) { $p = session_get_cookie_params(); setcookie(session_name(), '', ['expires'=>time()-42000,'path'=>$p['path'],'domain'=>$p['domain'],'secure'=>$p['secure'],'httponly'=>$p['httponly'],'samesite'=>'Strict']); }
        session_destroy(); respond(['ok'=>true]);
    }

    if ($path === 'docs' && $method === 'GET') {
        $q = trim(substr((string)($_GET['q'] ?? ''), 0, 120));
        if ($q !== '') { $s = '%'.$q.'%'; $stmt=$pdo->prepare('SELECT id, slug, title, summary, category, created_at, updated_at FROM documents WHERE title LIKE ? OR summary LIKE ? OR body LIKE ? ORDER BY updated_at DESC LIMIT 100'); $stmt->execute([$s,$s,$s]); }
        else { $stmt=$pdo->query('SELECT id, slug, title, summary, category, created_at, updated_at FROM documents ORDER BY updated_at DESC LIMIT 100'); }
        respond(['items'=>$stmt->fetchAll()]);
    }
    if (preg_match('#^docs/([a-zA-Z0-9-]+)$#', $path, $m) && $method === 'GET') {
        $stmt=$pdo->prepare('SELECT * FROM documents WHERE slug = ?'); $stmt->execute([$m[1]]); $item=$stmt->fetch();
        if (!$item) respond(['error'=>'Document not found.'],404); respond(['item'=>$item]);
    }
    if ($path === 'docs' && $method === 'POST') {
        requireCsrf(); requireAdmin($pdo); $d=bodyJson();
        $title=cleanText($d['title']??null,160,'Title'); $summary=cleanText($d['summary']??'',500,'Summary',false); $body=cleanText($d['body']??null,50000,'Body'); $category=cleanText($d['category']??'General',60,'Category');
        $base=slugify(cleanText($d['slug']??$title,100,'Slug')); $slug=$base; $n=2;
        while (true) { $s=$pdo->prepare('SELECT id FROM documents WHERE slug=?');$s->execute([$slug]);if(!$s->fetch())break;$slug=$base.'-'.$n++; }
        $id=uuid();$s=$pdo->prepare('INSERT INTO documents (id,slug,title,summary,body,category,created_at,updated_at) VALUES (?,?,?,?,?,?,NOW(),NOW())');$s->execute([$id,$slug,$title,$summary,$body,$category]);
        $s=$pdo->prepare('SELECT * FROM documents WHERE id=?');$s->execute([$id]);respond(['item'=>$s->fetch()],201);
    }
    if (preg_match('#^docs/([a-zA-Z0-9-]+)$#',$path,$m) && $method === 'PUT') {
        requireCsrf();requireAdmin($pdo);$d=bodyJson();$s=$pdo->prepare('SELECT * FROM documents WHERE slug=?');$s->execute([$m[1]]);$old=$s->fetch();if(!$old)respond(['error'=>'Document not found.'],404);
        $title=cleanText($d['title']??$old['title'],160,'Title');$summary=cleanText($d['summary']??$old['summary'],500,'Summary',false);$body=cleanText($d['body']??$old['body'],50000,'Body');$category=cleanText($d['category']??$old['category'],60,'Category');
        $s=$pdo->prepare('UPDATE documents SET title=?,summary=?,body=?,category=?,updated_at=NOW() WHERE id=?');$s->execute([$title,$summary,$body,$category,$old['id']]);$s=$pdo->prepare('SELECT * FROM documents WHERE id=?');$s->execute([$old['id']]);respond(['item'=>$s->fetch()]);
    }
    if ($path === 'threads' && $method === 'GET') {
        $q=trim(substr((string)($_GET['q']??''),0,120));
        $sql='SELECT t.id,t.title,t.body,t.category,t.author_name AS author,t.created_at,t.updated_at,t.locked,(SELECT COUNT(*) FROM replies r WHERE r.thread_id=t.id) AS reply_count FROM threads t';
        if($q!==''){$s='%'.$q.'%';$stmt=$pdo->prepare($sql.' WHERE t.title LIKE ? OR t.body LIKE ? OR t.category LIKE ? ORDER BY t.updated_at DESC LIMIT 100');$stmt->execute([$s,$s,$s]);}
        else {$stmt=$pdo->query($sql.' ORDER BY t.updated_at DESC LIMIT 100');}
        respond(['items'=>$stmt->fetchAll()]);
    }
    if (preg_match('#^threads/([a-f0-9-]+)$#',$path,$m) && $method === 'GET') {
        $stmt=$pdo->prepare('SELECT t.id,t.title,t.body,t.category,t.author_name AS author,t.created_at,t.updated_at,t.locked,(SELECT COUNT(*) FROM replies r WHERE r.thread_id=t.id) AS reply_count FROM threads t WHERE t.id=?');$stmt->execute([$m[1]]);$t=$stmt->fetch();if(!$t)respond(['error'=>'Discussion not found.'],404);
        $s=$pdo->prepare('SELECT id,thread_id,body,author_name AS author,created_at FROM replies WHERE thread_id=? ORDER BY created_at');$s->execute([$t['id']]);$t['replies']=$s->fetchAll();respond(['item'=>$t]);
    }
    if ($path === 'threads' && $method === 'POST') {
        requireCsrf();$user=requireUser($pdo);$d=bodyJson();$title=cleanText($d['title']??null,180,'Title');$body=cleanText($d['body']??null,20000,'Details');$category=cleanText($d['category']??'General',60,'Category');$id=uuid();
        $s=$pdo->prepare('INSERT INTO threads (id,title,body,category,author_id,author_name,created_at,updated_at,locked) VALUES (?,?,?,?,?,?,NOW(),NOW(),0)');$s->execute([$id,$title,$body,$category,$user['id'],$user['display_name']]);
        $s=$pdo->prepare('SELECT t.id,t.title,t.body,t.category,t.author_name AS author,t.created_at,t.updated_at,t.locked,0 AS reply_count FROM threads t WHERE t.id=?');$s->execute([$id]);respond(['item'=>$s->fetch()],201);
    }
    if (preg_match('#^threads/([a-f0-9-]+)/replies$#',$path,$m) && $method === 'POST') {
        requireCsrf();$user=requireUser($pdo);$d=bodyJson();$body=cleanText($d['body']??null,10000,'Reply');$s=$pdo->prepare('SELECT id,locked FROM threads WHERE id=?');$s->execute([$m[1]]);$thread=$s->fetch();if(!$thread)respond(['error'=>'Discussion not found.'],404);if((int)$thread['locked']===1)respond(['error'=>'This discussion is locked.'],409);
        $id=uuid();$s=$pdo->prepare('INSERT INTO replies (id,thread_id,body,author_id,author_name,created_at) VALUES (?,?,?,?,?,NOW())');$s->execute([$id,$thread['id'],$body,$user['id'],$user['display_name']]);$pdo->prepare('UPDATE threads SET updated_at=NOW() WHERE id=?')->execute([$thread['id']]);$s=$pdo->prepare('SELECT id,thread_id,body,author_name AS author,created_at FROM replies WHERE id=?');$s->execute([$id]);respond(['item'=>$s->fetch()],201);
    }
    respond(['error'=>'API endpoint not found.'],404);
} catch (Throwable $e) {
    error_log('Cwhip API error: '.$e->getMessage());
    respond(['error'=>'An unexpected server error occurred. Check the server error log.'],500);
}
