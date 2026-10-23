import errno
import socket
from http.server import HTTPServer


class ReusableHTTPServer(HTTPServer):
    allow_reuse_address = True


def create_stream_server(handler_class, preferred_port, host="0.0.0.0", max_tries=10):
    """Bind the stream server, falling back if the preferred port is busy."""
    last_error = None

    for port in range(preferred_port, preferred_port + max_tries):
        try:
            server = ReusableHTTPServer((host, port), handler_class)
            if port != preferred_port:
                print(f"[stream] Port {preferred_port} is busy; using {port} instead.")
            return server, port
        except OSError as exc:
            last_error = exc
            if exc.errno != errno.EADDRINUSE:
                raise

    raise RuntimeError(
        f"Could not bind stream server on ports {preferred_port}-"
        f"{preferred_port + max_tries - 1}. Stop the old process or choose "
        "a different stream port."
    ) from last_error


def stream_url(port):
    hostname = socket.gethostname().split(".")[0]
    return f"http://{hostname}.local:{port}"
