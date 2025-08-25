#!/bin/bash
# 
# VictorDB Server Manager Installation Script
#
# This script builds and installs the victor-server Python package

set -e

echo "VictorDB Server Manager - Installation Script"
echo "=============================================="

# Check if we're in the right directory
if [ ! -f "pyproject.toml" ] || [ ! -d "victor_server" ]; then
    echo "Error: Please run this script from the python-package directory"
    exit 1
fi

# Check Python version
python_version=$(python3 -c 'import sys; print(".".join(map(str, sys.version_info[:2])))')
echo "Detected Python version: $python_version"

# Build the package
echo "Building package..."
python3 -m pip install --upgrade build
python3 -m build

# Install the package
echo "Installing package..."
pip3 install dist/*.whl --force-reinstall

echo ""
echo "Installation completed successfully!"
echo ""
echo "You can now use the victor-server command:"
echo "  victor-server --help"
echo ""
echo "Or import it in Python:"
echo "  from victor_server import VictorServerManager, ServerConfig"
echo ""
echo "For examples, see the README.md file."
