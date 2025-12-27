#!/usr/bin/env python3
"""
ZMQ JSON Sender Script

This script connects to a ZMQ REP socket via REQ socket on TCP port 5559.
It scans the current directory for JSON files, allows the user to select one,
reads the JSON content and sends it as a string. Then it receives and prints the response.

Usage: python ZMQ_JSON_Sender.py <IP_ADDRESS>
"""

import zmq
import sys
import json
import os
import glob
import time

def scan_json_files():
    """
    Scan the current directory for JSON files and return a list of them.
    
    Returns:
        list: List of JSON file names found in the current directory
    """
    # Get the directory where this script is located
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Find all JSON files in the script directory
    json_pattern = os.path.join(script_dir, "*.json")
    json_files = glob.glob(json_pattern)
    
    # Return just the filenames, not full paths
    return [os.path.basename(f) for f in json_files]


def select_json_file(json_files):
    """
    Display available JSON files and let the user select one.
    
    Args:
        json_files (list): List of available JSON file names
        
    Returns:
        str: Selected JSON filename or None if cancelled
    """
    if not json_files:
        print("No JSON files found in the current directory.")
        return None
    
    print("\nAvailable JSON files:")
    for i, filename in enumerate(json_files, 1):
        print(f"{i}. {filename}")
    
    while True:
        try:
            choice = input(f"\nSelect a file (1-{len(json_files)}) or 'q' to quit: ").strip()
            
            if choice.lower() == 'q':
                return None
                
            choice_num = int(choice)
            if 1 <= choice_num <= len(json_files):
                return json_files[choice_num - 1]
            else:
                print(f"Please enter a number between 1 and {len(json_files)}")
                
        except ValueError:
            print("Please enter a valid number or 'q' to quit")


def read_json_file(filename):
    """
    Read and validate JSON content from the specified file.
    
    Args:
        filename (str): Name of the JSON file to read
        
    Returns:
        str: JSON content as string, or None if error occurred
    """
    try:
        # Get the directory where this script is located
        script_dir = os.path.dirname(os.path.abspath(__file__))
        file_path = os.path.join(script_dir, filename)
        
        with open(file_path, 'r', encoding='utf-8') as file:
            content = file.read()
            
        # Validate that it's proper JSON
        json.loads(content)
        return content
        
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found.")
        return None
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in file '{filename}': {e}")
        return None
    except Exception as e:
        print(f"Error reading file '{filename}': {e}")
        return None


def send_json_via_zmq(ip_address, json_content):
    """
    Send JSON content via ZMQ REQ socket and receive response.
    
    Args:
        ip_address (str): Target IP address
        json_content (str): JSON content to send
        
    Returns:
        str: Response received from server, or None if error occurred
    """
    context = None
    socket = None
    
    try:
        # Create ZMQ context and REQ socket
        context = zmq.Context()
        socket = context.socket(zmq.REQ)
        
        # Set socket timeout (50 seconds)
        socket.setsockopt(zmq.RCVTIMEO, 50000)
        socket.setsockopt(zmq.SNDTIMEO, 50000)
        
        # Connect to the server
        connection_string = f"tcp://{ip_address}:5559"
        socket.connect(connection_string)
        
        print(f"Connecting to {connection_string}...")
        
        # Send JSON content as string
        print("Sending JSON content...")
        socket.send_string(json_content)
        tstart = time.time()
        # Receive response
        print("Waiting for response...")
        response = socket.recv_string()
        tend = time.time()
        print(f"Response time: {(tend - tstart) * 1000} ms")
        return response
        
    except zmq.Again:
        print("Error: Timeout - no response received within 50 seconds")
        return None
    except zmq.ZMQError as e:
        print(f"ZMQ Error: {e}")
        return None
    except Exception as e:
        print(f"Unexpected error: {e}")
        return None
    finally:
        # Clean up
        if socket:
            socket.close()
        if context:
            context.term()


def main():
    """
    Main function that orchestrates the entire process.
    """
    # Check command line arguments
    if len(sys.argv) != 2:
        print("Usage: python ZMQ_JSON_Sender.py <IP_ADDRESS>")
        print("Example: python ZMQ_JSON_Sender.py 192.168.1.100")
        sys.exit(1)
    
    ip_address = sys.argv[1]
    
    print("ZMQ JSON Sender")
    print("=" * 50)
    
    # Step 1: Scan for JSON files
    print("Scanning for JSON files in current directory...")
    json_files = scan_json_files()
    
    if not json_files:
        print("No JSON files found. Exiting.")
        sys.exit(1)
    
    # Step 2: Let user select a JSON file
    selected_file = select_json_file(json_files)
    if not selected_file:
        print("No file selected. Exiting.")
        sys.exit(0)
    
    print(f"Selected file: {selected_file}")
    
    # Step 3: Read JSON content
    print("Reading JSON content...")
    json_content = read_json_file(selected_file)
    if not json_content:
        print("Failed to read JSON content. Exiting.")
        sys.exit(1)
    
    print("JSON content loaded successfully.")
    print(f"Content preview: {json_content[:100]}..." if len(json_content) > 100 else f"Content: {json_content}")
    
    # Step 4: Send via ZMQ and receive response
    response = send_json_via_zmq(ip_address, json_content)
    
    if response:
        print("\n" + "=" * 50)
        print("RESPONSE RECEIVED:")
        print("=" * 50)
        print(response)
        
        # Try to pretty-print if it's JSON
        try:
            response_json = json.loads(response)
            print("\nFormatted JSON response:")
            print(json.dumps(response_json, indent=2))
        except json.JSONDecodeError:
            print("\n(Response is not valid JSON, showing as plain text)")
    else:
        print("Failed to receive response.")
        sys.exit(1)


if __name__ == "__main__":
    main()
