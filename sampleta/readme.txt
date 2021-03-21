Qsee5 SConscript
cd build/ms
python build_all.py --verbose CHIPSET=sdm845 -b TZ.XF.5.0.1 --cfg build_config_deploy.xml sampleta

Microtrust isee.mk
source setenv.sh
make -f isee.mk

Trusty rules.mk
export PATH=$PATH:/home/sheldon/androidQ/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin
make M=sampleta:TA

Trustonic's kinibi
./sampleta/platform/kinibi/Locals/Build/build.sh
