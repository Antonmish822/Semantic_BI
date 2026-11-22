#!/usr/bin/env bash
# Repeatable operator-driven Semantic_BI VPS deployment.
set -Eeuo pipefail
umask 077

ROOT="$(git rev-parse --show-toplevel 2>/dev/null)" || { echo "[preflight] run from a Semantic_BI checkout" >&2; exit 2; }
cd "$ROOT"
ETC=/etc/semantic-bi
STATE=/var/lib/semantic-bi-deploy
LOCK=/var/lib/semantic-bi-deploy/deploy.lock
GRAPH_PROJECT=semantic-bi-graphdb
RUN_PROJECT=semantic-bi-runtime
GRAPH_FILE=deploy/graphdb/compose.vps.yaml
RUN_FILE=deploy/graphdb/compose.yaml
FIRST=semantic-bi-first-import
BUNDLE_TMP=''

usage() { cat <<'EOF'
Usage:
  deploy/vps.sh bootstrap --revision TAG_OR_SHA --bundle PATH [--resume]
  deploy/vps.sh bootstrap --revision TAG_OR_SHA --resume
  deploy/vps.sh status
  deploy/vps.sh start
  deploy/vps.sh update --release TAG

bootstrap validates and installs the private bundle, creates fresh volumes and
runs one detached --once import. --resume explicitly retries an interrupted
first import on the same GraphDB/checkpoint pair. Without --resume, an existing
installation is inspected and successful imports are never repeated.
status reports versions, containers, import result and log commands.
start accepts a confirmed successful first import and starts loader + monitor.
update switches loader/monitor images to an explicit published release tag;
GraphDB stays on 11.5.2. Requires Linux amd64, Bash, Git, Docker Engine with
Compose v2, curl, tar, Python 3, and sudo/root for /etc configuration files.
Run from the agreed deployment checkout and provide the same tag/SHA with
--revision; the script verifies it resolves to HEAD. Bundle is transferred securely.
EOF
}
die() { echo "[$1] $2. Diagnose with: deploy/vps.sh status; docker compose -p $RUN_PROJECT --env-file $ETC/runtime.env -f $RUN_FILE logs --tail 100" >&2; exit 1; }
need() { command -v "$1" >/dev/null || die preflight "missing dependency $1"; }
as_root() { if (( EUID == 0 )); then "$@"; else sudo "$@"; fi; }
compose_graph() { docker compose -p "$GRAPH_PROJECT" --env-file "$ETC/graphdb.env" -f "$GRAPH_FILE" "$@"; }
compose_run() { DOCKER_CONFIG="$ETC/docker-config" docker compose -p "$RUN_PROJECT" --env-file "$ETC/runtime.env" -f "$RUN_FILE" "$@"; }
stage() { printf '%s %s\n' "$(date -u +%FT%TZ)" "$1" > "$STATE/last-stage"; }
lock() {
  as_root install -d -o "$(id -u)" -g "$(id -g)" -m 700 "$STATE"
  exec 9>"$LOCK"; flock -n 9 || die lock "another deployment operation is active"
  # A detached import outlives flock. Never allow another mutation while it runs.
  if docker inspect "$FIRST" >/dev/null 2>&1; then
    local running; running=$(docker inspect -f '{{.State.Running}}' "$FIRST")
    [[ "$running" != true ]] || die lock "first import container $FIRST is still running"
  fi
}
check_fast() {
  python3 - "$ETC/loader.yaml" <<'PY'
import re,sys
s=open(sys.argv[1],encoding='utf-8').read()
m=re.search(r'(?m)^checkpoint:\s*\n((?:[ \t]+.*\n?)*)',s)
if not m: print('checkpoint.resume_mode: missing',file=sys.stderr);sys.exit(3)
v=re.search(r'(?m)^\s+resume_mode:\s*([^\s#]+)',m.group(1))
if not v: print('checkpoint.resume_mode: missing',file=sys.stderr);sys.exit(3)
if v.group(1) != 'fast':
 print(f"checkpoint.resume_mode: expected fast, got {v.group(1)}; refusing to start loader",file=sys.stderr);sys.exit(4)
PY
}
read_status() {
 docker run --rm -v semantic_bi_graphdb_state:/state:ro python:3.13-alpine python -c 'import json, pathlib; p=pathlib.Path("/state/status.json"); print(json.dumps(json.loads(p.read_text())))' 2>/dev/null
}
validate_bundle() {
 local archive=$1 tmp=$2
 python3 - "$archive" "$tmp" <<'PY'
import sys,tarfile, pathlib, stat
arc,dest=sys.argv[1:]
expected={'graphdb.env','runtime.env','loader.yaml','monitor.env','graphdb.license','docker-config','docker-config/config.json'}
seen=set()
try:
 with tarfile.open(arc,'r:gz') as t:
  for m in t.getmembers():
   n=m.name.rstrip('/')
   if n in seen: raise ValueError(f'duplicate path {n}')
   seen.add(n)
   if n not in expected: raise ValueError(f'unexpected path {n}')
   if m.issym() or m.islnk() or not (m.isfile() or (n=='docker-config' and m.isdir())): raise ValueError(f'link or special file {n}')
   if m.isfile() and m.size > 200*1024*1024: raise ValueError(f'oversized file {n}')
  if seen != expected: raise ValueError(f'missing expected bundle paths: {", ".join(sorted(expected-seen))}')
   # Extraction occurs only after the full member set is validated. Write regular
   # files ourselves so this also works on Python versions before tar filter=.
  for m in t.getmembers():
   n=m.name.rstrip('/')
   target=pathlib.Path(dest)/n
   if m.isdir(): target.mkdir(parents=True,exist_ok=True,mode=0o700)
   else:
    target.parent.mkdir(parents=True,exist_ok=True,mode=0o700)
    with t.extractfile(m) as src: target.write_bytes(src.read())
    target.chmod(0o600)
except Exception as e: print(f'bundle archive: {e}',file=sys.stderr);sys.exit(1)
PY
 python3 - "$tmp" <<'PY'
import sys,re,json,pathlib
p=pathlib.Path(sys.argv[1]); read=lambda n:(p/n).read_text()
def env(n):
 d={}
 for line in read(n).splitlines():
  if '=' in line and not line.lstrip().startswith('#'):
   k,v=line.split('=',1);d[k.strip()]=v.strip().strip('"\'')
 return d
g,r,m=env('graphdb.env'),env('runtime.env'),env('monitor.env'); y=read('loader.yaml')
errors=[]
if r.get('LOADER_IMAGE_TAG') != r.get('MONITOR_IMAGE_TAG') or not r.get('LOADER_IMAGE_TAG'): errors.append('runtime.env image tags must both be present and identical')
if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._-]*',r.get('LOADER_IMAGE_TAG','')): errors.append('runtime.env image tag has invalid syntax')
if not re.search(r'(?m)^\s+resume_mode:\s*fast\s*$',y):
 mode=re.search(r'(?m)^\s+resume_mode:\s*(\S+)',y)
 errors.append('checkpoint.resume_mode must be fast'+(f" (got {mode.group(1)})" if mode else ' (missing)'))
