@echo off

set BAK = % PATH %
		  set pwd = % ~dp0
					set pdir = % pwd % ..
							   set PATH = % PATH %;
% pdir % \tools

del / F % pdir % \bin\eagle.app.v6.flash.bin % pdir % \bin\eagle.app.v6.irom0text.bin % pdir % \bin\eagle.app.v6.dump % pdir % \bin\eagle.app.v6.S

pushd % pwd % .output\eagle\debug\image

set image = eagle.app.v6.out
			set name = % image:
					   ~0, -4 %
					   xt - objdump - x - s % image % > % pdir % \bin\ % name % .dump
					   xt - objdump - S % image % > % pdir % \bin\ % name % .v6.S

					   xt - objcopy --only - section .text - O binary % image % % name % .text.bin
					   xt - objcopy --only - section .data - O binary % image % % name % .data.bin
					   xt - objcopy --only - section .rodata - O binary % image % % name % .rodata.bin
					   xt - objcopy --only - section .irom0.text - O binary % image % % name % .irom0text.bin

					   del / F eagle.app.flash.bin
					   python C:
					   \usr\xtensa\esp_iot_sdk\tools\gen_appbin.py % image % 0 0 0 0
					   ::python C:
					   \usr\xtensa\esp_iot_sdk\tools\gen_flashbin.py eagle.app.flash.bin % name % .irom0text.bin

					   xcopy / y % name % .irom0text.bin % pdir % \bin\
					   ::xcopy / y % name % .flash.bin % pdir % \bin\
					   xcopy / y eagle.app.flash.bin % pdir % \bin
					   echo.
					   echo OUTPUT:
					   echo % pdir % \bin\eagle.app.flash.bin
					   popd

					   set PATH = % BAK %
								  set pdir =
									  set pwd =
											  set BAK =
													  set image =
															  set name =
