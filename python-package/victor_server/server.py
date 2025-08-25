#!/usr/bin/env python3
"""
VictorDB Server Manager

Core classes for managing VictorDB server processes.
"""

import os
import time
import signal
import subprocess
from typing import Optional, Dict, Any
from dataclasses import dataclass
import logging

# Configure logging
logger = logging.getLogger(__name__)


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
    table_debug: bool = False  # Debug mode for table server
    
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
            # Check environment variable first
            env_path = os.environ.get('VICTOR_INDEX_BIN')
            if env_path and os.path.isfile(env_path) and os.access(env_path, os.X_OK):
                self.config.victor_index_bin = os.path.abspath(env_path)
            else:
                # Look in installation paths first, then development paths
                search_paths = [
                    "/usr/local/bin/victor_index",  # Standard installation path
                    "/usr/bin/victor_index",        # System package path
                    "./victor_index",               # Current directory
                    "../src/victor_index",          # One level up src
                    "./src/victor_index",           # Local src directory
                ]
                
                for path in search_paths:
                    if os.path.isfile(path) and os.access(path, os.X_OK):
                        self.config.victor_index_bin = os.path.abspath(path)
                        break
        
        if not self.config.victor_table_bin:
            # Check environment variable first
            env_path = os.environ.get('VICTOR_TABLE_BIN')
            if env_path and os.path.isfile(env_path) and os.access(env_path, os.X_OK):
                self.config.victor_table_bin = os.path.abspath(env_path)
            else:
                # Look in installation paths first, then development paths
                search_paths = [
                    "/usr/local/bin/victor_table",  # Standard installation path
                    "/usr/bin/victor_table",        # System package path
                    "./victor_table",               # Current directory
                    "../src/victor_table",          # One level up src
                    "./src/victor_table",           # Local src directory
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
                    stderr_file.close() #type: ignore
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
        
        # Add debug flag if enabled
        if self.config.table_debug:
            cmd.append("-D")
        
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
                    stderr_file.close() #type: ignore
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
