# VictorDB Server Manager

[![Python](https://img.shields.io/badge/python-3.7+-blue.svg)](https://www.python.org/downloads/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](https://opensource.org/licenses/MIT)
[![PyPI](https://img.shields.io/pypi/v/victor-server.svg)](https://pypi.org/project/victor-server/)

A Python package for managing VictorDB servers with an easy-to-use API and command-line interface.

## Features

- 🚀 **Easy Server Management**: Start, stop, and monitor VictorDB servers with simple commands
- 🔧 **Flexible Configuration**: Support for multiple databases with unique configurations
- 📊 **Process Monitoring**: Built-in health checks and status monitoring
- 🗂️ **Automatic Logging**: Configurable log management and rotation
- 🐍 **Python API**: Programmatic control for integration into your applications
- 🖥️ **CLI Interface**: Command-line tools for interactive use

## Installation

### From PyPI (Recommended)

```bash
pip install victor-server
```

### From Source

```bash
git clone https://github.com/victor-base/victordb.git
cd victordb/python-package
pip install .
```

### Development Installation

```bash
git clone https://github.com/victor-base/victordb.git
cd victordb/python-package
pip install -e .
```

## Prerequisites

Before using the VictorDB Server Manager, you need to have the VictorDB binaries installed:

1. **Install libvictor**: Follow the installation guide in the main VictorDB repository
2. **Build VictorDB**: Compile `victor_index` and `victor_table` binaries
3. **Installation paths**: The manager will look for binaries in:
   - `/usr/local/bin/` (recommended for production)
   - `/usr/bin/`
   - Current directory
   - `../src/` and `./src/` (for development)

## Quick Start

### Command Line Usage

```bash
# Start default database
victor-server

# Start with custom configuration
victor-server --name myapp --index-dims 256

# Start only index server
victor-server --index-only --name vectors

# Get help
victor-server --help
```

### Python API Usage

```python
from victor_server import VictorServerManager, ServerConfig

# Create configuration
config = ServerConfig(
    name="mydb",
    index_dims=256,
    db_root="/data/victor"
)

# Start servers
with VictorServerManager(config) as manager:
    manager.start_all()
    
    # Your application code here
    print("Servers are running!")
    
    # Wait for shutdown signal
    manager.wait()

# Servers automatically stopped when exiting context
```

## Configuration Options

### ServerConfig Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `name` | `"default"` | Database name (used for sockets and logs) |
| `db_root` | `"/tmp/victord"` | Database root directory |
| `export_threshold` | `50` | Export threshold for operations |
| `index_dims` | `128` | Vector dimensions for index server |
| `index_type` | `"HNSW"` | Index algorithm (HNSW or FLAT) |
| `index_method` | `"cosine"` | Similarity method |
| `table_debug` | `False` | Enable debug mode for table server |
| `log_to_file` | `True` | Enable file logging |

### Command Line Options

```
Usage: victor-server [OPTIONS]

Options:
  --name TEXT              Database name [default: default]
  --db-root PATH          Database root directory
  --index-dims INTEGER    Vector dimensions [default: 128]
  --index-type [HNSW|FLAT] Index algorithm type [default: HNSW]
  --index-method TEXT     Similarity method [default: cosine]
  --index-only            Start only index server
  --table-only            Start only table server
  --debug                 Enable debug mode
  --no-log-files          Disable file logging
  --help                  Show help message
```

## Advanced Usage

### Multiple Database Instances

```python
# Start multiple databases
configs = [
    ServerConfig(name="vectors", index_dims=256),
    ServerConfig(name="embeddings", index_dims=512),
    ServerConfig(name="features", index_dims=128)
]

managers = []
for config in configs:
    manager = VictorServerManager(config)
    manager.start_all()
    managers.append(manager)

try:
    # Run your application
    time.sleep(60)
finally:
    # Clean shutdown
    for manager in managers:
        manager.stop_all()
```

### Custom Binary Paths

```python
config = ServerConfig(
    name="custom",
    victor_index_bin="/opt/victor/bin/victor_index",
    victor_table_bin="/opt/victor/bin/victor_table"
)
```

### Environment Variables

Set these environment variables to override default binary paths:

```bash
export VICTOR_INDEX_BIN="/path/to/victor_index"
export VICTOR_TABLE_BIN="/path/to/victor_table"
export VICTOR_DB_ROOT="/data/victor"
export VICTOR_EXPORT_THRESHOLD="100"
```

## API Reference

### VictorServerManager

Main class for managing VictorDB server processes.

#### Methods

- `start_all()`: Start both index and table servers
- `start_index_server()`: Start only the index server
- `start_table_server()`: Start only the table server
- `stop_all()`: Stop all running servers
- `stop_server(name)`: Stop a specific server
- `status()`: Get status of all servers
- `wait()`: Wait for servers to finish (blocks until interrupted)

#### Context Manager

```python
with VictorServerManager(config) as manager:
    manager.start_all()
    # Servers automatically stopped on exit
```

### ServerConfig

Configuration dataclass for server settings.

## Logging

The server manager provides comprehensive logging:

- **Console Logging**: Always enabled with configurable level
- **File Logging**: Optional, writes to `{db_root}/{name}/logs/`
- **Server Logs**: Separate stdout/stderr logs for each server

### Log Files

- `{db_root}/{name}/logs/index_stdout.log`
- `{db_root}/{name}/logs/index_stderr.log`
- `{db_root}/{name}/logs/table_stdout.log`
- `{db_root}/{name}/logs/table_stderr.log`

## Error Handling

```python
try:
    with VictorServerManager(config) as manager:
        if not manager.start_all():
            print("Failed to start servers")
            sys.exit(1)
        manager.wait()
except FileNotFoundError as e:
    print(f"VictorDB binaries not found: {e}")
except Exception as e:
    print(f"Unexpected error: {e}")
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Support

- **Documentation**: [VictorDB Wiki](https://github.com/victor-base/victordb/wiki)
- **Issues**: [GitHub Issues](https://github.com/victor-base/victordb/issues)
- **Discussions**: [GitHub Discussions](https://github.com/victor-base/victordb/discussions)

## Changelog

### v1.0.0
- Initial release
- Command-line interface
- Python API
- Multi-database support
- Automatic logging
- Process monitoring
