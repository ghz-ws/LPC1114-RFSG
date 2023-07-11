import serial
inst=serial.Serial("COM14",115200)
Freq=[100,100]  ##MHz unit
Ampl=[0,0]  ##dBm unit. 50ohm loaded.

buf=f'{Freq[0]*1000:07}'+f'{int(4-Ampl[0]):02}'+f'{Freq[1]*1000:07}'+f'{int(4-Ampl[1]):02}'
inst.write(buf.encode())
print('Ch1 Freq=',Freq[0],'MHz, Ampl=',Ampl[0],'dBm\nCh2 Freq=',Freq[1],'MHz, Ampl=',Ampl[1],'dBm')
buf=int(inst.read(1))
if buf==1:
    data1='Locked'
else:
    data1='Unlocked'
buf=int(inst.read(1))
if buf==1:
    data2='Locked'
else:
    data2='Unlocked'
print('Ch1:',data1,', Ch2:',data2,'\n')