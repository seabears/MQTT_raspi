from flask import Flask, request, redirect, render_template, url_for,flash
import os
#from crypto import compute_data_hash
#import crypto
from crypto import *

import sys
sys.path.append(os.path.abspath(".."))
from pub_file import *

app = Flask(__name__)
app.secret_key = 'secret_key'

UPLOAD_FOLDER = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'upload')
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

@app.route('/')
def upload_form():
    return render_template('upload.html')

@app.route('/upload', methods=['POST'])
def upload_file():
    if 'file' not in request.files:
        flash("파일을 선택하세요")
        return redirect(url_for('upload_form'))
    uploaded_file  = request.files['file']
    username = request.form['username']
    password = request.form['password']
    hashed_pw = compute_data_hash(password)
    print(f'{hashed_pw}')

    login_valid = 0
    # 파일에서 ID, PW 읽기
    base_dir = os.path.dirname(os.path.abspath(__file__))
    file_path = os.path.join(base_dir, 'login.txt')
    try:
        with open(file_path, 'r') as file:
            accounts = file.readlines()
    except FileNotFoundError:
        flash('file err.')
        return redirect(url_for('upload_form'))

    # 로그인 검증
    for account in accounts:
        # 줄 끝의 개행문자 제거 후 아이디:비밀번호 형태 분리
        account = account.strip()
        if not account:
            continue
        stored_username, stored_password = account.split(':')

        if username == stored_username and hashed_pw == stored_password:
            flash('valid id/pw.')
            login_valid = True

    if login_valid == 0:
        flash('invalid id/pw.')
        return redirect(url_for('upload_form'))


    print(f'file:{uploaded_file.filename}, Username:{username}, Password:{password}')

    if uploaded_file .filename == '':
        flash('파일이 존재하지 않습니다.')
        return redirect(url_for('upload_form'))
    if uploaded_file :
        file_path = os.path.join(app.config['UPLOAD_FOLDER'], uploaded_file.filename)
        uploaded_file.save(file_path)



        ######################## 개인키 파일을 보내려고 하는 파일과 함께 업로드 후, 서명하는 것이 올바른듯(아래는 아님)
        # 파일 신뢰성 검증
        file_data = uploaded_file.read()  # bytes로 읽기
        private_key_path = "C:/ota_code/web/key/Private_key1.pem"
        signature = sign_file(file_data, private_key_path, b'private')  # 정상 서명
        fake_signature = os.urandom(256)    # 거짓된 서명 (테스트용)
        # 서버에서 파일을 받으면       # signature 자리에 fake_signature 넣으면 거짓 서명 테스트 가능
        if verify_sign(signature, file_data, "C:/ota_code/web/key/Public_key1.pem"): 
            print("✅ 신뢰된 파일입니다.")
        else:
            print("❌ 신뢰되지 않은 파일입니다.")
            return redirect(url_for('upload_form'))
        
        # 서명 저장
        signature_path = "C:/ota_code/signature_file.txt"
        save_signature(signature, signature_path)
        
        ########################

        broker_ip = '192.168.137.104'
        send_file_to_broker(file_path, broker_ip, username, password)      # 그냥 파일 보내기
        #send_file_to_broker(signature_path, broker_ip, username, password)  # 서명 파일 보내기



        flash(f"File '{uploaded_file .filename}' 성공적으로 업로드 되었습니다!")
        return redirect(url_for('upload_form'))
        
def save_signature(signature, signature_path):
    with open(signature_path, 'wb') as signature_file:
        signature_file.write(signature)
    print(f"Signature saved to: {signature_path}")


if __name__ == "__main__":
    app.run(host='0.0.0.0', port=5000, debug=True)


'''
web 페이지에
login.txt에 기록된 id/pw로 로그인하고
파일 업로드하면
라즈베리파이(ip 주소) 브로커로 전송

sub_file.py로 파일 수신 tmp 폴더에 저장


'''