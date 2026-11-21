# syntax=docker/dockerfile:1
FROM ghcr.io/astral-sh/uv:0.9.10@sha256:4f7dc3f292d90396a903bfd15f8ab89389b207889e7cc2f7bf450762cf23656f AS uv

FROM python:3.13.9-slim-trixie@sha256:3f1ce1c415054488586812b63270daf89d97b4f23416e1288003816790eb08de AS builder
COPY --from=uv /uv /usr/local/bin/uv
ENV UV_PYTHON_DOWNLOADS=never UV_LINK_MODE=copy
WORKDIR /app
COPY pyproject.toml uv.lock .python-version ./
RUN UV_PROJECT_ENVIRONMENT=/opt/venv uv sync --locked --no-dev
COPY pipelines pipelines
COPY mappings/bitrix24 mappings/bitrix24
COPY tests tests
ENV PATH="/opt/venv/bin:$PATH"
RUN python -c 'import cccr_py; print(cccr_py.__file__)' \
    && python -m unittest discover -s tests -v

FROM python:3.13.9-slim-trixie@sha256:3f1ce1c415054488586812b63270daf89d97b4f23416e1288003816790eb08de AS runtime
LABEL org.opencontainers.image.source="https://github.com/Antonmish822/Semantic_BI"
ENV PATH="/opt/venv/bin:$PATH" PYTHONUNBUFFERED=1 PYTHONDONTWRITEBYTECODE=1
RUN useradd --uid 10001 --create-home loader && mkdir -p /var/lib/semantic-bi \
    && chown loader:loader /var/lib/semantic-bi
COPY --from=builder /opt/venv /opt/venv
WORKDIR /app
COPY pipelines pipelines
COPY mappings/bitrix24 mappings/bitrix24
USER loader
RUN --mount=type=bind,source=tests,target=/app/tests \
    python -c 'import cccr_py; print(cccr_py.__file__)' \
    && python -m unittest discover -s tests -v
ENTRYPOINT ["python", "-m", "pipelines.bitrix24.cli"]
CMD ["--config", "/run/secrets/loader_config"]
