from __future__ import annotations

import json
from dataclasses import dataclass
from typing import Any, Dict, List

import boto3


@dataclass(frozen=True)
class S3Client:
    endpoint_url: str
    access_key: str
    secret_key: str
    region: str
    use_ssl: bool

    def _client(self):
        return boto3.client(
            "s3",
            endpoint_url=self.endpoint_url,
            aws_access_key_id=self.access_key,
            aws_secret_access_key=self.secret_key,
            region_name=self.region,
            use_ssl=self.use_ssl,
        )

    def put_json(self, bucket: str, key: str, payload: Dict[str, Any]) -> None:
        client = self._client()
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        client.put_object(
            Bucket=bucket,
            Key=key,
            Body=body,
            ContentType="application/json",
        )


def build_raw_key(prefix: str, entity: str, date_str: str, batch_index: int) -> str:
    return f"{prefix.rstrip('/')}/{entity}/{date_str}/batch_{batch_index:04d}.json"
