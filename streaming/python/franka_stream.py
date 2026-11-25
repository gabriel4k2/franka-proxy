"""Small client for the standalone Panda streaming server; no robot SDK needed."""

from __future__ import annotations

import json
import math
import socket
import threading
from collections.abc import Sequence
from typing import Any


class FrankaStream:
    """Connection does not start control. Targets are absolute joint angles in radians."""

    def __init__(self, host: str, port: int = 5000, timeout: float = 10.0):
        self._socket: socket.socket | None = socket.create_connection((host, port), timeout)
        self._socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self._buffer = bytearray()
        self._lock = threading.Lock()
        self._started = False

    def _disconnect(self) -> None:
        if self._socket is not None:
            self._socket.close()
            self._socket = None
        self._started = False
        self._buffer.clear()

    def _request(self, request: dict[str, Any]) -> dict[str, Any]:
        payload = (json.dumps(request, allow_nan=False) + "\n").encode("utf-8")
        with self._lock:
            if self._socket is None:
                raise RuntimeError("connection is closed")
            try:
                self._socket.sendall(payload)
                while b"\n" not in self._buffer:
                    if len(self._buffer) >= 4096:
                        raise ValueError("oversized server response")
                    chunk = self._socket.recv(4096 - len(self._buffer))
                    if not chunk:
                        raise ConnectionError("server disconnected")
                    self._buffer.extend(chunk)
                line, _, remaining = self._buffer.partition(b"\n")
                self._buffer = bytearray(remaining)
                response = json.loads(line)
                if not isinstance(response, dict) or type(response.get("ok")) is not bool:
                    raise ValueError("invalid server response")
            except (OSError, ValueError):
                # Do not reuse a connection after an ambiguous request/response.
                # Closing it also asks the server to stop the session.
                self._disconnect()
                raise
            if not response["ok"]:
                raise RuntimeError(response.get("error", "server rejected request"))
            return response

    def start(self) -> dict[str, Any]:
        state = self._request({"op": "start"})
        if state["status"] == "failed":
            raise RuntimeError(state["message"])
        self._started = True
        return state

    def set_target(self, q: Sequence[float]) -> int:
        values = [float(value) for value in q]
        if len(values) != 7 or not all(math.isfinite(value) for value in values):
            raise ValueError("target must contain seven finite joint angles")
        return self._request({"op": "target", "q": values})["sequence"]

    def get_state(self) -> dict[str, Any]:
        return self._request({"op": "state"})

    def stop(self) -> dict[str, Any]:
        state = self._request({"op": "stop"})
        self._started = False
        if state["status"] == "failed":
            raise RuntimeError(state["message"])
        return state

    def close(self) -> None:
        try:
            if self._started:
                self.stop()
        finally:
            with self._lock:
                self._disconnect()

    def __enter__(self) -> FrankaStream:
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        if exc_type is not None:
            # Preserve the original exception. EOF requests a server-side stop.
            with self._lock:
                self._disconnect()
        else:
            self.close()
