#pragma once

#include "opendbc/safety/safety_declarations.h"
#include "opendbc/safety/modes/hyundai_common.h"

#define HYUNDAI_LIMITS(steer, rate_up, rate_down) { \
  .max_torque = (steer), \
  .max_rate_up = (rate_up), \
  .max_rate_down = (rate_down), \
  .max_rt_delta = 150, \
  .driver_torque_allowance = 60, \
  .driver_torque_multiplier = 2, \
  .type = TorqueDriverLimited, \
   /* the EPS faults when the steering angle is above a certain threshold for too long. to prevent this, */ \
   /* we allow setting CF_Lkas_ActToi bit to 0 while maintaining the requested torque value for two consecutive frames */ \
  .min_valid_request_frames = 89, \
  .max_invalid_request_frames = 2, \
  .min_valid_request_rt_interval = 810000,  /* 810ms; a ~10% buffer on cutting every 90 frames */ \
  .has_steer_req_tolerance = true, \
}

extern const LongitudinalLimits HYUNDAI_LONG_LIMITS;
const LongitudinalLimits HYUNDAI_LONG_LIMITS = {
  .max_accel = 200,   // 1/100 m/s2
  .min_accel = -370,  // 1/100 m/s2
};

#define HYUNDAI_COMMON_RX_CHECKS(legacy)                                                                                                                  \
  {.msg = {{0x260, 0, 8, .max_counter = 3U, .ignore_quality_flag = true, .frequency = 100U},                                                                                           \
           {0x371, 0, 8, .ignore_checksum = true, .ignore_counter = true, .ignore_quality_flag = true, .frequency = 100U},                                                             \
           {0x91,  0, 8, .ignore_checksum = true, .ignore_counter = true, .ignore_quality_flag = true, .frequency = 100U}}},                                                           \
  {.msg = {{0x386, 0, 8, .ignore_checksum = (legacy), .ignore_counter = (legacy), .max_counter = (legacy) ? 0U : 15U, .ignore_quality_flag = true, .frequency = 100U}, { 0 }, { 0 }}}, \
  {.msg = {{0x394, 0, 8, .ignore_checksum = (legacy), .ignore_counter = (legacy), .max_counter = (legacy) ? 0U : 7U, .ignore_quality_flag = true, .frequency = 100U}, { 0 }, { 0 }}},  \

#define HYUNDAI_SCC12_ADDR_CHECK(scc_bus)                                                                            \
  {.msg = {{0x421, (scc_bus), 8, .max_counter = 15U, .ignore_quality_flag = true, .frequency = 50U}, { 0 }, { 0 }}}, \

#define HYUNDAI_FCEV_GAS_ADDR_CHECK \
  {.msg = {{0x91,  0, 8, .ignore_checksum = true, .ignore_counter = true, .ignore_quality_flag = true, .frequency = 100U}, { 0 }, { 0 }}}, \

const CanMsg HYUNDAI_TX_MSGS[] = {
  {593, 2, 8, .check_relay = false},                              // MDPS12, Bus 2
  {832, 0, 8, .check_relay = true, .disable_static_blocking = true},                              // LKAS11, Bus 0
  {1056, 0, 8, .check_relay = false},                             // SCC11, Bus 0
  {1057, 0, 8, .check_relay = false},                             // SCC12, Bus 0
  {1290, 0, 8, .check_relay = false},                             // SCC13, Bus 0
  {905, 0, 8, .check_relay = false},                              // SCC14, Bus 0
  {909, 0, 8, .check_relay = false},                              // FCA11 Bus 0
  {1155, 0, 8, .check_relay = false},                             // FCA12 Bus 0
  {1157, 0, 4, .check_relay = false},                             // LFAHDA_MFC, Bus 0
  {1186, 0, 8, .check_relay = false},                             // FRT_RADAR11, Bus 0
  {1265, 0, 4, .check_relay = false}, {1265, 2, 4, .check_relay = false},               // CLU11, Bus 0, 2
};

static bool hyundai_legacy = false;

