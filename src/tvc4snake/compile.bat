@echo off
del tvc4snakes.cas 
del tvc4snakes
del tvc4snakes.lis
del tvc4snakes.c.lis

pushd c:\apps\z88dk
call z88dk_prompt.bat > null
popd
@echo on

zcc +tvc --list -create-app tvc4snakes.c -o tvc4snakes