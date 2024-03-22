g++ main.cpp -o app -lcurl  -lwiringPi  $(pkg-config --cflags --libs opencv)
