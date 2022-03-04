/* Copyright (c) 2020 XSPACE. All Rights Reserved.
 *
 * The information contained herein is property of XSPACE.
 * Terms and conditions of usage are described in detail in XSPACE
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */
#if defined (__USE_SW_I2C__)
#include "hal_gpio.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "sw_i2c_master.h"

static bool sw_i2c_master_clear_bus(void);
static bool sw_i2c_master_issue_startcondition(void);
static bool sw_i2c_master_issue_stopcondition(void);
static bool sw_i2c_master_clock_byte(uint_fast8_t databyte);
static bool sw_i2c_master_clock_byte_in(uint8_t *databyte, bool ack);
static bool sw_i2c_master_wait_while_scl_low(void);

bool sw_i2c_master_init(void)
{
    TRACE(1, "%s.", __func__);

    // Configure SCL as output
    I2C_SW_SCL_OUTPUT();
    I2C_SW_SCL_HIGH();

    // Configure SDA as output
    I2C_SW_SDA_OUTPUT();
    I2C_SW_SDA_HIGH();

    return sw_i2c_master_clear_bus();
}

bool sw_i2c_master_transfer(uint8_t address, uint8_t *data, uint16_t data_length, bool issue_stop_condition)
{
    bool transfer_succeeded = true;

    transfer_succeeded &= sw_i2c_master_issue_startcondition();
#if 0
    TRACE("\n\r transfer_succeeded = %d", transfer_succeeded);
#endif
    transfer_succeeded &= sw_i2c_master_clock_byte(address);

    I2C_SW_DELAYN_SHORT();

    if (address & I2C_READ_BIT)
    {
        /* Transfer direction is from Slave to Master */
        while (data_length-- && transfer_succeeded)
        {
            // To indicate to slave that we've finished transferring last data byte
            // we need to NACK the last transfer.
            if (data_length == 0)
            {
                transfer_succeeded &= sw_i2c_master_clock_byte_in(data, (bool)false);
            }
            else
            {
                transfer_succeeded &= sw_i2c_master_clock_byte_in(data, (bool)true);
            }

            data++;
        }
    }
    else
    {
        /* Transfer direction is from Master to Slave */
        while (data_length-- && transfer_succeeded)
        {
            transfer_succeeded &= sw_i2c_master_clock_byte(*data);
            data++;
        }
    }

    if (issue_stop_condition || !transfer_succeeded)
    {
        transfer_succeeded &= sw_i2c_master_issue_stopcondition();
    }

    return transfer_succeeded;
}

/**
 * Detects stuck slaves and tries to clear the bus.
 *
 * @return
 * @retval false Bus is stuck.
 * @retval true Bus is clear.
 */
static bool sw_i2c_master_clear_bus(void)
{
    bool bus_clear;

    I2C_SW_SDA_HIGH();
    I2C_SW_SCL_HIGH();
    I2C_SW_DELAYN();

    if (I2C_SW_SDA_READ() == 1 && I2C_SW_SCL_READ() == 1)
    {
        bus_clear = true;
    }
    else if (I2C_SW_SCL_READ() == 1)
    {
        bus_clear = false;

        // Clock max 18 pulses worst case scenario(9 for master to send the rest of command and 9 for slave to respond) to SCL line and wait for SDA come high
        for (uint_fast8_t i = 18; i--;)
        {
            I2C_SW_SCL_LOW();
            I2C_SW_DELAYN();
            I2C_SW_SCL_HIGH();
            I2C_SW_DELAYN();

            if (I2C_SW_SDA_READ() == 1)
            {
                bus_clear = true;
                break;
            }
        }
    }
    else
    {
        bus_clear = false;
    }

    return bus_clear;
}

/**
 * Issues TWI START condition to the bus.
 *
 * START condition is signaled by pulling SDA low while SCL is high. After this function SCL and SDA will be low.
 *
 * @return
 * @retval false Timeout detected
 * @retval true Clocking succeeded
 */
static bool sw_i2c_master_issue_startcondition(void)
{
    // Make sure both SDA and SCL are high before pulling SDA low.
    I2C_SW_SCL_HIGH();
    I2C_SW_SDA_HIGH();
    I2C_SW_DELAYN();

    if (!sw_i2c_master_wait_while_scl_low())
    {
        return false;
    }

    I2C_SW_SDA_LOW();
    I2C_SW_DELAYN();

    // Other module function expect SCL to be low
    I2C_SW_SCL_LOW();
    I2C_SW_DELAYN();

    return true;
}

/**
 * Issues TWI STOP condition to the bus.
 *
 * STOP condition is signaled by pulling SDA high while SCL is high. After this function SDA and SCL will be high.
 *
 * @return
 * @retval false Timeout detected
 * @retval true Clocking succeeded
 */
static bool sw_i2c_master_issue_stopcondition(void)
{
    I2C_SW_SDA_LOW();
    I2C_SW_DELAYN();

    if (!sw_i2c_master_wait_while_scl_low())
    {
        return false;
    }

    I2C_SW_SDA_HIGH();
    I2C_SW_DELAYN();

    return true;
}

