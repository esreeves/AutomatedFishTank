import serial

ser = serial.Serial('/dev/ttyACM0',9600)
s = [0]
while True:
        read_serial=ser.readline()
        s[0] = str((ser.readline()))
        print s[0]
        #print read_serial

        with open('PH_log.txt', 'w') as f:
            f.write(str(read_serial))
            f.close()
