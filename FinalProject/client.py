import cv2
import pytesseract
import warnings
import socket
from multiprocessing import Process, Lock
import threading
import time
import random



#PAY 利用模擬的方式，模擬顧客付錢的行為
def PAY(plate,server_address, server_port,lock):
    time.sleep(random.randrange(5,35,5))
    ss = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ss.connect((server_address, server_port))
    
    output="Pay!0!"+plate
    print(output)
    ss.send(bytes(output,"utf8"))

    while 1:
        data=ss.recv(256)
        data=str(data,encoding='utf-8')
        if "Fee:" in data:
            print("pay:"+data.split("Fee:")[1].strip())
            ss.send(bytes(data.split("Fee:")[1].strip(),"utf8"))
            reply=ss.recv(256)
            reply=str(reply,encoding='utf-8')
            if "Wrong Amount" in reply :
                print("Wrong Amount\n")
                continue
            elif "Payment Success" in  reply:
                print(reply)
                break
        
        
    ss.close()




    



def process_camera(path,windows_name,window_x,window_y, server_address, server_port, lock):
    cap = cv2.VideoCapture(path)
    times=0
    print("readySS")
    while True:

        ret, frame = cap.read()
        #cap.set(cv2.CAP_PROP_FRAME_WIDTH,320)
        #cap.set(cv2.CAP_PROP_FRAME_HEIGHT,180)
        #cap.set(cv2.CAP_PROP_FPS,30)
        cap.set(cv2.CAP_PROP_FOURCC,cv2.VideoWriter_fourcc('M','J','P','G'))
        if ret:
            frame=cv2.resize(frame,(320,180))
            x_s,y_s=10,50
            x_e,y_e=300,150
            cv2.rectangle(frame,(x_s,y_s),(x_e,y_e),(0,255,0),3) # 框出ROI 
            frame=cv2.cvtColor(frame,cv2.COLOR_BGR2GRAY)
            roi=frame[y_s:y_e,x_s:x_e] # 輸出ROI來做辨識，透過降低辨識範圍，減少運算量。 
            cv2.imshow(windows_name,frame)
        times+=1
       
        if times%120==0 and ret :
            txt=pytesseract.image_to_string(roi)
            if len(txt)==8 and txt[0:3].isalpha() and txt[4:7].isdigit() :
                #with lock: 
                    print()
                    print("CAMERA:"+windows_name)
                    if windows_name=="1" or windows_name=="2" :
                        output="Entry!"+windows_name+"!"+txt
                        
                        print(output)
                        #print(len(output))
                        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                        s.connect((server_address, server_port))
                        s.send(bytes(output,"utf8"))
                        reply=s.recv(256)
                        reply=str(reply,encoding='utf-8')
             
                        if "No space" in reply :
                           
                            print(reply)
                            s.close()
                        elif "Entry Success" in reply :
                            
                            print(reply)
                            s.close()
                            thread=threading.Thread(target=PAY,args=(txt,server_address, server_port,lock))
                            thread.start()
                        elif "already parked" in reply :
                            print(reply)
                            s.close()
                            
                            
                    else :
                        output="Exit!"+windows_name+"!"+txt
                        print(output)
                        #print(len(output))
                        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                        s.connect((server_address, server_port))
                        s.send(bytes(output,"utf8"))
                        reply=s.recv(256)
                        reply=str(reply,encoding='utf-8')
                        if "Exit Success" in reply :
                            print("Exit Success\n")
                            s.close()
                        elif "Exit Failed" in reply :
                            print(reply)
                            s.close()
                            thread=threading.Thread(target=PAY,args=(txt,server_address, server_port,lock))
                            thread.start()
                        elif "Car doesn't exist" in reply:
                            print(reply+"plz scan again!!!")
                            s.close()
                            
                        
                        
                        

        if cv2.waitKey(1)&0xFF ==ord('q'):
            break

    cap.release()
  

if __name__ == "__main__":
    # 定義webcam數量
    num_cameras = 4
    # 定義服務器端地址和端口
    server_address = '127.0.0.1'  # 服務器IP地址
    server_port = 4444  # 服務器端口

    # 創建進程列表
    processes = []
    # 創建鎖
    lock = Lock()
    camera_path=['/dev/video0','/dev/video2','/dev/video4','/dev/video6']
    names=["1","2","3","4"]
    
    window_x=[100,400,100,400]
    window_y=[100,100,400,400]

    # 啟動多個進程處理不同的webcam
    for i in range(num_cameras):
        p = Process(target=process_camera, args=(camera_path[i],names[i],window_x[i],window_y[i], server_address, server_port, lock))
        processes.append(p)
        p.start()

    for p in processes:
        p.join()
