# Created by IBM WorkFrame/2 MakeMake at 16:13:47 on 06/24/96
#
# This makefile should be run in the following directory:
#   d:\c\ipspeed
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
      rc.exe -r %s %|fF.RES

.C.obj:
      @echo WF::COMPILE::C Set ++ Compile
      icc.exe /Ss /Wcnscnveffobsprorearettruund /Fi /Si /Ti /G4 /Gi /C %s

.\ipspeed.exe: \
    .\IPSPEED.obj \
    d:\c\ipspeed\IPSPEED.DEF \
    .\IPSPEED.res \
    {$(LIB)}so32dll.lib \
    IPSPEEDD.MAK
      @echo WF::LINK::C Set ++ Link
      icc.exe @<<
 /B" /de /pmtype:pm /packd"
 /Feipspeed.exe 
 so32dll.lib 
 .\IPSPEED.obj
 d:\c\ipspeed\IPSPEED.DEF
<<
      @echo WF::BINd::Resource Bind
      rc.exe .\IPSPEED.res ipspeed.exe


!include IPSPEEDD.Dep
