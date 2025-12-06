#!/usr/bin/env python3
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time
from collections import namedtuple
from typing import Dict, Optional

# This code was partially created with the help of GPT-5.1-CODEX

HttpResponse = namedtuple("HttpResponse", ["status_line", "headers", "body"])


def generate_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def send_http_request(port: int, method: str, path: str, body: Optional[bytes] = None) -> HttpResponse:
    body_bytes = body or b""
    request = (
        f"{method} {path} HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n"
    )
    if body_bytes:
        request += f"Content-Length: {len(body_bytes)}\r\n"
        request += "Content-Type: application/json\r\n"
    request += "\r\n"
    request_bytes = request.encode("ascii") + body_bytes

    with socket.create_connection(("127.0.0.1", port), timeout=3.0) as sock:
        sock.sendall(request_bytes)
        sock.shutdown(socket.SHUT_WR)
        chunks = []
        while True:
            data = sock.recv(4096)
            if not data:
                break
            chunks.append(data)

    response_bytes = b"".join(chunks)
    header_bytes, _, body_data = response_bytes.partition(b"\r\n\r\n")
    header_lines = header_bytes.split(b"\r\n")
    status_line = header_lines[0].decode("ascii")
    headers = {}
    for line in header_lines[1:]:
        if not line:
            continue
        key, value = line.split(b":", 1)
        headers[key.decode("ascii").strip()] = value.decode("ascii").strip()
    return HttpResponse(status_line=status_line, headers=headers, body=body_data)


def status_text(status_line: str) -> str:
    return status_line.split(" ", 1)[1] if " " in status_line else status_line


