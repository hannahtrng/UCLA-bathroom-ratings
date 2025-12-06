#!/usr/bin/env python3
import os
import tempfile
import time
import subprocess
import sys
import threading
import socket

def generatePort() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]

def build_echo_response(request: bytes) -> bytes:
    """Build expected response for echo handler"""
    return (
        b"HTTP/1.1 200 OK\r\n"
        b"Content-Type: text/plain\r\n"
        b"Content-Length: " + str(len(request)).encode("ascii") + b"\r\n"
        b"Connection: close\r\n"
        b"\r\n" + request
    )

def build_static_response(content: bytes, content_type: str = "text/plain") -> bytes:
    """Build expected response for static file handler"""
    return (
        b"HTTP/1.1 200 OK\r\n"
        b"Content-Type: " + content_type.encode("ascii") + b"\r\n"
        b"Content-Length: " + str(len(content)).encode("ascii") + b"\r\n"
        b"Connection: close\r\n"
        b"\r\n" + content
    )

def build_404_response() -> bytes:
    """Build expected 404 response"""
    body = b"404 Not Found"
    return (
        b"HTTP/1.1 404 Not Found\r\n"
        b"Content-Type: text/plain\r\n"
        b"Content-Length: " + str(len(body)).encode("ascii") + b"\r\n"
        b"Connection: close\r\n"
        b"\r\n" + body
    )

def run_test_case(test_name: str, handler_config: str, request: bytes, expected_response_builder):
    """Run a single test case"""
    print(f"Running test: {test_name}")
    
    port = generatePort()
    cfg = f"server {{\n  listen {port};\n  location / {{\n    {handler_config}\n  }}\n}}\n"
    
    with tempfile.NamedTemporaryFile("w", delete=False) as f:
        f.write(cfg)
        cfg_path = f.name

    server_path = os.environ.get("SERVER_BINARY", "build/bin/server_main")
    if not os.path.exists(server_path):
        server_path = "build_coverage/bin/server_main"
    proc = subprocess.Popen([os.path.join(os.getcwd(), server_path), cfg_path])

    time.sleep(0.3)

    try:
        with socket.create_connection(("127.0.0.1", port), timeout=3.0) as s:
            s.sendall(request)
            s.shutdown(socket.SHUT_WR)
            chunks = []
            while True:
                data = s.recv(4096)
                if not data:
                    break
                chunks.append(data)
    finally:
        proc.terminate()
        os.unlink(cfg_path)

    response = b"".join(chunks)
    expected = expected_response_builder(request)

    if response != expected:
        print(f"  FAILED: {test_name}")
        print(f"  Expected: {expected}")
        print(f"  Got:      {response}")
        return False
    else:
        print(f"  PASSED: {test_name}")
        return True

def run_concurrency_test():
    """Test that the server can handle multiple requests concurrently."""
    print(f"Running test: concurrency_test")

    test_name = "concurrency_test"
    port = generatePort()
    sleep_duration = 3

    # Config with two locations: one for sleeping, one for echo.
    # This assumes a 'SleepHandler' is available in your server implementation.
    cfg = (
        f"server {{\n"
        f"  listen {port};\n"
        f"  location /sleep {{\n"
        f"    handler SleepHandler;\n"
        f"  }}\n"
        f"  location /echo {{\n"
        f"    handler EchoHandler;\n"
        f"  }}\n"
        f"}}\n"
    )

    with tempfile.NamedTemporaryFile("w", delete=False) as f:
        f.write(cfg)
        cfg_path = f.name

    server_path = os.environ.get("SERVER_BINARY", "build/bin/server_main")
    if not os.path.exists(server_path):
        server_path = "build_coverage/bin/server_main"
    proc = subprocess.Popen([os.path.join(os.getcwd(), server_path), cfg_path])

    time.sleep(0.3)

    echo_response = None
    echo_request_duration = -1

    # helper func to make echo request and record duration
    def make_echo_request():
        nonlocal echo_response, echo_request_duration   #lets the function set the out-of-scope vars
        request = b"GET /echo HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n"
        start_time = time.time()
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=sleep_duration / 2) as s:
                # echo must return with large enough margin from sleep duration
                s.sendall(request)
                s.shutdown(socket.SHUT_WR)
                echo_response = s.recv(4096)
        finally:
            end_time = time.time()
            echo_request_duration = end_time - start_time

    try:
        # Start a blocking request to /sleep in a separate thread
        sleep_thread = threading.Thread(target=lambda: socket.create_connection(("127.0.0.1", port), timeout=sleep_duration + 1).sendall(b"GET /sleep HTTP/1.1\r\n\r\n"))
        sleep_thread.start()
        time.sleep(0.1)  # wait for sleep request before sending echo request

        # While the first request is "blocking", make an echo request
        make_echo_request()

        expected = build_echo_response(b"GET /echo HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n")
        if echo_response != expected:
            print(f"  FAILED: {test_name}")
            print(f"  Expected: {expected}")
            print(f"  Got:      {echo_response}")
            return False
        elif echo_request_duration >= sleep_duration:
            print(f"  FAILED: {test_name}")
            print(f"  Echo request blocked by sleep request")
            return False
        else:
            return True
    finally:
        proc.terminate()
        os.unlink(cfg_path)

