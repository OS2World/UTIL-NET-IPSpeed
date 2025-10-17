# Created by IBM WorkFrame/2 MakeMake at 17:54:39 on 06/24/96
#
# This makefile should be run in the following directory:
#   y:\c\ipspeed
#
# The actions included in this makefile are:
#   BINy::Resource Bind
#   COMPILE::C Set ++ Compile
#   COMPILE::Resource Compile
#   LINK::C Set ++ Link

.all: \
  .\ipspeed.exe

.SUFFIXES:

.SUFFIXES: .C .RC

.RC.res:
      @echo WF::COMPILE::Resource Compile
      rc.exe -r -x2 %s %|fF.RES

.C.obj:
      @echo WF::COMPILE::C Set ++ Compile
      icc.exe /Ss /Wcnscnveffobsprorearettruund /Fi /Si /O /G4 /Gi /C %s

.\ipspeed.exe: \
    .\IPSPEED.obj \
    y:\c\ipspeed\IPSPEED.DEF \
    .\IPSPEED.res \
    {$(LIB)}so32dll.lib \
    IPSPEED.MAK
      @echo WF::LINK::C Set ++ Link
      icc.exe @<<
 /B" /exepack:2 /pmtype:pm /packd"
 /Feipspeed.exe 
 so32dll.lib 
 .\IPSPEED.obj
 y:\c\ipspeed\IPSPEED.DEF
<<
      @echo WF::BINy::Resource Bind
      rc.exe -x2 .\IPSPEED.res ipspeed.exe


!include IPSPEED.Dep