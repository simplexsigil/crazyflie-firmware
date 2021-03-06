
#include "stabilizer.h"
#include "stabilizer_types.h"

#include "attitude_controller.h"
#include "sensfusion6.h"
#include "position_controller.h"
#include "controller_pid.h"

#include "log.h"
#include "param.h"

#define ATTITUDE_UPDATE_DT    (float)(1.0f/ATTITUDE_RATE)

static bool tiltCompensationEnabled = false;

static attitude_t attitudeDesired;
static attitude_t rateDesired;
static float actuatorThrust;

static float actuatorThrustBeforePositionCorrection;
attitude_t attitudeDesiredBeforePositionCorrection;
setpoint_t setpointBeforePostitionCorrection;
state_t stateBeforePositionCorrection;

static float actuatorThrustAfterPositionCorrection;
attitude_t attitudeDesiredAfterPositionCorrection;
setpoint_t setpointAfterPostitionCorrection;
state_t stateAfterPositionCorrection;

void controllerPidInit(void)
{
  attitudeControllerInit(ATTITUDE_UPDATE_DT);
  positionControllerInit();
}

bool controllerPidTest(void)
{
  bool pass = true;

  pass &= attitudeControllerTest();

  return pass;
}

void controllerPid(control_t *control, setpoint_t *setpoint,
                                         const sensorData_t *sensors,
                                         const state_t *state,
                                         const uint32_t tick)
{

  // The if will only execute this every second iteration.
  if (RATE_DO_EXECUTE(ATTITUDE_RATE, tick)) {
    // Rate-controled YAW is moving YAW angle setpoint
    if (setpoint->mode.yaw == modeVelocity) {
       attitudeDesired.yaw += setpoint->attitudeRate.yaw * ATTITUDE_UPDATE_DT;
      while (attitudeDesired.yaw > 180.0f)
        attitudeDesired.yaw -= 360.0f;
      while (attitudeDesired.yaw < -180.0f)
        attitudeDesired.yaw += 360.0f;
    } else {
      attitudeDesired.yaw = setpoint->attitude.yaw;
    }
  }

  // The if will only execute this every 10th iteration.
  if (RATE_DO_EXECUTE(POSITION_RATE, tick)) {
	// TODO: Write out data before position correction.
	// Problem: What has happened since the setpoint was given as command so far? can we take the setpoint just like that?

	actuatorThrustBeforePositionCorrection = actuatorThrust;
	attitudeDesiredBeforePositionCorrection = attitudeDesired;
	setpointBeforePostitionCorrection = (*setpoint);
	stateBeforePositionCorrection = (*state);

    positionController(&actuatorThrust, &attitudeDesired, setpoint, state);

    actuatorThrustAfterPositionCorrection = actuatorThrust;
    attitudeDesiredAfterPositionCorrection = attitudeDesired;
    setpointAfterPostitionCorrection = (*setpoint);
    stateAfterPositionCorrection = (*state);

    // TODO: Write out data after position correction.
  }

  // The if will only execute this every second iteration.
  if (RATE_DO_EXECUTE(ATTITUDE_RATE, tick)) {
    // Switch between manual and automatic position control
    if (setpoint->mode.z == modeDisable) {
      actuatorThrust = setpoint->thrust;
    }
    if (setpoint->mode.x == modeDisable || setpoint->mode.y == modeDisable) {
      attitudeDesired.roll = setpoint->attitude.roll;
      attitudeDesired.pitch = setpoint->attitude.pitch;
    }

    attitudeControllerCorrectAttitudePID(state->attitude.roll, state->attitude.pitch, state->attitude.yaw,
                                attitudeDesired.roll, attitudeDesired.pitch, attitudeDesired.yaw,
                                &rateDesired.roll, &rateDesired.pitch, &rateDesired.yaw);

    // For roll and pitch, if velocity mode, overwrite rateDesired with the setpoint
    // value. Also reset the PID to avoid error buildup, which can lead to unstable
    // behavior if level mode is engaged later
    if (setpoint->mode.roll == modeVelocity) {
      rateDesired.roll = setpoint->attitudeRate.roll;
      attitudeControllerResetRollAttitudePID();
    }
    if (setpoint->mode.pitch == modeVelocity) {
      rateDesired.pitch = setpoint->attitudeRate.pitch;
      attitudeControllerResetPitchAttitudePID();
    }

    // TODO: Investigate possibility to subtract gyro drift.
    attitudeControllerCorrectRatePID(sensors->gyro.x, -sensors->gyro.y, sensors->gyro.z,
                             rateDesired.roll, rateDesired.pitch, rateDesired.yaw);

    attitudeControllerGetActuatorOutput(&control->roll,
                                        &control->pitch,
                                        &control->yaw);

    control->yaw = -control->yaw;
  }

  if (tiltCompensationEnabled)
  {
    control->thrust = actuatorThrust / sensfusion6GetInvThrustCompensationForTilt();
  }
  else
  {
    control->thrust = actuatorThrust;
  }

  if (control->thrust == 0)
  {
    control->thrust = 0;
    control->roll = 0;
    control->pitch = 0;
    control->yaw = 0;

    attitudeControllerResetAllPID();
    positionControllerResetAllPID();

    // Reset the calculated YAW angle for rate control
    attitudeDesired.yaw = state->attitude.yaw;
  }
}

