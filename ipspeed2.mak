# Created by IBM WorkFrame/2 MakeMake at 21:09:54 on 06/25/96
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
  .\ipspeed2.exe

.SUFFIXES:

.SUFFIXES: .C .RC

.RC.res:
      @echo WF::COMPILE::Resource Compile
      rc.exe -r %s %|fF.RES

.C.obj:
      @echo WF::COMPILE::C Set ++ Compile
      icc.exe /DUSE16BIT /Ss /Wcnscnveffobsprorearettruund /Fi /Si /O /G4 /Gi /C %s

.\ipspeed2.exe: \
    .\IPSPEED.obj \
    y:\c\ipspeed\IPSPEED2.DEF \
    .\IPSPEED.res \
    IPSPEED2.MAK
      @echo WF::LINK::C Set ++ Link
      icc.exe @<<
 /B" /pmtype:pm /packd"
 /Feipspeed2.exe 
 .\IPSPEED.obj
 y:\c\ipspeed\IPSPEED2.DEF
<<
      @echo WF::BINy::Resource Bind
      rc.exe .\IPSPEED.res ipspeed2.exe


!include IPSPEED2.Dep