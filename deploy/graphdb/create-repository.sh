#!/bin/sh
set -eu

case "${GRAPHDB_REPOSITORY:-}" in
  ""|*[!A-Za-z0-9_-]*)
    echo "GRAPHDB_REPOSITORY must contain only letters, digits, hyphens, or underscores" >&2
    exit 2
    ;;
esac

url="${GRAPHDB_URL:-http://graphdb:7200}"
attempt=0
until curl --fail --silent --show-error --max-time 5 "$url/rest/repositories" >/dev/null 2>&1; do
  attempt=$((attempt + 1))
  if [ "$attempt" -ge 60 ]; then
    echo "GraphDB did not become ready at $url" >&2
    exit 1
  fi
  sleep 5
done

repo_config="/tmp/repository-$GRAPHDB_REPOSITORY.json"
cleanup() { rm -f "$repo_config"; }
trap cleanup EXIT
trap 'exit 1' HUP INT TERM

verify_repository_config() {
  if ! grep -Eq '"ruleset"[[:space:]]*:[[:space:]]*\{[^}]*"value"[[:space:]]*:[[:space:]]*"owl2-rl"' "$repo_config"; then
    echo "GraphDB repository '$GRAPHDB_REPOSITORY' does not use ruleset owl2-rl; refusing to modify it." >&2
    return 1
  fi
  if ! grep -Eq '"enableContextIndex"[[:space:]]*:[[:space:]]*\{[^}]*"value"[[:space:]]*:[[:space:]]*"true"' "$repo_config"; then
    echo "GraphDB repository '$GRAPHDB_REPOSITORY' does not have the context index enabled; refusing to modify it." >&2
    return 1
  fi
}

if curl --fail --silent --show-error --max-time 10 \
  "$url/rest/repositories/$GRAPHDB_REPOSITORY" -o "$repo_config"; then
  verify_repository_config || exit 1
  echo "GraphDB repository '$GRAPHDB_REPOSITORY' is ready (owl2-rl, context index enabled)."
  exit 0
fi

sed "s/__REPOSITORY_ID__/$GRAPHDB_REPOSITORY/g" \
  /config/repository.ttl > "$repo_config"

curl --fail-with-body --silent --show-error \
  -X POST \
  -H 'Content-Type: multipart/form-data' \
  -F "config=@$repo_config" \
  "$url/rest/repositories"

if ! curl --fail --silent --show-error --max-time 10 \
  "$url/rest/repositories/$GRAPHDB_REPOSITORY" -o "$repo_config"; then
  echo "GraphDB accepted repository creation but '$GRAPHDB_REPOSITORY' is not ready." >&2
  exit 1
fi
verify_repository_config || { echo "Created GraphDB repository '$GRAPHDB_REPOSITORY' failed its configuration check." >&2; exit 1; }
echo "Created and verified GraphDB repository '$GRAPHDB_REPOSITORY'."
