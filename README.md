[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/victor-base/victordb)

# VictorDB

VictorDB is a high-performance, distributed vector database system designed for efficient storage, indexing, and retrieval of high-dimensional vectors. It provides both vector similarity search capabilities and traditional key-value storage in a unified, scalable architecture.

## Features

- **High-Performance Vector Search**: Optimized vector similarity search with multiple algorithms (HNSW, FLAT)
- **Multiple Similarity Metrics**: Support for cosine similarity, Euclidean distance, and dot product
- **Dual Storage Architecture**: Separate vector index server and key-value table server
- **CBOR Protocol**: Efficient binary serialization using CBOR (Concise Binary Object Representation)
- **Unix Domain Sockets**: Low-latency communication via Unix domain sockets
- **Memory-Mapped Storage**: Efficient disk I/O with memory-mapped file operations
- **Configurable Dimensions**: Support for vectors of arbitrary dimensions
- **Production Ready**: Robust error handling, logging, and monitoring capabilities
- **Python Integration**: Easy-to-use Python wrapper for server management
- **Official Python Client**: Full-featured Python client available via `pip install victordb`

## Architecture

VictorDB consists of two main components:

1. **Vector Index Server** (`victor_index`): Handles vector storage, indexing, and similarity search operations
2. **Key-Value Table Server** (`victor_table`): Manages traditional key-value storage for metadata and auxiliary data

Both servers communicate using CBOR-encoded messages over Unix domain sockets, providing high throughput and low latency.

## Quick Start

### Prerequisites

Before installing VictorDB, ensure you have the following dependencies:

**System Dependencies:**
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential pkg-config libcbor-dev

# macOS (with Homebrew)
brew install libcbor pkg-config

# CentOS/RHEL/Fedora
sudo dnf install gcc make pkg-config libcbor-devel
# or for older versions:
# sudo yum install gcc make pkg-config libcbor-devel
```

**Required Library - libvictor:**

VictorDB requires the `libvictor` library, which must be installed separately:

```bash
# Clone and install libvictor
git clone https://github.com/victor-base/libvictor.git
cd libvictor
make all
sudo make install

# Verify installation
pkg-config --libs libvictor
```

Make sure `libvictor` is properly installed and accessible via `pkg-config` before proceeding with VictorDB installation.

### Installation

#### Option 1: Install from Source

1. **Clone the repository:**
   ```bash
   git clone https://github.com/victor-base/victordb.git
   cd victordb
   ```

2. **Ensure libvictor is installed:**
   ```bash
   # If not already installed, install libvictor first
   git clone https://github.com/victor-base/libvictor.git
   cd libvictor
   make all && sudo make install
   cd ../victordb
   ```

3. **Build the project:**
   ```bash
   cd src
   make
   ```

4. **Install system-wide (optional):**
   ```bash
   sudo make install
   ```
   
   This installs the binaries to `/usr/local/bin/` and makes them available system-wide.

5. **Verify installation:**
   ```bash
   victor_index --help
   victor_table --help
   ```

#### Option 2: Development Setup

For development or testing, you can run the servers directly from the build directory:

```bash
cd src
make
./victor_index --help
./victor_table --help
```

### Basic Usage

#### Using the Python Server Manager (Recommended)

The easiest way to start VictorDB is using the included Python server manager:

```bash
# Start with default settings (128-dimensional vectors)
python3 scripts/victor_server.py

# Start with custom configuration
python3 scripts/victor_server.py --name myapp --index-dims 256 --index-type HNSW

# Start only the vector index server
python3 scripts/victor_server.py --index-only --index-dims 384

# Start with custom database location
python3 scripts/victor_server.py --db-root /data/vectordb --name production
```

#### Manual Server Startup

You can also start the servers manually:

```bash
# Start vector index server
victor_index -n mydb -d 256 -t hnsw -m cosine -u /tmp/victor_mydb_index.sock

# Start key-value table server (in another terminal)
victor_table -n mydb -u /tmp/victor_mydb_table.sock
```

### Configuration Options

#### Vector Index Server Options

- `-n, --name`: Database name (default: "default")
- `-d, --dims`: Vector dimensions (default: 128)
- `-t, --type`: Index type - "hnsw" or "flat" (default: "hnsw")
- `-m, --method`: Similarity method - "cosine", "euclidean", "dotp" (default: "cosine")
- `-u, --socket`: Unix socket path
- `--db-root`: Database root directory

#### Key-Value Table Server Options

- `-n, --name`: Database name (default: "default")
- `-u, --socket`: Unix socket path
- `--db-root`: Database root directory

#### Python Server Manager Options

```bash
python3 victor_server.py --help
```

Key options:
- `--name`: Database instance name
- `--index-dims`: Vector dimensions
- `--index-type`: HNSW or FLAT indexing
- `--index-method`: Similarity metric (cosine, euclidean, dotp)
- `--db-root`: Data storage location
- `--log-dir`: Log file directory
- `--index-only`: Start only vector server
- `--table-only`: Start only key-value server

## Advanced Usage

### Environment Variables

VictorDB supports several environment variables for configuration:

- `VICTOR_INDEX_BIN`: Path to victor_index binary
- `VICTOR_TABLE_BIN`: Path to victor_table binary
- `VICTOR_DB_ROOT`: Default database root directory
- `VICTOR_EXPORT_THRESHOLD`: Export threshold for operations

### Client Integration

#### Python Client (Recommended)

VictorDB provides an official Python client that can be easily installed via pip:

```bash
pip install victordb
```

Example usage with the Python client:

```python
from victordb import VictorClient