static uint8_t hyundai_get_counter(const CANPacket_t *to_push) {
  int addr = GET_ADDR(to_push);

  uint8_t cnt = 0;
  if (addr == 0x260) {
    cnt = (GET_BYTE(to_push, 7) >> 4) & 0x3U;
  } else if (addr == 0x386) {
    cnt = ((GET_BYTE(to_push, 3) >> 6) << 2) | (GET_BYTE(to_push, 1) >> 6);
  } else if (addr == 0x394) {
    cnt = (GET_BYTE(to_push, 1) >> 5) & 0x7U;
  } else if (addr == 0x421) {
    cnt = GET_BYTE(to_push, 7) & 0xFU;
  } else if (addr == 0x4F1) {
    cnt = (GET_BYTE(to_push, 3) >> 4) & 0xFU;
  } else {
  }
  return cnt;
}

static uint32_t hyundai_get_checksum(const CANPacket_t *to_push) {
  int addr = GET_ADDR(to_push);

  uint8_t chksum = 0;
  if (addr == 0x260) {
    chksum = GET_BYTE(to_push, 7) & 0xFU;
  } else if (addr == 0x386) {
    chksum = ((GET_BYTE(to_push, 7) >> 6) << 2) | (GET_BYTE(to_push, 5) >> 6);
  } else if (addr == 0x394) {
    chksum = GET_BYTE(to_push, 6) & 0xFU;
  } else if (addr == 0x421) {
    chksum = GET_BYTE(to_push, 7) >> 4;
  } else {
  }
  return chksum;
}

static uint32_t hyundai_compute_checksum(const CANPacket_t *to_push) {
  int addr = GET_ADDR(to_push);

  uint8_t chksum = 0;
  if (addr == 0x386) {
    // count the bits
    for (int i = 0; i < 8; i++) {
      uint8_t b = GET_BYTE(to_push, i);
      for (int j = 0; j < 8; j++) {
        uint8_t bit = 0;
        // exclude checksum and counter
        if (((i != 1) || (j < 6)) && ((i != 3) || (j < 6)) && ((i != 5) || (j < 6)) && ((i != 7) || (j < 6))) {
          bit = (b >> (uint8_t)j) & 1U;
        }
        chksum += bit;
      }
    }
    chksum = (chksum ^ 9U) & 15U;
  } else {
    // sum of nibbles
    for (int i = 0; i < 8; i++) {
      if ((addr == 0x394) && (i == 7)) {
        continue; // exclude
      }
      uint8_t b = GET_BYTE(to_push, i);
      if (((addr == 0x260) && (i == 7)) || ((addr == 0x394) && (i == 6)) || ((addr == 0x421) && (i == 7))) {
        b &= (addr == 0x421) ? 0x0FU : 0xF0U; // remove checksum
      }
      chksum += (b % 16U) + (b / 16U);
    }
    chksum = (16U - (chksum %  16U)) % 16U;
  }

  return chksum;
}

