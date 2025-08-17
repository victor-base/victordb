#!/usr/bin/env python3
"""
VictorDB Server Manager

A Python wrapper for starting, stopping, and managing VictorDB servers.
Provides an easy way to launch victor_index and victor_table servers
with proper configuration and monitoring.

Features:
- Multi-database support with unique socket paths
- Automatic log management and rotation
- Environment variable configuration
- Graceful shutdown handling
- Process monitoring and health checks

Usage:
    python3 victor_server.py --name mydb --index-dims 256
    python3 victor_server.py --help

Author: VictorDB Team
License: See LICENSE file
"""

import os
import sys
import time
import signal
import subprocess
from typing import Optional, Dict, Any
from dataclasses import dataclass
import logging

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


def print_banner():
    """Print VictorDB banner"""
    banner = """
╔═══════════════════════════════════════╗
║        VictorDB Server Manager        ║
║      High-Performance Vector DB       ║
╚═══════════════════════════════════════╝
"""
    print(banner)


def print_quick_help():
    """Print quick help information"""
    help_text = """
Quick Help:
  --name <db_name>     Database name (default: 'default')
  --index-dims <num>   Vector dimensions (default: 128)
  --index-only         Start only index server
  --table-only         Start only table server
  --help               Show full help

Examples:
  python3 victor_server.py --name mydb
  python3 victor_server.py --index-dims 256 --name vectors
  python3 victor_server.py --help

Press Ctrl+C to stop servers
"""
    print(help_text)


@dataclass
class ServerConfig:
    """Configuration for VictorDB servers"""
    
    # Database configuration
    name: str = "default"
    db_root: str = "/tmp/victord"
    export_threshold: int = 50
    
    # Index server configuration  
    index_socket: str = ""  # Will be auto-generated based on name
    index_dims: int = 128
    index_type: str = "HNSW"  # or "FLAT"
    index_method: str = "cosine"  # cosine, euclidean, etc.
    
    # Table server configuration
    table_socket: str = ""  # Will be auto-generated based on name
    
    # Logging configuration
    log_dir: str = ""  # Will be auto-generated based on name
    log_to_file: bool = True
    redirect_stdout: bool = True
    
    # Binary paths
    victor_index_bin: Optional[str] = None
    victor_table_bin: Optional[str] = None
    
    def __post_init__(self):
        """Auto-generate paths based on database name if not provided"""
        if not self.index_socket:
            self.index_socket = f"/tmp/victor_{self.name}_index.sock"
        
        if not self.table_socket:
            self.table_socket = f"/tmp/victor_{self.name}_table.sock"
        
        if not self.log_dir:
            self.log_dir = f"{self.db_root}/{self.name}/logs"


