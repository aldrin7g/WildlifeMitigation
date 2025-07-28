import serial
import time

serial_port = '/dev/ttyS0'
baud_rate = 9600
phone_number = '6382667630'
message = 'Demo Message!'

def send_at_command(command, expected_response, timeout=1):
    ser.write((command + '\r\n').encode())
    time.sleep(timeout)
    response = ser.read_all().decode()
    print(f'Sent: {command}, Received: {response.strip()}')
    return expected_response in response

try:
    ser = serial.Serial(serial_port, baud_rate, timeout=1)
    time.sleep(2)

    if send_at_command('AT', 'OK'):
        print('SIM800 module is ready')

        if send_at_command('AT+CMGF=1', 'OK'):
            print('SMS text mode set')

            if send_at_command(f'AT+CMGS="{phone_number}"', '>'):
                ser.write((message + '\x1A').encode())
                time.sleep(3)
                print('Message sent')
            else:
                print('Failed to set recipient number')
        else:
            print('Failed to set SMS text mode')
    else:
        print('SIM800 module not responding')

except Exception as e:
    print(f'Error: {e}')

finally:
    ser.close()
