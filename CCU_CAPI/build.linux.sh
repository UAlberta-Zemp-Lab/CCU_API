gcc -fPIC -shared ccu.c -I ./external/libserialport -L./precompiled -l:libserialport.so -o precompiled/ccu.so

gcc examples/CliniSonixBiasTester/CliniSonixBiasTester.c ccu.c -static-libgcc -static-libstdc++ -static -I ./external/libserialport -L./precompiled -l:libserialport.a -o examples/CliniSonixBiasTester/CliniSonixBiasTester
