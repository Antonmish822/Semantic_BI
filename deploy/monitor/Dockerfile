FROM ghcr.io/astral-sh/uv:0.9.10 AS uv
FROM python:3.13-slim
LABEL org.opencontainers.image.source="https://github.com/Antonmish822/Semantic_BI"

COPY --from=uv /uv /usr/local/bin/uv
WORKDIR /app
COPY fuseki_dashboard/pyproject.toml fuseki_dashboard/uv.lock ./
RUN uv sync --locked --no-dev
COPY fuseki_dashboard/server.py ./server.py
COPY fuseki_dashboard/queries ./queries
COPY fuseki_dashboard/static ./static
ENV PATH="/app/.venv/bin:$PATH" PYTHONUNBUFFERED=1 PYTHONDONTWRITEBYTECODE=1
EXPOSE 8080
ENTRYPOINT ["python", "server.py"]
