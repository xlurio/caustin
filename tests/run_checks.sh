#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT_DIR/build/caustin"
TMP_DIR="$ROOT_DIR/build/test-fixtures"

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR/pkg/sub" "$TMP_DIR/venv/lib" "$TMP_DIR/docs"

cat > "$TMP_DIR/pkg/ok.py" <<'PY'
print('ok')
PY

# 3 lines total
cat > "$TMP_DIR/pkg/sub/too_long.pyi" <<'PY'
print('1')
print('2')
print('3')
PY

cat > "$TMP_DIR/venv/lib/skip.pyx" <<'PY'
print('skip')
print('skip')
print('skip')
print('skip')
PY

cat > "$TMP_DIR/docs/ignored.txt" <<'TXT'
line 1
line 2
line 3
line 4
TXT

# 1) Should fail without at least one required suffix
set +e
"$BIN" --max-lines 2 "$TMP_DIR" >/dev/null 2>&1
STATUS=$?
set -e
if [[ $STATUS -ne 2 ]]; then
    echo "expected exit 2 for missing suffix, got $STATUS"
    exit 1
fi

# 2) Should fail at max-lines 2 because too_long.pyi has 3 lines
set +e
"$BIN" --suffix ".py" --suffix ".pyi" --max-lines 2 "$TMP_DIR" >/dev/null 2>&1
STATUS=$?
set -e
if [[ $STATUS -ne 1 ]]; then
    echo "expected exit 1 for violation, got $STATUS"
    exit 1
fi

# 3) Should pass when excluding matching violations
"$BIN" --suffix ".py" --suffix ".pyi" --suffix ".pyx" --max-lines 2 --exclude "pkg/sub/**" --exclude "venv/**" "$TMP_DIR" >/dev/null 2>&1

# 4) Should ignore non-matching extensions even when they exceed max-lines
"$BIN" --suffix ".py" --max-lines 2 "$TMP_DIR/docs" >/dev/null 2>&1

# 5) Should pass with high threshold across repeated suffixes
"$BIN" --suffix ".py" --suffix ".pyi" --suffix ".pyx" --max-lines 10 "$TMP_DIR" >/dev/null 2>&1

exit 0
