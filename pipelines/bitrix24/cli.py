"""Bitrix24 S3 to RDF named graph loader."""
import argparse
import logging
import time
from pathlib import Path

from .config import load_config
from .mapper import load_mappings
from .source import make_s3
from ..core.checkpoint import Checkpoint
from .runner import run_cycle
from ..stores.fuseki import FusekiStore
from ..stores.graphdb import GraphDBStore


def make_store(config):
    args = (config.store_url, config.store_username, config.store_password, config.store_timeout)
    if config.backend == "fuseki":
        return FusekiStore(*args)
    if config.backend == "graphdb":
        return GraphDBStore(config.store_url, config.graphdb_repository,
                            config.store_username, config.store_password, config.store_timeout)
    raise ValueError(f"Неизвестный backend: {config.backend}")


def main(argv=None):
    parser = argparse.ArgumentParser(description="Bitrix24 S3 manifests → named RDF graphs")
    parser.add_argument("--config", type=Path, default=Path("deploy/loader/config.yaml"))
    parser.add_argument("--once", action="store_true", help="Выполнить один цикл")
    parser.add_argument("--log-level", default="INFO")
    args = parser.parse_args(argv)
    logging.basicConfig(level=getattr(logging, args.log_level.upper(), logging.INFO),
                        format="%(asctime)s [%(levelname)s] %(message)s")
    config = load_config(args.config.resolve())
    store = make_store(config)
    mappings = load_mappings(config)
    s3 = make_s3(config)
    checkpoint = Checkpoint(config.checkpoint_path)
    identity = store.endpoint
    try:
        checkpoint.bind_store(config.backend, identity)
        while True:
            try:
                run_cycle(s3, config, checkpoint, mappings, store)
            except Exception:
                logging.getLogger("rdf_sync").exception("Ошибка цикла синхронизации; checkpoint сохранён для повтора")
                if args.once:
                    raise
            if args.once:
                return
            time.sleep(config.poll_interval)
    finally:
        checkpoint.close()


if __name__ == "__main__":
    main()
