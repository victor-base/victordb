import socket
import struct
import cbor2
from typing import Optional, List, Tuple
from enum import IntEnum


class MessageType(IntEnum):
    # Vector index operations
    INSERT = 0x01
    INSERT_RESULT = 0x02
    DELETE = 0x03
    DELETE_RESULT = 0x04
    SEARCH = 0x05
    MATCH_RESULT = 0x06
    ERROR = 0x07
    
    # Key-value table operations
    PUT = 0x08
    PUT_RESULT = 0x09
    GET = 0x0A
    GET_RESULT = 0x0B
    DEL = 0x0C
    DEL_RESULT = 0x0D


class VictorError(Exception):
    def __init__(self, code: int, message: str):
        super().__init__(f"[{code}] {message}")
        self.code = code
        self.message = message


class VictorClientBase:
    """
    Base client class for VictorDB server communication.
    
    Handles low-level socket operations, message serialization/deserialization,
    and connection management. This class should be inherited by specific
    client implementations for vector index and table operations.
    """
    
    def __init__(self):
        self.sock: Optional[socket.socket] = None

    def connect(self, *, host: Optional[str] = None, port: Optional[int] = None, unix_path: Optional[str] = None):
        """
        Connects to the server via TCP or Unix socket.
        
        Args:
            host: TCP hostname or IP address
            port: TCP port number
            unix_path: Unix domain socket path
            
        Raises:
            ValueError: If neither host/port nor unix_path is provided
            ConnectionError: If connection fails
        """
        if unix_path:
            self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.sock.connect(unix_path)
        elif host and port:
            self.sock = socket.create_connection((host, port))
        else:
            raise ValueError("Must provide either host/port or unix_path")

    def close(self):
        """Close the socket connection."""
        if self.sock:
            self.sock.close()
            self.sock = None

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    def _send_msg(self, msg_type: int, payload: bytes):
        """
        Send a message to the server.
        
        Args:
            msg_type: Message type identifier
            payload: CBOR-encoded message payload
            
        Raises:
            ConnectionError: If socket is not connected
            ValueError: If message type or payload is invalid
        """
        if not self.sock:
            raise ConnectionError("Socket is not connected")

        if msg_type > 0xF or len(payload) > 0x0FFFFFFF:
            raise ValueError("Invalid message type or payload too large")

        header = ((msg_type & 0xF) << 28) | (len(payload) & 0x0FFFFFFF)
        raw = struct.pack("!I", header)
        self.sock.sendall(raw + payload)

    def _recv_msg(self) -> Tuple[int, bytes]:
        """
        Receive a message from the server.
        
        Returns:
            Tuple of (message_type, payload)
            
        Raises:
            VictorError: If server returns an error message
            ConnectionError: If socket connection is lost
        """
        hdr_raw = self._recv_all(4)
        hdr_val = struct.unpack("!I", hdr_raw)[0]
        msg_type = (hdr_val >> 28) & 0xF
        msg_len = hdr_val & 0x0FFFFFFF
        payload = self._recv_all(msg_len)

        if msg_type == MessageType.ERROR:
            code, msg = cbor2.loads(payload)
            raise VictorError(code, msg)

        return msg_type, payload

    def _recv_all(self, n: int) -> bytes:
        """
        Receive exactly n bytes from the socket.
        
        Args:
            n: Number of bytes to receive
            
        Returns:
            Received bytes
            
        Raises:
            ConnectionError: If socket is closed before receiving all bytes
        """
        buf = bytearray()
        while len(buf) < n:
            chunk = self.sock.recv(n - len(buf))
            if not chunk:
                raise ConnectionError("Socket closed")
            buf.extend(chunk)
        return bytes(buf)


class VictorIndexClient(VictorClientBase):
    """
    Client for VictorDB vector index operations.
    
    Provides high-level interface for vector operations including INSERT,
    SEARCH, and DELETE on the vector index server.
    """

    def insert(self, id: int, vector: List[float]) -> int:
        """
        Insert a vector into the index.
        
        Args:
            id: Unique identifier for the vector
            vector: Vector data as list of floats
            
        Returns:
            The ID of the inserted vector
            
        Raises:
            VictorError: If insertion fails
        """
        msg = cbor2.dumps([id, vector])
        self._send_msg(MessageType.INSERT, msg)
        msg_type, payload = self._recv_msg()
        if msg_type != MessageType.INSERT_RESULT:
            raise VictorError(-1, f"Unexpected message type {msg_type}, expected INSERT_RESULT")
        [code, message] = cbor2.loads(payload)
        if code != 0:  # SUCCESS = 0
            raise VictorError(code, message)
        return id

    def delete(self, id_: int) -> bool:
        """
        Delete a vector from the index.
        
        Args:
            id_: ID of the vector to delete
            
        Returns:
            True if deletion was successful
            
        Raises:
            VictorError: If deletion fails
        """
        msg = cbor2.dumps([id_])
        self._send_msg(MessageType.DELETE, msg)
        msg_type, payload = self._recv_msg()
        if msg_type != MessageType.DELETE_RESULT:
            raise VictorError(-1, f"Unexpected message type {msg_type}, expected DELETE_RESULT")
        [code, message] = cbor2.loads(payload)
        if code != 0:
            raise VictorError(code, message)
        return code == 0  # SUCCESS = 0

    def search(self, vector: List[float], topk: int) -> List[Tuple[int, float]]:
        """
        Search for similar vectors in the index.
        
        Args:
            vector: Query vector as list of floats
            topk: Number of top results to return
            
        Returns:
            List of (id, distance) tuples for the closest vectors
            
        Raises:
            VictorError: If search fails
        """
        msg = cbor2.dumps([vector, topk])
        self._send_msg(MessageType.SEARCH, msg)
        msg_type, payload = self._recv_msg()
        
        if msg_type != MessageType.MATCH_RESULT:
            raise VictorError(-1, f"Unexpected message type {msg_type}, expected MATCH_RESULT")
        
        results = cbor2.loads(payload)
        return [(int(id_), float(distance)) for id_, distance in results]