static void hyundai_rx_hook(const CANPacket_t *to_push) {
  int bus = GET_BUS(to_push);
  int addr = GET_ADDR(to_push);

  // SCC12 is on bus 2 for camera-based SCC cars, bus 0 on all others
  /*if ((addr == 0x421) && (((bus == 0) && !hyundai_camera_scc) || ((bus == 2) && hyundai_camera_scc))) {
    // 2 bits: 13-14
    int cruise_engaged = (GET_BYTES(to_push, 0, 4) >> 13) & 0x3U;
    hyundai_common_cruise_state_check(cruise_engaged);
  }*/

  if (addr == 0x420) { //  MainMode_ACC
    // 1 bits: 0
    int cruise_available = GET_BIT(to_push, 0U);
    hyundai_common_cruise_state_check(cruise_available);
  }

  if (bus == 0) {
    if (addr == 0x251) {
      int torque_driver_new = ((int)(GET_BYTES(to_push, 0, 4) & 0x7ffU) - 982) * 0.4;
      update_sample(&torque_driver, torque_driver_new);
    }

    // ACC steering wheel buttons
    if (addr == 0x4F1) {
      int cruise_button = GET_BYTE(to_push, 0) & 0x7U;
      bool main_button = GET_BIT(to_push, 3U);
      hyundai_common_cruise_buttons_check(cruise_button, main_button);
    }

    // gas press, different for EV, hybrid, and ICE models
    if ((addr == 0x371) && hyundai_ev_gas_signal) {
      gas_pressed = (((GET_BYTE(to_push, 4) & 0x7FU) << 1) | GET_BYTE(to_push, 3) >> 7) != 0U;
    } else if ((addr == 0x371) && hyundai_hybrid_gas_signal) {
      gas_pressed = GET_BYTE(to_push, 7) != 0U;
    } else if ((addr == 0x91) && hyundai_fcev_gas_signal) {
      gas_pressed = GET_BYTE(to_push, 6) != 0U;
    } else if ((addr == 0x260) && !hyundai_ev_gas_signal && !hyundai_hybrid_gas_signal) {
      gas_pressed = (GET_BYTE(to_push, 7) >> 6) != 0U;
    } else {
    }

    // sample wheel speed, averaging opposite corners
    if (addr == 0x386) {
      uint32_t front_left_speed = GET_BYTES(to_push, 0, 2) & 0x3FFFU;
      uint32_t rear_right_speed = GET_BYTES(to_push, 6, 2) & 0x3FFFU;
      vehicle_moving = (front_left_speed > HYUNDAI_STANDSTILL_THRSLD) || (rear_right_speed > HYUNDAI_STANDSTILL_THRSLD);
    }

    if (addr == 0x394) {
      brake_pressed = ((GET_BYTE(to_push, 5) >> 5U) & 0x3U) == 0x2U;
    }
  }
}

uint32_t last_ts_lkas11_from_op = 0;
uint32_t last_ts_scc12_from_op = 0;
uint32_t last_ts_mdps12_from_op = 0;
uint32_t last_ts_fca11_from_op = 0;

static bool hyundai_tx_hook(const CANPacket_t *to_send) {
  const TorqueSteeringLimits HYUNDAI_STEERING_LIMITS = HYUNDAI_LIMITS(384, 3, 7);
  const TorqueSteeringLimits HYUNDAI_STEERING_LIMITS_ALT = HYUNDAI_LIMITS(270, 2, 3);
  const TorqueSteeringLimits HYUNDAI_STEERING_LIMITS_ALT_2 = HYUNDAI_LIMITS(270, 2, 3);

  bool tx = true;
  int addr = GET_ADDR(to_send);

  // FCA11: Block any potential actuation
  if (addr == 0x38D) {
    int CR_VSM_DecCmd = GET_BYTE(to_send, 1);
    bool FCA_CmdAct = GET_BIT(to_send, 20U);
    bool CF_VSM_DecCmdAct = GET_BIT(to_send, 31U);

    if ((CR_VSM_DecCmd != 0) || FCA_CmdAct || CF_VSM_DecCmdAct) {
      tx = false;
    }
  }

  // ACCEL: safety check
  if (addr == 0x421) {
    int desired_accel_raw = (((GET_BYTE(to_send, 4) & 0x7U) << 8) | GET_BYTE(to_send, 3)) - 1023U;
    int desired_accel_val = ((GET_BYTE(to_send, 5) << 3) | (GET_BYTE(to_send, 4) >> 5)) - 1023U;

    bool violation = false;

    violation |= longitudinal_accel_checks(desired_accel_raw, HYUNDAI_LONG_LIMITS);
    violation |= longitudinal_accel_checks(desired_accel_val, HYUNDAI_LONG_LIMITS);

    if (violation) {
      tx = false;
    }
  }

  // LKA STEER: safety check
  if (addr == 0x340) {
    int desired_torque = ((GET_BYTES(to_send, 0, 4) >> 16) & 0x7ffU) - 1024U;
    bool steer_req = GET_BIT(to_send, 27U);

    const TorqueSteeringLimits limits = hyundai_alt_limits_2 ? HYUNDAI_STEERING_LIMITS_ALT_2 :
                                        hyundai_alt_limits ? HYUNDAI_STEERING_LIMITS_ALT : HYUNDAI_STEERING_LIMITS;

    if (steer_torque_cmd_checks(desired_torque, steer_req, limits)) {
      tx = false;
    }
  }

  // UDS: Only tester present ("\x02\x3E\x80\x00\x00\x00\x00\x00") allowed on diagnostics address
  if (addr == 0x7D0) {
    if ((GET_BYTES(to_send, 0, 4) != 0x00803E02U) || (GET_BYTES(to_send, 4, 4) != 0x0U)) {
      tx = false;
    }
  }

  // BUTTONS: used for resume spamming and cruise cancellation
  /*if ((addr == 0x4F1) && !hyundai_longitudinal) {
    int button = GET_BYTE(to_send, 0) & 0x7U;

    bool allowed_resume = (button == 1) && controls_allowed;
    bool allowed_cancel = (button == 4) && cruise_engaged_prev;
    if (!(allowed_resume || allowed_cancel)) {
      tx = false;
    }
  }*/

  if(addr == 832)
    last_ts_lkas11_from_op = (tx == 0 ? 0 : microsecond_timer_get());
  else if(addr == 1057)
    last_ts_scc12_from_op = (tx == 0 ? 0 : microsecond_timer_get());
  else if(addr == 593)
    last_ts_mdps12_from_op = (tx == 0 ? 0 : microsecond_timer_get());
  else if(addr == 909)
    last_ts_fca11_from_op = (tx == 0 ? 0 : microsecond_timer_get());

  return tx;
}