class VictorServerManager:
    """
    Manages VictorDB server processes.
    
    Provides functionality to start, stop, and monitor victor_index and
    victor_table servers with proper configuration and cleanup.
    """
    
    def __init__(self, config: ServerConfig):
        self.config = config
        self.processes: Dict[str, subprocess.Popen] = {}
        self.running = False
        self._setup_environment()
        self._find_binaries()
    
    def _setup_environment(self):
        """Set up environment variables for VictorDB"""
        os.environ['VICTOR_DB_ROOT'] = self.config.db_root
        os.environ['VICTOR_EXPORT_THRESHOLD'] = str(self.config.export_threshold)
        
        # Create database root directory
        os.makedirs(self.config.db_root, exist_ok=True)
        
        # Create log directory if logging to file is enabled
        if self.config.log_to_file:
            os.makedirs(self.config.log_dir, exist_ok=True)
            logger.info(f"Log directory: {self.config.log_dir}")
        
        logger.info(f"Database root: {self.config.db_root}")
        logger.info(f"Export threshold: {self.config.export_threshold}")
    
    def _get_log_files(self, server_name: str):
        """Get log file paths for a server"""
        if not self.config.log_to_file:
            return None, None
        
        stdout_log = os.path.join(self.config.log_dir, f"{server_name}_stdout.log")
        stderr_log = os.path.join(self.config.log_dir, f"{server_name}_stderr.log")
        return stdout_log, stderr_log
    
    def _find_binaries(self):
        """Find VictorDB binary executables"""
        if not self.config.victor_index_bin:
            # Look in common locations
            search_paths = [
                "./victor_index",
                "../src/victor_index", 
                "./src/victor_index",
                "/usr/local/bin/victor_index",
                "/usr/bin/victor_index"
            ]
            
            for path in search_paths:
                if os.path.isfile(path) and os.access(path, os.X_OK):
                    self.config.victor_index_bin = os.path.abspath(path)
                    break
        
        if not self.config.victor_table_bin:
            # Look in common locations
            search_paths = [
                "./victor_table",
                "../src/victor_table",
                "./src/victor_table", 
                "/usr/local/bin/victor_table",
                "/usr/bin/victor_table"
            ]
            
            for path in search_paths:
                if os.path.isfile(path) and os.access(path, os.X_OK):
                    self.config.victor_table_bin = os.path.abspath(path)
                    break
        
        if not self.config.victor_index_bin:
            raise FileNotFoundError("victor_index binary not found")
        if not self.config.victor_table_bin:
            raise FileNotFoundError("victor_table binary not found")
        
        logger.info(f"Index binary: {self.config.victor_index_bin}")
        logger.info(f"Table binary: {self.config.victor_table_bin}")
    
    def start_index_server(self) -> bool:
        """Start the vector index server"""
        if "index" in self.processes:
            logger.warning("Index server already running")
            return True
        
        # Remove old socket if exists
        if os.path.exists(self.config.index_socket):
            os.unlink(self.config.index_socket)
        
        cmd = [
            self.config.victor_index_bin,
            "-n", self.config.name,
            "-d", str(self.config.index_dims),
            "-t", self.config.index_type.lower(),
            "-m", self.config.index_method,
            "-u", self.config.index_socket
        ]
        
        # Set up log redirection
        stdout_log, stderr_log = self._get_log_files("index")
        
        try:
            logger.info(f"Starting index server: {' '.join(cmd)}")
            
            if self.config.log_to_file and stdout_log and stderr_log:
                logger.info(f"Index logs: {stdout_log} | {stderr_log}")
                stdout_file = open(stdout_log, "w")
                stderr_file = open(stderr_log, "w")
            else:
                # Redirect to /dev/null if not logging to file
                if self.config.redirect_stdout:
                    devnull = open(os.devnull, "w")
                    stdout_file = devnull
                    stderr_file = devnull
                else:
                    stdout_file = None
                    stderr_file = subprocess.PIPE
            
            process = subprocess.Popen(
                cmd,
                stdout=stdout_file,
                stderr=stderr_file,
                universal_newlines=True,
                cwd=os.path.dirname(self.config.victor_index_bin) if self.config.victor_index_bin else None
            )
            
            # Give it a moment to start
            time.sleep(0.5)
            
            if process.poll() is None:
                self.processes["index"] = process
                logger.info(f"Index server started (PID: {process.pid})")
                logger.info(f"Index socket: {self.config.index_socket}")
                return True
            else:
                if not self.config.log_to_file and stderr_file == subprocess.PIPE:
                    _, stderr = process.communicate()
                    logger.error("Index server failed to start:")
                    logger.error(f"STDERR: {stderr}")
                else:
                    logger.error("Index server failed to start - check log files")
                
                # Close files if they were opened
                if stdout_file and hasattr(stdout_file, 'close'):
                    stdout_file.close()
                if stderr_file and hasattr(stderr_file, 'close') and stderr_file is not subprocess.PIPE:
                    stderr_file.close()
                return False
                
        except Exception as e:
            logger.error(f"Failed to start index server: {e}")
            return False
    
    def start_table_server(self) -> bool:
        """Start the key-value table server"""
        if "table" in self.processes:
            logger.warning("Table server already running")
            return True
        
        # Remove old socket if exists
        if os.path.exists(self.config.table_socket):
            os.unlink(self.config.table_socket)
        
        cmd = [
            self.config.victor_table_bin,
            "-n", self.config.name,
            "-u", self.config.table_socket
        ]
        
        # Set up log redirection
        stdout_log, stderr_log = self._get_log_files("table")
        
        try:
            logger.info(f"Starting table server: {' '.join(cmd)}")
            
            if self.config.log_to_file and stdout_log and stderr_log:
                logger.info(f"Table logs: {stdout_log} | {stderr_log}")
                stdout_file = open(stdout_log, "w")
                stderr_file = open(stderr_log, "w")
            else:
                # Redirect to /dev/null if not logging to file
                if self.config.redirect_stdout:
                    devnull = open(os.devnull, "w")
                    stdout_file = devnull
                    stderr_file = devnull
                else:
                    stdout_file = None
                    stderr_file = subprocess.PIPE
            
            process = subprocess.Popen(
                cmd,
                stdout=stdout_file,
                stderr=stderr_file,
                universal_newlines=True,
                cwd=os.path.dirname(self.config.victor_table_bin) if self.config.victor_table_bin else None
            )
            
            # Give it a moment to start
            time.sleep(0.5)
            
            if process.poll() is None:
                self.processes["table"] = process
                logger.info(f"Table server started (PID: {process.pid})")
                logger.info(f"Table socket: {self.config.table_socket}")
                return True
            else:
                if not self.config.log_to_file and stderr_file == subprocess.PIPE:
                    _, stderr = process.communicate()
                    logger.error("Table server failed to start:")
                    logger.error(f"STDERR: {stderr}")
                else:
                    logger.error("Table server failed to start - check log files")
                
                # Close files if they were opened
                if stdout_file and hasattr(stdout_file, 'close'):
                    stdout_file.close()
                if stderr_file and hasattr(stderr_file, 'close') and stderr_file is not subprocess.PIPE:
                    stderr_file.close()
                return False
                
        except Exception as e:
            logger.error(f"Failed to start table server: {e}")
            return False
    
    def start_all(self) -> bool:
        """Start both servers"""
        logger.info("Starting VictorDB servers...")
        
        index_ok = self.start_index_server()
        table_ok = self.start_table_server()
        
        if index_ok and table_ok:
            self.running = True
            logger.info("All servers started successfully!")
            return True
        else:
            logger.error("Failed to start all servers")
            self.stop_all()
            return False
    
    def stop_server(self, server_name: str):
        """Stop a specific server"""
        if server_name in self.processes:
            process = self.processes[server_name]
            logger.info(f"Stopping {server_name} server (PID: {process.pid})")
            
            try:
                process.terminate()
                process.wait(timeout=5)
                logger.info(f"{server_name} server stopped")
            except subprocess.TimeoutExpired:
                logger.warning(f"{server_name} server didn't stop gracefully, killing...")
                process.kill()
                process.wait()
                logger.info(f"{server_name} server killed")
            except Exception as e:
                logger.error(f"Error stopping {server_name} server: {e}")
            
            del self.processes[server_name]
    
    def stop_all(self):
        """Stop all servers"""
        logger.info("Stopping all VictorDB servers...")
        
        for server_name in list(self.processes.keys()):
            self.stop_server(server_name)
        
        # Clean up sockets
        for socket_path in [self.config.index_socket, self.config.table_socket]:
            if os.path.exists(socket_path):
                try:
                    os.unlink(socket_path)
                    logger.info(f"Cleaned up socket: {socket_path}")
                except Exception as e:
                    logger.warning(f"Failed to clean up {socket_path}: {e}")
        
        self.running = False
        logger.info("All servers stopped")
    
    def status(self) -> Dict[str, Any]:
        """Get status of all servers"""
        status = {
            "running": self.running,
            "servers": {}
        }
        
        for server_name, process in self.processes.items():
            if process.poll() is None:
                status["servers"][server_name] = {
                    "status": "running",
                    "pid": process.pid
                }
            else:
                status["servers"][server_name] = {
                    "status": "stopped",
                    "returncode": process.returncode
                }
        
        return status
    
    def wait(self):
        """Wait for all servers to finish"""
        try:
            while self.running and any(p.poll() is None for p in self.processes.values()):
                time.sleep(1)
        except KeyboardInterrupt:
            logger.info("Received interrupt signal")
            self.stop_all()
    
    def __enter__(self):
        """Context manager entry"""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit - cleanup"""
        self.stop_all()


def main():
    """Command-line interface for VictorDB server manager"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="VictorDB Server Manager - Python wrapper for managing VictorDB servers",
        epilog="""
Examples:
  # Start default database
  python3 victor_server.py
  
  # Start a specific database
  python3 victor_server.py --name myapp --db-root /data/victor
  
  # Start only index server with custom dimensions
  python3 victor_server.py --index-only --index-dims 256 --name vectors
  
  # Start with custom configuration
  python3 victor_server.py --name ecommerce --export-threshold 100 --index-type FLAT
  
For more information, visit: https://github.com/victor-base/victordb
        """,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--name", default="default", 
                        help="Database name (used for socket names and logs)")
    parser.add_argument("--db-root", default="/tmp/victord", 
                        help="Database root directory where data is stored")
    parser.add_argument("--export-threshold", type=int, default=50, 
                        help="Export threshold for database operations")
    parser.add_argument("--index-socket", 
                        help="Custom index server socket path (default: /tmp/victor_{name}_index.sock)")
    parser.add_argument("--table-socket", 
                        help="Custom table server socket path (default: /tmp/victor_{name}_table.sock)")
    parser.add_argument("--index-dims", type=int, default=128, 
                        help="Vector dimensions for index server")
    parser.add_argument("--index-type", default="HNSW", choices=["HNSW", "FLAT"], 
                        help="Index algorithm type")
    parser.add_argument("--index-method", default="cosine", 
                        help="Similarity method (cosine, dotp, l2norm)")
    parser.add_argument("--index-only", action="store_true", 
                        help="Start only the index server")
    parser.add_argument("--table-only", action="store_true", 
                        help="Start only the table server")
    parser.add_argument("--log-dir", 
                        help="Custom log directory (default: {db_root}/{name}/logs)")
    parser.add_argument("--no-log-files", action="store_true", 
                        help="Disable logging to files (log to console only)")
    parser.add_argument("--no-redirect-stdout", action="store_true", 
                        help="Don't redirect stdout to /dev/null (useful for debugging)")
    
    args = parser.parse_args()
    
    config = ServerConfig(
        name=args.name,
        db_root=args.db_root,
        export_threshold=args.export_threshold,
        index_socket=args.index_socket or "",  # Will be auto-generated if empty
        table_socket=args.table_socket or "",  # Will be auto-generated if empty
        index_dims=args.index_dims,
        index_type=args.index_type,
        index_method=args.index_method,
        log_dir=args.log_dir or "",  # Will be auto-generated if empty
        log_to_file=not args.no_log_files,
        redirect_stdout=not args.no_redirect_stdout
    )
    
    # Handle signal for graceful shutdown
    def signal_handler(signum, frame):
        logger.info(f"Received signal {signum}")
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Show banner for interactive usage
    if len(sys.argv) == 1 or (len(sys.argv) == 2 and sys.argv[1] not in ['--help', '-h']):
        print_banner()
    
    try:
        with VictorServerManager(config) as manager:
            if args.index_only:
                if manager.start_index_server():
                    manager.wait()
            elif args.table_only:
                if manager.start_table_server():
                    manager.wait()
            else:
                if manager.start_all():
                    manager.wait()
    
    except Exception as e:
        logger.error(f"Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
