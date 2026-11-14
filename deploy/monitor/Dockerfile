FROM ghcr.io/astral-sh/uv:0.9.10 AS uv
FROM python:3.13-slim
LABEL org.opencontainers.image.source="https://github.com/Antonmish822/Semantic_BI"

COPY --from=uv /uv /usr/local/bin/uv
WORKDIR /app
COPY rdf_store_monitor/pyproject.toml rdf_store_monitor/uv.lock ./
RUN uv sync --locked --no-dev
COPY rdf_store_monitor/server.py ./server.py
COPY rdf_store_monitor/queries ./queries
COPY rdf_store_monitor/static ./static
ENV PATH="/app/.venv/bin:$PATH" PYTHONUNBUFFERED=1 PYTHONDONTWRITEBYTECODE=1
EXPOSE 8080
ENTRYPOINT ["python", "server.py"]