def setup_test_files():
    """Create temporary test files for static handler tests"""
    test_dir = tempfile.mkdtemp()
    
    # Create test.txt
    with open(os.path.join(test_dir, "test.txt"), "w") as f:
        f.write("Hello, World!")
    
    # Create index.html
    with open(os.path.join(test_dir, "index.html"), "w") as f:
        f.write("<!DOCTYPE html><html><body><h1>Test Page</h1></body></html>")
    
    # Create style.css
    with open(os.path.join(test_dir, "style.css"), "w") as f:
        f.write("body { background-color: #f0f0f0; }")
    
    return test_dir

def cleanup_test_files(test_dir):
    """Remove temporary test files"""
    import shutil
    shutil.rmtree(test_dir, ignore_errors=True)

def main():
    # Set up test files
    test_dir = setup_test_files()
    
    try:
        # Define test cases
        test_cases = [
            {
                "name": "echo_handler_get",
                "handler_config": "handler EchoHandler;",
                "request": b"GET / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n",
                "expected_builder": build_echo_response
            },
            {
                "name": "static_handler_txt",
                "handler_config": f"handler StaticHandler;\n    root {test_dir};",
                "request": b"GET /test.txt HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n",
                "expected_builder": lambda req: build_static_response(b"Hello, World!", "text/plain")
            },
            {
                "name": "static_handler_html",
                "handler_config": f"handler StaticHandler;\n    root {test_dir};",
                "request": b"GET /index.html HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n",
                "expected_builder": lambda req: build_static_response(
                    b"<!DOCTYPE html><html><body><h1>Test Page</h1></body></html>",
                    "text/html"
                )
            },
            {
                "name": "static_handler_css",
                "handler_config": f"handler StaticHandler;\n    root {test_dir};",
                "request": b"GET /style.css HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n",
                "expected_builder": lambda req: build_static_response(
                    b"body { background-color: #f0f0f0; }",
                    "text/css"
                )
            },
            {
                "name": "static_handler_404_missing_file",
                "handler_config": f"handler StaticHandler;\n    root {test_dir};",
                "request": b"GET /nonexistent.txt HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n",
                "expected_builder": lambda req: build_404_response()
            },
        ]

        # Run all test cases
        results = []
        for test in test_cases:
            success = run_test_case(
                test["name"],
                test["handler_config"],
                test["request"],
                test["expected_builder"]
            )
            results.append((test["name"], success))

        # Run concurrency test
        concurrency_success = run_concurrency_test()
        if concurrency_success:
            print(f"  PASSED: concurrency_test")
        else:
            print(f"  FAILED: concurrency_test")
        results.append(("concurrency_test", concurrency_success))

        # Print summary
        print("\n" + "="*50)
        print("Test Summary:")
        print("="*50)
        passed = sum(1 for _, success in results if success)
        total = len(results)

        for name, success in results:
            status = "✓ PASSED" if success else "✗ FAILED"
            print(f"  {status}: {name}")
        
        print(f"\nTotal: {passed}/{total} tests passed")
        
        return 0 if passed == total else 1
    
    finally:
        # Clean up test files
        cleanup_test_files(test_dir)

if __name__ == "__main__":
    sys.exit(main())
