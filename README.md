step 1: make clean
step 2: make
step 3: sudo insmod usb_mouse_driver.ko
step 4: lsmod | grep usb_mouse
step 5: gcc -o pub pub.c -lmosquitto
step 6: gcc -o sub sub.c -lmosquitto -lmysqlclient
step 7: ./sub
step 8: sudo ./pub
step 9: chek data in mysql



