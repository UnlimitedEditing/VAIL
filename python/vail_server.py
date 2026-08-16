"""
VAIL FastMCP Server
Standalone Model Context Protocol server exposing universal VAIL primitives.
"""

import logging
import socket
import json
from contextlib import asynccontextmanager
from typing import AsyncIterator, Dict, Any, Optional

try:
    from mcp.server.mcpserver import MCPServer as FastMCP
except ImportError:
    from mcp.server.fastmcp import FastMCP

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[logging.FileHandler('vail_server.log')]
)
logger = logging.getLogger("VAILServer")

UNREAL_HOST = "127.0.0.1"
UNREAL_PORT = 55557

class VAILConnection:
    """TCP Socket connection to Unreal Engine VAIL plugin."""
    
    def __init__(self):
        self.socket = None
        self.connected = False

    def connect(self) -> bool:
        try:
            if self.socket:
                try: self.socket.close()
                except: pass
            
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(5)
            self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self.socket.connect((UNREAL_HOST, UNREAL_PORT))
            self.connected = True
            return True
        except Exception as e:
            logger.error(f"Failed to connect to Unreal Engine VAIL on {UNREAL_HOST}:{UNREAL_PORT}: {e}")
            self.connected = False
            return False

    def disconnect(self):
        if self.socket:
            try: self.socket.close()
            except: pass
        self.socket = None
        self.connected = False

    def send_command(self, command: str, params: Dict[str, Any] = None) -> Optional[Dict[str, Any]]:
        if not self.connect():
            return {"status": "error", "error": "Could not connect to Unreal Engine VAIL plugin"}

        try:
            payload = json.dumps({"type": command, "params": params or {}}) + "\n"
            self.socket.sendall(payload.encode('utf-8'))

            # Read response
            data = b""
            while True:
                chunk = self.socket.recv(4096)
                if not chunk:
                    break
                data += chunk
                if b"\n" in data or (data.startswith(b"{") and data.endswith(b"}\n")):
                    break

            self.disconnect()
            response = json.loads(data.decode('utf-8').strip())
            return response
        except Exception as e:
            logger.error(f"Error sending command {command}: {e}")
            self.disconnect()
            return {"status": "error", "error": str(e)}

_vail_connection: Optional[VAILConnection] = None

def get_vail_connection() -> Optional[VAILConnection]:
    global _vail_connection
    if _vail_connection is None:
        _vail_connection = VAILConnection()
    return _vail_connection

@asynccontextmanager
async def server_lifespan(server: FastMCP) -> AsyncIterator[Dict[str, Any]]:
    logger.info("VAIL MCP server starting up")
    yield {}
    logger.info("VAIL MCP server shut down")

mcp = FastMCP(
    "VAIL",
    description="Virtual Agent Interface Layer for Unreal Engine 5.8",
    lifespan=server_lifespan
)

from tools.vail_tools import register_vail_tools
register_vail_tools(mcp)

if __name__ == "__main__":
    mcp.run(transport='stdio')
