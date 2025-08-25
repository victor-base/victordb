#!/usr/bin/env python3
"""
Setup script for VictorDB Server Manager
"""

from setuptools import setup, find_packages
import os

# Read the README file
def read_readme():
    readme_path = os.path.join(os.path.dirname(__file__), 'README.md')
    if os.path.exists(readme_path):
        with open(readme_path, 'r', encoding='utf-8') as f:
            return f.read()
    return "VictorDB Server Manager - Python wrapper for managing VictorDB servers"

# Read requirements
def read_requirements():
    req_path = os.path.join(os.path.dirname(__file__), 'requirements.txt')
    if os.path.exists(req_path):
        with open(req_path, 'r') as f:
            return [line.strip() for line in f if line.strip() and not line.startswith('#')]
    return []

setup(
    name="victor-server",
    version="1.0.0",
    author="VictorDB Team",
    author_email="support@victordb.com",
    description="Python wrapper for managing VictorDB servers",
    long_description=read_readme(),
    long_description_content_type="text/markdown",
    url="https://github.com/victor-base/victordb",
    packages=find_packages(),
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Topic :: Database",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Operating System :: OS Independent",
        "Operating System :: POSIX :: Linux",
        "Operating System :: MacOS",
    ],
    python_requires=">=3.7",
    install_requires=read_requirements(),
    keywords="vectordb database vector similarity search machine-learning ai",
    project_urls={
        "Bug Reports": "https://github.com/victor-base/victordb/issues",
        "Source": "https://github.com/victor-base/victordb",
        "Documentation": "https://github.com/victor-base/victordb/wiki",
    },
    include_package_data=True,
    zip_safe=False,
)
