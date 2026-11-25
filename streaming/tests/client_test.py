"""Only framing/error handling and disconnect-on-timeout; no robot or emulator."""

import json
from pathlib import Path
import socket
import sys
import threading
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from franka_stream import FrankaStream


class ClientTest(unittest.TestCase):
    def endpoint(self, handler):
        listener = socket.socket()
        listener.bind(("127.0.0.1", 0))
        listener.listen()
        listener.settimeout(3)
        errors = []

        def serve():
            try:
                with listener:
                    connection, _ = listener.accept()
                    with connection:
                        connection.settimeout(3)
                        handler(connection)
            except Exception as error:
                errors.append(error)

        thread = threading.Thread(target=serve)
        thread.start()

        def finish():
            thread.join(4)
            self.assertFalse(thread.is_alive(), "endpoint did not terminate")
            if errors:
                raise errors[0]

        self.addCleanup(finish)
        return listener.getsockname()[1]

    def test_framing_rejection_and_context_stop(self):
        operations = []

        def serve(connection):
            with connection.makefile("rb") as stream:
                for reply in (
                    {"ok": True, "status": "holding"},
                    {"ok": False, "error": "target rejected"},
                    {"ok": True, "sequence": 2},
                    {"ok": True, "status": "inactive"},
                ):
                    operations.append(json.loads(stream.readline())["op"])
                    data = (json.dumps(reply) + "\n").encode()
                    connection.sendall(data[:3])
                    connection.sendall(data[3:])
                self.assertEqual(stream.read(), b"")

        port = self.endpoint(serve)
        with FrankaStream("127.0.0.1", port) as client:
            client.start()
            with self.assertRaises(ValueError):
                client.set_target([float("nan")] * 7)
            with self.assertRaisesRegex(RuntimeError, "target rejected"):
                client.set_target([0.0] * 7)
            self.assertEqual(client.set_target([0.0] * 7), 2)
        self.assertEqual(operations, ["start", "target", "target", "stop"])

    def test_timeout_closes_ambiguous_connection(self):
        def serve(connection):
            with connection.makefile("rb") as stream:
                self.assertEqual(json.loads(stream.readline())["op"], "state")
                self.assertEqual(stream.read(), b"")  # client must close, not retry

        port = self.endpoint(serve)
        with FrankaStream("127.0.0.1", port, timeout=0.2) as client:
            with self.assertRaises(TimeoutError):
                client.get_state()
            with self.assertRaisesRegex(RuntimeError, "closed"):
                client.get_state()


if __name__ == "__main__":
    unittest.main()
