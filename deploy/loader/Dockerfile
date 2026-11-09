# syntax=docker/dockerfile:1
FROM ghcr.io/astral-sh/uv:0.9.10@sha256:4f7dc3f292d90396a903bfd15f8ab89389b207889e7cc2f7bf450762cf23656f AS uv
FROM python:3.13.9-slim-trixie@sha256:3f1ce1c415054488586812b63270daf89d97b4f23416e1288003816790eb08de AS builder
COPY --from=uv /uv /usr/local/bin/uv
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build pkg-config \
    librdf0-dev libraptor2-dev librasqal3-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*
ENV UV_PYTHON_DOWNLOADS=never UV_LINK_MODE=copy CMAKE_BUILD_PARALLEL_LEVEL=2
WORKDIR /app
COPY cccr_engine /app/cccr_engine
COPY pyproject.toml uv.lock .python-version ./
COPY scripts/build_cccr.sh scripts/build_cccr.sh
RUN uv sync --locked --group build --no-install-package cccr-py
RUN bash scripts/build_cccr.sh /wheels
# Install only application dependencies into the runtime environment.
RUN UV_PROJECT_ENVIRONMENT=/opt/venv uv sync --locked --no-dev --no-install-package cccr-py \
    && uv pip install --python /opt/venv/bin/python --no-deps /wheels/*.whl
ENV PATH="/opt/venv/bin:$PATH"
COPY pipelines pipelines
COPY mappings/bitrix24 mappings/bitrix24
COPY tests tests
RUN python -c 'import cccr_py; print(cccr_py.__file__)' \
    && python -m unittest discover -s tests -v

FROM scratch AS native-artifacts
COPY --from=builder /wheels /

FROM python:3.13.9-slim-trixie@sha256:3f1ce1c415054488586812b63270daf89d97b4f23416e1288003816790eb08de AS runtime
ENV PATH="/opt/venv/bin:$PATH" PYTHONUNBUFFERED=1 PYTHONDONTWRITEBYTECODE=1
RUN useradd --uid 10001 --create-home loader && mkdir -p /var/lib/semantic-bi \
    && chown loader:loader /var/lib/semantic-bi
COPY --from=builder /opt/venv /opt/venv
COPY --from=builder /wheels /opt/cccr-artifacts
WORKDIR /app
COPY pipelines pipelines
COPY mappings/bitrix24 mappings/bitrix24
USER loader
RUN --mount=type=bind,source=tests,target=/app/tests \
    python -c 'import cccr_py; print(cccr_py.__file__)' \
    && python -m unittest discover -s tests -v
ENTRYPOINT ["python", "-m", "pipelines.bitrix24.cli"]
CMD ["--config", "/run/secrets/loader_config"]