/*

*/

LOG_GROUP_START(controller)
LOG_ADD(LOG_FLOAT, actuatorThrust, &actuatorThrust)
LOG_ADD(LOG_FLOAT, roll,      &attitudeDesired.roll)
LOG_ADD(LOG_FLOAT, pitch,     &attitudeDesired.pitch)
LOG_ADD(LOG_FLOAT, yaw,       &attitudeDesired.yaw)
LOG_ADD(LOG_FLOAT, rollRate,  &rateDesired.roll)
LOG_ADD(LOG_FLOAT, pitchRate, &rateDesired.pitch)
LOG_ADD(LOG_FLOAT, yawRate,   &rateDesired.yaw)
LOG_GROUP_STOP(controller)

/*
LOG_GROUP_START(setpbposc)
LOG_ADD(LOG_FLOAT, actuatorThrust, &actuatorThrustBeforePositionCorrection)
LOG_ADD(LOG_FLOAT, roll,      &attitudeDesiredBeforePositionCorrection.roll)
LOG_ADD(LOG_FLOAT, pitch,     &attitudeDesiredBeforePositionCorrection.pitch)
LOG_ADD(LOG_FLOAT, yaw,       &attitudeDesiredBeforePositionCorrection.yaw)
LOG_GROUP_STOP(setpbposc)

LOG_GROUP_START(setpaposc)
LOG_ADD(LOG_FLOAT, actuatorThrust, &actuatorThrustBeforePositionCorrection)
LOG_ADD(LOG_FLOAT, roll,      &attitudeDesiredBeforePositionCorrection.roll)
LOG_ADD(LOG_FLOAT, pitch,     &attitudeDesiredBeforePositionCorrection.pitch)
LOG_ADD(LOG_FLOAT, yaw,       &attitudeDesiredBeforePositionCorrection.yaw)
LOG_GROUP_STOP(setpaposc)
*/

LOG_GROUP_START(attbposc)
LOG_ADD(LOG_FLOAT, actuatorThrust, &actuatorThrustBeforePositionCorrection)
LOG_ADD(LOG_FLOAT, roll,      &attitudeDesiredBeforePositionCorrection.roll)
LOG_ADD(LOG_FLOAT, pitch,     &attitudeDesiredBeforePositionCorrection.pitch)
LOG_ADD(LOG_FLOAT, yaw,       &attitudeDesiredBeforePositionCorrection.yaw)
LOG_GROUP_STOP(attbposc)

LOG_GROUP_START(attaposc)
LOG_ADD(LOG_FLOAT, actuatorThrust, &actuatorThrustAfterPositionCorrection)
LOG_ADD(LOG_FLOAT, roll,      &attitudeDesiredAfterPositionCorrection.roll)
LOG_ADD(LOG_FLOAT, pitch,     &attitudeDesiredAfterPositionCorrection.pitch)
LOG_ADD(LOG_FLOAT, yaw,       &attitudeDesiredAfterPositionCorrection.yaw)
LOG_GROUP_STOP(attaposc)

LOG_GROUP_START(setpointposlog)
LOG_ADD(LOG_FLOAT, x,      &setpointBeforePostitionCorrection.position.x)
LOG_ADD(LOG_FLOAT, y,     &setpointBeforePostitionCorrection.position.y)
LOG_ADD(LOG_FLOAT, z,       &setpointBeforePostitionCorrection.position.z)
LOG_GROUP_STOP(setpointposlog)

LOG_GROUP_START(setpointvellog)
LOG_ADD(LOG_FLOAT, xdot,      &setpointBeforePostitionCorrection.velocity.x)
LOG_ADD(LOG_FLOAT, ydot,     &setpointBeforePostitionCorrection.velocity.y)
LOG_ADD(LOG_FLOAT, zdot,       &setpointBeforePostitionCorrection.velocity.z)
LOG_GROUP_STOP(setpointvellog)

LOG_GROUP_START(setpointacclog)
LOG_ADD(LOG_FLOAT, x2dot,      &setpointBeforePostitionCorrection.acceleration.x)
LOG_ADD(LOG_FLOAT, y2dot,     &setpointBeforePostitionCorrection.acceleration.y)
LOG_ADD(LOG_FLOAT, z2dot,       &setpointBeforePostitionCorrection.acceleration.z)
LOG_GROUP_STOP(setpointacclog)



PARAM_GROUP_START(controller)
PARAM_ADD(PARAM_UINT8, tiltComp, &tiltCompensationEnabled)
PARAM_GROUP_STOP(controller)
