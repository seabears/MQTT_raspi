from canlib import canlib, Frame
import time
import os

# Kvaser 연결 및 설정 class
class Kvaser:
    def __init__(self, channel=0):
        self.channel = channel
        self.openFlags = canlib.canOPEN_ACCEPT_VIRTUAL
        self.bitrate = canlib.canBITRATE_125K
        self.bitrateFlags = canlib.canDRIVER_NORMAL

        self.valid = False
        self.ch = None
        self.device_name = ''
        self.card_upc_no = ''
        try:
            self.ch = canlib.openChannel(self.channel, self.openFlags)
            self.ch.setBusOutputControl(self.bitrateFlags)
            self.ch.setBusParams(self.bitrate)
            self.ch.iocontrol.timer_scale = 1
            self.ch.iocontrol.local_txecho = True
            self.ch.busOn()
            self.valid = True
            self.device_name = canlib.ChannelData.channel_name
            self.card_upc_no = canlib.ChannelData(self.channel).card_upc_no
        except canlib.exceptions.CanGeneralError as e:
            print(f"Error initializing Kvaser channel: {e}")
            self.valid = False
            self.ch = None

    def __del__(self):
        if self.ch:
            self.tearDownChannel()

    def read(self, id, timeout_ms=-1): #CAN read 대기시간
        try:
            result = self.ch.read(timeout=timeout_ms)
            if result.id == id:
                return result
        except canlib.canNoMsg:
            print("No message received")
        except canlib.canError as e:
            print(f"Error reading from Kvaser channel: {e}")
        return None

    def transmit_data(self, id: int, data: bytes, msgFlag=canlib.canMSG_STD):
        frame = Frame(id_=id, data=data, flags=msgFlag)
        try:
            self.ch.write(frame)
        except canlib.exceptions.CanGeneralError as e:
            print(f"Error transmitting data: {e}")

    def __iter__(self):
        while True:
            try:
                frame = self.ch.read()
                yield frame
            except canlib.canNoMsg:
                yield 0
            except canlib.canError:
                return

    def tearDownChannel(self):
        self.ch.busOff()
        self.ch.close()

# 큰 데이터를 CAN 전송을 위해 분할하기
def split_data_into_chunks(data, chunk_size=8):
    chunks = []
    total_chunks = (len(data) + chunk_size - 1) // chunk_size
    for i in range(total_chunks):
        chunk = data[i*chunk_size:(i+1)*chunk_size]
        chunks.append(chunk)
    return chunks

# PART_B.bin 파일을 읽고 데이터를 전송하는 함수
def transmit_firmware(file_path):
    transmitter = Kvaser()

    try:
        # 1. 펌웨어 업데이트 시작
        transmitter.transmit_data(id=0x7B, data=b'abcdefgh')

        # 2. 파일 크기 전송
        file_size = os.path.getsize(file_path)
        size_bytes = file_size.to_bytes(4, byteorder='little')
        transmitter.transmit_data(id=0x71, data=size_bytes)
        print(f"Sent firmware size: {file_size} bytes")
        time.sleep(0.2)

        # 3. 파일 데이터 읽기
        with open(file_path, 'rb') as firmware_file:
            data = firmware_file.read()

        # 4. 데이터 분할 전송
        chunks = split_data_into_chunks(data, chunk_size=8)
        total_chunks = len(chunks)
        sent_chunks = 0

        for chunk in chunks:
            # 남은 데이터가 8바이트보다 작으면 패딩
            if len(chunk) < 8:
                chunk += bytes([0xFF] * (8 - len(chunk)))  # 패딩
            transmitter.transmit_data(id=0x70, data=chunk)
            sent_chunks += 1

            # 진행률 계산
            progress = (sent_chunks / total_chunks) * 100
            print(f"Transmitting... {progress:.2f}% completed")
            time.sleep(0.2)

    except KeyboardInterrupt:
        print("Transmission stopped by user.")
    finally:
        del transmitter

def main():
    file_path = r'C:\ota_code\stm32\PART_B\Debug\PART_B.bin'  # 파일 경로 설정
    #file_path = r'C:\ota_code\stm32\PART_A\Debug\PART_A.bin'  # 파일 경로 설정
    
    transmit_firmware(file_path)

if __name__ == "__main__":
    main()