static bool hyundai_fwd_hook(int bus_num, int addr) {
  bool block_msg = false;

  uint32_t now = microsecond_timer_get();

  if (bus_num == 0) {
    if(addr == 593) {
      if(now - last_ts_mdps12_from_op < 200000) {
        block_msg = true;
      }
    }
  }

  if (bus_num == 2) {
    bool is_lkas_msg = addr == 832;
    bool is_lfahda_msg = addr == 1157;
    bool is_scc_msg = addr == 1056 || addr == 1057 || addr == 1290 || addr == 905;
    bool is_fca_msg = addr == 909 || addr == 1155;

    block_msg = is_lkas_msg || is_lfahda_msg || is_scc_msg || is_fca_msg;
    if(block_msg) {
      if(is_lkas_msg || is_lfahda_msg) {
        if(now - last_ts_lkas11_from_op >= 200000) {
          block_msg = false;
        }
      }
      else if(is_scc_msg) {
        if(now - last_ts_scc12_from_op >= 400000)
          block_msg = false;
      }
      else if(is_fca_msg) {
        if(now - last_ts_fca11_from_op >= 400000)
          block_msg = false;
      }
    }
  }

  return block_msg;
}

static safety_config hyundai_init(uint16_t param) {
  static const CanMsg HYUNDAI_LONG_TX_MSGS[] = {
	  {593, 2, 8, .check_relay = false},  // MDPS12, Bus 2
	  {832, 0, 8, .check_relay = true, .disable_static_blocking = true},  // LKAS11 Bus 0
	  {1265, 0, 4, .check_relay = false}, {1265, 2, 4, .check_relay = false},               // CLU11, Bus 0, 2
	  {1157, 0, 4, .check_relay = false}, // LFAHDA_MFC Bus 0
	  {1056, 0, 8, .check_relay = false}, // SCC11 Bus 0
	  {1057, 0, 8, .check_relay = false}, // SCC12 Bus 0
	  {1290, 0, 8, .check_relay = false}, // SCC13 Bus 0
	  {905, 0, 8, .check_relay = false},  // SCC14 Bus 0
	  {1186, 0, 2, .check_relay = false}, // FRT_RADAR11 Bus 0
	  {909, 0, 8, .check_relay = false},  // FCA11 Bus 0
	  {1155, 0, 8, .check_relay = false}, // FCA12 Bus 0
	  {2000, 0, 8, .check_relay = false}, // radar UDS TX addr Bus 0 (for radar disable)
	};

	static const CanMsg HYUNDAI_CAMERA_SCC_TX_MSGS[] = {
	  {593, 2, 8, .check_relay = false},                              // MDPS12, Bus 2
	  {832, 0, 8, .check_relay = true},                              // LKAS11, Bus 0
	  {1056, 0, 8, .check_relay = false},                             // SCC11, Bus 0
	  {1057, 0, 8, .check_relay = false},                             // SCC12, Bus 0
	  {1290, 0, 8, .check_relay = false},                             // SCC13, Bus 0
	  {905, 0, 8, .check_relay = false},                              // SCC14, Bus 0
	  {909, 0, 8, .check_relay = false},                              // FCA11 Bus 0
	  {1155, 0, 8, .check_relay = false},                             // FCA12 Bus 0
	  {1157, 0, 4, .check_relay = false},                             // LFAHDA_MFC, Bus 0
	  {1186, 0, 8, .check_relay = false},                             // FRT_RADAR11, Bus 0
	  {1265, 0, 4, .check_relay = false}, {1265, 2, 4, .check_relay = false},               // CLU11, Bus 0, 2
	 };

  hyundai_common_init(param);
  hyundai_legacy = false;

  safety_config ret;
  if (hyundai_longitudinal) {
    static RxCheck hyundai_long_rx_checks[] = {
      HYUNDAI_COMMON_RX_CHECKS(false)
      // Use CLU11 (buttons) to manage controls allowed instead of SCC cruise state
      {.msg = {{0x4F1, 0, 4, .ignore_checksum = true, .max_counter = 15U, .frequency = 50U}, { 0 }, { 0 }}},
    };

    ret = BUILD_SAFETY_CFG(hyundai_long_rx_checks, HYUNDAI_LONG_TX_MSGS);
  } else if (hyundai_camera_scc) {
    static RxCheck hyundai_cam_scc_rx_checks[] = {
      HYUNDAI_COMMON_RX_CHECKS(false)
      HYUNDAI_SCC12_ADDR_CHECK(2)
    };

    ret = BUILD_SAFETY_CFG(hyundai_cam_scc_rx_checks, HYUNDAI_CAMERA_SCC_TX_MSGS);
  } else {
    static RxCheck hyundai_rx_checks[] = {
       HYUNDAI_COMMON_RX_CHECKS(false)
       HYUNDAI_SCC12_ADDR_CHECK(0)
    };

    ret = BUILD_SAFETY_CFG(hyundai_rx_checks, HYUNDAI_TX_MSGS);
  }
  return ret;
}

