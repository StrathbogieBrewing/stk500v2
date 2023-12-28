# Pico SDK Docker Image

Run from 
```$ docker run --rm  -v `pwd`:/project -w /project -it ghcr.io/strathbogiebrewing/stk500v2/pico-build:v1.0.0```

sudo docker run --rm -v `pwd`:/project -w /project --privileged -v /dev/bus/usb:/dev/bus/usb -it f664da2b0931

src/openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" -c "program /workdir/pico/build/stk500v2.elf verify" -c "reset" -c "exit" -s tcl