import os
import can

os.system('sudo ip link set can0 type can bitrate 125000')
os.system('sudo ip link set can0 up')

can0 = can.interface.Bus(channel='can0', bustype='socketcan')
flag = False

file_data_chunks = []

while True:
    frame = can0.recv()
    if frame.arbitration_id == 0x123:
        print(frame.data)

        if frame.data == b'\xff\x00\xff\x00\xff\x00\xff\x00':
            print("Start of file Received")
            flag = True
            file_data_chunks = []
            continue

        if frame.data == b'\x00\xff\x00\xff\x00\xff\x00\xff':
            print("End of file transmission")
            flag = False
            if file_data_chunks:
                data = b"".join(file_data_chunks).decode('utf-8')
                file_name, file_data = data.split(':', 1)
                download_path = os.path.join(os.path.dirname(__file__), 'received_files')
                os.makedirs(download_path, exist_ok=True)
                file_path = os.path.join(download_path, file_name)
                with open(file_path, 'wb') as f:
                    f.write(file_data.encode('utf-8'))
                print(f"File '{file_name}' received and saved")
            continue

        if flag:
            file_data_chunks.append(frame.data)
