def print_binary_file(file_path):
    with open(file_path, "rb") as f:
        byte = f.read(1)
        while byte:
            # 각 바이트를 이진수로 출력
            print(format(ord(byte), '08b'), end=" ")
            byte = f.read(1)


def print_hex_file(file_path):
    with open(file_path, "rb") as f:
        byte = f.read(1)
        while byte:
            # 각 바이트를 16진수로 출력
            print(format(ord(byte), '02x'), end=" ")
            byte = f.read(1)

# 파일 경로 지정
file_path = "C:\ota_code\stm32\PART_B.bin"
print_hex_file(file_path)   # .bin 파일 내용을 hex로 변환