if not re.search(r'(?m)^\s+path:\s*["\']?/var/lib/semantic-bi/state.sqlite3["\']?\s*$',y): errors.append('checkpoint.path must be /var/lib/semantic-bi/state.sqlite3')
bucket=re.search(r'(?m)^\s+bucket:\s*["\']?([^"\'\s]+)',y); portal=re.search(r'(?m)^\s+portal_id:\s*["\']?([^"\'\s]+)',y)
if not bucket or bucket.group(1)!=m.get('S3_BUCKET'): errors.append('S3_BUCKET disagrees between loader.yaml and monitor.env')
if not portal or portal.group(1)!=m.get('S3_PORTAL_ID'): errors.append('S3_PORTAL_ID disagrees between loader.yaml and monitor.env')
for field, pattern in [('S3_ENDPOINT_URL',r'(?m)^\s+endpoint_url:\s*["\']?([^"\'\s]+)'),('S3_REGION',r'(?m)^\s+region:\s*["\']?([^"\'\s]+)')]:
 value=re.search(pattern,y)
 if not value or value.group(1)!=m.get(field): errors.append(f'{field} disagrees between loader.yaml and monitor.env')
for key,expected in [('GRAPHDB_REPOSITORY','bitrix24')]:
 if g.get(key)!=expected or m.get(key)!=expected or not re.search(rf'(?m)^\s+repository:\s*["\']?{expected}["\']?\s*$',y): errors.append(f'{key}/loader repository mismatch (expected {expected})')
