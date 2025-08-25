"""
VictorDB Server Manager Package

A Python package for managing VictorDB servers with an easy-to-use API
and command-line interface.

This package provides:
- Server manager classes for programmatic control
- Command-line interface via `python -m victor_server`
- Configuration management
- Process monitoring and health checks
- Automatic cleanup and graceful shutdown

Usage:
    # Command line
    $ python -m victor_server --name mydb --index-dims 256
    
    # Python API
    from victor_server import VictorServerManager, ServerConfig
    
    config = ServerConfig(name="mydb", index_dims=256)
    with VictorServerManager(config) as manager:
        manager.start_all()
        manager.wait()

Author: VictorDB Team
License: MIT
"""

from .server import VictorServerManager, ServerConfig

__version__ = "1.0.0"
__author__ = "VictorDB Team"
__email__ = "support@victordb.com"
__license__ = "MIT"

__all__ = [
    "VictorServerManager",
    "ServerConfig",
    "__version__",
]
