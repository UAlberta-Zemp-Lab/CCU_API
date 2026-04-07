gcc -shared ccu.c -I ./external/libserialport -L./precompiled -l:libserialport.dll -o precompiled/ccu.dll

gcc examples/CliniSonixBiasTester/CliniSonixBiasTester.c ccu.c -static-libgcc -static-libstdc++ -static -I ./external/libserialport -L./precompiled -l:serialport.lib -lsetupapi -o examples/CliniSonixBiasTester/CliniSonixBiasTester.exe