/**
 * Clocks one data byte out and reads slave acknowledgement.
 *
 * Can handle clock stretching.
 * After calling this function SCL is low and SDA low/high depending on the
 * value of LSB of the data byte.
 * SCL is expected to be output and low when entering this function.
 *
 * @param databyte Data byte to clock out.
 * @return
 * @retval true Slave acknowledged byte.
 * @retval false Timeout or slave didn't acknowledge byte.
 */
static bool sw_i2c_master_clock_byte(uint_fast8_t databyte)
{
    bool transfer_succeeded = true;

    // Make sure SDA is an output
    I2C_SW_SDA_OUTPUT();

    // MSB first
    for (uint_fast8_t i = 0x80; i != 0; i >>= 1)
    {
        I2C_SW_SCL_LOW();
        I2C_SW_DELAYN();

        if (databyte & i)
        {
            I2C_SW_SDA_HIGH();
        }
        else
        {
            I2C_SW_SDA_LOW();
        }

        if (!sw_i2c_master_wait_while_scl_low())
        {
            transfer_succeeded = false; // Timeout
            break;
        }
    }

#if 0
    TRACE("\n\r transfer_succeeded = %d", transfer_succeeded);
#else
    I2C_SW_DELAYN();
#endif
    // Finish last data bit by pulling SCL low
    I2C_SW_SCL_LOW();
    I2C_SW_DELAYN();

    // Configure TWI_SDA pin as input for receiving the ACK bit
    I2C_SW_SDA_INPUT();

    // Give some time for the slave to load the ACK bit on the line
    I2C_SW_DELAYN();

    // Pull SCL high and wait a moment for SDA line to settle
    // Make sure slave is not stretching the clock
    transfer_succeeded &= sw_i2c_master_wait_while_scl_low();
#if 0
    TRACE("\n\r transfer_succeeded = %d", transfer_succeeded);
#else
    I2C_SW_DELAYN();
#endif
    // Read ACK/NACK. NACK == 1, ACK == 0
    transfer_succeeded &= !(I2C_SW_SDA_READ());

    // Finish ACK/NACK bit clock cycle and give slave a moment to release control
    // of the SDA line
    I2C_SW_SCL_LOW();
    I2C_SW_DELAYN();

    // Configure TWI_SDA pin as output as other module functions expect that
    I2C_SW_SDA_OUTPUT();

    return transfer_succeeded;
}

/**
 * Clocks one data byte in and sends ACK/NACK bit.
 *
 * Can handle clock stretching.
 * SCL is expected to be output and low when entering this function.
 * After calling this function, SCL is high and SDA low/high depending if ACK/NACK was sent.
 *
 * @param databyte Data byte to clock out.
 * @param ack If true, send ACK. Otherwise send NACK.
 * @return
 * @retval true Byte read succesfully
 * @retval false Timeout detected
 */
static bool sw_i2c_master_clock_byte_in(uint8_t *databyte, bool ack)
{
    uint_fast8_t byte_read = 0;
    bool transfer_succeeded = true;

    // Make sure SDA is an input
    I2C_SW_SDA_INPUT();

    // SCL state is guaranteed to be high here

    // MSB first
    for (uint_fast8_t i = 0x80; i != 0; i >>= 1)
    {
        if (!sw_i2c_master_wait_while_scl_low())
        {
            transfer_succeeded = false;
            break;
        }

        if (I2C_SW_SDA_READ())
        {
            byte_read |= i;
        }
        else
        {
            // No need to do anything
        }

        I2C_SW_SCL_LOW();
        I2C_SW_DELAYN();
    }

    // Make sure SDA is an output before we exit the function
    I2C_SW_SDA_OUTPUT();

    *databyte = (uint8_t)byte_read;

    // Send ACK bit

    // SDA high == NACK, SDA low == ACK
    if (ack)
    {
        I2C_SW_SDA_LOW();
    }
    else
    {
        I2C_SW_SDA_HIGH();
    }

    // Let SDA line settle for a moment
    I2C_SW_DELAYN();

    // Drive SCL high to start ACK/NACK bit transfer
    // Wait until SCL is high, or timeout occurs
    if (!sw_i2c_master_wait_while_scl_low())
    {
        transfer_succeeded = false; // Timeout
    }

    // Finish ACK/NACK bit clock cycle and give slave a moment to react
    I2C_SW_SCL_LOW();
    I2C_SW_DELAYN();

    return transfer_succeeded;
}

/**
 * Pulls SCL high and waits until it is high or timeout occurs.
 *
 * SCL is expected to be output before entering this function.
 * @note If TWI_MASTER_TIMEOUT_COUNTER_LOAD_VALUE is set to zero, timeout functionality is not compiled in.
 * @return
 * @retval true SCL is now high.
 * @retval false Timeout occurred and SCL is still low.
 */
static bool sw_i2c_master_wait_while_scl_low(void)
{
    uint32_t volatile timeout_counter = 1000; //TWI_MASTER_TIMEOUT_COUNTER_LOAD_VALUE;

    // Pull SCL high just in case if something left it low
    I2C_SW_SCL_HIGH();
    I2C_SW_DELAYN_SHORT();

    while (I2C_SW_SCL_READ() == 0)
    {
        // If SCL is low, one of the slaves is busy and we must wait

        if (timeout_counter-- == 0)
        {
            // If timeout_detected, return false
            return false;
        }
    }

    return true;
}

#endif

