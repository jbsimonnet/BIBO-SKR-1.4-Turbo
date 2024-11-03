#include "ProbeHeightControl.h"
#include "includes.h"

#define ENDSTOP_CMD     "M211 S%d\n"
#define ENDSTOP_CMD_RRF "M564 S%d H%d\n"  // for RRF
#define MOVE_Z_CMD      "G1 Z%.2f F%d\n"

#define PROBE_REFRESH_TIME 200  // 1 seconds is 1000

static uint8_t origEndstopsState = DISABLED;
static float origAblState = DISABLED;

// enable probe height.
// Temporary disable software endstops and save ABL state
void probeHeightEnable(void)
{
  origEndstopsState = infoMachineSettings.softwareEndstops;
  origAblState = getParameter(P_ABL_STATE, 0);

  if (origEndstopsState == ENABLED)  // if software endstops is enabled, disable it temporary
  {
    if (infoMachineSettings.firmwareType != FW_REPRAPFW)
      mustStoreCmd(ENDSTOP_CMD, 0);  // disable software endstops to move nozzle lower than Z0 if necessary
    else
      mustStoreCmd(ENDSTOP_CMD_RRF, 0, 0);
  }
}

// disable probe height.
// Restore original software endstops state and ABL state
void probeHeightDisable(void)
{
  if (origEndstopsState == ENABLED)  // if software endstops was originally enabled, enable it again
  {
    if (infoMachineSettings.firmwareType != FW_REPRAPFW)
      mustStoreCmd(ENDSTOP_CMD, 1);  // enable software endstops
    else
      mustStoreCmd(ENDSTOP_CMD_RRF, 1, 1);
  }

  if (origAblState == ENABLED)  // if ABL was originally enabled, enable it again
    mustStoreCmd(infoMachineSettings.firmwareType != FW_REPRAPFW ? "M420 S1\n" : "G29 S1\n");  // enable ABL
}

// home, disable ABL and raise nozzle
void probeHeightHome(void)
{
  mustStoreCmd("G28\n");  // home printer
  mustStoreCmd(infoMachineSettings.firmwareType != FW_REPRAPFW ? "M420 S0\n" : "G29 S2\n");  // disable ABL
  probeHeightStop(infoSettings.probing_z_raise);  // raise nozzle
}

// home and disable ABL
void probeHeightHomeAndNoABL(void)
{
  mustStoreCmd("G28\n");  // home printer
  mustStoreCmd(infoMachineSettings.firmwareType != FW_REPRAPFW ? "M420 S0\n" : "G29 S2\n");  // disable ABL
}

// home and raise nozzle
void probeHeightHomeAndRaise(void)
{
  mustStoreCmd("G28\n");  // home printer
  probeHeightStop(infoSettings.probing_z_raise);  // raise nozzle
}

// start probe height
void probeHeightStart(float initialHeight, bool relativeHeight)
{
  if (relativeHeight)
    probeHeightRelative();                                // set relative position mode
  else
    probeHeightAbsolute();                                // set absolute position mode

  mustStoreCmd(MOVE_Z_CMD,
               initialHeight,
               infoSettings.level_feedrate[FEEDRATE_Z]);  // move nozzle to provided absolute Z point and set feedrate
  probeHeightRelative();                                  // set relative position mode
}

// stop probe height
void probeHeightStop(float raisedHeight)
{
  probeHeightRelative();                                  // set relative position mode
  mustStoreCmd(MOVE_Z_CMD,
               raisedHeight,
               infoSettings.level_feedrate[FEEDRATE_Z]);  // raise Z and set feedrate
  probeHeightAbsolute();                                  // set absolute position mode
}

// set probe height to relative position mode
void probeHeightRelative(void)
{
  mustStoreCmd("G91\n");  // set relative position mode
}

// set probe height to absolute position mode
void probeHeightAbsolute(void)
{
  mustStoreCmd("G90\n");  // set absolute position mode
}

// change probe height
void probeHeightMove(float unit)
{
  storeCmd(MOVE_Z_CMD, unit, infoSettings.level_feedrate[FEEDRATE_Z]);
}

// query for new coordinates
void probeHeightQueryCoord(void)
{
  static uint32_t nextUpdateTime = 0;

  if (OS_GetTimeMs() < nextUpdateTime)
    return;

  nextUpdateTime = OS_GetTimeMs() + PROBE_REFRESH_TIME;

  coordinateQuery(0);  // query position manually for delay less than 1 second
}