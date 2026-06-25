#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/build/caustin"
TMP_DIR="$ROOT_DIR/build/test-fixtures"

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR/pkg/sub" "$TMP_DIR/venv/lib"

cat > "$TMP_DIR/pkg/ok.py" <<'PY'
print('ok')
PY

# 3 lines total
cat > "$TMP_DIR/pkg/sub/too_long.py" <<'PY'
print('1')
print('2')
print('3')
PY

cat > "$TMP_DIR/venv/lib/skip.py" <<'PY'
print('skip')
print('skip')
print('skip')
print('skip')
PY

# 1) Should fail at max-lines 2 because too_long.py has 3 lines
set +e
"$BIN" --max-lines 2 "$TMP_DIR" >/dev/null 2>&1
STATUS=$?
set -e
if [[ $STATUS -ne 1 ]]; then
    echo "expected exit 1 for violation, got $STATUS"
    exit 1
fi

# 2) Should pass when excluding pkg/sub/** and venv/**
"$BIN" --max-lines 2 --exclude "pkg/sub/**" --exclude "venv/**" "$TMP_DIR" >/dev/null 2>&1

# 3) Should pass with high threshold
"$BIN" --max-lines 10 "$TMP_DIR" >/dev/null 2>&1

exit 0
