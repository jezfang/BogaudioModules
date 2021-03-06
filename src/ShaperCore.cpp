
#include <math.h>
#include "ShaperCore.hpp"

void ShaperCore::reset() {
	_trigger.reset();
	_stage = STOPPED_STAGE;
	_stageProgress = 0.0;
}

void ShaperCore::step() {
	bool complete = false;
	bool slow = _speedParam.value <= 0.0;
	if (_trigger.process(_triggerParam.value + _triggerInput.value)) {
		_stage = ATTACK_STAGE;
		_stageProgress = 0.0;
	}
	else {
		switch (_stage) {
			case STOPPED_STAGE: {
				break;
			}
			case ATTACK_STAGE: {
				if (stepStage(_attackParam, _attackInput, slow)) {
					_stage = ON_STAGE;
					_stageProgress = 0.0;
				}
				break;
			}
			case ON_STAGE: {
				if (stepStage(_onParam, _onInput, slow)) {
					_stage = DECAY_STAGE;
					_stageProgress = 0.0;
				}
				break;
			}
			case DECAY_STAGE: {
				if (stepStage(_decayParam, _decayInput, slow)) {
					_stage = OFF_STAGE;
					_stageProgress = 0.0;
				}
				break;
			}
			case OFF_STAGE: {
				if (stepStage(_offParam, _offInput, slow)) {
					complete = true;
					if (_loopParam.value <= 0.0) {
						_stage = ATTACK_STAGE;
						_stageProgress = 0.0;
					}
					else {
						_stage = STOPPED_STAGE;
					}
				}
				break;
			}
		}
	}

	float envelope = 0.0;
	switch (_stage) {
		case STOPPED_STAGE: {
			break;
		}
		case ATTACK_STAGE: {
			envelope = _stageProgress * 10.0;
			break;
		}
		case ON_STAGE: {
			envelope = 10.0;
			break;
		}
		case DECAY_STAGE: {
			envelope = (1.0 - _stageProgress) * 10.0;
			break;
		}
		case OFF_STAGE: {
			break;
		}
	}

	float signalLevel = levelParam(_signalParam, _signalCVInput);
	_signalOutput.value = signalLevel * envelope * _signalInput.normalize(0.0);

	float envLevel = levelParam(_envParam, _envInput);
	float envOutput = clampf(envLevel * envelope, 0.0, 10.0);
	_envOutput.value = envOutput;
	_invOutput.value = 10.0 - envOutput;
	_triggerOutput.value = complete ? 5.0 : 0.0;

  if (_attackOutput) {
    _attackOutput->value = _stage == ATTACK_STAGE ? 5.0 : 0.0;
	}
  if (_onOutput) {
    _onOutput->value = _stage == ON_STAGE ? 5.0 : 0.0;
	}
  if (_decayOutput) {
    _decayOutput->value = _stage == DECAY_STAGE ? 5.0 : 0.0;
	}
  if (_offOutput) {
	  _offOutput->value = _stage == OFF_STAGE ? 5.0 : 0.0;
	}

	_attackLight.value = _stage == ATTACK_STAGE;
	_onLight.value = _stage == ON_STAGE;
	_decayLight.value = _stage == DECAY_STAGE;
	_offLight.value = _stage == OFF_STAGE;
}

bool ShaperCore::stepStage(const Param& knob, const Input* cv, bool slow) {
	float t = levelParam(knob, cv);
	t = pow(t, 2);
	t = fmaxf(t, 0.001);
	t *= slow ? 100.0 : 10.0;
  _stageProgress += engineGetSampleTime() / t;
	return _stageProgress > 1.0;
}

float ShaperCore::levelParam(const Param& knob, const Input* cv) const {
  float v = clampf(knob.value, 0.0, 1.0);
  if (cv && cv->active) {
    v *= clampf(cv->value / 10.0, 0.0, 1.0);
	}
  return v;
}
