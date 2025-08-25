#!/usr/bin/env python3
"""
VictorDB Server Manager - Main entry point

This module provides the command-line interface when the package is executed directly.
Usage: python -m victor_server [arguments]
"""

import sys
import signal
import logging
import argparse
from .server import VictorServerManager, ServerConfig

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


def main():
    """Command-line interface for VictorDB server manager"""
    
    parser = argparse.ArgumentParser(
        description="VictorDB Server Manager - Python wrapper for managing VictorDB servers",
        epilog="""
Examples:
  # Start default database
  python -m victor_server
  
  # Start a specific database
  python -m victor_server --name myapp --db-root /data/victor
  
  # Start only index server with custom dimensions
  python -m victor_server --index-only --index-dims 256 --name vectors
  
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
                        help="Custom index server socket path")
    parser.add_argument("--table-socket", 
                        help="Custom table server socket path")
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
                        help="Custom log directory")
    parser.add_argument("--no-log-files", action="store_true", 
                        help="Disable logging to files (log to console only)")
    parser.add_argument("--no-redirect-stdout", action="store_true", 
                        help="Don't redirect stdout to /dev/null (useful for debugging)")
    parser.add_argument("--debug", action="store_true", 
                        help="Enable debug mode (dumps all keys at table server startup)")
    parser.add_argument("--version", action="version", version="%(prog)s 1.0.0")
    
    args = parser.parse_args()
    
    config = ServerConfig(
        name=args.name,
        db_root=args.db_root,
        export_threshold=args.export_threshold,
        index_socket=args.index_socket or "",
        table_socket=args.table_socket or "",
        index_dims=args.index_dims,
        index_type=args.index_type,
        index_method=args.index_method,
        table_debug=args.debug,
        log_dir=args.log_dir or "",
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
    if len(sys.argv) == 1:
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