if g.get('GRAPHDB_LICENSE_FILE')!='/etc/semantic-bi/graphdb.license': errors.append('GRAPHDB_LICENSE_FILE must be /etc/semantic-bi/graphdb.license')
if m.get('RDF_URL')!='http://graphdb:7200' or not re.search(r'(?m)^\s+url:\s*["\']?http://graphdb:7200',y): errors.append('GraphDB addresses must be http://graphdb:7200')
if m.get('LOADER_STATUS_PATH')!='/var/lib/semantic-bi/status.json': errors.append('LOADER_STATUS_PATH mismatch')
try:
 d=json.loads((p/'docker-config/config.json').read_text()); a=d['auths']['ghcr.io']; assert a.get('auth') or a.get('identitytoken')
except Exception: errors.append('docker-config/config.json must contain transferable ghcr.io auth')
if errors:
 for e in errors: print(e,file=sys.stderr)
 sys.exit(1)
PY
}
install_bundle() {
 local tmp=$1 uid gid
 uid=$(id -u); gid=$(id -g)
 as_root install -d -o root -g root -m 711 "$ETC"
 as_root install -d -o "$uid" -g "$gid" -m 700 "$ETC/docker-config"
 as_root install -o "$uid" -g "$gid" -m 600 "$tmp/graphdb.env" "$ETC/graphdb.env"
 as_root install -o "$uid" -g "$gid" -m 600 "$tmp/runtime.env" "$ETC/runtime.env"
 as_root install -o 10001 -g 10001 -m 600 "$tmp/loader.yaml" "$ETC/loader.yaml"
 as_root install -o 10001 -g 10001 -m 600 "$tmp/monitor.env" "$ETC/monitor.env"
 as_root install -o root -g root -m 600 "$tmp/graphdb.license" "$ETC/graphdb.license"
 as_root install -o "$uid" -g "$gid" -m 600 "$tmp/docker-config/config.json" "$ETC/docker-config/config.json"
}
check_compose() { compose_graph config --quiet || die compose "GraphDB Compose invalid"; compose_run config --quiet || die compose "runtime Compose invalid"; }
bootstrap() {
 local bundle='' revision='' resume=0
 while (($#)); do case "$1" in --bundle) bundle=${2:-}; shift 2;; --revision) revision=${2:-}; shift 2;; --resume) resume=1; shift;; *) die args "unknown bootstrap argument $1";; esac; done
 [[ -n "$bundle" || -f "$ETC/graphdb.env" ]] || die args "bootstrap requires --bundle PATH"
 [[ -n "$revision" && "$revision" =~ ^[A-Za-z0-9][A-Za-z0-9._/-]*$ && "$revision" != *..* ]] || die args "bootstrap requires --revision TAG_OR_SHA"
 need docker; need git; need python3; need tar; need flock; need curl; need ss
 [[ $(uname -s) == Linux && $(uname -m) == x86_64 ]] || die preflight "Linux amd64 is required"
 docker info >/dev/null 2>&1 || die preflight "Docker daemon unavailable"
 docker compose version --short | grep -Eq '^v?2\.' || die preflight "Docker Compose v2 required"
 local docker_cpus docker_memory; read -r docker_cpus docker_memory < <(docker info --format '{{.NCPU}} {{.MemTotal}}')
 [[ "$docker_cpus" =~ ^[0-9]+$ && "$docker_cpus" -ge 2 && "$docker_memory" =~ ^[0-9]+$ && "$docker_memory" -ge 4294967296 ]] || die preflight "Docker host needs at least 2 CPUs and 4 GiB memory"
 if (( EUID != 0 )); then need sudo; sudo -n true || die preflight "sudo access required for configuration installation"; fi
 git rev-parse --is-inside-work-tree >/dev/null
 [[ -z "$(git status --porcelain)" ]] || die preflight "checkout has uncommitted or untracked files; use the clean agreed deployment revision"
 local expected_revision actual_revision
 expected_revision=$(git rev-parse --verify "$revision^{commit}" 2>/dev/null) || die preflight "requested revision $revision is not present in this checkout"
 actual_revision=$(git rev-parse HEAD)
 [[ "$expected_revision" == "$actual_revision" ]] || die preflight "checkout HEAD $actual_revision does not match requested revision $revision ($expected_revision)"
 df -Pk / | awk 'NR==2 && $4<10485760 {exit 1}' || die preflight "less than 10 GiB free on root filesystem"
 lock
 if [[ -n "$bundle" ]]; then
  [[ -f "$bundle" ]] || die bundle "bundle file missing"
  BUNDLE_TMP=$(mktemp -d); chmod 700 "$BUNDLE_TMP"; trap '[[ -z "${BUNDLE_TMP:-}" ]] || rm -rf "$BUNDLE_TMP"' EXIT
  validate_bundle "$bundle" "$BUNDLE_TMP" || die bundle "bundle validation failed before installation"
  stage bundle_validated
  if [[ -f "$STATE/config.sha256" ]]; then
   cmp -s "$BUNDLE_TMP/loader.yaml" "$ETC/loader.yaml" && cmp -s "$BUNDLE_TMP/graphdb.env" "$ETC/graphdb.env" && cmp -s "$BUNDLE_TMP/monitor.env" "$ETC/monitor.env" || die bundle "bundle does not match the installed S3/GraphDB configuration; existing files and volumes preserved"
  else
   install_bundle "$BUNDLE_TMP"
   stage configs_installed
  fi
 fi
 check_fast || die config "loader fast-mode validation failed"
 check_compose
 if [[ -f "$STATE/config.sha256" ]]; then
  [[ "$(sha256sum "$ETC/loader.yaml" "$ETC/graphdb.env" | sha256sum | awk '{print $1}')" == "$(cat "$STATE/config.sha256")" ]] || die state "loader/GraphDB config differs from the config used for these volumes"
 fi
 if docker network inspect graphdb_default >/dev/null 2>&1; then
  local net_owner; net_owner=$(docker network inspect -f '{{ index .Labels "com.docker.compose.project" }}' graphdb_default)
  [[ "$net_owner" == "$GRAPH_PROJECT" ]] || die resources "network graphdb_default is unowned or belongs to Compose project $net_owner"
 fi
 if (( ! resume )); then
  docker inspect "$FIRST" >/dev/null 2>&1 && die resources "container name $FIRST already exists; inspect its origin before proceeding"
  for port in 7200 8081; do
   local conflicts; conflicts=$(docker ps -a --filter "publish=$port" --format '{{.Names}}')
   [[ -z "$conflicts" ]] || die resources "host port $port is already published by container(s): ${conflicts//$'\n'/, }"
   local listeners; listeners=$(ss -ltnH | awk -v p=":$port" '$4 ~ (p "$") {print $4}')
   [[ -z "$listeners" ]] || die resources "host port $port is already listened on: ${listeners//$'\n'/, }"
  done
  local existing_graph existing_runtime
  existing_graph=$(docker ps -a -q --filter "label=com.docker.compose.project=$GRAPH_PROJECT")
  existing_runtime=$(docker ps -a -q --filter "label=com.docker.compose.project=$RUN_PROJECT")
  [[ -z "$existing_graph" && -z "$existing_runtime" ]] || die resources "deployment Compose project containers already exist; inspect with deploy/vps.sh status"
 fi
 if [[ -f "$STATE/first-import.ok" ]]; then
  accepted_import || die state "accepted import evidence conflicts with container exit status"
  echo "First import already confirmed; volumes preserved. Run deploy/vps.sh status and then deploy/vps.sh start."; return
 fi
 if status_check_success; then
  echo "First import already confirmed; volumes preserved. Run deploy/vps.sh status and then deploy/vps.sh start."; return
 fi
 if docker volume inspect semantic_bi_graphdb_home >/dev/null 2>&1 || docker volume inspect semantic_bi_graphdb_state >/dev/null 2>&1; then
  if (( ! resume )); then die resources "GraphDB/checkpoint volume already exists; inspect status and use --resume only for the same interrupted installation"; fi
  docker volume inspect semantic_bi_graphdb_home >/dev/null && docker volume inspect semantic_bi_graphdb_state >/dev/null || die resources "both existing volumes are required for resume"
  for n in semantic_bi_graphdb_home semantic_bi_graphdb_state; do
   [[ $(docker volume inspect -f '{{ index .Labels "semantic-bi.managed" }}' "$n") == true ]] || die resources "volume $n lacks this deploy script's ownership label; refusing resume"
  done
  [[ -f "$STATE/config.sha256" ]] || die state "deployment config fingerprint missing; refusing to reuse volumes"
 else
  (( ! resume )) || die resources "--resume requested but volumes do not exist"
  # Avoid stealing project/network/port names.
  docker volume create --label semantic-bi.managed=true --label "com.docker.compose.project=$GRAPH_PROJECT" --label com.docker.compose.volume=graphdb_home semantic_bi_graphdb_home >/dev/null
  docker volume create --label semantic-bi.managed=true semantic_bi_graphdb_state >/dev/null
  sha256sum "$ETC/loader.yaml" "$ETC/graphdb.env" | sha256sum | awk '{print $1}' > "$STATE/config.sha256"
  docker run --rm -v semantic_bi_graphdb_state:/state busybox:1.36 chown 10001:10001 /state
 fi
 compose_graph pull graphdb || die pull "GraphDB image pull failed"
 compose_run pull loader monitor || die pull "runtime image pull failed; verify GHCR auth without printing config.json"
 stage images_pulled
 compose_graph up -d graphdb || die graphdb "GraphDB start failed"
 compose_graph run --rm repository-init || die repository "repository-init failed"
 docker run --rm --network graphdb_default curlimages/curl:8.12.1 --fail --silent --max-time 10 http://graphdb:7200/rest/repositories/bitrix24 >/dev/null || die network "GraphDB peer check failed"
 stage graphdb_ready
 if (( ! resume )); then
  local size; size=$(docker run --rm --network graphdb_default curlimages/curl:8.12.1 --fail --silent --max-time 10 http://graphdb:7200/repositories/bitrix24/size 2>/dev/null) || die repository "could not verify fresh repository size"
  [[ "$size" =~ ^0+$ ]] || die repository "fresh repository bitrix24 is not empty (size=$size); preserving resources"
  docker run --rm -v semantic_bi_graphdb_state:/state busybox:1.36 sh -c 'test -z "$(find /state -mindepth 1 -print -quit)"' || die checkpoint "new checkpoint volume is not empty"
 fi
 docker run --rm --user 10001:10001 -v semantic_bi_graphdb_state:/state busybox:1.36 sh -c 'touch /state/.write-check && rm /state/.write-check' || die checkpoint "loader UID 10001 cannot write checkpoint volume"
 if docker inspect "$FIRST" >/dev/null 2>&1; then
  (( resume )) || die import "named first-import container exists; inspect it and use --resume after a failure"
  [[ $(docker inspect -f '{{.State.Running}}' "$FIRST") != true ]] || die import "first import is still running in $FIRST"
  docker logs "$FIRST" > "$STATE/previous-first-import.log" 2>&1 || true
  docker rm "$FIRST" >/dev/null || die import "could not archive/remove stopped known first-import container"
  date -u +%s > "$STATE/first-import-started"
  compose_run run -d --no-deps --name "$FIRST" loader --config /run/secrets/loader_config --once >/dev/null || die import "could not resume detached first import"
 else
  date -u +%s > "$STATE/first-import-started"
  compose_run run -d --no-deps --name "$FIRST" loader --config /run/secrets/loader_config --once >/dev/null || die import "could not launch detached first import"
 fi
 [[ -f "$STATE/first-import-started" ]] || date -u +%s > "$STATE/first-import-started"
 stage first_import_running
 echo "First import is running in container $FIRST. Reconnect and run deploy/vps.sh status; logs: docker logs -f $FIRST. No persistent services started."
}
status_check_success() {
 local started data
 [[ -f "$STATE/first-import-started" ]] || return 1
 started=$(cat "$STATE/first-import-started")
 data=$(read_status) || return 1
 python3 -c 'import json,sys,datetime; d=json.loads(sys.argv[1]); t=datetime.datetime.fromisoformat(d["updated_at"].replace("Z","+00:00")).timestamp(); print("ok" if d.get("mode")=="once" and d.get("state")=="idle" and d.get("error") is None and d.get("contract_version")==2 and t>=int(sys.argv[2]) else "bad")' "$data" "$started" | grep -qx ok || return 1
 [[ $(docker inspect -f '{{.State.Running}} {{.State.ExitCode}}' "$FIRST" 2>/dev/null) == 'false 0' ]] || return 1
 printf '%s\n' "$data" > "$STATE/first-import.ok"
 stage first_import_succeeded
}
accepted_import() {
 [[ -s "$STATE/first-import.ok" ]] || return 1
 [[ $(docker inspect -f '{{.State.Running}} {{.State.ExitCode}}' "$FIRST" 2>/dev/null) == 'false 0' ]] || return 1
 python3 - "$STATE/first-import.ok" <<'PY'
import json,sys
d=json.load(open(sys.argv[1]))
raise SystemExit(0 if d.get('mode')=='once' and d.get('state')=='idle' and d.get('error') is None and d.get('contract_version')==2 else 1)
PY
}
status() {
 echo "Checkout: $ROOT ($(git rev-parse --short HEAD))"
 [[ -f "$STATE/last-stage" ]] && echo "Last confirmed stage: $(cat "$STATE/last-stage")" || echo "Last confirmed stage: not started"
 [[ -f "$STATE/last-update" ]] && echo "Last update: $(cat "$STATE/last-update")"
 [[ -f "$ETC/runtime.env" ]] && echo "Installed tags: $(sed -n 's/^\(LOADER_IMAGE_TAG\|MONITOR_IMAGE_TAG\)=/\1=/p' "$ETC/runtime.env" | tr '\n' ' ')" || echo "Installed version: not configured"
 echo "GraphDB containers:"; docker ps -a --filter label=com.docker.compose.project="$GRAPH_PROJECT" --format '  {{.Names}} {{.Status}}' || true
 echo "Runtime containers:"; docker ps -a --filter label=com.docker.compose.project="$RUN_PROJECT" --format '  {{.Names}} {{.Status}}' || true
 if docker inspect "$FIRST" >/dev/null 2>&1; then
  local run rc; run=$(docker inspect -f '{{.State.Running}}' "$FIRST"); rc=$(docker inspect -f '{{.State.ExitCode}}' "$FIRST")
  if [[ "$run" == true ]]; then echo "First import: RUNNING (container $FIRST)";
  elif accepted_import || status_check_success; then echo "First import: SUCCESS (exit 0, fresh status accepted)"; read_status | python3 -c 'import json,sys;d=json.load(sys.stdin);print("  revision:",d.get("observed_state_revision"),"base:",d.get("base_manifest_key"),"tip:",d.get("applied_tip_run_id"))';
  else echo "First import: FAILED/UNCONFIRMED (exit $rc); inspect docker logs $FIRST and checkpoint status before deploy/vps.sh bootstrap --bundle PATH --resume"; fi
 else echo "First import: not launched"; fi
 echo "Logs: docker logs -f $FIRST; docker compose -p $RUN_PROJECT --env-file $ETC/runtime.env -f $RUN_FILE logs -f loader monitor"
}
start() {
 lock; check_fast || die config "loader fast-mode validation failed"; check_compose
 accepted_import || status_check_success || die acceptance "first import has no fresh confirmed success; use status and bootstrap --resume after diagnosing"
 local old; old=$(read_status) || die acceptance "cannot read checkpoint status"
 local oldtime; oldtime=$(python3 -c 'import json,sys,datetime;print(datetime.datetime.fromisoformat(json.loads(sys.argv[1])["updated_at"].replace("Z","+00:00")).timestamp())' "$old")
 compose_run up -d loader monitor || die start "runtime services failed to start"
 curl --fail --silent --max-time 10 http://127.0.0.1:8081/api/status >/dev/null || die monitor "monitor local API check failed"
 echo "Waiting up to 30 minutes for a fresh loader cycle..."
 local i data; for ((i=0;i<180;i++)); do
  data=$(read_status 2>/dev/null || true)
  if [[ -n "$data" ]] && python3 - "$data" "$oldtime" <<'PY'
import json,sys,datetime
d=json.loads(sys.argv[1]);t=datetime.datetime.fromisoformat(d['updated_at'].replace('Z','+00:00')).timestamp()
raise SystemExit(0 if t>float(sys.argv[2]) and d.get('mode')=='continuous' and d.get('state')=='waiting' and d.get('error') is None else 1)
PY
  then echo "Runtime started; monitor API and fresh successful loader cycle confirmed."; return; fi
  sleep 10
 done
 die start "monitor answered but no fresh successful loader cycle arrived; services and volumes preserved"
}
update() {
 local release=''; while (($#)); do case "$1" in --release) release=${2:-}; shift 2;; *) die args "unknown update argument $1";; esac; done
 [[ "$release" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]] || die args "update requires --release TAG"
 [[ -f "$ETC/runtime.env" ]] || die update "bootstrap not installed"
 lock; check_fast || die config "loader fast-mode validation failed"; check_compose
 accepted_import || die update "first import has not been accepted"
 local previous; previous=$(sed -n 's/^LOADER_IMAGE_TAG=//p' "$ETC/runtime.env")
 local before beforetime; before=$(read_status 2>/dev/null || true)
 beforetime=0
 if [[ -n "$before" ]]; then beforetime=$(python3 -c 'import json,sys,datetime;print(datetime.datetime.fromisoformat(json.loads(sys.argv[1])["updated_at"].replace("Z","+00:00")).timestamp())' "$before"); fi
 printf '%s -> %s: validating target\n' "$previous" "$release" > "$STATE/last-update"
 sed -i -E "s/^LOADER_IMAGE_TAG=.*/LOADER_IMAGE_TAG=$release/;s/^MONITOR_IMAGE_TAG=.*/MONITOR_IMAGE_TAG=$release/" "$ETC/runtime.env"
 if ! compose_run config --quiet || ! compose_run pull loader monitor; then
  sed -i -E "s/^LOADER_IMAGE_TAG=.*/LOADER_IMAGE_TAG=$previous/;s/^MONITOR_IMAGE_TAG=.*/MONITOR_IMAGE_TAG=$previous/" "$ETC/runtime.env"
  printf '%s -> %s: validation/pull failed; active version remains %s\n' "$previous" "$release" "$previous" > "$STATE/last-update"
  die update "target validation/pull failed; active version remains $previous"
 fi
 compose_run up -d --no-deps loader monitor || { printf '%s -> %s: partial container recreation\n' "$previous" "$release" > "$STATE/last-update"; die update "partial update; previous=$previous target=$release; inspect docker ps and status"; }
 curl --fail --silent --max-time 10 http://127.0.0.1:8081/api/status >/dev/null || { printf '%s -> %s: monitor check failed\n' "$previous" "$release" > "$STATE/last-update"; die update "monitor check failed after recreation; inspect docker ps and status"; }
 local i data; for ((i=0;i<180;i++)); do
  data=$(read_status 2>/dev/null || true)
  if [[ -n "$data" ]] && python3 - "$data" "$beforetime" <<'PY'
import json,sys,datetime
d=json.loads(sys.argv[1]);t=datetime.datetime.fromisoformat(d['updated_at'].replace('Z','+00:00')).timestamp()
raise SystemExit(0 if t>float(sys.argv[2]) and d.get('mode')=='continuous' and d.get('state')=='waiting' and d.get('error') is None else 1)
PY
  then printf '%s -> %s: monitor and fresh loader cycle confirmed\n' "$previous" "$release" > "$STATE/last-update"; echo "Updated loader/monitor: $previous -> $release; monitor and fresh successful loader cycle confirmed. GraphDB remains 11.5.2."; return; fi
  sleep 10
 done
 printf '%s -> %s: containers switched, fresh successful cycle not confirmed\n' "$previous" "$release" > "$STATE/last-update"
 die update "target containers are running but no fresh successful loader cycle arrived; previous=$previous target=$release"
}
cmd=${1:-}; shift || true
case "$cmd" in -h|--help|help) usage;; bootstrap) bootstrap "$@";; status) status;; start) start;; update) update "$@";; *) usage >&2; exit 2;; esac