def expect(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def read_file(path: str) -> str:
    with open(path, "r") as f:
        return f.read()


def read_store_snapshot(data_dir: str) -> Dict[str, Dict[str, str]]:
    snapshot: Dict[str, Dict[str, str]] = {}
    if not os.path.exists(data_dir):
        return snapshot
    for entity_name in sorted(os.listdir(data_dir)):
        entity_dir = os.path.join(data_dir, entity_name)
        if not os.path.isdir(entity_dir):
            continue
        entries: Dict[str, str] = {}
        for file_name in sorted(os.listdir(entity_dir)):
            file_path = os.path.join(entity_dir, file_name)
            if os.path.isfile(file_path):
                entries[file_name] = read_file(file_path)
        snapshot[entity_name] = entries
    return snapshot


def expect_store_state(data_dir: str, expected_state: Dict[str, Dict[str, str]]) -> None:
    actual_state = read_store_snapshot(data_dir)
    expect(
        actual_state == expected_state,
        "Unexpected API store contents.\nExpected: "
        f"{json.dumps(expected_state, sort_keys=True)}\nActual: "
        f"{json.dumps(actual_state, sort_keys=True)}",
    )


def run_api_entity_flow() -> None:
    port = generate_port()
    data_dir = tempfile.mkdtemp(prefix="api_store_", dir=os.getcwd())

    cfg = f"""server {{
  listen {port};
  location /api {{
    handler ApiHandler;
    data_path {data_dir};
  }}
}}
"""
    with tempfile.NamedTemporaryFile("w", delete=False) as tmp_cfg:
        tmp_cfg.write(cfg)
        cfg_path = tmp_cfg.name

    server_path = os.environ.get("SERVER_BINARY", "build/bin/server_main")
    if not os.path.exists(server_path):
        server_path = "build_coverage/bin/server_main"
    server_binary = os.path.join(os.getcwd(), server_path)
    proc = subprocess.Popen([server_binary, cfg_path])

    time.sleep(0.4)

    try:
        expected_store: Dict[str, Dict[str, str]] = {}

        def record_entity(entity_type: str, entity_id: int, payload: bytes) -> None:
            expected_store.setdefault(entity_type, {})[
                str(entity_id)
            ] = payload.decode("utf-8")

        def remove_entity(entity_type: str, entity_id: int) -> None:
            entity_entries = expected_store.setdefault(entity_type, {})
            entity_entries.pop(str(entity_id), None)

        def assert_store() -> None:
            expect_store_state(data_dir, expected_store)

        # Create two entities of different types.
        shoe_payload = b'{"style":"sneaker"}'
        print("Creating shoe entity")
        resp = send_http_request(port, "POST", "/api/Shoes", shoe_payload)
        expect(status_text(resp.status_line) == "200 OK",
               f"Unexpected status: {resp.status_line}")
        shoe_id = json.loads(resp.body.decode("utf-8"))["id"]
        shoe_file = os.path.join(data_dir, "Shoes", str(shoe_id))
        expect(os.path.exists(shoe_file), "Shoe entity file not created")
        expect(read_file(shoe_file) == shoe_payload.decode("utf-8"),
               "Shoe entity file contents mismatch after create")
        record_entity("Shoes", shoe_id, shoe_payload)
        assert_store()

        hat_payload = b'{"color":"blue"}'
        print("Creating hat entity")
        resp = send_http_request(port, "POST", "/api/Hats", hat_payload)
        expect(status_text(resp.status_line) == "200 OK",
               f"Unexpected status: {resp.status_line}")
        hat_id = json.loads(resp.body.decode("utf-8"))["id"]
        hat_file = os.path.join(data_dir, "Hats", str(hat_id))
        expect(os.path.exists(hat_file), "Hat entity file not created")
        expect(read_file(hat_file) == hat_payload.decode("utf-8"),
               "Hat entity file contents mismatch after create")
        record_entity("Hats", hat_id, hat_payload)
        assert_store()

        # List entities to confirm they were stored.
        print("Listing shoes")
        resp = send_http_request(port, "GET", "/api/Shoes")
        expect(status_text(resp.status_line) ==
               "200 OK", "Listing shoes failed")
        expect(json.loads(resp.body.decode("utf-8")) == [shoe_id],
               f"Unexpected shoe list: {resp.body}")
        assert_store()

        print("Listing hats")
        resp = send_http_request(port, "GET", "/api/Hats")
        expect(status_text(resp.status_line) ==
               "200 OK", "Listing hats failed")
        expect(json.loads(resp.body.decode("utf-8")) == [hat_id],
               f"Unexpected hat list: {resp.body}")
        assert_store()

        # Retrieve a specific entity.
        print("Fetching created shoe")
        resp = send_http_request(port, "GET", f"/api/Shoes/{shoe_id}")
        expect(status_text(resp.status_line) ==
               "200 OK", "Fetching shoe failed")
        expect(resp.body.decode("utf-8") == shoe_payload.decode("utf-8"),
               "Fetched shoe does not match stored data")
        assert_store()

        # Update shoe.
        updated_shoe_payload = b'{"style":"boot"}'
        print("Updating shoe entity")
        resp = send_http_request(
            port, "PUT", f"/api/Shoes/{shoe_id}", updated_shoe_payload)
        expect(status_text(resp.status_line) ==
               "200 OK", "Updating shoe failed")
        expect(read_file(shoe_file) == updated_shoe_payload.decode("utf-8"),
               "Shoe entity file not updated on disk")
        record_entity("Shoes", shoe_id, updated_shoe_payload)
        assert_store()

        resp = send_http_request(port, "GET", f"/api/Shoes/{shoe_id}")
        expect(resp.body.decode("utf-8") == updated_shoe_payload.decode("utf-8"),
               "Updated shoe retrieval mismatch")
        assert_store()

        # Delete hat and ensure filesystem state updates.
        print("Deleting hat entity")
        resp = send_http_request(port, "DELETE", f"/api/Hats/{hat_id}")
        expect(status_text(resp.status_line) ==
               "200 OK", "Deleting hat failed")
        expect(not os.path.exists(hat_file),
               "Hat entity file still exists after deletion")
        remove_entity("Hats", hat_id)
        assert_store()

        resp = send_http_request(port, "GET", "/api/Hats")
        expect(json.loads(resp.body.decode("utf-8")) == [],
               "Hat list not empty after deletion")
        assert_store()
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            proc.kill()
        os.unlink(cfg_path)
        shutil.rmtree(data_dir, ignore_errors=True)


def main() -> int:
    try:
        run_api_entity_flow()
        print("API entity store integration test PASSED")
        return 0
    except AssertionError as exc:
        print(f"API entity store integration test FAILED: {exc}")
        return 1
    except Exception as exc:  # pragma: no cover - unexpected errors
        print(f"Unexpected error running API integration test: {exc}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
