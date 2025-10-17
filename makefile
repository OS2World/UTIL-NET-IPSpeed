# Created by IBM WorkFrame/2 MakeMake at 15:33:11 on 06/20/96
#
# This makefile should be run in the following directory:
#   d:\c\ipspeed
#
# The actions included in this makefile are:
#   BIND::Resource Bind
#   COMPILE::C Set ++ Compile
#   COMPILE::Resource Compile
#   LINK::C Set ++ Link

.all: \
  .\ipspeed.exe

.SUFFIXES:

.SUFFIXES: .C .RC

.RC.res:
      @echo WF::COMPILE::Resource Compile
      rc.exe -r %s %|fF.RES

.C.obj:
      @echo WF::COMPILE::C Set ++ Compile
      icc.exe /Ss /Wcnscnveffobsprorearettruund /Fi /Si /Ti /Gd /G4 /C %s

.\ipspeed.exe: \
    .\IPSPEED.obj \
    .\IPSPEED.res \
    MAKEFILE
      @echo WF::LINK::C Set ++ Link
      icc.exe @<<
 /Feipspeed.exe 
 .\IPSPEED.obj
<<
      @echo WF::BIND::Resource Bind
      rc.exe .\IPSPEED.res ipspeed.exe


!include MAKEFILE.Dep