static safety_config hyundai_legacy_init(uint16_t param) {
  // older hyundai models have less checks due to missing counters and checksums
  static RxCheck hyundai_legacy_rx_checks[] = {
    HYUNDAI_COMMON_RX_CHECKS(true)
    //HYUNDAI_SCC12_ADDR_CHECK(0)
  };

  hyundai_common_init(param);
  hyundai_legacy = true;
  hyundai_longitudinal = false;
  hyundai_camera_scc = false;
  return BUILD_SAFETY_CFG(hyundai_legacy_rx_checks, HYUNDAI_TX_MSGS);
}

const safety_hooks hyundai_hooks = {
  .init = hyundai_init,
  .rx = hyundai_rx_hook,
  .tx = hyundai_tx_hook,
  .fwd = hyundai_fwd_hook,
  .get_counter = hyundai_get_counter,
  .get_checksum = hyundai_get_checksum,
  .compute_checksum = hyundai_compute_checksum,
};

const safety_hooks hyundai_legacy_hooks = {
  .init = hyundai_legacy_init,
  .rx = hyundai_rx_hook,
  .tx = hyundai_tx_hook,
  .fwd = hyundai_fwd_hook,
  .get_counter = hyundai_get_counter,
  .get_checksum = hyundai_get_checksum,
  .compute_checksum = hyundai_compute_checksum,
};
