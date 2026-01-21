//dc.h
//header for dc.cpp *yup, really*
#pragma once

bool Init_DC();
//this is to be called from emulation code
bool SoftReset_DC();
//this is to be called from external thread
bool Reset_DC(bool Manual);
void Term_DC();
void Start_DC();
void Stop_DC();
void LoadBiosFiles();
bool IsDCInited();
void SwitchCPU_DC();
