import socket
import threading
import time
import struct
import sys

class Client:
    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    def connect(self, host_ip, port=8000):
        self.sock.connect((host_ip, port))

        print(f"[CLIENT] Connected to {host_ip}:{port}")

    def send(self, data):
        if isinstance(data, str):
            data = data.encode('utf-8')

        self.sock.sendall(data)

    def recv(self, buffer_size=1024):
        return self.sock.recv(buffer_size)
    
    def send_u32_values(self, values):
        values = [int(value) & 0xFFFFFFFF for value in values]
        packet = struct.pack(f"!{len(values) + 1}I", len(values), *values)
        self.send(packet)

    def close(self):
        self.sock.close()

class AppClient:
    def __init__(self, host_ip="192.168.2.1", port=8000)->None:
        self.client  = Client()
        self.host_ip = host_ip
        self.port    = port

        self.adc_average_n_elements = 10
        self.adc_delay = 10000
        self.adc_burst_limit = 20

        self.command_list = [
            "set",
            "get",
            "exit",
            "disconnect"
        ]

        self.app_command_list = {
            # ADC commands
            "adc_average_n" : {
                "r_enable" : True,
                "w_enable" : True,
                "r_params" : 0,
                "w_params" : 1,
                "r"        : lambda : self.adc_average_n_elements,
                "w"        : lambda value : setattr(self, 'adc_average_n_elements', value)
            },
            "adc_delay" : {
                "r_enable" : True,
                "w_enable" : True,
                "r_params" : 0,
                "w_params" : 1,
                "r"        : lambda : self.adc_delay,
                "w"        : lambda value : setattr(self, 'adc_delay', value)
            },
            "adc_burst_limit" : {
                "r_enable" : True,
                "w_enable" : True,
                "r_params" : 0,
                "w_params" : 1,
                "r"        : lambda : self.adc_burst_limit,
                "w"        : lambda value : setattr(self, 'adc_burst_limit', value)
            },
            "adc_start_w" : {
                "r_enable" : True,
                "w_enable" : False,
                "r_params" : 0,
                "w_params" : 1,
                "r"        : lambda : self.adc_start_w(),
                "w"        : lambda value : None
            }
        }
    
    def adc_start_w(self):
        # Simulate starting ADC acquisition and returning a dummy value

        values = []
        for i in range(self.adc_average_n_elements):
            values.append((i * 100) % 0xFFFFFFFF)  # Generate dummy values based on index

        # Wait burst_limit * 694 us
        total_delay = self.adc_burst_limit * 694 / 1_000_000  # Convert to seconds
        time.sleep(total_delay)

        return values

    def encode_return_values(self, value):
        if value is None:
            return []

        if isinstance(value, (list, tuple)):
            return [int(v) & 0xFFFFFFFF for v in value]

        return [int(value) & 0xFFFFFFFF]

    def send_set_response(self, success):
        """Send set command response: <1> <status> where status is 1 for success, 0 for failure"""
        status = 1 if success else 0
        packet = struct.pack("!II", 1, status)
        self.client.send(packet)

    def send_get_response(self, values):
        """Send get command response: <2> <n_values> <value_1> ... <value_n>"""
        encoded_values = self.encode_return_values(values)
        n_values = len(encoded_values)
        packet = struct.pack(f"!II{n_values}I", 2, n_values, *encoded_values)
        self.client.send(packet)

    def send_help_response(self, help_message):
        """Send help command response: <3> <help_message> with utf-8 decoded text"""
        command_type = struct.pack("!I", 3)
        help_text = help_message.encode('utf-8')
        self.client.send(command_type + help_text)

    def wait_for_connection(self):
        """Wait for connection to host with retry logic"""
        while True:
            try:
                # Create a new client for each connection attempt
                # (sockets cannot be reused after failed connection attempts)
                self.client = Client()
                self.client.connect(self.host_ip, self.port)
                print(f"[APP CLIENT] Connection established with host at {self.host_ip}:{self.port}")
                break
            except (ConnectionRefusedError, OSError) as e:
                print(f"[APP CLIENT] Connection failed, retrying in 1 second...")
                time.sleep(1)

    def handle_set_command(self, command):
        """Handle set command: set <app_command> [params...]"""
        if len(command) < 2:
            print(f"[APP CLIENT] Error: set command requires at least 1 parameter")
            self.send_set_response(False)
            return
        
        app_command = command[1].lower()
        if app_command not in self.app_command_list:
            print(f"[APP CLIENT] Error: {app_command} is not a valid command")
            self.send_set_response(False)
            return
        
        # Check if w_enable is True
        if not self.app_command_list[app_command]["w_enable"]:
            print(f"[APP CLIENT] Error: {app_command} is not writable")
            self.send_set_response(False)
            return
        
        # Check if the number of parameters is correct
        expected_params = self.app_command_list[app_command]["w_params"]
        actual_params = len(command) - 2
        if actual_params != expected_params:
            print(f"[APP CLIENT] Error: {app_command} requires {expected_params} parameters, got {actual_params}")
            self.send_set_response(False)
            return
        
        # Execute the command
        try:
            params = command[2:]
            result = self.app_command_list[app_command]["w"](*params)
            self.send_set_response(True)
        except Exception as e:
            print(f"[APP CLIENT] Error executing set command: {e}")
            self.send_set_response(False)

    def handle_get_command(self, command):
        """Handle get command: get <app_command> [params...]"""
        if len(command) < 2:
            print(f"[APP CLIENT] Error: get command requires at least 1 parameter")
            self.send_get_response([])
            return
        
        app_command = command[1].lower()
        if app_command not in self.app_command_list:
            print(f"[APP CLIENT] Error: {app_command} is not a valid command")
            self.send_get_response([])
            return
        
        # Check if r_enable is True
        if not self.app_command_list[app_command]["r_enable"]:
            print(f"[APP CLIENT] Error: {app_command} is not readable")
            self.send_get_response([])
            return
        
        # Check if the number of parameters is correct
        expected_params = self.app_command_list[app_command]["r_params"]
        actual_params = len(command) - 2
        if actual_params != expected_params:
            print(f"[APP CLIENT] Error: {app_command} requires {expected_params} parameters, got {actual_params}")
            self.send_get_response([])
            return
        
        # Execute the command
        try:
            params = command[2:]
            value = self.app_command_list[app_command]["r"](*params)
            self.send_get_response(value)
        except Exception as e:
            print(f"[APP CLIENT] Error executing get command: {e}")
            self.send_get_response([])

    def handle_disconnect_command(self, command):
        """Handle disconnect command - disconnect and wait for reconnection"""
        print(f"[APP CLIENT] Disconnecting...")

        # Return dummy get response
        self.send_get_response([])  # Send dummy response to acknowledge disconnect command

        try:
            self.client.close()
        except Exception as e:
            print(f"[APP CLIENT] Error closing connection: {e}")
        
        # Create new client socket
        self.client = Client()
        
        # Wait for new connection
        print(f"[APP CLIENT] Waiting for reconnection...")
        self.wait_for_connection()
        return True  # Signal to continue loop

    def handle_exit_command(self, command):
        """Handle exit command"""
        print(f"[APP CLIENT] Exiting...")
        return False  # Signal to exit loop

    # Program loop
    def loop(self):
        """Receive commands from the host and execute them"""
        while True:
            data = self.client.recv(1024)
            if not data:
                print(f"[APP CLIENT] Connection closed by host")
                break

            # Parse command: split by whitespace
            command = data.decode('utf-8').strip().split()
            print(f"[APP CLIENT] Received command: {command}")

            # Ensure command is a list
            if isinstance(command, str):
                command = [command]

            if not command:
                print(f"[APP CLIENT] Error: empty command received")
                continue

            # Dispatch to appropriate handler
            command_type = command[0].lower()
            
            if command_type == "exit":
                if self.handle_exit_command(command) == False:
                    break
            elif command_type == "disconnect":
                self.handle_disconnect_command(command)
            elif command_type == "set":
                self.handle_set_command(command)
            elif command_type == "get":
                self.handle_get_command(command)
            else:
                print(f"[APP CLIENT] Error: '{command_type}' is not a valid command type")
                self.send_set_response(False)

        # Close the connection
        self.client.close()

if __name__ == "__main__":
    # Run against a host/server on the same machine with:
    #   python client.py test
    app_client = AppClient(host_ip="127.0.0.1", port=8000)

    app_client.wait_for_connection()
    app_client.loop()
    