#!/usr/bin/env bash
set -euo pipefail

# Запуск конвейера cccr_pipeline в WSL.
# 1) Убедитесь, что модуль cccr_py установлен в используемом venv
# 2) Настройте cccr_pipeline/config.yaml (копия из config.example.yaml)
# 3) (опционально) Укажите VENV_PATH, если используете не ROOT/.venv

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)

# Выбор виртуального окружения: переменная VENV_PATH имеет приоритет
VENV_PATH_DEFAULT="${ROOT_DIR}/.venv"
VENV_PATH_IN="${VENV_PATH:-$VENV_PATH_DEFAULT}"

# Активируем (или создаём) виртуальное окружение
if [[ ! -d "${VENV_PATH_IN}" ]]; then
  echo "Создаю venv: ${VENV_PATH_IN}" >&2
  python3 -m venv "${VENV_PATH_IN}"
fi
source "${VENV_PATH_IN}/bin/activate"

# Минимальные зависимости пайплайна (не затрагивает установленный cccr_py)
pip -q install -U pip
pip -q install -r "${SCRIPT_DIR}/requirements.txt"

# Конфиг обязателен
if [[ ! -f "${SCRIPT_DIR}/config.yaml" ]]; then
  cp "${SCRIPT_DIR}/config.example.yaml" "${SCRIPT_DIR}/config.yaml"
  echo "Создан шаблон ${SCRIPT_DIR}/config.yaml — заполните его перед запуском." >&2
  exit 1
fi

# Проверка наличия cccr_py
if ! python3 - <<'PY'
import sys
try:
    import cccr_py  # noqa
except Exception as e:
    print("[FATAL] Модуль cccr_py недоступен из активного интерпретатора.", file=sys.stderr)
    print("Подсказка: экспортируйте VENV_PATH=</путь/к/вашему/venv> или установите пакет в ", file=sys.stderr)
    print("текущее окружение: {}".format(sys.executable), file=sys.stderr)
    raise SystemExit(1)
PY
then
  exit 1
fi

# Запуск
exec python3 "${SCRIPT_DIR}/cccr_pipeline.py" "$@"