class VictorTableClient(VictorClientBase):
    """
    Client for VictorDB table (key-value) operations.
    
    Provides high-level interface for key-value operations including PUT,
    GET, and DEL on the table server.
    """

    def put(self, key: bytes, value: bytes) -> bool:
        """
        Store a key-value pair in the table.
        
        Args:
            key: Key as bytes
            value: Value as bytes
            
        Returns:
            True if operation was successful
            
        Raises:
            VictorError: If put operation fails
        """
        msg = cbor2.dumps([key, value])
        self._send_msg(MessageType.PUT, msg)
        msg_type, payload = self._recv_msg()
        if msg_type != MessageType.PUT_RESULT:
            raise VictorError(-1, f"Unexpected message type {msg_type}, expected PUT_RESULT")
        [code, message] = cbor2.loads(payload)
        if code != 0:
            raise VictorError(code, message)
        return True

    def get(self, key: bytes) -> bytes:
        """
        Retrieve a value by key from the table.
        
        Args:
            key: Key as bytes
            
        Returns:
            Value as bytes
            
        Raises:
            VictorError: If key is not found or operation fails
        """
        msg = cbor2.dumps([key])
        self._send_msg(MessageType.GET, msg)
        msg_type, payload = self._recv_msg()
        if msg_type != MessageType.GET_RESULT:
            raise VictorError(-1, f"Unexpected message type {msg_type}, expected GET_RESULT")
        [value] = cbor2.loads(payload)
        return value

    def delete(self, key: bytes) -> bool:
        """
        Delete a key-value pair from the table.
        
        Args:
            key: Key as bytes
            
        Returns:
            True if deletion was successful
            
        Raises:
            VictorError: If deletion fails
        """
        msg = cbor2.dumps([key])
        self._send_msg(MessageType.DEL, msg)
        msg_type, payload = self._recv_msg()
        if msg_type != MessageType.DEL_RESULT:
            raise VictorError(-1, f"Unexpected message type {msg_type}, expected DEL_RESULT")

        [code, message] = cbor2.loads(payload)
        if code != 0:
            raise VictorError(code, message)
        return True  # SUCCESS = 0


# Legacy alias for backward compatibility
VictorClient = VictorIndexClient


if __name__ == "__main__":
    # Example usage for vector index operations
    """
    index_client = VictorIndexClient()
    index_client.connect(unix_path="/var/lib/victord/pepito/socket.unix")

    try:
        # INSERT a vector
        result_id = index_client.insert(5, [0.13, 0.22])
        print(f"Inserted vector with ID: {result_id}")
    except VictorError as e:
        print(f"Insert error: {e}")

    try:
        # SEARCH for similar vectors
        matches = index_client.search([0.13, 0.22], 4)
        print("Search results:", matches)
    except VictorError as e:
        print(f"Search error: {e}")

    try:
        # DELETE a vector
        deleted = index_client.delete(5)
        print(f"Delete successful: {deleted}")
    except VictorError as e:
        print(f"Delete error: {e}")

    index_client.close()
    """
    # Example usage for table operations
    table_client = VictorTableClient()
    table_client.connect(unix_path="/var/lib/victord/jose/socket.unix")

    try:
        # PUT a key-value pair
        success = table_client.put(b"user:123", b'{"name": "Alice", "age": 30}')
        print(f"Put successful: {success}")
    except VictorError as e:
        print(f"Put error: {e}")

    try:
        # GET a value by key
        value = table_client.get(b"user:123")
        print(f"Retrieved value: {value}")
    except VictorError as e:
        print(f"Get error: {e}")

    try:
        # DELETE a key
        deleted = table_client.delete(b"user:123")
        print(f"Delete successful: {deleted}")
    except VictorError as e:
        print(f"Delete error: {e}")

    table_client.close()