# Connect to VictorDB
client = VictorClient(
    index_socket='/tmp/victor_default_index.sock',
    table_socket='/tmp/victor_default_table.sock'
)

# Insert vectors
vector_id = 12345
vector_data = [0.1, 0.2, 0.3, 0.4, ...]  # Your vector data
client.insert_vector(vector_id, vector_data)

# Search for similar vectors
query_vector = [0.15, 0.25, 0.35, 0.45, ...]
results = client.search_vectors(query_vector, num_results=10)

# Store key-value data
client.put_data("user:123", {"name": "John", "age": 30})
user_data = client.get_data("user:123")
```

#### Manual CBOR Integration

For other languages or custom implementations, VictorDB uses CBOR protocol for client communication:

```python
import socket
import cbor2

# Connect to vector index server
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect('/tmp/victor_default_index.sock')

# Insert a vector
vector_data = [0.1, 0.2, 0.3, ...]  # Your vector data
message = [12345, vector_data]  # [id, vector]
encoded = cbor2.dumps(message)
sock.send(encoded)

# Search for similar vectors
query_vector = [0.15, 0.25, 0.35, ...]
search_message = [query_vector, 10]  # [vector, num_results]
encoded = cbor2.dumps(search_message)
sock.send(encoded)
response = sock.recv(4096)
results = cbor2.loads(response)
```

### Performance Tuning

#### Index Selection

- **HNSW**: Best for high-dimensional vectors with good recall/performance balance
- **FLAT**: Best for smaller datasets where exact results are required

#### Similarity Metrics

- **Cosine**: Good for normalized vectors, text embeddings
- **Euclidean (L2)**: Standard distance metric, good for most use cases
- **Dot Product**: Fast, good for already normalized vectors

#### Memory Management

- Adjust `VICTOR_EXPORT_THRESHOLD` based on your memory constraints
- Use appropriate vector dimensions for your use case
- Monitor memory usage with large datasets

## Building from Source

### Development Requirements

- GCC or Clang compiler
- GNU Make
- pkg-config
- libcbor development headers
- **libvictor library** (from https://github.com/victor-base/libvictor)

### Build Process

```bash
# Install libvictor dependency first
git clone https://github.com/victor-base/libvictor.git
cd libvictor
make all && sudo make install
cd ..

# Build VictorDB
git clone https://github.com/victor-base/victordb.git
cd victordb/src

# Clean build
make clean
make

# Install system-wide
sudo make install

# Build specific targets
make index    # Build only vector index server
make table    # Build only key-value server
```

### Makefile Targets

- `make all` or `make`: Build both servers
- `make index`: Build vector index server only
- `make table`: Build key-value server only
- `make install`: Install binaries to `/usr/local/bin`
- `make uninstall`: Remove installed binaries
- `make clean`: Remove build artifacts

### Troubleshooting

**Common Issues:**

1. **libvictor not found**: Ensure libvictor is installed and accessible via pkg-config:
   ```bash
   pkg-config --libs libvictor
   # Should output: -lvictor
   ```

2. **CBOR library missing**: Install libcbor development packages for your system

3. **Permission denied during installation**: Use `sudo make install` for system-wide installation

4. **Binary not found**: Ensure `/usr/local/bin` is in your PATH, or use full paths to binaries

## Project Structure

```
victordb/
├── src/                    # Source code
│   ├── victor_index        # Vector index server binary
│   ├── victor_table        # Key-value table server binary
│   ├── buffer.c/h          # Buffer management
│   ├── viproto.c/h         # Vector protocol implementation
│   ├── kvproto.c/h         # Key-value protocol implementation
│   ├── server.c/h          # Core server functionality
│   ├── socket.c/h          # Socket communication
│   ├── fileutils.c/h       # File I/O utilities
│   ├── log.c/h             # Logging system
│   └── Makefile            # Build configuration
├── scripts/
│   └── victor_server.py    # Python server manager
├── LICENSE                 # GPL v3 license
└── README.md              # This file
```

## Contributing

We welcome contributions to VictorDB! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes with appropriate tests
4. Ensure code follows the existing style
5. Commit your changes (`git commit -m 'Add amazing feature'`)
6. Push to the branch (`git push origin feature/amazing-feature`)
7. Open a Pull Request

### Development Guidelines

- Write clear, commented code
- Include error handling for all operations
- Add unit tests for new functionality
- Update documentation as needed
- Follow C99 standard conventions

## Support and Documentation

- **GitHub Issues**: Report bugs and request features
- **Documentation**: See inline code documentation
- **Examples**: Check the `scripts/` directory for usage examples

## License

VictorDB is released under the GNU General Public License v3.0. See the [LICENSE](LICENSE) file for details.

## Acknowledgments

VictorDB uses the following open-source libraries and tools:

- [libvictor](https://github.com/victor-base/libvictor) - Core vector database library (required dependency)
- [libcbor](https://github.com/PJK/libcbor) - CBOR protocol implementation
- [Python VictorDB Client](https://pypi.org/project/victordb/) - Official Python client library
- Standard C libraries for system operations

---

**VictorDB** - High-Performance Vector Database System
